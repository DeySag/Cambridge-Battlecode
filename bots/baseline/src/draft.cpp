#include <cambc.h>
#include "draft.h"
#include <algorithm>
#include <array>

void play_draft() {
    using D = cambc::champion_draft;
    // Prefer independent, durable bodies. Linked deaths and the patched caster
    // growth make the old fixed Cassi/Stellar opening unnecessarily fragile.
    int normal_gems = 0, members = 0, count = 0;
    for (const auto& group : cambc::map::camp_groups()) {
        if (std::any_of(group.begin(), group.end(), [](const auto& camp) { return camp.is_monolith(); })) continue;
        normal_gems += int(group.gem_value());
        members += int(group.camps().size());
        ++count;
    }
    const bool dense = members > count * 1.5f;
    const std::array ordinary{dense ? D::varg : D::tavi, dense ? D::tavi : D::varg,
        D::heart_of_ruin, D::marcella, D::vesper_vale};
    // Range-four spells no longer outrange monoliths. Establish an economy and
    // field a shielded frontline instead of repeatedly sacrificing spellcasters.
    const std::array siege{D::varg, D::tavi, D::willow, D::heart_of_ruin, D::marcella};
    const auto& roster = normal_gems ? ordinary : siege;
    const auto chosen = cambc::draft::current_draft(cambc::team::ally);
    for (auto pick : roster) {
        if (std::find(chosen.begin(), chosen.end(), pick) == chosen.end()) {
            cambc::draft::pick(pick);
            return;
        }
    }
}
