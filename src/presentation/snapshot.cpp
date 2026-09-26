#include "daggorath/snapshot.hpp"

namespace dag {

ViewSnapshot snapshot_from(const Game& game) {
    ViewSnapshot view;
    view.row = game.player().row;
    view.col = game.player().col;
    view.dir = static_cast<int>(game.player().dir);
    view.regular_light = game.player().regular_light;
    view.magic_light = game.player().magic_light;
    view.mode = static_cast<int>(game.display_mode());
    view.map_features = game.player().map_features;
    view.level = game.level_index();
    view.maze = game.maze();
    static constexpr int dr[4] = {-1, 0, 1, 0};
    static constexpr int dc[4] = {0, 1, 0, -1};
    int r = view.row;
    int c = view.col;
    for (int i = 0; i < 5; ++i) {
        if (r < 0 || c < 0 || r >= 32 || c >= 32) view.ahead[i] = 0xFF;
        else view.ahead[i] = game.maze().at(r, c);
        r += dr[view.dir & 3];
        c += dc[view.dir & 3];
    }
    for (const Ccb& ccb : game.creatures()) {
        if (!ccb.in_use) continue;
        view.creatures.push_back(
            SeenCreature{ccb.row, ccb.col, ccb.type, ccb.magic_offense});
    }
    for (const Ocb& o : game.objects()) {
        view.objects.push_back(SeenObject{o.row, o.col, o.level, o.owner, o.cls});
    }
    return view;
}

}  // namespace dag
