#pragma once
#include "daggorath/game.hpp"
#include "daggorath/render_state.hpp"

namespace dag {

// The cell underfoot and the next four cells along the facing, plus light and mode.
ViewSnapshot snapshot_from(const Game& game);

}  // namespace dag
