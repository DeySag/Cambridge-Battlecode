#include "bot.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <type_traits>
#include <variant>

namespace bc = cambc;
using bc::vec2;
namespace {
constexpr int infinity = 60000;
int distance(vec2 a, vec2 b) { return int(a.chebyshev_distance(b)); }
bc::bitboard around(vec2 p, int radius) {
    bc::bitboard result;
    result.draw_chebyshev(p, float(radius));
    return result;
}
bool exclusive(const bc::ability& ability) {
    return ability.info() && std::visit([](const auto& value) { return value.exclusive_with_movement; }, *ability.info());
}
int identity(const bc::champion& champion) {
    return int(static_cast<bc::handle_champion_t>(champion));
}
}

bool Bot::has_budget(unsigned minimum) const {
    return int(bc::round_number()) == round && bc::fuel_remaining() >= minimum;
}

void Bot::initialize() {
    stage = "initialize";
    width = bc::map::width(); height = bc::map::height();
    auto& me = bc::self();
    terrain = bc::map::walls() | bc::map::holes() | bc::map::altars() | bc::map::spawn_region(bc::team::enemy);
    auto allies = bc::champions(bc::team::ally);
    team_size = int(allies.size());
    camp_multiplier = me.base_stats().camp_damage_multiplier;
    std::sort(allies.begin(), allies.end(), [](const auto& a, const auto& b) { return identity(a) < identity(b); });
    for (int i = 0; i < int(allies.size()) && i < 8; ++i)
        if (allies[i] == me) slot = i;
    for (const auto& view : bc::map::camp_groups()) {
        Group group{view};
        group.gems = int(view.gem_value());
        group.gold = int(view.gold_value());
        for (const auto& camp : view.camps()) {
            const auto stats = camp.base_stats();
            group.tiles.push_back(camp.spawn_position());
            group.center += camp.spawn_position();
            group.full_health += stats.base_health;
            group.damage += stats.base_strength;
            group.range = std::max(group.range, camp.attack_range());
            group.monolith |= camp.is_monolith();
        }
        group.center /= int(group.tiles.size());
        group.health = group.full_health;
        groups.push_back(std::move(group));
    }
    boss_only = std::none_of(groups.begin(), groups.end(), [](const auto& group) { return group.gems > 0 && !group.monolith; });
    bc::bitboard roots, visited;
    for (auto altar : bc::map::altars().positions()) roots |= around(altar, bc::constants::submit_gems_range);
    roots &= ~terrain;
    altar_distance.compute(terrain, roots, visited);
    home_distance.compute(terrain, bc::map::spawn_region(bc::team::ally), visited);
    altar_values = altar_distance.export_linear_buffer();
    home_values = home_distance.export_linear_buffer();
    me.set_priority(100 - std::max(0, slot));
    initialized = true;
}

void Bot::reserve(int group) {
    if (slot >= 0 && slot < int(std::min<std::size_t>(8, bc::shared_memory.size()))) {
        const unsigned char value = group >= 0 && group < 254 ? static_cast<unsigned char>(group + 1) : 0;
        bc::shared_memory[slot] = value;
        reservations[slot] = value;
    }
}

void Bot::observe() {
    stage = "observe";
    camps.clear(); enemies.clear(); allies.clear();
    bc::shared_memory.read(0, std::span{reservations.data(), std::min<std::size_t>(8, bc::shared_memory.size())});
    // Local fights must remain observable even when a large camp census uses
    // the planning budget. Never query live state through a hidden identity.
    for (const auto& ally : bc::vision::champions(bc::team::ally)) {
        if (ally != bc::self() && ally.is_alive())
            allies.push_back({ally, ally.position(), ally.health(), ally.strength(), ally.focus(), ally.attack_range()});
    }
    for (const auto& enemy : bc::vision::champions(bc::team::enemy)) {
        if (!enemy.is_alive()) continue;
        Enemy seen{enemy, enemy.position(), enemy.health(), enemy.strength(), enemy.attack_range()};
        const auto buffs = enemy.buffs();
        seen.stunned = buffs.has_stun();
        if (buffs.has_shield()) seen.health += buffs.get<bc::buff_shield>().value;
        const auto abilities = enemy.abilities();
        for (int i = 0; i < 2; ++i) {
            const auto& ability = abilities[i];
            if (!ability.info()) continue;
            auto& spell = seen.spells[i];
            spell.cooldown = ability.cooldown();
            if (ability.is_attack_reset()) { spell.damage = seen.strength; spell.range = seen.range; }
            else if (ability.is_damage() || ability.is_cc() || ability.is_aoe_damage() || ability.is_dash()) {
                spell.damage = ability.damage_amount();
                spell.stun = ability.is_cc();
                spell.dash = ability.is_dash();
                std::visit([&](const auto& info) {
                    if constexpr (requires { info.range; }) spell.range = int(std::ceil(info.range));
                    if constexpr (requires { info.radius; }) spell.range += int(std::ceil(info.radius));
                    if constexpr (requires { info.duration; }) spell.stun |= info.duration > 0;
                }, *ability.info());
                if (spell.dash) spell.range += seen.range;
            }
        }
        enemies.push_back(seen);
    }
    if (!enemies.empty()) last_contact = round;
    std::erase_if(dangers, [&](const auto& danger) { return danger.expires <= round; });
    const auto visible = bc::vision::visible_tiles();
    for (int i = 0; i < int(groups.size()); ++i) {
        if (!has_budget(12000)) return;
        auto& group = groups[i];
        bool seen = false;
        for (auto tile : group.tiles) if (visible.is_set(tile)) { seen = true; break; }
        if (!seen) {
            if (group.state == 3 && round >= group.ready_round) {
                group.state = 0;
                group.health = group.full_health;
            }
            continue;
        }
        group.seen_round = round;
        const auto state = group.view.state();
        if (!std::holds_alternative<bc::map::camp_group_reward_available>(state)) group.capture_wait_from = -1;
        if (const auto* waiting = std::get_if<bc::map::camp_group_respawning>(&state)) {
            group.state = 3;
            group.ready_round = round + waiting->timer + 1;
            group.health = 0;
        } else if (std::holds_alternative<bc::map::camp_group_reward_available>(state)) {
            group.state = 2;
            group.health = 0;
        } else {
            group.state = 1;
            group.health = 0;
            for (const auto& member : group.view.camps()) {
                const auto tile = member.spawn_position();
                if (!visible.is_set(tile)) {
                    group.health += member.base_stats().base_health;
                    continue;
                }
                // Visibility of a tile does not imply its original camp is still alive.
                if (!bc::vision::has_camp(tile)) continue;
                const auto camp = bc::vision::camp_at(tile);
                const float health = camp.health();
                if (health <= 0) continue;
                group.health += health;
                camps.push_back({camp, tile, health, camp.strength(), camp.attack_range(), i, camp.is_awake()});
            }
        }
    }
}

void Bot::shop() {
    stage = "shop";
    auto& me = bc::self();
    // Completed families provide sustain as well as damage. Cluster income
    // opens a jungle emblem, then health prevents costly deaths against camps
    // and burst. Buy complete five-shard blocks so passives arrive promptly.
    for (int n = 0; n < 40 && me.gold_count() >= 30; ++n) {
        if (!has_budget(4000)) break;
        const bool caster = type == bc::champion_type::heart_of_ruin || type == bc::champion_type::stellar
            || type == bc::champion_type::cassi || type == bc::champion_type::prism;
        const bool health = strength_shards >= 5 && health_shards < ((strength_shards + 5) / 10) * 5;
        bc::item_type item = health ? bc::item_type::bruiser_shard_of_health
            : caster ? bc::item_type::power_shard_of_focus : bc::item_type::power_shard_of_strength;
        if (!health && strength_shards < 5) item = caster ? bc::item_type::jungle_shard_of_focus : bc::item_type::jungle_shard_of_strength;
        else if (!health && strength_shards < 10) item = caster ? bc::item_type::monster_shard_of_focus : bc::item_type::monster_shard_of_strength;
        if (health && health_shards < 5) item = bc::item_type::durability_shard_of_health;
        if (!me.can_buy(item)) break;
        me.buy(item);
        if (health) ++health_shards; else ++strength_shards;
    }
    if (strength_shards >= 5 && me.can_buy(bc::consumable_type::altar_lock)) me.buy(bc::consumable_type::altar_lock);
    if (me.can_buy(bc::consumable_type::tp_crystal)) me.buy(bc::consumable_type::tp_crystal);
}

bc::bitboard Bot::blocked(bool avoid_danger) const {
    auto result = terrain | bc::vision::champion_tiles() | bc::vision::camp_tiles() | bc::vision::teleport_tiles();
    if (avoid_danger) {
        for (const auto& enemy : enemies) {
            if (enemy.stunned || enemy.health < strength * 1.2f) continue;
            int reach = enemy.range + 1;
            for (const auto& spell : enemy.spells)
                if (spell.cooldown <= 2) reach = std::max(reach, spell.range);
            result |= around(enemy.position, reach);
        }
    }
    result.set(position, false);
    return result;
}

float Bot::incoming(vec2 position, int group) const {
    float damage = 0;
    for (const auto& camp : camps)
        if (camp.awake && (group < 0 || camp.group == group) && distance(position, camp.position) <= camp.range)
            damage += camp.damage;
    return damage + enemy_pressure(position, 1);
}

float Bot::enemy_pressure(vec2 tile, int turns) const {
    float damage = 0;
    for (const auto& enemy : enemies) {
        const int gap = distance(tile, enemy.position);
        const float reach = gap <= enemy.range ? 1.0f : gap == enemy.range + 1 ? 0.65f : 0.0f;
        float basic = enemy.strength * turns * reach;
        float spell_damage = 0;
        // The two slots share one cast per turn. Do not sum two simultaneous casts.
        for (const auto& spell : enemy.spells) {
            if (spell.range == 0 || gap > spell.range + (turns > 1 ? 1 : 0)) continue;
            const float readiness = spell.cooldown < turns ? 1.0f : spell.cooldown <= turns + 1 ? 0.35f : 0.0f;
            const float reach = gap <= spell.range ? 1.0f : 0.5f;
            const float value = spell.damage + (spell.stun ? enemy.strength * 0.8f : 0);
            spell_damage = std::max(spell_damage, value * readiness * reach);
        }
        damage += (basic + spell_damage) * (enemy.stunned ? 0.35f : 1.0f);
    }
    return damage;
}

float Bot::protection() const {
    return hp + shield_value + available_shield * 0.8f;
}

bool Bot::endangered() const {
    const float threat = enemy_pressure(position, 2);
    return threat > 0 && (threat * 1.35f > protection()
        || (hp < max_hp * 0.55f && threat > hp * 0.5f));
}

void Bot::remember_danger() {
    if (dangers.size() >= 6) dangers.erase(dangers.begin());
    dangers.push_back({position, round + 180});
}

float Bot::danger_memory(vec2 tile) const {
    float penalty = 1;
    for (const auto& danger : dangers)
        if (distance(tile, danger.position) <= 6) {
            const float remaining = std::clamp(float(danger.expires - round) / 120, 0.0f, 1.0f);
            penalty *= 1 - 0.75f * remaining;
        }
    return std::max(0.06f, penalty);
}

void Bot::wake_group(int group) {
    for (auto& camp : camps) if (camp.group == group) camp.awake = true;
}

bool Bot::navigate(bc::bitboard goals, bool dash, bool avoid_danger) {
    stage = "navigate";
    if (!has_budget(12000)) return false;
    auto& me = bc::self();
    if (!me.can_move() || goals.is_empty() || goals.is_set(position)) return false;
    const bool bridge_travel = bridge_valid && bridge_goals == goals && !avoid_danger;
    if (bridge_travel) {
        if (position == bridge_from) return cross_obstacle(goals);
        goals = around(bridge_from, 0);
    }
    auto walls = blocked(avoid_danger);
    // Static routes must be able to lead up to a monster blocking a corridor.
    // Treating all camps as path walls stranded entire teams on several replays.
    bc::bitboard path_walls = terrain;
    if (avoid_danger) path_walls |= walls & ~(bc::vision::camp_tiles() | bc::vision::champion_tiles() | bc::vision::teleport_tiles());
    path_walls.set(position, false);
    if (!route_valid || route_goals != goals || route_avoid_danger != avoid_danger
        || round - route_round > (avoid_danger ? 2 : 20) || blocked_turns >= 3) {
        route_goals = goals;
        auto free_goals = goals & ~path_walls;
        if (free_goals.is_empty()) return false;
        // On a persistent blockage, try an occupancy-aware detour once.
        if (blocked_turns >= 3) path_walls |= walls & ~bc::vision::camp_tiles();
        route_field.compute_to(path_walls, free_goals, around(position, 0), route_visited);
        route_round = round; route_avoid_danger = avoid_danger; route_valid = true;
        if (!has_budget(5500)) return false;
    }
    if (!route_visited.is_set(position)) {
        route_valid = false;
        // A temporary unit blockage is not evidence of disconnected terrain.
        // Portal planning never calls navigation recursively.
        if (bridge_travel) { bridge_valid = false; return false; }
        if (blocked_turns >= 3) { blocked_turns = 0; return false; }
        return avoid_danger ? false : cross_obstacle(goals);
    }
    bool success = false;
    const int current = route_field.get(position);
    std::vector<std::pair<float, vec2>> options;
    for (auto tile : (around(position, 1) & ~walls).positions()) {
        if (tile == position || !route_visited.is_set(tile)) continue;
        const int cost = route_field.get(tile);
        if (cost > current + (blocked_turns > 0 ? 1 : -1)) continue;
        float score = float(cost) * 10 + incoming(tile) * (banking ? 0.08f : 0.03f);
        if (tile == previous_position && blocked_turns < 3) score += 16;
        options.emplace_back(score, tile);
    }
    std::sort(options.begin(), options.end());
    for (auto [cost, tile] : options) {
        if (walls.is_set(tile)) continue;
        if (!has_budget(6000) || !me.is_alive() || !me.can_move()) return false;
        if (me.move(tile - position)) {
            previous_position = position; position = tile; moved = true;
            last_progress = round; blocked_turns = 0; success = true; break;
        }
        walls |= blocked(avoid_danger);
    }
    if (!success) {
        ++blocked_turns;
        for (auto delta : route_field.next_moves_from(position)) {
            const auto tile = position + delta;
            if (!bc::vision::has_camp(tile)) continue;
            for (const auto& camp : camps) if (camp.position == tile) {
                target = camp.group; reserve(target);
                // Clearing this group is a useful route action, even while banking.
                if (has_budget(10000)) combat(false);
                return true;
            }
        }
    }
    if (!dash || !me.can_cast() || goals.is_set(position) || !has_budget(14000)) return success;
    for (const auto& ability : me.abilities()) {
        if (!ability.is_dash() || !ability.is_ready()) continue;
        const auto info = ability.get<bc::ability_dash>();
        // Keep a damaging/stunning dash for an actual fight, unless escaping.
        if (info.modifier > 0 && enemy_pressure(position, 2) > 0) break;
        auto dash_walls = bc::map::walls() | bc::map::altars() | bc::map::spawn_region(bc::team::enemy)
            | bc::vision::champion_tiles() | bc::vision::camp_tiles() | bc::vision::teleport_tiles();
        dash_walls.set(position, false);
        auto landings = dash_walls.reachable_tiles(position, int(info.range)) & ability.valid_tiles() & ~blocked(avoid_danger);
        std::vector<std::pair<int, vec2>> jumps;
        for (auto tile : landings.positions())
            if (route_visited.is_set(tile)) jumps.emplace_back(route_field.get(tile), tile);
        std::sort(jumps.begin(), jumps.end());
        const int now = route_field.get(position);
        for (auto [cost, tile] : jumps) {
            if (cost + 1 >= now) continue;
            if (blocked(avoid_danger).is_set(tile)) continue;
            if (incoming(tile) > protection() * 0.35f && incoming(tile) > incoming(position)) continue;
            if (!has_budget(6000) || !me.is_alive() || !me.can_cast()) return success;
            if (ability.use(tile)) { position = tile; last_progress = round; return true; }
        }
    }
    return success;
}

bool Bot::cross_obstacle(const bc::bitboard& goals) {
    stage = "bridge";
    auto& me = bc::self();
    if (!has_budget(14000) || !me.is_alive()) return false;
    if (!bridge_valid || bridge_goals != goals) {
        bc::distance_field from;
        bc::bitboard reached;
        from.compute(terrain, around(position, 0), reached);
        if (!has_budget(11000)) return false;
        // route_field contains distances in the destination component. Only
        // accept a crossing that demonstrably connects the two components.
        if (route_goals != goals) {
            route_field.compute(terrain, goals & ~terrain, route_visited);
            if (!has_budget(10000)) return false;
        }
        const bool crystal = me.consumables()[bc::consumable_type::tp_crystal].count() > 0;
        bool dash_available = false;
        for (const auto& ability : me.abilities()) dash_available |= ability.is_dash();
        if (!crystal && !dash_available) return false;
        float best = std::numeric_limits<float>::max();
        const auto occupancy = blocked();
        // The SDK shifts masks without materializing a full movement graph.
        for (int dy = -3; dy <= 3; ++dy) for (int dx = -3; dx <= 3; ++dx) {
            if ((!dx && !dy) || !has_budget(6500)) continue;
            if (!crystal && std::max(std::abs(dx), std::abs(dy)) > 2) continue;
            auto landings = reached.shift_right(dx).shift_up(dy) & route_visited & ~occupancy;
            for (auto landing : landings.positions()) {
                const vec2 source = landing - vec2{dx, dy};
                if (!bc::map::is_in_bounds(source) || !reached.is_set(source)) continue;
                const float lower_cost = from.get(source) + route_field.get(landing) + 1;
                if (lower_cost >= best) continue;
                bool legal_dash = false;
                if (dash_available && std::max(std::abs(dx), std::abs(dy)) <= 2) {
                    auto dash_walls = bc::map::walls() | bc::map::altars() | bc::map::spawn_region(bc::team::enemy);
                    legal_dash = dash_walls.reachable_tiles(source, 2).is_set(landing);
                }
                if (!crystal && !legal_dash) continue;
                const float cost = from.get(source) + route_field.get(landing) + (legal_dash ? 1 : 4)
                    + enemy_pressure(landing, 2) * 0.1f;
                if (cost < best) { best = cost; bridge_from = source; bridge_to = landing; }
                if (!has_budget(6500)) break;
            }
        }
        if (best == std::numeric_limits<float>::max()) return false;
        bridge_valid = true; bridge_goals = goals;
    }
    if (position != bridge_from) return false; // Approach the anchor next turn.
    if (blocked().is_set(bridge_to)) { bridge_valid = false; return false; }
    for (const auto& ability : me.abilities()) {
        if (!me.can_cast() || !ability.is_dash() || !ability.is_ready() || !ability.valid_tiles().is_set(bridge_to)) continue;
        auto walls = bc::map::walls() | bc::map::altars() | bc::map::spawn_region(bc::team::enemy)
            | bc::vision::champion_tiles() | bc::vision::camp_tiles() | bc::vision::teleport_tiles();
        walls.set(position, false);
        if (!walls.reachable_tiles(position, int(ability.get<bc::ability_dash>().range)).is_set(bridge_to)) continue;
        if (!has_budget(6000) || !me.is_alive() || !me.can_cast()) return true;
        if (ability.use(bridge_to)) { position = bridge_to; last_progress = round; bridge_valid = route_valid = false; return true; }
    }
    if (me.consumables()[bc::consumable_type::tp_crystal].count() && distance(position, bridge_to) <= 3) {
        if (!has_budget(6000) || !me.is_alive() || me.is_teleporting() || me.is_recalling() || me.is_submitting_gems()) return true;
        if (me.consumables()[bc::consumable_type::tp_crystal].use(bridge_to)) { bridge_valid = route_valid = false; last_progress = round; return true; }
    }
    return false;
}

bool Bot::tactical_step(vec2 goal, bool flee) {
    stage = flee ? "tactical retreat" : "tactical approach";
    auto& me = bc::self();
    if (!has_budget(7000) || !me.is_alive() || !me.can_move()) return false;
    auto walls = blocked();
    float best = std::numeric_limits<float>::max();
    vec2 chosen = position;
    auto value = [&](vec2 tile) {
        const float threat = incoming(tile);
        float score = threat * (flee ? 4 : 1.2f);
        const int gap = distance(tile, goal);
        score += flee ? -gap * 15.0f : std::abs(gap - attack_range) * 22.0f;
        for (const auto& ally : allies) score += std::min(7, distance(tile, ally.position)) * (flee ? 1.0f : 1.5f);
        if (tile == previous_position) score += 5;
        return score;
    };
    best = value(position);
    for (auto tile : (around(position, 1) & ~walls).positions()) {
        if (tile == position) continue;
        if (!flee && incoming(tile) * 2 > protection() * 0.7f) continue;
        const float score = value(tile);
        if (score < best) { best = score; chosen = tile; }
    }
    if (chosen == position) return false;
    if (!has_budget(6000) || !me.is_alive() || !me.can_move()) return false;
    if (me.move(chosen - position)) { previous_position = position; position = chosen; moved = true; last_progress = round; return true; }
    return false;
}

bc::bitboard Bot::group_goals(int group_id) const {
    const auto& group = groups[group_id];
    bc::bitboard goals;
    if (group.state == 2) {
        for (auto tile : group.tiles) goals.set(tile);
    } else {
        bool any = false;
        for (const auto& camp : camps) {
            if (camp.group != group_id) continue;
            goals |= around(camp.position, attack_range);
            any = true;
        }
        // A hidden camp must be approached, not treated as already empty.
        if (!any) for (auto tile : group.tiles) goals |= around(tile, 1);
    }
    return goals;
}

// This is a conservative camp-ranking estimate, not a combat simulator.
// In particular, do not rely on every retaliation or splash hit landing.
float Bot::estimated_dps(const Group& group) const {
    const auto& me = bc::self();
    float damage = strength * 1.12f;
    if (type == bc::champion_type::varg)
        damage *= std::min(3.5f, 1.0f + (group.tiles.size() - 1) * 0.8f);
    if (type == bc::champion_type::tavi) damage += focus * 0.15f;
    if (type == bc::champion_type::heart_of_ruin) damage += focus * 0.18f * std::min(3.0f, float(group.tiles.size()));
    if (type == bc::champion_type::ryth) damage += focus * 0.12f * std::min(3.0f, float(group.tiles.size()));
    return std::max(1.0f, damage * std::max(1.0f, camp_multiplier));
}

int Bot::choose_group() {
    stage = "choose";
    auto& me = bc::self();
    // A large-map search may consume most of a turn. Retain its result and
    // finish scoring next turn instead of repeating the same unfinished search.
    const bool large_map = true;
    if (!choice_ready || distance(choice_origin, position) > 3 || round - choice_round > 8) {
        const auto walls = terrain;
        bc::distance_field distances;
        bc::bitboard visited;
        distances.compute(walls, around(position, 0), visited);
        if (!large_map && !has_budget(12000)) return -1;
        choice_distances = distances.export_linear_buffer();
        choice_walls = walls.export_linear_buffer();
        choice_origin = position;
        choice_round = round;
        choice_cursor = 0;
        choice_best = -1;
        choice_score = -1;
        choice_ready = true;
    }
    if (large_map && !has_budget(12000)) return -1;
    for (; choice_cursor < int(groups.size()); ++choice_cursor) {
        const int i = choice_cursor;
        const auto& group = groups[i];
        if (group.failed_until > round) continue;
        if (group.monolith && group.state != 2 && (!boss_only || strength_shards < 5 || health_shards < 5)) continue;
        int travel = infinity, delivery = infinity;
        // Terrain distances include the camp's spawn tile. Subtract interaction
        // range instead of scanning a square around every member every turn.
        for (auto tile : group.tiles) {
            const int index = tile.y * width + tile.x;
            if (choice_walls[index] || choice_distances[index] == bc::distance_field::unreachable) continue;
            travel = std::min(travel, std::max(0, int(choice_distances[index]) - (group.state == 2 ? 0 : attack_range)));
            delivery = std::min(delivery, int(altar_values[index]));
        }
        if (travel >= infinity) {
            // Disconnected islands can be reached by a dash/crystal. The
            // navigator must prove a legal crossing before attempting it.
            bool transport = bc::self().consumables()[bc::consumable_type::tp_crystal].count() > 0;
            for (const auto& ability : bc::self().abilities()) transport |= ability.is_dash();
            if (!transport) continue;
            travel = distance(position, group.center) + 12;
            const auto altars = bc::map::altars().positions();
            delivery = altars.empty() ? 0 : distance(group.center, altars.front());
        }
        const int wait = group.state == 3 ? std::max(0, group.ready_round - round - travel) : 0;
        if (wait > 12) continue;
        float dps = estimated_dps(group);
        float nearby_health = 0;
        for (const auto& ally : allies) if (distance(ally.position, group.center) <= 6) {
            dps += ally.strength * std::max(1.0f, camp_multiplier) * 0.65f;
            nearby_health += ally.health * 0.45f;
        }
        const float health = group.state == 3 ? group.full_health : group.health;
        float clear = health / dps;
        int claims = 0;
        for (int j = 0; j < 8; ++j)
            if (j != slot && reservations[j] == i + 1) ++claims;
        // Do not send the entire team to the same unguarded resource.
        if (claims) clear /= 1.0f + claims * 0.7f;
        const float reward = group.gems + group.gold * (strength_shards < 10 ? 0.50f : 0.15f) + 1;
        float score = reward / (travel + clear + 6 + wait + delivery * 0.30f);
        if (claims) score *= group.monolith ? 0.8f : group.state == 2 ? 0.08f : 0.28f;
        // Retaliation falls as monsters die. Shielding gives Varg substantial sustain.
        float risk = group.damage * clear * (group.tiles.size() > 1 ? 0.55f : 0.95f);
        if (strength_shards >= 5) risk *= 0.70f;
        if (health_shards >= 5) risk *= 0.90f;
        if (type == bc::champion_type::varg) risk = std::max(0.0f, risk - focus * 2.5f * clear / 8);
        if (type == bc::champion_type::heart_of_ruin) risk = std::max(0.0f, risk - focus * 1.5f * clear / 8);
        if (risk > hp + nearby_health - max_hp * 0.20f) score *= std::max(0.015f, (hp + nearby_health) / std::max(1.0f, risk) * 0.20f);
        if (group.monolith && nearby_health < max_hp * 0.5f && strength_shards < 20) score *= 0.15f;
        for (const auto& enemy : enemies)
            if (distance(enemy.position, group.center) <= 5 && enemy.health > hp * 0.8f) score *= 0.45f;
        score *= danger_memory(group.center);
        if (enemy_pressure(group.center, 2) > hp * 0.55f) score *= 0.25f;
        if (i == target) score *= 1.25f;
        // Prefer our side early, without permanently giving up the middle.
        const auto start = group.tiles.front();
        const int home = home_values[start.y * width + start.x];
        if (round < 120 && home > bc::map::width() * 0.6f) score *= 0.65f;
        if (score > choice_score) { choice_score = score; choice_best = i; }
        if (large_map && !has_budget(18000)) { ++choice_cursor; return -1; }
        if (!large_map && bc::fuel_remaining() < 16000) break;
    }
    decision_round = round;
    choice_ready = false;
    return choice_best;
}

bool Bot::bank() {
    stage = "bank";
    auto& me = bc::self();
    reserve(-1);
    if (me.gem_count() == 0) { banking = false; bank_altar = {-1, -1}; return false; }
    if (bank_altar.x < 0 || round - bank_plan_round >= 16) {
        if (!has_budget(22000)) return true;
        bc::distance_field travel;
        bc::bitboard visited;
        travel.compute(terrain, around(position, 0), visited);
        bank_plan_round = round;
        bank_altar = {-1, -1};
        float best = std::numeric_limits<float>::max();
        for (auto altar : bc::map::altars().positions()) {
            float cost = std::numeric_limits<float>::max();
            for (auto tile : (around(altar, bc::constants::submit_gems_range) & ~terrain).positions()) {
                if (!visited.is_set(tile)) continue;
                const float risk = enemy_pressure(tile, 3);
                cost = std::min(cost, travel.get(tile) + risk * 0.09f
                    + (1 - danger_memory(tile)) * 24);
            }
            for (const auto& lock : bc::vision::altar_locks(bc::team::enemy))
                if (lock.position() == altar) cost += 5 * lock.hits_remaining();
            if (cost < best) { best = cost; bank_altar = altar; }
        }
        if (bank_altar.x < 0) {
            // A crystal may connect an altar on another walking component.
            for (auto altar : bc::map::altars().positions())
                if (bank_altar.x < 0 || distance(position, altar) < distance(position, bank_altar)) bank_altar = altar;
        }
        route_valid = false;
        if (!has_budget(14000)) return true;
    }
    if (bank_altar.x < 0) { retreat(); return true; }
    bc::bitboard goals;
    for (auto altar : bc::map::altars().positions()) {
        if (altar != bank_altar) continue;
        if (distance(position, altar) <= bc::constants::submit_gems_range) {
            std::optional<bc::altar_lock> hostile;
            for (const auto& lock : bc::vision::altar_locks(bc::team::enemy))
                if (lock.position() == altar) { hostile = lock; break; }
            if (hostile) {
                if (me.can_attack()) {
                    me.attack(*hostile);
                    if (hostile->hits_remaining() > 1) {
                        for (const auto& ability : me.abilities())
                            if (me.can_cast() && ability.is_attack_reset() && ability.is_ready()) {
                                ability.use();
                                for (const auto& lock : bc::vision::altar_locks(bc::team::enemy))
                                    if (lock.position() == altar && me.can_attack()) { me.attack(lock); break; }
                                break;
                            }
                    }
                }
            } else {
                // Long channels are unsafe under active pressure, even with a healthy bag.
                if (enemy_pressure(position, 3) > protection() * 0.7f) {
                    bank_altar = {-1, -1};
                    escape();
                    return true;
                }
                if (me.consumables()[bc::consumable_type::altar_lock].count() > 0) {
                    bool already = false;
                    for (const auto& lock : bc::vision::altar_locks(bc::team::ally)) already |= lock.position() == altar;
                    if (!already) me.consumables()[bc::consumable_type::altar_lock].use(altar);
                }
                me.submit_gems(altar);
            }
            return true;
        }
        for (auto tile : (around(altar, bc::constants::submit_gems_range) & ~terrain).positions())
            if (enemy_pressure(tile, 2) < protection() * 0.6f) goals.set(tile);
    }
    if (!navigate(goals, true, !enemies.empty())) {
        bank_altar = {-1, -1};
        if (enemy_pressure(position, 2) > 0) escape();
        else if (!enemies.empty()) retreat();
    }
    // Any champion can clear an actual path obstruction; ranged carriers must
    // not wait forever behind a monster sealing a corridor.
    if (target >= 0 && has_budget(11000)) combat(false);
    return true;
}

bool Bot::escape() {
    stage = "escape";
    auto& me = bc::self();
    if (!has_budget(10000) || !me.is_alive()) return false;
    // A shield is compatible with walking. Prefer it before taking another hit.
    for (const auto& ability : me.abilities())
        if (me.can_cast() && ability.is_shield() && ability.is_ready()
            && incoming(position) > 0 && !me.buffs().has_shield()) {
            if (!has_budget(6000) || !me.is_alive() || !me.can_cast()) return false;
            ability.use(); break;
        }
    auto walls = blocked();
    auto cost = [&](vec2 tile) {
        float value = enemy_pressure(tile, 2) * 3;
        for (const auto& camp : camps)
            if (camp.awake && distance(tile, camp.position) <= camp.range) value += camp.damage * 3;
        // Continuous separation breaks ties inside a large enemy attack zone.
        for (const auto& enemy : enemies) value += std::max(0, 9 - distance(tile, enemy.position)) * 12;
        value += home_values[tile.y * width + tile.x] * 0.3f;
        return value;
    };
    bool success = false;
    if (me.can_move()) {
        stage = "escape walk";
        std::vector<std::pair<float, vec2>> steps;
        for (auto tile : (around(position, 1) & ~walls).positions())
            if (tile != position) steps.emplace_back(cost(tile), tile);
        std::sort(steps.begin(), steps.end());
        for (auto [value, tile] : steps) {
            if (value > cost(position) + 1) break;
            if (walls.is_set(tile)) continue;
            if (!has_budget(6000) || !me.is_alive() || !me.can_move()) return success;
            if (me.move(tile - position)) { position = tile; moved = true; success = true; break; }
            walls |= blocked();
        }
    }
    if (me.can_cast() && has_budget(10000)) {
        stage = "escape dash";
        for (const auto& ability : me.abilities()) {
            if (!ability.is_dash() || !ability.is_ready()) continue;
            auto dash_walls = bc::map::walls() | bc::map::altars() | bc::map::spawn_region(bc::team::enemy)
                | bc::vision::champion_tiles() | bc::vision::camp_tiles() | bc::vision::teleport_tiles();
            dash_walls.set(position, false);
            std::vector<std::pair<float, vec2>> options;
            for (auto tile : (dash_walls.reachable_tiles(position, 2) & ability.valid_tiles() & ~blocked()).positions())
                if (tile != position) options.emplace_back(cost(tile), tile);
            std::sort(options.begin(), options.end());
            for (auto [value, tile] : options) {
                if (value >= cost(position) - 5) break;
                if (blocked().is_set(tile)) continue;
                if (!has_budget(6000) || !me.is_alive() || !me.can_cast()) return success;
                if (ability.use(tile)) { position = tile; success = true; break; }
            }
            break;
        }
    }
    if (has_budget(10000)) { stage = "escape defense"; combat(true, true); }
    return success;
}

bool Bot::retreat() {
    stage = "retreat";
    auto& me = bc::self();
    reserve(-1);
    if (bc::map::spawn_region(bc::team::ally).is_set(position)) return true;
    if (incoming(position) > 0 || enemy_pressure(position, 2) > 0) escape();
    if (has_budget(6000) && me.is_alive() && incoming(position) == 0 && enemy_pressure(position, 2) == 0
        && (me.can_move() || me.can_attack() || me.can_cast())) { stage = "retreat recall"; me.recall(); last_recall = round; }
    return true;
}

bool Bot::engage() {
    auto& me = bc::self();
    if (!has_budget(17000) || enemies.empty() || banking || hp < max_hp * 0.48f || me.gem_count() >= 60 || endangered()) return false;
    if (target >= 0 && groups[target].state == 2 && !endangered()
        && std::find(groups[target].tiles.begin(), groups[target].tiles.end(), position) != groups[target].tiles.end()) return false;
    const Enemy* chosen = nullptr;
    float best = -1;
    for (const auto& enemy : enemies) {
        // Spawn heals can make an idle enemy an endless, unproductive target.
        if (bc::map::spawn_region(bc::team::enemy).is_set(enemy.position)) continue;
        if (distance(position, enemy.position) > 6) continue;
        float friendly = protection() * (strength * 1.15f + focus * 0.18f);
        float hostile = enemy.health * (enemy.strength * 1.15f + enemy.view.focus() * 0.18f);
        for (const auto& ally : allies)
            if (distance(ally.position, enemy.position) <= ally.range + 2)
                friendly += ally.health * (ally.strength * 1.15f + ally.focus * 0.18f) * 0.7f;
        for (const auto& other : enemies)
            if (&other != &enemy && distance(other.position, position) <= other.range + 2)
                hostile += other.health * (other.strength * 1.15f + other.view.focus() * 0.18f) * 0.85f;
        if (friendly < hostile * 1.15f && enemy.health > strength + focus * 1.2f) continue;
        if (enemy_pressure(position, 2) > protection() * 0.65f) continue;
        const float score = 300 + enemy.view.gem_count() * 5 - enemy.health * 0.08f
            - distance(position, enemy.position) * 20;
        if (score > best) { best = score; chosen = &enemy; }
    }
    if (!chosen) return false;
    if (!has_budget(13000)) return false;
    reserve(-1);
    bool acted = combat(true, true);
    if (me.is_alive() && has_budget(9000) && me.can_move()) acted |= tactical_step(chosen->position);
    if (me.is_alive() && has_budget(10000)) acted |= combat(true, true);
    return acted;
}

bool Bot::combat(bool allow_enemies, bool defensive) {
    stage = "combat";
    if (!has_budget(10000)) return false;
    auto& me = bc::self();
    if (!me.is_alive()) return false;
    bool acted = false;
    const Enemy* foe = nullptr;
    float foe_score = 0;
    if (allow_enemies) for (const auto& enemy : enemies) {
        if (!enemy.view.is_visible() || !enemy.view.is_alive()
            || bc::map::spawn_region(bc::team::enemy).is_set(enemy.position)
            || distance(position, enemy.position) > 5) continue;
        const bool lethal = enemy.health <= strength * 1.15f + focus * 1.5f;
        if (!defensive && !lethal && (me.gem_count() >= 70 || endangered())) continue;
        const float value = (lethal ? 700 : 0) + enemy.view.gem_count() * 4 + enemy.strength
            + 250 - enemy.health * 0.08f - distance(position, enemy.position) * 12;
        if (value > foe_score) { foe_score = value; foe = &enemy; }
    }
    // Keep the engagement policy's filtering. Merely seeing a champion must
    // not consume a ranged farmer's movement or interrupt a safe delivery.
    const Enemy* basic_foe = foe;
    if (foe && distance(position, foe->position) > attack_range) {
        basic_foe = nullptr;
        float basic_score = -1;
        for (const auto& enemy : enemies) {
            if (distance(position, enemy.position) > attack_range || !enemy.view.is_visible() || !enemy.view.is_alive()
                || bc::map::spawn_region(bc::team::enemy).is_set(enemy.position)) continue;
            const bool lethal = enemy.health <= strength * 1.15f + focus * 1.5f;
            if (!defensive && !lethal && (me.gem_count() >= 70 || endangered())) continue;
            const float score = (enemy.health <= strength ? 1000.0f : 0) + enemy.view.gem_count() * 4
                + enemy.strength - enemy.health * 0.1f;
            if (score > basic_score) { basic_score = score; basic_foe = &enemy; }
        }
    }
    auto select_camp = [&]() -> std::optional<bc::camp> {
        float best = -1;
        std::optional<bc::camp> chosen;
        for (const auto& camp : camps) {
            if (defensive && !camp.awake) continue;
            if (!defensive && target >= 0 && camp.group != target) continue;
            if (distance(position, camp.position) > attack_range || !bc::vision::has_camp(camp.position)) continue;
            const auto current = bc::vision::camp_at(camp.position);
            const float basic = strength * std::max(1.0f, camp_multiplier);
            float score = std::min(basic, current.health());
            if (current.health() <= basic * 1.02f) score += 150 + camp.damage * 2;
            if (type == bc::champion_type::varg) for (const auto& other : camps)
                if (other.position != camp.position && distance(other.position, camp.position) <= 1
                    && bc::vision::has_camp(other.position)) score += std::min(basic, other.health);
            score += camp.damage * 0.6f;
            if (score > best) { best = score; chosen = current; }
        }
        return chosen;
    };
    // Shields preserve an attack and a walk. Do this before retaliation can
    // kill us; self health is refreshed after every actual attack below.
    const auto abilities = me.abilities();
    for (const auto& ability : abilities) {
        if (!has_budget(6500)) return acted;
        if (!me.can_cast() || !ability.is_shield() || !ability.is_ready() || me.buffs().has_shield()) continue;
        float threat = incoming(position);
        if (target >= 0 && !defensive) for (const auto& camp : camps)
            if (camp.group == target && distance(position, camp.position) <= camp.range) threat += camp.damage * 0.5f;
        if (threat > 0 && (hp < max_hp * 0.85f || threat * 4 > ability.shield_amount())) {
            if (!has_budget(6000) || !me.is_alive() || !me.can_cast()) return acted;
            if (ability.use()) { acted = true; last_progress = round; shield_value = available_shield; available_shield = 0; }
            break;
        }
    }
    auto attack = [&]() {
        if (!has_budget(6000) || !me.is_alive() || !me.can_attack()) return;
        if (basic_foe && basic_foe->view.is_visible() && basic_foe->view.is_alive() && distance(position, basic_foe->position) <= attack_range) {
            me.attack(basic_foe->view); attacked = acted = true; last_progress = round;
        } else if (const auto camp = select_camp()) {
            if (!has_budget(6000) || !me.is_alive() || !me.can_attack()) return;
            const auto hit = camp->spawn_position();
            me.attack(*camp); attacked = acted = true; last_progress = round;
            for (const auto& other : camps)
                if (other.position == hit || (type == bc::champion_type::varg && distance(other.position, hit) <= 1)) wake_group(other.group);
        }
        if (me.is_alive()) hp = me.health();
    };
    attack();
    if (!me.is_alive() || !me.can_cast() || !has_budget(8000)) return acted;
    int chosen_slot = -1;
    std::optional<vec2> ground;
    std::optional<bc::camp> unit_camp;
    std::optional<bc::champion> unit_champion;
    float best = 0;
    const float danger = incoming(position);
    for (int slot_id = 0; slot_id < 2; ++slot_id) {
        if (!has_budget(7000)) break;
        const auto& ability = abilities[slot_id];
        if (!ability.is_ready() || (moved && exclusive(ability))) continue;
        float score = 0;
        std::optional<vec2> p;
        std::optional<bc::camp> c;
        std::optional<bc::champion> e;
        if (ability.is_attack_reset()) {
            if ((attacked || moved) && !me.can_attack()) {
                if (basic_foe && basic_foe->view.is_visible() && basic_foe->view.is_alive()) score = strength * 1.3f;
                else if (const auto candidate = select_camp()) {
                    score = std::min(candidate->health(), strength * std::max(1.0f, camp_multiplier));
                    if (type == bc::champion_type::varg) for (const auto& other : camps)
                        if (other.position != candidate->spawn_position() && distance(other.position, candidate->spawn_position()) <= 1
                            && bc::vision::has_camp(other.position)) score += std::min(other.health, strength * std::max(1.0f, camp_multiplier));
                }
            }
        } else if (ability.is_heal()) {
            const auto range = ability.valid_tiles();
            for (const auto& ally : allies) {
                if (!ally.view.is_visible() || !ally.view.is_alive() || !range.is_set(ally.position)) continue;
                const float value = ability.heal_amount(ally.view) * (ally.health < ally.view.max_health() * 0.5f ? 1.8f : 1.0f);
                if (value > score) { score = value; e = ally.view; }
            }
        } else if (ability.is_shield()) {
            if (danger > 0 && !me.buffs().has_shield()) score = std::min(ability.shield_amount(), danger * 5) * 1.2f;
        } else if (ability.is_dash() && ability.get<bc::ability_dash>().modifier > 0) {
            const auto info = ability.get<bc::ability_dash>();
            auto walls = bc::map::walls() | bc::map::altars() | bc::map::spawn_region(bc::team::enemy)
                | bc::vision::champion_tiles() | bc::vision::camp_tiles() | bc::vision::teleport_tiles();
            walls.set(position, false);
            const auto landings = walls.reachable_tiles(position, int(info.range)) & ability.valid_tiles() & ~blocked();
            const float damage = ability.damage_amount();
            for (auto tile : landings.positions()) {
                if (!has_budget(6500)) break;
                if (tile == position) continue;
                float value = 0;
                if (allow_enemies) for (const auto& enemy : enemies) {
                    if (!enemy.view.is_visible() || !enemy.view.is_alive() || distance(tile, enemy.position) > info.radius
                        || bc::map::spawn_region(bc::team::enemy).is_set(enemy.position)) continue;
                    value += std::min(enemy.health, damage) * 1.4f + (info.duration ? enemy.strength * 1.5f + 70 : 0);
                    if (enemy.health <= damage) value += 200 + enemy.view.gem_count() * 3;
                }
                if (!defensive) for (const auto& camp : camps) {
                    if (target >= 0 && camp.group != target) continue;
                    if (distance(tile, camp.position) <= info.radius && bc::vision::has_camp(camp.position))
                        value += std::min(camp.health, damage * std::max(1.0f, camp_multiplier)) + camp.damage * (info.duration ? 1.0f : 0);
                }
                const float landing_threat = incoming(tile);
                if (landing_threat * 2.5f > protection() * 0.8f && landing_threat > incoming(position)) continue;
                value -= std::max(0.0f, landing_threat - incoming(position)) * 0.8f;
                if (value > score) { score = value; p = tile; }
            }
        } else if (ability.is_damage() || ability.is_cc() || ability.is_aoe_damage()) {
            const auto mask = ability.valid_tiles();
            const float damage = ability.damage_amount();
            if (foe && foe->view.is_visible() && foe->view.is_alive()
                && (ability.can_target_enemies() || ability.can_target_ground()) && mask.is_set(foe->position)) {
                if (ability.can_target_ground()) p = foe->position; else e = foe->view;
                score = std::min(foe->health, ability.damage_amount(foe->view)) * 1.4f
                    + (ability.is_cc() ? (defensive ? 100 + foe->strength : 50 + foe->strength * 0.5f) : 0);
                if (foe->health <= damage) score += 160 + foe->view.gem_count() * 3;
                if (ability.can_target_ground()) for (const auto& other : enemies)
                    if (&other != foe && distance(other.position, foe->position) <= 1 && other.view.is_visible()) score += std::min(other.health, damage) * 1.3f;
            }
            for (const auto& camp : camps) {
                if (!has_budget(6500)) break;
                if (defensive && !camp.awake) continue;
                if (!defensive && target >= 0 && camp.group != target) continue;
                if (!mask.is_set(camp.position) || !bc::vision::has_camp(camp.position)) continue;
                const float effective = damage * std::max(1.0f, camp_multiplier);
                float value = std::min(bc::vision::camp_at(camp.position).health(), effective);
                if (ability.is_cc()) value += camp.damage;
                if (ability.can_target_ground()) for (const auto& other : camps)
                    if (other.position != camp.position && distance(camp.position, other.position) <= 1
                        && bc::vision::has_camp(other.position)) value += std::min(other.health, effective);
                if (value > score) {
                    score = value; e.reset();
                    if (ability.can_target_ground()) { p = camp.position; c.reset(); }
                    else if (ability.can_target_camps()) { c = bc::vision::camp_at(camp.position); p.reset(); }
                    else score = 0;
                }
            }
        }
        if (score > best) { best = score; chosen_slot = slot_id; ground = p; unit_camp = c; unit_champion = e; }
    }
    if (chosen_slot < 0 || !has_budget(6500) || !me.is_alive() || !me.can_cast()) return acted;
    const auto& ability = abilities[chosen_slot];
    if (exclusive(ability) && (moved || (attack_range == 1 && !me.can_move()))) return acted;
    bool success = false;
    if (unit_champion) {
        if (!unit_champion->is_visible() || !unit_champion->is_alive()) return acted;
        success = ability.use(*unit_champion);
    } else if (ability.is_attack_reset() || ability.is_shield()) success = ability.use();
    else if (ground) {
        if (ability.is_dash() && blocked().is_set(*ground)) return acted;
        if (!has_budget(6000) || !me.is_alive() || !me.can_cast()) return acted;
        success = ability.use(*ground);
    } else if (unit_camp && bc::vision::has_camp(unit_camp->spawn_position())) success = ability.use(*unit_camp);
    if (!success) return acted;
    acted = true; last_progress = round;
    if (ability.is_shield()) { shield_value = available_shield; available_shield = 0; }
    if (ability.is_dash() && ground) position = me.position();
    if (unit_camp) for (const auto& camp : camps) if (camp.position == unit_camp->spawn_position()) wake_group(camp.group);
    if (ground) for (const auto& camp : camps)
        if (distance(camp.position, *ground) <= 1) wake_group(camp.group);
    if (me.is_alive() && ability.is_attack_reset()) attack();
    return acted;
}

void Bot::ward() {
    stage = "ward";
    auto& me = bc::self();
    if (!has_budget(9000) || !me.is_alive() || me.is_teleporting() || me.is_recalling() || me.is_submitting_gems()
        || me.restrictions().contains(bc::buff_restrictions::ward)) return;
    if ((round + slot) % 8 != 0 || me.ward_cooldown() > 0) return;
    auto legal = me.ward_placement_tiles() & ~terrain & ~bc::vision::ward_tiles(bc::team::ally)
        & ~bc::vision::champion_tiles() & ~bc::vision::camp_tiles();
    const vec2 goal = target >= 0 ? groups[target].center : position;
    auto tiles = legal.positions();
    if (tiles.empty()) return;
    const auto best = *std::min_element(tiles.begin(), tiles.end(), [&](auto a, auto b) { return distance(a, goal) < distance(b, goal); });
    if (has_budget(6000) && me.is_alive() && !me.is_teleporting() && !me.is_recalling() && !me.is_submitting_gems()) me.place_ward(best);
}

void Bot::play_turn() {
    auto& me = bc::self();
    round = int(bc::round_number());
    moved = false; attacked = false;
    if (!initialized) initialize();
    if (!has_budget(10000)) return;
    type = me.champion_name();
    shop();
    if (me.is_alive()) {
        position = me.position(); hp = me.health(); max_hp = me.max_health();
        strength = me.strength(); focus = me.focus(); attack_range = me.attack_range();
        const auto buffs = me.buffs();
        shield_value = buffs.has_shield() ? buffs.get<bc::buff_shield>().value : 0;
        available_shield = 0;
        if (!buffs.has_shield()) for (const auto& ability : me.abilities())
            if (ability.is_shield() && ability.is_ready()) available_shield = ability.shield_amount();
    }
    if (!me.is_alive()) {
        if (was_alive) remember_danger();
        was_alive = false;
        reserve(-1); target = -1; recovering = false; banking = false;
        bank_altar = {-1, -1}; route_valid = bridge_valid = false; choice_ready = false; last_progress = round; return;
    }
    was_alive = true;
    const bool spawn = bc::map::spawn_region(bc::team::ally).is_set(position);
    if (me.is_teleporting()) { reserve(-1); return; }
    if (spawn) {
        recovering = false;

    }
    observe();
    if (!has_budget(12000)) return;
    if (spawn && hp < max_hp * 0.99f) { reserve(-1); combat(true, true); return; }
    if (me.is_recalling() || me.is_submitting_gems()) {
        reserve(-1);
        const int remaining = me.remaining_channel_time();
        const float threat = enemy_pressure(position, std::min(5, remaining + 1));
        // Damage does not cancel tribute. Keep a nearly completed, survivable channel.
        if (threat > protection() * 1.1f && remaining > 1 && enemy_pressure(position, 1) > hp * 0.55f) {
            if (me.is_recalling()) me.cancel_recall(); else me.cancel_submit_gems();
            bank_altar = {-1, -1};
            target = -1;
            escape();
        }
        return;
    }
    if (!me.can_move() && !me.can_attack() && !me.can_cast()) return;
    if (last_gems != int(me.gem_count())) last_progress = round;
    last_gems = int(me.gem_count());
    if (round - last_progress > 16) {
        // A stale/unreachable objective must never disable a unit for the rest
        // of a match. Capture waits and channels have their own progress paths.
        if (target >= 0) groups[target].failed_until = round + 30;
        target = -1; decision_round = -100; reserve(-1);
        route_valid = bridge_valid = choice_ready = false;
        bank_altar = {-1, -1}; bank_plan_round = -100;
        if (hp > max_hp * 0.55f) recovering = false;
        last_progress = round;
        if (banking && enemy_pressure(position, 2) == 0 && round - last_recall > 30 && !spawn) {
            me.recall(); last_recall = round; return;
        }
    }
    // Finishing a camp is often safer than abandoning it, but never gamble the bag.
    float remaining = target >= 0 ? groups[target].health / estimated_dps(groups[target]) : 100;
    float camp_danger = incoming(position) - enemy_pressure(position, 1);
    if (strength_shards >= 5) camp_danger *= 0.7f;
    if (health_shards >= 5) camp_danger *= 0.9f;
    const bool urgent = endangered() || (camp_danger > 0 && protection() < std::max(130.0f, camp_danger * 2.5f));
    const bool low = hp < max_hp * 0.30f && remaining > 2;
    if (!spawn && (recovering || urgent || low)) {
        recovering = true; target = -1;
        retreat();
        return;
    }
    if (target >= 0 && groups[target].state == 2) {
        for (auto tile : groups[target].tiles)
            if (tile == position) {
                auto& group = groups[target];
                reserve(target);
                if (group.capture_wait_from < 0) { group.capture_wait_from = round; last_progress = round; }
                if (round - group.capture_wait_from <= bc::constants::capture_time + 2) { combat(); return; }
                // A connected region may span the map. An unseen opponent on
                // any member prevents capture, so occupying one tile forever
                // is not progress. Sweep toward the far side and clear its
                // defender rather than declaring every waiting turn productive.
                vec2 patrol = tile;
                int farthest = home_values[tile.y * width + tile.x];
                for (auto candidate : group.tiles) {
                    const int home = home_values[candidate.y * width + candidate.x];
                    if (!terrain.is_set(candidate) && home != bc::distance_field::unreachable && home > farthest) {
                        farthest = home; patrol = candidate;
                    }
                }
                if (has_budget(12000)) combat(true, true);
                if (has_budget(12000) && me.is_alive() && me.can_move() && patrol != position)
                    navigate(around(patrol, 0), true, false);
                if (has_budget(10000) && me.is_alive()) combat(true, true);
                return;
            }
    }
    if (me.gem_count() == 0) { banking = false; bank_altar = {-1, -1}; }
    const int bank_distance = altar_values[position.y * width + position.x];
    const bool exposed_bag = me.gem_count() >= 22 && round - last_contact < 20 && bank_distance <= 20
        && (hp < max_hp * 0.65f || enemy_pressure(position, 2) > hp * 0.25f);
    const bool convenient_bag = me.gem_count() >= 30 && bank_distance <= 10;
    if (me.gem_count() > 0 && (banking || me.gem_count() >= 75 || bank_distance <= 3 || exposed_bag || convenient_bag
        || me.gem_count() + bc::global_submitted_gems(bc::team::ally) >= bc::constants::winning_gems
        || round + std::min(bank_distance, width + height) + int(me.gem_count() / 10) + 15 >= int(bc::constants::max_rounds))) {
        if (!banking) bank_altar = {-1, -1};
        banking = true; target = -1; bank(); return;
    }
    // Survival and deposits take precedence over opportunistic pursuit.
    if (!recovering && engage()) return;
    // Recall for a meaningful power spike, not after every small gold reward.
    if (!spawn && type != bc::champion_type::tavi && me.gold_count() >= (strength_shards < 5 ? 150u : 240u)
        && round - last_recall > 18 && (target < 0 || groups[target].state == 3 || remaining > 6) && me.gem_count() < 40) {
        recovering = true; target = -1; retreat(); return;
    }
    if (target < 0 || groups[target].state == 3 || groups[target].failed_until > round || round - decision_round >= 35) {
        const int next = choose_group();
        if (next >= 0 || target < 0 || groups[target].state == 3) target = next;
    }
    if (target < 0) {
        reserve(-1);
        if (me.gem_count()) { banking = true; bank(); }
        return;
    }
    reserve(target);
    if (groups[target].state == 2) {
        if (!navigate(group_goals(target), true, enemy_pressure(groups[target].center, 2) > hp * 0.35f)
            && enemy_pressure(position, 2) > protection() * 0.5f) escape();
        combat();
        return;
    }
    if (!has_budget(10000)) return;
    combat();
    if (!me.is_alive()) { reserve(-1); return; }
    if (!has_budget(12000)) return;
    // Fresh occupancy after combat allows moving onto a newly cleared tile.
    bool alive = false;
    for (const auto& camp : camps) if (camp.group == target && bc::vision::has_camp(camp.position)) { alive = true; break; }
    if (!alive && groups[target].seen_round == round) {
        const auto visible = bc::vision::visible_tiles();
        bool group_visible = false;
        for (auto tile : groups[target].tiles) group_visible |= visible.is_set(tile);
        if (group_visible && has_budget(8000)) {
            const auto state = groups[target].view.state();
            if (std::holds_alternative<bc::map::camp_group_reward_available>(state)) groups[target].state = 2;
        }
    }
    if (me.can_move()) {
        const bool caution = !enemies.empty() && enemy_pressure(groups[target].center, 2) > protection() * 0.65f
            && (me.gem_count() >= 22 || hp < max_hp * 0.7f);
        if (!navigate(group_goals(target), true, caution) && caution && !groups[target].tiles.empty()
            && distance(position, groups[target].center) > attack_range + 1) {
            target = -1; decision_round = -100; reserve(-1);
            if (enemy_pressure(position, 2) > 0) escape();
        }
    }
    if (!has_budget(10000)) return;
    if (me.can_attack() || me.can_cast()) combat();
    if (bc::fuel_remaining() > 20000) ward();
}
