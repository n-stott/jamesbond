#include "tourney.h"
#include "gamearena.h"
#include "players/random.h"
#include "players/qlearner.h"
#include "player.h"
#include "fmt/core.h"
#include <memory>
#include <vector>
#include <random>

void testA() {
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

    Tourney::Result Q0vsQ1 = Tourney::run(10000, q0.get(), q1.get(), false);
    fmt::print("QLearn0 vs QLearn1: ties={} winsA={} winsB={}\n\n", Q0vsQ1.ties, Q0vsQ1.winsA, Q0vsQ1.winsB);
}

std::unique_ptr<Player> createAndtrainQLearnerVsRandom(int seed) {
    std::unique_ptr<Player> r = std::make_unique<RandomPlayer>(seed);
    std::unique_ptr<Player> q = std::make_unique<QLearner>(seed);
    Tourney::run(100000, r.get(), q.get());
    return q;
}

void testB() {
    std::vector<std::unique_ptr<Player>> players;
    const int bracketSize = 64;
    for(int i = 0; i < bracketSize; ++i) {
        fmt::print("Training #{}\n", i);
        players.push_back(createAndtrainQLearnerVsRandom(i));
    }
    std::vector<int> wins(bracketSize, 0);
    std::vector<int> ties(bracketSize, 0);
    std::vector<int> played(bracketSize, 0);
    for(int round = 0; round < 10; ++round) {
        fmt::print("Round #{}\n", round);
        std::vector<int> order;
        for(int i = 0; i < bracketSize; ++i) order.push_back(i);
        auto rng = std::default_random_engine {};
        std::shuffle(order.begin(), order.end(), rng);
        for(int i = 0; i < bracketSize; ++i) {
            for(int j = i+1; j < bracketSize; ++j) {
                int a = order[i];
                int b = order[j];
                Tourney::Result AvsB = Tourney::run(1000, players[a].get(), players[b].get(), true);
                wins[a] += AvsB.winsA;
                wins[b] += AvsB.winsB;
                ties[a] += AvsB.ties;
                ties[b] += AvsB.ties;
                played[a] += 1000;
                played[b] += 1000;
            }
        }
    }
    for(int i = 0; i < bracketSize; ++i) {
        fmt::print("{:2} : wins={} ({:.2}%)  ties={} ({:.2}%)\n", i, wins[i], 100*(double)wins[i]/played[i], ties[i], 100*(double)ties[i]/played[i]);
    }
}

int main() {
    // testA();
    testB();
}