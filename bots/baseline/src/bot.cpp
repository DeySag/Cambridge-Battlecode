#include <cambc.h>
#include "bot.h"
#include <algorithm>
#include <limits>
#include <optional>
#include <variant>
#include <type_traits>
namespace bc = cambc;
using bc::vec2;

namespace {
bc::bitboard around(vec2 p, int radius) {
    bc::bitboard result;
    result.draw_chebyshev(p, radius);
    return result;
}
bc::bitboard blocked() {
    auto result = bc::map::walls() | bc::map::holes() | bc::map::altars()
        | bc::map::spawn_region(bc::team::enemy) | bc::vision::champion_tiles()
        | bc::vision::camp_tiles() | bc::vision::teleport_tiles();
    result.set(bc::self().position(), false);
    return result;
}
bool walk(bc::bitboard goals) {
    auto& me = bc::self();
    if (!me.can_move()) return false;
    auto walls = blocked();
    goals &= ~walls;
    if (goals.is_empty() || goals.is_set(me.position())) return false;
    bc::distance_field field;
    bc::bitboard visited;
    field.compute_to(walls, goals, around(me.position(), 0), visited);
    for (auto delta : field.next_moves_from(me.position()))
        if (me.move(delta)) return true; // Hidden blockers preserve the allowance.
    return false;
}
// Basic attack first, then reset and attack again, or cast a damage spell.
template<class Target>
void fight(const Target& target, bool walked = false) {
    auto& me = bc::self();
    bool attacked = false;
    if (target.is_visible() && target.health() > 0 && me.can_attack()
        && me.attack_tiles().is_set(target.position())) {
        me.attack(target);
        attacked = true;
    }
    if (!target.is_visible() || target.health() <= 0) return;
    for (auto ability : me.abilities()) {
        if (!me.can_cast() || !ability.is_ready()) continue;
        if (ability.is_attack_reset() && attacked) {
            ability.use();
            if (target.is_visible() && target.health() > 0 && me.can_attack()) me.attack(target);
            return;
        }
        if (ability.is_shield() && me.health() < me.max_health() * 0.7f) {
            ability.use();
            return;
        }
        if (!(ability.is_damage() || ability.is_cc() || ability.is_aoe_damage())) continue;
        if (!ability.valid_tiles().is_set(target.position())) continue;
        if (std::visit([](const auto& info) { return info.exclusive_with_movement; }, *ability.info())
            && walked) continue;
        if constexpr (std::is_same_v<Target, bc::camp>) {
            if (!ability.can_target_camps() && !ability.can_target_ground()) continue;
        } else {
            if (!ability.can_target_enemies() && !ability.can_target_ground()) continue;
        }
        if (ability.can_target_ground()) ability.use(target.position());
        else ability.use(target);
        return;
    }
}
bool bank() {
    auto& me = bc::self();
    bc::bitboard goals;
    for (auto altar : bc::map::altars().positions()) {
        const bool enemy_lock = bc::vision::has_altar_lock(altar)
            && bc::vision::altar_lock_at(altar).is_enemy();
        if (me.position().chebyshev_distance(altar) <= 1) {
            me.submit_gems(altar); // Enemy locks turn this into a basic attack.
            return true;
        }
        if (!enemy_lock) goals |= around(altar, 1);
    }
    if (goals.is_empty())
        for (auto altar : bc::map::altars().positions()) goals |= around(altar, 1);
    return walk(goals);
}
}

void Bot::play_turn() {
    auto& me = bc::self();
    const bool spawn = me.is_alive() && bc::map::spawn_region(bc::team::ally).is_set(me.position());
    // Dead champions can shop too; Tavi's can_buy allows field purchases.
    while (me.gold_count() >= 30 && bc::fuel_remaining() > 10000) {
        const bool health = health_shards < 5 || (damage_shards >= 10 && health_shards * 2 < damage_shards);
        const bool caster = me.champion_name() == bc::champion_type::stellar
            || me.champion_name() == bc::champion_type::prism;
        const auto item = health ? bc::item_type::jungle_shard_of_health
            : caster ? bc::item_type::monster_shard_of_focus : bc::item_type::monster_shard_of_strength;
        if (!me.can_buy(item)) break;
        me.buy(item);
        if (health) ++health_shards; else ++damage_shards;
    }
    if (!me.is_alive() || me.is_recalling() || me.is_submitting_gems() || me.is_teleporting()) return;
    if (spawn && recovering) {
        recovering = false;
        return; // Spawn recovery happens at turn end: do not walk away early.
    }
    if (!me.can_move() && !me.can_attack() && !me.can_cast()) return;
    me.set_priority(100);
    if (!spawn && (recovering || me.health() < me.max_health() * 0.45f || me.gold_count() >= 150)) {
        recovering = true;
        // Reach our spawn on foot if an enemy could repeatedly interrupt recall.
        bool threatened = !bc::vision::champions(bc::team::enemy).empty();
        for (const auto& camp : bc::vision::camps())
            if (camp.strength() > 0 && camp.position().chebyshev_distance(me.position()) <= camp.attack_range()) threatened = true;
        if (threatened) walk(bc::map::spawn_region(bc::team::ally));
        else me.recall();
        return;
    }
    if (me.gem_count() > 0) {
        bank();
        return;
    }
    for (const auto& enemy : bc::vision::champions(bc::team::enemy))
        if (me.attack_tiles().is_set(enemy.position()) && enemy.health() < me.health() * 0.6f) {
            fight(enemy);
            return;
        }

    // One distance field per turn prices every camp using actual walking distance.
    const auto walls = blocked();
    bc::distance_field distances;
    bc::bitboard visited;
    distances.compute(walls, around(me.position(), 0), visited);
    std::optional<bc::map::camp_group> choice;
    bc::bitboard chosen_goals;
    float best = -1;
    const auto visible = bc::vision::visible_tiles();
    for (const auto& group : bc::map::camp_groups()) {
        const auto tiles = group.spawn_tiles();
        const bool seen = !(tiles & visible).is_empty();
        const bool reward = seen && std::holds_alternative<bc::map::camp_group_reward_available>(group.state());
        if (seen && std::holds_alternative<bc::map::camp_group_respawning>(group.state())) continue;
        bool monolith = false;
        for (const auto& camp : group.camps()) monolith |= camp.is_monolith();
        if (monolith) continue; // Baseline cannot safely solo this boss.
        if (reward && tiles.is_set(me.position())) return; // Hold through all capture phases.
        auto goals = reward ? tiles : bc::bitboard{};
        if (!reward) for (auto tile : tiles.positions()) goals |= around(tile, seen ? int(me.attack_range()) : 1);
        goals &= ~walls;
        int nearest = std::numeric_limits<int>::max();
        for (auto tile : goals.positions())
            if (visited.is_set(tile)) nearest = std::min(nearest, int(distances.get(tile)));
        if (nearest == std::numeric_limits<int>::max()) continue;
        float health = 0, incoming = 0;
        for (const auto& camp : group.camps()) {
            health += camp.is_visible() ? std::max(0.0f, camp.health()) : camp.base_stats().base_health;
            incoming += camp.base_stats().base_strength;
        }
        const float damage_per_turn = me.strength() * 1.25f + me.focus() * 0.15f;
        // Avoid solo camps whose retaliation would consume most of our health.
        const float risk = incoming * health / std::max(1.0f, damage_per_turn);
        float score = (group.gem_value() + group.gold_value() * 0.2f + 1)
            / (nearest + health / std::max(1.0f, damage_per_turn) + 6);
        if (!reward && risk > me.health() * 0.7f) score *= 0.2f;
        // Visible allies near this camp reduce duplication without sharing hidden vision.
        for (const auto& ally : bc::vision::champions(bc::team::ally))
            if (ally != me && ally.is_alive())
                for (auto tile : tiles.positions())
                    if (ally.position().chebyshev_distance(tile) <= 2) { score *= 0.5f; break; }
        if (score > best) { best = score; choice = group; chosen_goals = goals; }
    }
    if (!choice) return;
    bool live_visible = false;
    for (const auto& camp : choice->camps()) live_visible |= camp.is_visible() && camp.health() > 0;
    if (!live_visible && (choice->spawn_tiles() & visible).is_empty()) {
        chosen_goals.clear();
        for (const auto& camp : choice->camps()) chosen_goals |= around(camp.spawn_position(), 1);
    }
    for (const auto& camp : choice->camps())
        if (camp.is_visible() && camp.health() > 0 && me.position().chebyshev_distance(camp.position()) <= 4) {
            fight(camp);
            if (!me.can_move()) return;
            break;
        }
    const bool walked = walk(chosen_goals);
    for (const auto& camp : choice->camps())
        if (camp.is_visible() && camp.health() > 0 && me.attack_tiles().is_set(camp.position())) {
            fight(camp, walked);
            break;
        }
}
