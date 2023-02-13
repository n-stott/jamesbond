#include "players/shapley.h"
#include "gamestate.h"
#include "bilinearminmax.h"
#include "fmt/core.h"
#include <algorithm>
#include <cassert>
#include <set>
#include <deque>
#include <memory>

class DummyPlayer : public Player {
public:
    explicit DummyPlayer(const Rules& rules) : Player(rules) { }
private:
    Action nextAction(const PlayerState&, const PlayerState&) { return Action::Shoot; }
    void learnFromGame(const GameRecording&) { }
};

static constexpr ssize_t AWIN = -1;
static constexpr ssize_t BWIN = -2;
static constexpr ssize_t TIE = -3;

struct GameGraph {
    explicit GameGraph(const Rules& rules) : a(rules), b(rules) { }

    DummyPlayer a;
    DummyPlayer b;
    std::vector<GameState> states;
    ssize_t entrypoint;
    std::vector<std::array<std::array<ssize_t, 3>, 3>> edges;
    std::vector<std::array<std::array<double, 3>, 3>> edgesCost;
};

struct StateComparator {
    bool operator()(const GameState& s, const GameState& t) const {
        std::array<int, 6> vs {
            s.stateA().lives(),
            s.stateA().bullets(),
            s.stateA().remainingShields(),
            s.stateB().lives(),
            s.stateB().bullets(),
            s.stateB().remainingShields(),
        };
        std::array<int, 6> vt {
            t.stateA().lives(),
            t.stateA().bullets(),
            t.stateA().remainingShields(),
            t.stateB().lives(),
            t.stateB().bullets(),
            t.stateB().remainingShields(),
        };
        return std::lexicographical_compare(vs.begin(), vs.end(), vt.begin(), vt.end());
    }
};

struct StateComparator2 {
    bool operator()(const GameState& s, const std::pair<PlayerState, PlayerState>& t) const {
        std::array<int, 6> vs {
            s.stateA().lives(),
            s.stateA().bullets(),
            s.stateA().remainingShields(),
            s.stateB().lives(),
            s.stateB().bullets(),
            s.stateB().remainingShields(),
        };
        std::array<int, 6> vt {
            t.first.lives(),
            t.first.bullets(),
            t.first.remainingShields(),
            t.second.lives(),
            t.second.bullets(),
            t.second.remainingShields(),
        };
        return std::lexicographical_compare(vs.begin(), vs.end(), vt.begin(), vt.end());
    }
};

static double playerStateValue(const Rules& rules, const PlayerState& s) {
    return (rules.maxShields+1)*((rules.maxBullets+1)*s.lives() + s.bullets()) + s.remainingShields();
}

// The quantity to be minimized by A
static double gameStateValue(const Rules& rules, const PlayerState& stateA, const PlayerState& stateB) {
    return playerStateValue(rules, stateB) - playerStateValue(rules, stateA);
}

// The quantity to be minimized by A
static double gameStateValue(const Rules& rules, const GameState& s) {
    return gameStateValue(rules, s.stateA(), s.stateB());
}

static std::unique_ptr<GameGraph> make_graph(const Rules& rules) {
    GameGraph graph(rules);

    std::set<GameState, StateComparator> visitedStates;
    std::deque<GameState> stateQueue;
    stateQueue.push_back(GameState{});
    std::array<Action, 3> actions { Action::Reload, Action::Shield, Action::Shoot };
    while(!stateQueue.empty()) {
        GameState s = stateQueue.back();
        stateQueue.pop_back();
        visitedStates.emplace(s);
        for(Action a : actions) {
            for(Action b : actions) {
                GameState t = s;
                t.resolve(a, b, rules);
                if(t.gameOver()) continue;
                if(visitedStates.find(t) != visitedStates.end()) continue;
                stateQueue.push_back(t);
            }
        }
    }
    // fmt::print("Game has {} non-terminal states\n", visitedStates.size());

    graph.states.insert(graph.states.begin(), visitedStates.begin(), visitedStates.end());
    std::sort(graph.states.begin(), graph.states.end(), StateComparator{});
    for(const auto& s : graph.states) {
        auto& edgesTable = graph.edges.emplace_back();
        auto& edgesCostTable = graph.edgesCost.emplace_back();
        for(Action a : actions) {
            for(Action b : actions) {
                auto& edgeEntry = edgesTable[(int)a][(int)b];
                auto& edgesCostEntry = edgesCostTable[(int)a][(int)b];
                GameState t = s;
                t.resolve(a, b, rules);
                if(t.gameOver()) {
                    const Player* winner = t.winner(&graph.a, &graph.b);
                    if(winner == &graph.a) {
                        edgeEntry = AWIN;
                        edgesCostEntry = -std::numeric_limits<double>::infinity();
                    } else if (winner == &graph.b) {
                        edgeEntry = BWIN;
                        edgesCostEntry = +std::numeric_limits<double>::infinity();
                    } else {
                        edgeEntry = TIE;
                        edgesCostEntry = 0.0;
                    }
                } else {
                    auto it = std::lower_bound(graph.states.begin(), graph.states.end(), t, StateComparator{});
                    if(it == graph.states.end()) {
                        fmt::print("Error in transition table\n");
                        return {};
                    }
                    edgeEntry = std::distance(graph.states.begin(), it);
                    edgesCostEntry = gameStateValue(rules, t) - gameStateValue(rules, s);
                }
            }
        }
    }
    GameState start;
    auto entry = std::lower_bound(graph.states.begin(), graph.states.end(), start, StateComparator{});
    if(entry == graph.states.end()) {
        // fmt::print("No entrypoint in graph\n");
        return {};
    }
    graph.entrypoint = std::distance(graph.states.begin(), entry);

    return std::make_unique<GameGraph>(std::move(graph));
}

[[maybe_unused]] static inline void printCostMatrix(const std::array<std::array<double, 3>, 3>& A) {
    // fmt::print("{} {} {}\n", A[0][0], A[0][1], A[0][2]);
    // fmt::print("{} {} {}\n", A[1][0], A[1][1], A[1][2]);
    // fmt::print("{} {} {}\n", A[2][0], A[2][1], A[2][2]);
    fmt::print("{} {} {} {} {} {} {} {} {}\n", A[0][0], A[0][1], A[0][2], A[1][0], A[1][1], A[1][2], A[2][0], A[2][1], A[2][2]);
}

static std::array<std::array<double, 3>, 3> formCostMatrix(const GameGraph& g, const std::vector<StrategyPoint>& payoff, size_t i)  {
    const double BIG_NUMBER = 500;
    auto A = g.edgesCost[i];
    for(auto& row : A) for(auto& e : row) {
        if(e == -std::numeric_limits<double>::infinity()) e = -BIG_NUMBER;
        if(e == +std::numeric_limits<double>::infinity()) e = +BIG_NUMBER;
    }
    for(int a = 0; a < 3; ++a) {
        for(int b = 0; b < 3; ++b) {
            ssize_t dst = g.edges[i][a][b];
            if(dst >= 0) {
                assert((size_t)dst < payoff.size());
                A[a][b] += payoff[dst].value;
            } else {
                if(dst == AWIN) {
                    A[a][b] += -BIG_NUMBER;
                } else if (dst == BWIN) {
                    A[a][b] += +BIG_NUMBER;
                } else {
                    assert(dst == TIE);
                }
            }
        }
    }
    return A;
}

[[maybe_unused]] static inline double distance(const std::vector<StrategyPoint>& a, const std::vector<StrategyPoint>& b) {
    assert(a.size() == b.size());
    double d = 0;
    for(size_t i = 0; i < a.size(); ++i) {
        d += std::abs(a[i].value - b[i].value);
    }
    return d;
}

static std::vector<StrategyPoint> approximateMeanPayoff(const GameGraph& g) {
    std::vector<StrategyPoint> v(g.states.size());
    std::vector<StrategyPoint> vNext(g.states.size());
    const int MAX_ITERATIONS = 200;
    int iter = 0;
    for(iter = 0; iter < MAX_ITERATIONS; ++iter) {
        for(size_t i = 0; i < g.states.size(); ++i) {
            auto A = formCostMatrix(g, v, i);
            auto solution = BilinearMinMax::solve(A);
            vNext[i] = solution;
        }
        // size_t diffSize = 0;
        // size_t diffInf = 0;
        // size_t finiteMagn = 0;
        // for(size_t i = 0; i < v.size(); ++i) {
        //     diffSize += (ssize_t)(v[i].value - vNext[i].value) != 0;
        //     diffInf += (std::isinf(vNext[i].value) != std::isinf(v[i].value));
        //     if(!std::isinf(vNext[i].value) && !std::isinf(v[i].value)) finiteMagn += std::abs(vNext[i].value - v[i].value);
        // }
        // double d = distance(v, vNext);
        // fmt::print("Iter #{:4}  DiffNz={} DiffInfNz={} diffMagn={} |v-v+|={}\n", iter, diffSize, diffInf, finiteMagn, d);
        v.swap(vNext);
    }

    std::for_each(v.begin(), v.end(), [&](auto& e) { e.value /= iter; });

    return v;
}

std::unique_ptr<ShapleyPlayer> ShapleyPlayer::tryCreate(const Rules& rules, int seed) {
    if(rules.startLives > 5) return {};
    if(rules.maxBullets > 5) return {};
    if(rules.maxShields > 5) return {};
    return std::unique_ptr<ShapleyPlayer>(new ShapleyPlayer(rules, seed));
}

ShapleyPlayer::ShapleyPlayer(const Rules& rules, int seed) : Player(rules), rand_(seed) {
    gameGraph_ = make_graph(rules_);
    if(!gameGraph_) return;
    meanPayoff_ = approximateMeanPayoff(*gameGraph_);
}

ShapleyPlayer::~ShapleyPlayer() = default;

Action ShapleyPlayer::nextAction(const PlayerState& stateA, const PlayerState& stateB) {
    std::pair<PlayerState, PlayerState> p { stateA, stateB };
    auto it = std::lower_bound(gameGraph_->states.begin(), gameGraph_->states.end(), p, StateComparator2{});
    size_t pos = std::distance(gameGraph_->states.begin(), it);
    const auto& payoff = meanPayoff_[pos];
    return actionWithBias(rand_, payoff.p.p[0], payoff.p.p[1], payoff.p.p[2]);
}

void ShapleyPlayer::learnFromGame(const GameRecording&) {

}