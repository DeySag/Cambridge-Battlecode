#include <cambc.h>
#include "draft.h"
#include <algorithm>
#include <array>

void play_draft() {
    using D = cambc::champion_draft;
    // Splash wins dense camps; Vendetta's high attack wins isolated camps.
    int members = 0, groups = 0, normal_gems = 0;
    for (const auto& group : cambc::map::camp_groups()) {
        if (std::any_of(group.begin(), group.end(), [](const auto& camp) { return camp.is_monolith(); })) continue;
        normal_gems += int(group.gem_value());
        members += int(group.camps().size());
        ++groups;
    }
    const bool dense = members > groups * 1.35f;
    const std::array ordinary{dense ? D::varg : D::vendetta,
        dense ? D::vendetta : D::varg, D::ren, D::tavi, D::vesper_vale};
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
