#include "tourney.h"
#include "gamearena.h"
#include "players/random.h"
#include "players/qlearner.h"
#include "player.h"
#include <memory>
#include <fmt/core.h>

int main() {
    std::unique_ptr<Player> a = std::make_unique<RandomPlayer>(0);
    std::unique_ptr<Player> b = std::make_unique<RandomPlayer>(1);
    Tourney::Result AvsB = Tourney::run(10000, a.get(), b.get());
    fmt::print("Random vs Random: ties={} winsA={} winsB={}\n\n", AvsB.ties, AvsB.winsA, AvsB.winsB);

    std::unique_ptr<Player> q0 = std::make_unique<QLearner>(0);
    Tourney::Result AvsQ0;
    for(int i = 0; i < 10; ++i) {
        AvsQ0 = Tourney::run(100000, a.get(), q0.get());
        fmt::print("Random vs QLearn0: ties={} winsA={} winsC={}\n", AvsQ0.ties, AvsQ0.winsA, AvsQ0.winsB);
    }
    fmt::print("\n");

    std::unique_ptr<Player> q1 = std::make_unique<QLearner>(1);
    Tourney::Result AvsQ1;
    for(int i = 0; i < 10; ++i) {
        AvsQ1 = Tourney::run(100000, a.get(), q1.get());
        fmt::print("Random vs QLearn1: ties={} winsA={} winsC={}\n", AvsQ1.ties, AvsQ1.winsA, AvsQ1.winsB);
    }
    fmt::print("\n");

    Tourney::Result Q0vsQ1 = Tourney::run(10000, q0.get(), q1.get());
    fmt::print("QLearn0 vs QLearn1: ties={} winsA={} winsB={}\n\n", Q0vsQ1.ties, Q0vsQ1.winsA, Q0vsQ1.winsB);
}