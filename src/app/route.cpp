// Prints a keystroke script that walks to the nearest floor object of a type.
// Usage: route <type> [--second N]
#include "daggorath/game.hpp"
#include "daggorath/lexicon_tables.hpp"
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
    int level = 0;
    bool frozen = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--second" && i + 1 < argc) second = std::atoi(argv[++i]);
        else if (std::string(argv[i]) == "--level" && i + 1 < argc) level = std::atoi(argv[++i]);
        else if (std::string(argv[i]) == "--frozen") frozen = true;
    }
    dag::Game game(static_cast<std::uint8_t>(second), level);
    if (frozen) game.set_frozen(true);
    game.set_frozen(true);
    int goal_row = -1;
    int goal_col = -1;
    int best = 1 << 20;
    for (const dag::Ocb& object : game.objects()) {
        if (object.level != level || object.type != want) continue;
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
        std::cerr << "no placed object of type " << want << " on level " << level << "\n";
        return 1;
    }

    if (carried) {
        std::string script;
        std::uint64_t jiffy = 1;
        auto add = [&](const std::string& text) {
            for (char ch : text) {
                script += std::to_string(jiffy) + ' ' + (ch == ' ' ? "SPACE" : std::string(1, ch)) + '\n';
                ++jiffy;
            }
            script += std::to_string(jiffy) + " CR\n";
            jiffy += 1;
        };
        auto replay = [&]() {
            dag::Game simulated(static_cast<std::uint8_t>(second), level);
            if (frozen) simulated.set_frozen(true);
            std::string error;
            simulated.load_script(dag::parse_script(script, error));
            simulated.advance_jiffies(jiffy + 2);
            return simulated;
        };
        add("PULL LEFT TORCH");
        add("USE LEFT");
        add("PULL LEFT SWORD");
        bool caught = false;
        for (int step = 0; step < (frozen ? 400 : 80) && !caught; ++step) {
            dag::Game simulated = replay();
            int carrier_row = -1;
            int carrier_col = -1;
            for (int slot = 0; slot < dag::kCcbSlots; ++slot) {
                const dag::Ccb& creature = simulated.creatures()[static_cast<std::size_t>(slot)];
                if (!creature.in_use) continue;
                for (int obj = creature.object_head; obj >= 0;
                     obj = simulated.objects()[static_cast<std::size_t>(obj)].next) {
                    if (simulated.objects()[static_cast<std::size_t>(obj)].type != want) continue;
                    carrier_row = creature.row;
                    carrier_col = creature.col;
                }
            }
            if (carrier_row < 0) break;
            if (!frozen && (simulated.player().dead ||
                            simulated.player().damage + 15 >= simulated.player().power)) {
                break;
            }
            if (simulated.player().row == carrier_row && simulated.player().col == carrier_col) {
                caught = true;
                for (int swing = 0; swing < 20; ++swing) {
                    const std::string saved = script;
                    const std::uint64_t saved_jiffy = jiffy;
                    add("ATTACK LEFT");
                    dag::Game after = replay();
                    bool killed = false;
                    for (const auto& event : after.trace()) {
                        if (event.kind == "KILL") killed = true;
                    }
                    if (after.player().dead || after.player().damage + 15 >= after.player().power) {
                        script = saved;
                        jiffy = saved_jiffy;
                        break;
                    }
                    if (killed) break;
                }
                dag::Game after_kill = replay();
                bool killed = false;
                for (const auto& event : after_kill.trace()) {
                    if (event.kind == "KILL") killed = true;
                }
                if (killed && after_kill.player().right_hand < 0) {
                    const std::string saved = script;
                    const std::uint64_t saved_jiffy = jiffy;
                    add("GET RIGHT RING");
                    dag::Game got = replay();
                    if (got.player().right_hand < 0) {
                        script = saved;
                        jiffy = saved_jiffy;
                    } else {
                        const dag::Ocb& ring =
                            got.objects()[static_cast<std::size_t>(got.player().right_hand)];
                        if (ring.spec[1] != 0) {
                            for (const dag::TokenEntry& entry : dag::kAdjTab) {
                                if (entry.index != ring.spec[1]) continue;
                                add("INCANT " + std::string(entry.word));
                                break;
                            }
                        }
                    }
                }
                break;
            }
            std::vector<Node> nodes;
            std::vector<int> queue;
            std::vector<char> seen(32 * 32, 0);
            nodes.push_back({simulated.player().row, simulated.player().col, -1});
            queue.push_back(0);
            seen[simulated.player().row * 32 + simulated.player().col] = 1;
            int found = -1;
            for (std::size_t qi = 0; qi < queue.size() && found < 0; ++qi) {
                const Node here = nodes[static_cast<std::size_t>(queue[qi])];
                if (here.row == carrier_row && here.col == carrier_col) {
                    found = queue[qi];
                    break;
                }
                for (int dir = 0; dir < 4; ++dir) {
                    int nr = 0;
                    int nc = 0;
                    if (!dag::step_ok(simulated.maze(), here.row, here.col, static_cast<dag::Dir>(dir), nr, nc))
                        continue;
                    const int key = nr * 32 + nc;
                    if (seen[static_cast<std::size_t>(key)]) continue;
                    seen[static_cast<std::size_t>(key)] = 1;
                    nodes.push_back({nr, nc, queue[qi]});
                    queue.push_back(static_cast<int>(nodes.size()) - 1);
                }
            }
            if (found < 0) break;
            int next = found;
            while (nodes[static_cast<std::size_t>(next)].parent > 0) next = nodes[static_cast<std::size_t>(next)].parent;
            const Node& step_to = nodes[static_cast<std::size_t>(next)];
            int step_dir = 0;
            const int drow = step_to.row - simulated.player().row;
            const int dcol = step_to.col - simulated.player().col;
            if (drow == -1) step_dir = 0;
            else if (dcol == 1) step_dir = 1;
            else if (drow == 1) step_dir = 2;
            else step_dir = 3;
            const int facing = static_cast<int>(simulated.player().dir);
            const int delta = (step_dir - facing) & 3;
            const std::string saved = script;
            const std::uint64_t saved_jiffy = jiffy;
            if (delta == 1) add("TURN RIGHT");
            else if (delta == 3) add("TURN LEFT");
            else if (delta == 2) add("TURN AROUND");
            add("MOVE");
            dag::Game after = replay();
            if (after.player().dead ||
                (!frozen && after.player().damage + 15 >= after.player().power)) {
                script = saved;
                jiffy = saved_jiffy;
                break;
            }
        }
        std::cout << script;
        dag::Game done = replay();
        std::cerr << "pursue row=" << done.player().row << " col=" << done.player().col
                  << " jiffies " << jiffy << "\n";
        return 0;
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
