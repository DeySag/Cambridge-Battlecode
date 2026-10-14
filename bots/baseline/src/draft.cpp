#include <cambc.h>
#include "draft.h"
#include <array>
#include <algorithm>
void play_draft() {
    using D = cambc::champion_draft;
    const std::array roster{D::tavi, D::vesper_vale, D::stellar, D::prism, D::varg};
    const auto chosen = cambc::draft::current_draft(cambc::team::ally);
    for (auto pick : roster)
        if (std::find(chosen.begin(), chosen.end(), pick) == chosen.end()) {
            cambc::draft::pick(pick);
            return;
        }
}
