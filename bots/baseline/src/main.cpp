// Each champion keeps its own Bot across turns. Errors are visible in replay logs.
#include <cambc.h>
#include <iostream>
#include "bot.h"
#include "draft.h"

int main() {
    Bot bot;
    while (true) {
        const auto start_round = cambc::round_number();
        try {
            if (cambc::is_draft()) play_draft();
            else bot.play_turn();
        } catch (const std::exception& error) {
            std::cout << "bot error stage=" << bot.stage << " round=" << cambc::round_number();
            if (!cambc::is_draft()) {
                auto& me = cambc::self();
                std::cout << " champion=" << cambc::to_string(me.champion_name())
                    << " alive=" << me.is_alive();
                if (me.is_alive()) std::cout << " position=" << cambc::to_string(me.position());
            }
            std::cout << ": " << error.what() << std::endl;
        } catch (...) {
            std::cout << "bot error: unknown exception" << std::endl;
        }
        if (cambc::round_number() != start_round) {
            std::cout << "fuel spill stage=" << bot.stage << " " << start_round << " -> " << cambc::round_number() << std::endl;
            continue; // Resumed during another turn: rebuild its observations now.
        }
        cambc::end_turn();
    }
}
