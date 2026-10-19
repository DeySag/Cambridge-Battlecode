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
    camps.clear(); enemies.clear();
    bc::shared_memory.read(0, std::span{reservations.data(), std::min<std::size_t>(8, bc::shared_memory.size())});
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
    for (const auto& enemy : bc::vision::champions(bc::team::enemy)) {
        if (!enemy.is_alive()) continue;
        Enemy seen{enemy, enemy.position(), enemy.health(), enemy.strength(), enemy.attack_range()};
        seen.stunned = enemy.buffs().has_stun();
        const auto abilities = enemy.abilities();
        for (int i = 0; i < 2; ++i) {
            const auto& ability = abilities[i];
            if (!ability.info()) continue;
            auto& spell = seen.spells[i];
            spell.cooldown = ability.cooldown();
            if (ability.is_attack_reset()) { spell.damage = seen.strength; spell.range = seen.range; }
            else if (ability.is_damage() || ability.is_cc() || ability.is_aoe_damage()) {
                spell.damage = ability.damage_amount();
                spell.stun = ability.is_cc();
                std::visit([&](const auto& info) {
                    if constexpr (requires { info.range; }) spell.range = int(std::ceil(info.range));
                    if constexpr (requires { info.radius; }) spell.range += int(std::ceil(info.radius));
                    if constexpr (requires { info.duration; }) spell.stun |= info.duration > 0;
                }, *ability.info());
            }
        }
        enemies.push_back(seen);
    }
    if (!enemies.empty()) last_contact = round;
    std::erase_if(dangers, [&](const auto& danger) { return danger.expires <= round; });
}

void Bot::shop() {
    stage = "shop";
    auto& me = bc::self();
    // Power increases both farming and player damage. Add health gradually;
    // spellcasters buy Focus while basic attackers buy Strength.
    for (int n = 0; n < 40 && me.gold_count() >= 30; ++n) {
        const bool caster = (boss_only && type != bc::champion_type::issac) || type == bc::champion_type::stellar
            || type == bc::champion_type::cassi || type == bc::champion_type::prism || type == bc::champion_type::tavi;
        const bool health = !boss_only && strength_shards >= 5 && health_shards * 4 < strength_shards;
        const bc::item_type item = health ? bc::item_type::power_shard_of_health
            : caster ? bc::item_type::power_shard_of_focus : bc::item_type::power_shard_of_strength;
        if (!me.can_buy(item)) break;
        me.buy(item);
        if (health) ++health_shards; else ++strength_shards;
    }
    if (strength_shards >= 5 && me.can_buy(bc::consumable_type::altar_lock)) me.buy(bc::consumable_type::altar_lock);
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
        float basic = gap <= enemy.range + 1 ? enemy.strength * turns : 0;
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
    const auto buffs = bc::self().buffs();
    float shield = buffs.has_shield() ? buffs.get<bc::buff_shield>().value : 0;
    for (const auto& ability : bc::self().abilities())
        if (ability.is_shield() && ability.is_ready() && !buffs.has_shield()) shield += ability.shield_amount() * 0.8f;
    return hp + shield;
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

bool Bot::navigate(bc::bitboard goals, bool dash, bool avoid_danger) {
    stage = "navigate";
    if (!has_budget(15000)) return false;
    auto& me = bc::self();
    if (!me.can_move()) return false;
    auto walls = blocked(avoid_danger);
    if (goals.is_empty() || goals.is_set(position)) return false;
    // Reuse the reverse distance field while the objective is unchanged. Every
    // step is still checked against fresh occupancy and newly revealed blockers.
    if (width * height < 7000 || !route_valid || route_goals != goals || route_avoid_danger != avoid_danger || round - route_round > 8) {
        route_goals = goals;
        auto free_goals = goals & ~walls;
        if (free_goals.is_empty()) return false;
        route_field.compute_to(walls, free_goals, around(position, 0), route_visited);
        route_round = round;
        route_avoid_danger = avoid_danger;
        route_valid = true;
        if (!has_budget(6500)) return false;
    }
    if (!route_visited.is_set(position)) {
        route_valid = false;
        return false;
    }
    auto& field = route_field;
    auto& visited = route_visited;
    bool success = false;
    for (auto delta : field.next_moves_from(position)) {
        const auto next = position + delta;
        if (walls.is_set(next)) continue;
        if (!has_budget(1000) || !me.is_alive() || !me.can_move()) return false;
        if (me.move(delta)) { position += delta; moved = true; success = true; break; }
        walls |= blocked(); // A failed move can reveal several previously hidden units.
        walls.set(next);
    }
    if (!success) route_valid = false;
    if (!dash || !me.can_cast() || goals.is_set(position) || bc::fuel_remaining() < 22000) return success;
    for (const auto& ability : me.abilities()) {
        if (!ability.is_dash() || !ability.is_ready()) continue;
        // Legal two-step paths may cross holes, but cannot cross walls/units.
        auto dash_walls = bc::map::walls() | bc::map::altars() | bc::map::spawn_region(bc::team::enemy)
            | bc::vision::champion_tiles() | bc::vision::camp_tiles() | bc::vision::teleport_tiles();
        dash_walls.set(position, false);
        auto landings = dash_walls.reachable_tiles(position, 2) & ability.valid_tiles() & ~blocked(avoid_danger);
        std::vector<std::pair<int, vec2>> options;
        for (auto tile : landings.positions())
            if (visited.is_set(tile)) options.emplace_back(field.get(tile), tile);
        std::sort(options.begin(), options.end());
        const int current_distance = field.get(position);
        for (auto [cost, tile] : options) {
            if (cost + 1 >= current_distance) continue;
            if (!has_budget(1000) || !me.is_alive() || !me.can_cast()) return success;
            if (ability.use(tile)) { position = tile; return true; }
        }
    }
    return success;
}

bc::bitboard Bot::group_goals(int group_id) const {
    const auto& group = groups[group_id];
    bc::bitboard goals;
    if (group.state == 2) {
        for (auto tile : group.tiles) goals.set(tile);
    } else if (group.monolith) {
        for (auto tile : group.tiles) goals |= around(tile, 4) & ~around(tile, 3);
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
    if (group.monolith) {
        float modifier = 0;
        for (const auto& ability : me.abilities())
            if (ability.is_damage() || ability.is_cc() || ability.is_aoe_damage()) modifier += ability.damage_amount();
        return std::max(1.0f, modifier / 8.0f);
    }
    float damage = strength * 1.12f;
    if (type == bc::champion_type::varg)
        damage *= std::min(3.5f, 1.0f + (group.tiles.size() - 1) * 0.8f);
    if (type == bc::champion_type::tavi) damage += focus * 0.24f;
    if (type == bc::champion_type::ryth) damage += focus * 0.12f * std::min(3.0f, float(group.tiles.size()));
    return std::max(1.0f, damage * 1.1f);
}

int Bot::choose_group() {
    stage = "choose";
    auto& me = bc::self();
    // A large-map search may consume most of a turn. Retain its result and
    // finish scoring next turn instead of repeating the same unfinished search.
    const bool large_map = width * height >= 7000;
    if (!large_map || !choice_ready || choice_origin != position || round - choice_round > 3) {
        const auto walls = blocked();
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
        if (group.monolith && (!boss_only || type == bc::champion_type::issac)) continue;
        int travel = infinity, delivery = infinity;
        // Look at legal approach tiles rather than pathing onto live camps.
        for (auto tile : group.tiles) {
            const int radius = group.monolith ? 4 : 1;
            for (int dy = -radius; dy <= radius; ++dy) for (int dx = -radius; dx <= radius; ++dx) {
                if (group.monolith && group.state != 2 && std::max(std::abs(dx), std::abs(dy)) != 4) continue;
                const auto p = tile + vec2{dx, dy};
                if (p.x < 0 || p.y < 0 || p.x >= width || p.y >= height) continue;
                const int index = p.y * width + p.x;
                if (choice_walls[index] || choice_distances[index] == bc::distance_field::unreachable) continue;
                travel = std::min(travel, int(choice_distances[index]));
                delivery = std::min(delivery, int(altar_values[index]));
            }
        }
        if (travel >= infinity) continue;
        const int wait = group.state == 3 ? std::max(0, group.ready_round - round - travel) : 0;
        if (wait > 12) continue;
        const float dps = estimated_dps(group);
        const float health = group.state == 3 ? group.full_health : group.health;
        float clear = health / dps;
        int claims = 0;
        for (int j = 0; j < 8; ++j)
            if (j != slot && reservations[j] == i + 1) ++claims;
        // Do not send the entire team to the same unguarded resource.
        if (claims) clear /= 1.0f + claims * 0.7f;
        const float reward = group.gems + group.gold * (strength_shards < 10 ? 0.22f : 0.10f) + 1;
        float score = reward / (travel + clear + 6 + wait + delivery * 0.30f);
        if (claims) score *= group.monolith ? 0.8f : group.state == 2 ? 0.08f : 0.28f;
        // Retaliation falls as monsters die. Shielding gives Varg substantial sustain.
        float risk = group.monolith ? 0 : group.damage * clear * 0.60f;
        if (type == bc::champion_type::varg) risk = std::max(0.0f, risk - focus * 2.5f * clear / 8);
        if (risk > hp * 0.85f) score *= std::max(0.06f, hp * 0.55f / std::max(1.0f, risk));
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
    // Melee champions may clear an obstruction without delaying their delivery.
    if (attack_range == 1) combat(false);
    return true;
}

bool Bot::escape() {
    stage = "escape";
    auto& me = bc::self();
    if (!has_budget(10000) || !me.is_alive()) return false;
    // A shield is compatible with walking. Prefer it before taking another hit.
    for (const auto& ability : me.abilities())
        if (me.can_cast() && ability.is_shield() && ability.is_ready()
            && incoming(position) > 0 && !me.buffs().has_shield()) { ability.use(); break; }
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
            if (!has_budget(1000) || !me.is_alive() || !me.can_move()) return success;
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
                if (!has_budget(1000) || !me.is_alive() || !me.can_cast()) return success;
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
        && (me.can_move() || me.can_attack() || me.can_cast())) { stage = "retreat recall"; me.recall(); }
    return true;
}

bool Bot::engage() {
    auto& me = bc::self();
    if (!has_budget(18000) || enemies.empty() || banking || hp < max_hp * 0.42f || me.gem_count() >= 80) return false;
    if (target >= 0 && groups[target].state == 2 && !endangered()
        && std::find(groups[target].tiles.begin(), groups[target].tiles.end(), position) != groups[target].tiles.end()) return false;
    const Enemy* chosen = nullptr;
    float best = -1;
    for (const auto& enemy : enemies) {
        // Spawn heals can make an idle enemy an endless, unproductive target.
        if (bc::map::spawn_region(bc::team::enemy).is_set(enemy.position)) continue;
        if (distance(position, enemy.position) > 7) continue;
        float friendly = hp + strength * 4 + focus;
        float hostile = 0;
        for (const auto& ally : bc::vision::champions(bc::team::ally))
            if (ally != me && ally.is_alive() && distance(ally.position(), enemy.position) <= 6)
                friendly += ally.health() + ally.strength() * 4 + ally.focus();
        for (const auto& other : enemies)
            if (distance(other.position, enemy.position) <= 6) hostile += other.health + other.strength * 4 + other.view.focus();
        if (friendly < hostile * 0.92f && enemy.health > strength * 2 + focus * 1.5f) continue;
        const float score = 300 + enemy.view.gem_count() * 5 - enemy.health * 0.08f
            - distance(position, enemy.position) * 20;
        if (score > best) { best = score; chosen = &enemy; }
    }
    if (!chosen) return false;
    if (!has_budget(13000)) return false;
    target = -1; reserve(-1);
    bool acted = combat(true, true);
    if (me.is_alive() && has_budget(14000) && me.can_move()) acted |= navigate(around(chosen->position, attack_range));
    if (me.is_alive() && has_budget(10000)) acted |= combat(true, true);
    return acted;
}

bool Bot::combat(bool allow_enemies, bool defensive) {
    stage = "combat";
    if (!has_budget(12000)) return false;
    auto& me = bc::self();
    if (!me.is_alive()) return false;
    bool acted = false;
    // Select a profitable enemy already in reach, never chase across the map.
    const Enemy* foe = nullptr;
    if (allow_enemies) {
        float best = 0;
        for (const auto& enemy : enemies) {
            if (!enemy.view.is_visible()) continue;
            if (bc::map::spawn_region(bc::team::enemy).is_set(enemy.position)) continue;
            if (distance(position, enemy.position) > std::max(4, attack_range)) continue;
            const bool lethal = enemy.health <= strength * 2 + focus * 1.5f;
            if (!defensive && !lethal && (hp < max_hp * 0.35f || me.gem_count() >= 100)) continue;
            const float score = (lethal ? 1000.0f : 0) + enemy.view.gem_count() * 3 + 250 - enemy.health * 0.1f;
            if (score > best) { best = score; foe = &enemy; }
        }
    }
    auto select_camp = [&]() -> std::optional<bc::camp> {
        if (defensive) return std::nullopt;
        float best = -1;
        std::optional<bc::camp> chosen;
        for (const auto& camp : camps) {
            if (target >= 0 && camp.group != target) continue;
            if (distance(position, camp.position) > attack_range) continue;
            if (!bc::vision::has_camp(camp.position)) continue;
            const auto current = bc::vision::camp_at(camp.position);
            float score = std::min(strength, current.health());
            if (current.health() <= strength * 1.1f) score += 100;
            if (type == bc::champion_type::varg) {
                for (const auto& other : camps)
                    if (other.position != camp.position && distance(other.position, camp.position) <= 1
                        && bc::vision::has_camp(other.position)) score += std::min(strength, other.health);
            }
            score += camp.damage * 0.3f;
            if (score > best) { best = score; chosen = current; }
        }
        return chosen;
    };
    if (me.can_attack()) {
        if (foe && foe->view.is_visible() && distance(position, foe->position) <= attack_range) {
            me.attack(foe->view); attacked = true; acted = true;
        } else if (const auto camp = select_camp(); camp && has_budget(5000) && me.is_alive() && me.can_attack()) {
            me.attack(*camp); attacked = true; acted = true;
        }
    }
    if (!me.is_alive() || !me.can_cast()) return acted;
    // Score reset, damage, CC, area hits and sustain in comparable damage units.
    int chosen_slot = -1;
    std::optional<vec2> ground;
    std::optional<bc::camp> unit_camp;
    std::optional<bc::champion> unit_enemy;
    float best = 0;
    const float danger = incoming(position, target);
    const auto abilities = me.abilities();
    for (int slot_id = 0; slot_id < 2; ++slot_id) {
        if (!has_budget(7000)) break;
        const auto& ability = abilities[slot_id];
        if (!ability.is_ready() || (moved && exclusive(ability))) continue;
        float score = 0;
        std::optional<vec2> p;
        std::optional<bc::camp> c;
        std::optional<bc::champion> e;
        if (ability.is_attack_reset()) {
            if (attacked && !me.can_attack()) {
                if (foe && foe->view.is_visible() && distance(position, foe->position) <= attack_range) score = strength;
                else if (const auto candidate = select_camp()) {
                    c = candidate;
                    score = std::min(candidate->health(), strength);
                    if (type == bc::champion_type::varg)
                        for (const auto& other : camps)
                            if (other.position != candidate->spawn_position() && distance(other.position, candidate->spawn_position()) <= 1
                                && bc::vision::has_camp(other.position)) score += std::min(other.health, strength);
                }
            }
        } else if (ability.is_shield()) {
            if (danger > 0 && !me.buffs().has_shield()) {
                score = std::min(ability.shield_amount(), danger * 5) * (hp < max_hp * 0.65f ? 1.3f : 0.8f);
            }
        } else if (ability.is_damage() || ability.is_cc() || ability.is_aoe_damage()) {
            const auto mask = ability.valid_tiles();
            const float damage = ability.damage_amount();
            if (foe && foe->view.is_visible() && (ability.can_target_enemies() || ability.can_target_ground()) && mask.is_set(foe->position)) {
                if (ability.can_target_ground()) p = foe->position;
                else e = foe->view;
                score = std::min(foe->view.health(), damage) * 1.3f
                    + (ability.is_cc() ? (defensive ? 100 + foe->strength : 35) : 0);
            }
            for (const auto& camp : camps) {
                if (defensive) break;
                if (bc::fuel_remaining() < 8000) break;
                if (target >= 0 && camp.group != target) continue;
                if (!mask.is_set(camp.position) || !bc::vision::has_camp(camp.position)) continue;
                float value = std::min(bc::vision::camp_at(camp.position).health(), damage);
                if (ability.is_cc()) value += camp.damage;
                if (ability.can_target_ground()) {
                    for (const auto& other : camps)
                        if (other.position != camp.position && distance(camp.position, other.position) <= 1
                            && bc::vision::has_camp(other.position)) value += std::min(other.health, damage);
                }
                if (value > score) {
                    score = value; e.reset();
                    if (ability.can_target_ground()) { p = camp.position; c.reset(); }
                    else if (ability.can_target_camps()) { c = bc::vision::camp_at(camp.position); p.reset(); }
                    else score = 0;
                }
            }
        }
        if (score > best) { best = score; chosen_slot = slot_id; ground = p; unit_camp = c; unit_enemy = e; }
    }
    if (chosen_slot < 0) return acted;
    if (!has_budget(4000) || !me.is_alive() || !me.can_cast()) return acted;
    const auto& ability = abilities[chosen_slot];
    if (ability.is_attack_reset() || ability.is_shield()) ability.use();
    else if (ground) ability.use(*ground);
    else if (unit_camp) ability.use(*unit_camp);
    else if (unit_enemy) ability.use(*unit_enemy);
    else return acted;
    acted = true;
    if (me.is_alive() && ability.is_attack_reset() && me.can_attack()) {
        if (foe && foe->view.is_visible() && foe->view.is_alive() && distance(position, foe->position) <= attack_range) me.attack(foe->view);
        else if (const auto camp = select_camp()) me.attack(*camp);
    }
    return acted;
}

void Bot::ward() {
    stage = "ward";
    auto& me = bc::self();
    if ((round + slot) % 8 != 0 || me.ward_cooldown() > 0) return;
    auto legal = me.ward_placement_tiles() & ~terrain & ~bc::vision::ward_tiles(bc::team::ally)
        & ~bc::vision::champion_tiles() & ~bc::vision::camp_tiles();
    const vec2 goal = target >= 0 ? groups[target].center : position;
    auto tiles = legal.positions();
    if (tiles.empty()) return;
    const auto best = *std::min_element(tiles.begin(), tiles.end(), [&](auto a, auto b) { return distance(a, goal) < distance(b, goal); });
    me.place_ward(best);
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
    }
    if (!me.is_alive()) {
        if (was_alive) remember_danger();
        was_alive = false;
        reserve(-1); target = -1; recovering = false; banking = false;
        bank_altar = {-1, -1}; route_valid = false; choice_ready = false; return;
    }
    was_alive = true;
    const bool spawn = bc::map::spawn_region(bc::team::ally).is_set(position);
    if (me.is_teleporting()) { reserve(-1); return; }
    if (spawn) {
        recovering = false;
        if (hp < max_hp * 0.99f) { reserve(-1); return; }
    }
    observe();
    if (!has_budget(12000)) return;
    if (me.is_recalling() || me.is_submitting_gems()) {
        reserve(-1);
        const int remaining = me.remaining_channel_time();
        const float threat = enemy_pressure(position, std::min(5, remaining + 1));
        // Damage does not cancel tribute. Keep a nearly completed, survivable channel.
        if (threat > protection() * 0.85f && remaining > 1) {
            if (me.is_recalling()) me.cancel_recall(); else me.cancel_submit_gems();
            bank_altar = {-1, -1};
            target = -1;
            escape();
        }
        return;
    }
    if (!me.can_move() && !me.can_attack() && !me.can_cast()) return;
    if (!recovering && engage()) return;
    // Finishing a camp is often safer than abandoning it, but never gamble the bag.
    float remaining = target >= 0 ? groups[target].health / estimated_dps(groups[target]) : 100;
    const float camp_danger = incoming(position, target) - enemy_pressure(position, 1);
    const bool urgent = endangered() || (camp_danger > 0 && hp < std::max(130.0f, camp_danger * 2.2f));
    const bool low = hp < max_hp * 0.28f && remaining > 3;
    if (!spawn && (recovering || urgent || low)) {
        recovering = true; target = -1;
        retreat();
        return;
    }
    if (target >= 0 && groups[target].state == 2) {
        for (auto tile : groups[target].tiles)
            if (tile == position) { reserve(target); combat(); return; }
    }
    if (me.gem_count() == 0) { banking = false; bank_altar = {-1, -1}; }
    const int bank_distance = altar_values[position.y * width + position.x];
    const bool exposed_bag = me.gem_count() >= 44 && round - last_contact < 20 && bank_distance <= 18
        && (hp < max_hp * 0.65f || enemy_pressure(position, 2) > hp * 0.25f);
    const bool convenient_bag = me.gem_count() >= 50 && bank_distance <= 10;
    if (me.gem_count() > 0 && (banking || me.gem_count() >= 100 || bank_distance <= 3 || exposed_bag || convenient_bag
        || me.gem_count() + bc::global_submitted_gems(bc::team::ally) >= bc::constants::winning_gems
        || round + bank_distance + int(me.gem_count() / 10) + 15 >= int(bc::constants::max_rounds))) {
        if (!banking) bank_altar = {-1, -1};
        banking = true; target = -1; bank(); return;
    }
    // Recall for a meaningful power spike, not after every small gold reward.
    if (!spawn && type != bc::champion_type::tavi && me.gold_count() >= 300
        && (target < 0 || groups[target].state == 3) && me.gem_count() < 60) {
        recovering = true; target = -1; retreat(); return;
    }
    if (target < 0 || groups[target].state == 3 || round - decision_round >= 30) {
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
        const auto state = groups[target].view.state();
        if (std::holds_alternative<bc::map::camp_group_reward_available>(state)) groups[target].state = 2;
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
