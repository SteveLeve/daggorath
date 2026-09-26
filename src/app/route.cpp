// Prints a keystroke script that walks to the nearest floor object of a type.
// Usage: route <type> [--second N]
#include "daggorath/game.hpp"
#include "daggorath/maze.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Node {
    int row = 0;
    int col = 0;
    int parent = -1;
};

void emit_line(std::uint64_t& jiffy, const std::string& text) {
    for (char ch : text) {
        std::cout << jiffy << ' ' << (ch == ' ' ? "SPACE" : std::string(1, ch)) << '\n';
        ++jiffy;
    }
    std::cout << jiffy << " CR\n";
    jiffy += 15;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: route <object-type> [--second N]\n";
        return 2;
    }
    const int want = std::atoi(argv[1]);
    int second = 1;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--second" && i + 1 < argc) second = std::atoi(argv[++i]);
    }
    dag::Game game(static_cast<std::uint8_t>(second), 0);
    game.set_frozen(true);
    int goal_row = -1;
    int goal_col = -1;
    int best = 1 << 20;
    for (const dag::Ocb& object : game.objects()) {
        if (object.level != 0 || object.type != want) continue;
        if (object.row == 0 && object.col == 0) continue;
        const int dist = std::abs(object.row - game.player().row) + std::abs(object.col - game.player().col);
        if (dist < best) {
            best = dist;
            goal_row = object.row;
            goal_col = object.col;
        }
    }
    bool carried = false;
    if (goal_row < 0) {
        for (int slot = 0; slot < dag::kCcbSlots; ++slot) {
            const dag::Ccb& creature = game.creatures()[static_cast<std::size_t>(slot)];
            if (!creature.in_use) continue;
            for (int obj = creature.object_head; obj >= 0;
                 obj = game.objects()[static_cast<std::size_t>(obj)].next) {
                if (game.objects()[static_cast<std::size_t>(obj)].type != want) continue;
                const int dist = std::abs(creature.row - game.player().row) +
                                 std::abs(creature.col - game.player().col);
                if (dist < best) {
                    best = dist;
                    goal_row = creature.row;
                    goal_col = creature.col;
                    carried = true;
                }
            }
        }
    }
    if (goal_row < 0) {
        std::cerr << "no placed object of type " << want << " on level 0\n";
        return 1;
    }

    std::vector<Node> nodes;
    std::vector<int> queue;
    std::vector<char> seen(32 * 32, 0);
    nodes.push_back({game.player().row, game.player().col, -1});
    queue.push_back(0);
    seen[game.player().row * 32 + game.player().col] = 1;
    int found = -1;
    for (std::size_t qi = 0; qi < queue.size() && found < 0; ++qi) {
        const Node here = nodes[static_cast<std::size_t>(queue[qi])];
        if (here.row == goal_row && here.col == goal_col) {
            found = queue[qi];
            break;
        }
        for (int dir = 0; dir < 4; ++dir) {
            int nr = 0;
            int nc = 0;
            if (!dag::step_ok(game.maze(), here.row, here.col, static_cast<dag::Dir>(dir), nr, nc))
                continue;
            const int key = nr * 32 + nc;
            if (seen[static_cast<std::size_t>(key)]) continue;
            seen[static_cast<std::size_t>(key)] = 1;
            nodes.push_back({nr, nc, queue[qi]});
            queue.push_back(static_cast<int>(nodes.size()) - 1);
        }
    }
    if (found < 0) {
        std::cerr << "no walk to type " << want << "\n";
        return 1;
    }
    std::vector<Node> path;
    for (int n = found; n >= 0; n = nodes[static_cast<std::size_t>(n)].parent)
        path.push_back(nodes[static_cast<std::size_t>(n)]);
    std::reverse(path.begin(), path.end());

    std::uint64_t jiffy = 1;
    emit_line(jiffy, "PULL LEFT SWORD");
    int facing = static_cast<int>(game.player().dir);
    for (std::size_t i = 1; i < path.size(); ++i) {
        int step_dir = 0;
        const int drow = path[i].row - path[i - 1].row;
        const int dcol = path[i].col - path[i - 1].col;
        if (drow == -1) step_dir = 0;
        else if (dcol == 1) step_dir = 1;
        else if (drow == 1) step_dir = 2;
        else step_dir = 3;
        const int delta = (step_dir - facing) & 3;
        if (delta == 1) emit_line(jiffy, "TURN RIGHT");
        else if (delta == 3) emit_line(jiffy, "TURN LEFT");
        else if (delta == 2) emit_line(jiffy, "TURN AROUND");
        facing = step_dir;
        emit_line(jiffy, "MOVE");
    }
    if (!carried) emit_line(jiffy, "GET LEFT RING");
    std::cerr << (carried ? "carried" : "floor") << " target " << goal_row << "," << goal_col
              << " jiffies " << jiffy << "\n";
    return 0;
}
