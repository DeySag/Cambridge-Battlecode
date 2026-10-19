#include <cambc.h>
#include "draft.h"
#include <algorithm>
#include <array>

void play_draft() {
    using D = cambc::champion_draft;
    // Pair splash farming with range-four spell pressure and the extra linked unit.
    int normal_gems = 0;
    for (const auto& group : cambc::map::camp_groups()) {
        if (std::any_of(group.begin(), group.end(), [](const auto& camp) { return camp.is_monolith(); })) continue;
        normal_gems += int(group.gem_value());
    }
    const std::array ordinary{D::varg, D::stellar, D::cassi_and_issac, D::ryth, D::tavi};
    const std::array siege{D::tavi, D::stellar, D::prism, D::cassi_and_issac, D::ren};
    const auto& roster = normal_gems ? ordinary : siege;
    const auto chosen = cambc::draft::current_draft(cambc::team::ally);
    for (auto pick : roster) {
        if (std::find(chosen.begin(), chosen.end(), pick) == chosen.end()) {
            cambc::draft::pick(pick);
            return;
        }
    }
}
