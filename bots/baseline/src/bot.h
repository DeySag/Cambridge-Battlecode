#pragma once
#include <cambc.h>
#include <array>
#include <optional>
#include <vector>

// Persistent state belongs to this champion, not to the whole team.
class Bot {
    struct Group {
        cambc::map::camp_group view;
        std::vector<cambc::vec2> tiles;
        cambc::vec2 center{};
        float full_health = 0, health = 0, damage = 0;
        int gems = 0, gold = 0, state = 0, ready_round = 0, seen_round = -1;
        bool monolith = false;
    };
    struct Camp {
        cambc::camp view;
        cambc::vec2 position;
        float health, damage;
        int range, group;
        bool awake;
    };
    struct Enemy {
        struct Spell { float damage = 0; int range = 0, cooldown = 0; bool stun = false; };
        cambc::champion view;
        cambc::vec2 position;
        float health, strength;
        int range;
        std::array<Spell, 2> spells{};
        bool stunned = false;
    };
    struct Danger { cambc::vec2 position; int expires; };
    std::vector<Group> groups;
    std::vector<Camp> camps;
    std::vector<Enemy> enemies;
    std::vector<Danger> dangers;
    cambc::bitboard terrain;
    cambc::distance_field altar_distance, home_distance, route_field;
    cambc::bitboard route_goals, route_visited;
    int route_round = -100;
    bool route_valid = false, route_avoid_danger = false;
    std::vector<std::uint16_t> choice_distances;
    std::vector<std::uint8_t> choice_walls;
    cambc::vec2 choice_origin{};
    int choice_round = -100, choice_cursor = 0, choice_best = -1;
    float choice_score = -1;
    bool choice_ready = false;
    std::array<unsigned char, 8> reservations{};
    int slot = -1, target = -1, round = 0, decision_round = -100;
    int health_shards = 0, strength_shards = 0;
    cambc::vec2 position{};
    cambc::vec2 bank_altar{-1, -1};
    int bank_plan_round = -100, last_contact = -100;
    cambc::champion_type type{};
    float hp = 0, max_hp = 0, strength = 0, focus = 0;
    int attack_range = 1, width = 0, height = 0;
    std::vector<std::uint16_t> altar_values, home_values;
    bool boss_only = false;
    bool initialized = false, banking = false, recovering = false, moved = false, attacked = false;
    bool was_alive = false;

    bool has_budget(unsigned minimum = 6000) const;
    void initialize();
    void observe();
    void reserve(int group);
    void shop();
    cambc::bitboard blocked(bool avoid_danger = false) const;
    cambc::bitboard group_goals(int group) const;
    bool navigate(cambc::bitboard goals, bool dash = true, bool avoid_danger = false);
    bool bank();
    bool escape();
    bool engage();
    bool retreat();
    bool combat(bool allow_enemies = true, bool defensive = false);
    void ward();
    float incoming(cambc::vec2 position, int group = -1) const;
    float enemy_pressure(cambc::vec2 position, int turns = 2) const;
    float danger_memory(cambc::vec2 position) const;
    float protection() const;
    bool endangered() const;
    void remember_danger();
    float estimated_dps(const Group& group) const;
    int choose_group();
public:
    const char* stage = "startup";
    void play_turn();
};
