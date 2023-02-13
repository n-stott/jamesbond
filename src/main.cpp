#include "tourney.h"
#include "gamearena.h"
#include "players/random.h"
#include "players/biasedrandom.h"
#include "players/qlearner.h"
#include "players/bilinear.h"
#include "players/shapley.h"
#include "player.h"
#include "fmt/core.h"
#include <memory>
#include <vector>
#include <random>

void testA() {
    Rules rules;
    RandomPlayer a(rules, 0);
    RandomPlayer b(rules, 1);
    Tourney::Params closed{false, false}; 
    Tourney::Result AvsB = Tourney::play2v2(10000, &a, &b, closed);
    fmt::print("Random vs Random: ties={} winsA={} winsB={}\n\n", AvsB.ties, AvsB.winsA, AvsB.winsB);

    Tourney::Params open{true, true};
    auto q = QLearner::tryCreate(rules, 2);
    assert(!!q);
    QLearner q0 = *q;
    Tourney::Result AvsQ0;
    for(int i = 0; i < 10; ++i) { 
        AvsQ0 = Tourney::play2v2(100000, &a, &q0, open);
        fmt::print("Random vs QLearn0: ties={} winsA={} winsC={}\n", AvsQ0.ties, AvsQ0.winsA, AvsQ0.winsB);
    }
    fmt::print("q0 confidence:{}%\n", q0.confidence());
    fmt::print("\n");

    q = QLearner::tryCreate(rules, 3);
    assert(!!q);
    QLearner q1 = *q;
    Tourney::Result AvsQ1;
    for(int i = 0; i < 10; ++i) {
        AvsQ1 = Tourney::play2v2(100000, &a, &q1, open);
        fmt::print("Random vs QLearn1: ties={} winsA={} winsC={}\n", AvsQ1.ties, AvsQ1.winsA, AvsQ1.winsB);
    }
    fmt::print("q1 confidence:{}%\n", q1.confidence());
    fmt::print("\n");

    Tourney::Result Q0vsQ1 = Tourney::play2v2(10000, &q0, &q1, closed);
    fmt::print("QLearn0 vs QLearn1: ties={} winsA={} winsB={}\n\n", Q0vsQ1.ties, Q0vsQ1.winsA, Q0vsQ1.winsB);

    
    Tourney::Params semiA{true, false};
    Tourney::Params semiB{false, true};
    Q0vsQ1 = Tourney::play2v2(10000, &q0, &q1, semiA);
    fmt::print("QLearn0 vs QLearn1: ties={} winsA={} winsB={}\n\n", Q0vsQ1.ties, Q0vsQ1.winsA, Q0vsQ1.winsB);

    Q0vsQ1 = Tourney::play2v2(10000, &q0, &q1, semiB);
    fmt::print("QLearn0 vs QLearn1: ties={} winsA={} winsB={}\n\n", Q0vsQ1.ties, Q0vsQ1.winsA, Q0vsQ1.winsB);
}

std::unique_ptr<QLearner> createAndtrainQLearnerVsRandom(const Rules& rules, int seed) {
    std::unique_ptr<Player> r = std::make_unique<RandomPlayer>(rules, seed);
    std::unique_ptr<QLearner> q = QLearner::tryCreate(rules, 421*seed+1);
    Tourney::Params semiB{false, true};
    Tourney::play2v2(100000, r.get(), q.get(), semiB);
    return q;
}

void testB() {
    Rules rules;
    std::vector<std::unique_ptr<Player>> players;
    const int bracketSize = 64;
    for(int i = 0; i < bracketSize; ++i) {
        fmt::print("Training #{}\n", i);
        players.push_back(createAndtrainQLearnerVsRandom(rules, i));
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
                Tourney::Params open{true, true};
                Tourney::Result AvsB = Tourney::play2v2(1000, players[a].get(), players[b].get(), open);
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

void testC() {
    Rules rules;
    auto s = ShapleyPlayer::tryCreate(rules);
    assert(!!s);
    RandomPlayer b(rules, 0);
    auto q = createAndtrainQLearnerVsRandom(rules, 1);

    Tourney::Params closed{false, false}; 
    Tourney::Params open{true, true}; 

    Tourney::Result SvsR = Tourney::play2v2(10000, s.get(), &b, closed);
    fmt::print("Shapley vs Random: ties={} winsA={} winsB={}\n\n", SvsR.ties, SvsR.winsA, SvsR.winsB);

    Tourney::Result SvsQ = Tourney::play2v2(10000, s.get(), q.get(), closed);
    fmt::print("Shapley vs QLearner: ties={} winsA={} winsB={}\n\n", SvsQ.ties, SvsQ.winsA, SvsQ.winsB);

    SvsQ = Tourney::play2v2(1000000, s.get(), q.get(), open);
    fmt::print("Shapley vs QLearner: ties={} winsA={} winsB={}\n\n", SvsQ.ties, SvsQ.winsA, SvsQ.winsB);

    // GameArena arena;
    // GameRecording rec(&b, &s);
    // arena.play(&b, &s, &rec);
    // arena.replay(rec);
}

void testD() {
    Rules rules;
    
    RandomPlayer random(rules, 0);
    
    BiasedRandomPlayer reloader(rules, 1, 5, 1, 1);
    BiasedRandomPlayer protective(rules, 2, 1, 5, 1);
    BiasedRandomPlayer aggressive(rules, 3, 1, 1, 5);

    auto qlearner = QLearner::tryCreate(rules, 4);
    auto trainedQlearner = createAndtrainQLearnerVsRandom(rules, 5);

    BilinearPlayer bilinear(rules, 6);
    auto shapley = ShapleyPlayer::tryCreate(rules, 7);

    Tourney tourney;
    tourney.addPlayer("[NS] pure random", &random);
    tourney.addPlayer("[NS] random b/reload", &reloader);
    tourney.addPlayer("[NS] random b/shield", &protective);
    tourney.addPlayer("[NS] random b/shoot", &aggressive);
    tourney.addPlayer("[NS] base qlearner", qlearner.get());
    tourney.addPlayer("[NS] trained qlearner", trainedQlearner.get());
    tourney.addPlayer("[NS] bilinear", &bilinear);
    tourney.addPlayer("[NS] shapley", shapley.get());

    tourney.run(1000);
}

int main() {
    // testA();
    // testB();
    // testC();
    testD();
}