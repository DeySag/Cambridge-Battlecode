#include <cambc.h>
#include "draft.h"
#include <algorithm>
#include <array>

void play_draft() {
    using D = cambc::champion_draft;
    // Keep v3's normal farming core. Three-pick teams gain mobile ranged
    // damage and bonus gold; boss economies need a durable control frontline.
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
    auto adjusted = ordinary;
    if (cambc::draft::team_size() == 3) adjusted = std::array{D::varg, D::vesper_vale, D::heart_of_ruin, D::marcella, D::tavi};
    const std::array siege_control{D::varg, D::heart_of_ruin, D::marcella, D::ryth, D::vesper_vale};
    const auto& roster = normal_gems ? adjusted : siege_control;
    const auto chosen = cambc::draft::current_draft(cambc::team::ally);
    for (auto pick : roster) {
        if (std::find(chosen.begin(), chosen.end(), pick) == chosen.end()) {
            cambc::draft::pick(pick);
            return;
        }
    }
}
