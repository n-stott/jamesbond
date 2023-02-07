#include "players/shapley.h"
#include "gamestate.h"
#include "fmt/core.h"
#include <algorithm>
#include <cassert>
#include <set>
#include <deque>
#include <memory>

class DummyPlayer : public Player {
    Action nextAction(const PlayerState&, const PlayerState&) { return Action::Shoot; }
    void learnFromGame(const GameRecording&) { }
};

static constexpr ssize_t AWIN = -1;
static constexpr ssize_t BWIN = -2;
static constexpr ssize_t TIE = -3;

struct GameGraph {
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

static double playerStateValue(const PlayerState& s) {
    return 6*6*s.lives() + 6*s.bullets() + s.remainingShields();
}

// The quantity to be minimized by A
static double gameStateValue(const PlayerState& stateA, const PlayerState& stateB) {
    return playerStateValue(stateA) - playerStateValue(stateB);
}

// The quantity to be minimized by A
static double gameStateValue(const GameState& s) {
    return gameStateValue(s.stateA(), s.stateB());
}

static std::unique_ptr<GameGraph> make_graph() {
    GameGraph graph;

    std::set<GameState, StateComparator> visitedStates;
    std::deque<GameState> stateQueue;
    stateQueue.push_back(GameState{&graph.a, &graph.b});
    std::array<Action, 3> actions { Action::Reload, Action::Shield, Action::Shoot };
    while(!stateQueue.empty()) {
        GameState s = stateQueue.back();
        stateQueue.pop_back();
        visitedStates.emplace(s);
        for(Action a : actions) {
            for(Action b : actions) {
                GameState t = s;
                t.resolve(a, b);
                if(t.gameOver()) continue;
                if(visitedStates.find(t) != visitedStates.end()) continue;
                stateQueue.push_back(t);
            }
        }
    }
    fmt::print("Game has {} non-terminal states\n", visitedStates.size());

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
                t.resolve(a, b);
                if(t.gameOver()) {
                    const Player* winner = t.winner();
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
                    edgesCostEntry = gameStateValue(t) - gameStateValue(s);
                }
            }
        }
    }
    GameState start(&graph.a, &graph.b);
    auto entry = std::lower_bound(graph.states.begin(), graph.states.end(), start, StateComparator{});
    if(entry == graph.states.end()) {
        fmt::print("No entrypoint in graph\n");
        return {};
    }
    graph.entrypoint = std::distance(graph.states.begin(), entry);

    // auto s = graph.states[0];
    // const auto& sa = s.stateA();
    // const auto& sb = s.stateB();
    // auto t = s;
    // t.resolve(Action::Shoot, Action::Reload);
    // const auto& ta = t.stateA();
    // const auto& tb = t.stateB();

    // fmt::print("{} {}/{}/{}  {}/{}/{}\n", 0, sa.lives(), sa.bullets(), sa.remainingShields(), sb.lives(), sb.bullets(), sb.remainingShields());
    // fmt::print("{} {}/{}/{}  {}/{}/{}\n", 0, ta.lives(), ta.bullets(), ta.remainingShields(), tb.lives(), tb.bullets(), tb.remainingShields());
    // fmt::print("{} {} {} {}\n", t.gameOver(), t.winner() == &graph.a, t.winner() == &graph.b, !!t.winner());


    // for(size_t i = 0; i < 10 ; ++i) { //graph.states.size(); ++i) {
    //     const auto& sa = graph.states[i].stateA();
    //     const auto& sb = graph.states[i].stateB();
    //     fmt::print("{} {}/{}/{}  {}/{}/{}\n", i, sa.lives(), sa.bullets(), sa.remainingShields(), sb.lives(), sb.bullets(), sb.remainingShields());
    //     fmt::print("   --({}, {}, {})-> {}\n", 0, 0, graph.edgesCost[i][0][0], graph.edges[i][0][0]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 0, 1, graph.edgesCost[i][0][1], graph.edges[i][0][1]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 0, 2, graph.edgesCost[i][0][2], graph.edges[i][0][2]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 1, 0, graph.edgesCost[i][1][0], graph.edges[i][1][0]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 1, 1, graph.edgesCost[i][1][1], graph.edges[i][1][1]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 1, 2, graph.edgesCost[i][1][2], graph.edges[i][1][2]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 2, 0, graph.edgesCost[i][2][0], graph.edges[i][2][0]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 2, 1, graph.edgesCost[i][2][1], graph.edges[i][2][1]);
    //     fmt::print("   --({}, {}, {})-> {}\n", 2, 2, graph.edgesCost[i][2][2], graph.edges[i][2][2]);
    // }

    return std::make_unique<GameGraph>(std::move(graph));
}

struct BilinearMinMax {

    static constexpr double RESOLUTION = 1.0/6.0;

    static StrategyPoint solveBad(const std::array<std::array<double, 3>, 3>& A) {
        Point x;
        Point y;
        double bestXValue = +std::numeric_limits<double>::infinity();
        Point bestXPoint {0, 0, 0};
        Point bestXYPoint {0, 0, 0};
        for(x.p[0] = 0; x.p[0] <= 1; x.p[0] += RESOLUTION) {
            for(x.p[1] = 0; x.p[1] <= 1 - x.p[0]; x.p[1] += RESOLUTION) {
                x.p[2] = 1 - x.p[0] - x.p[1];
                double bestYValue = -std::numeric_limits<double>::infinity();
                Point bestYPoint {0, 0, 0};
                auto eval = [&](const Point& y) {
                    double val = 0.0;
                    for(int i = 0; i < 3; ++i) {
                        if(x.p[i] == 0) continue;
                        for(int j = 0; j < 3; ++j) {
                            if(y.p[j] == 0) continue;
                            assert(A[i][j] == A[i][j]);
                            val += A[i][j] * x.p[i] * y.p[j];
                        }
                    }
                    assert(val == val);
                    return val;
                };
                for(y.p[0] = 0; y.p[0] <= 1; y.p[0] += RESOLUTION) {
                    for(y.p[1] = 0; y.p[1] <= 1 - y.p[0]; y.p[1] += RESOLUTION) {
                        y.p[2] = 1 - y.p[0] - y.p[1];
                        double val = eval(y);
                        if(val > bestYValue) {
                            bestYValue = val;
                            bestYPoint = y;
                        }
                    }
                }
                if(bestYValue < bestXValue) {
                    bestXValue = bestYValue;
                    bestXPoint = x;
                    bestXYPoint = bestYPoint;
                }
            }
        }
        return StrategyPoint { bestXValue, bestXPoint, bestXYPoint };
    }

    static StrategyPoint solve(const std::array<std::array<double, 3>, 3>& A, size_t* pureSolves) {
        const double inf = std::numeric_limits<double>::infinity();
        std::array<double, 3> maxByRow {{ -inf, -inf, -inf }};
        std::array<double, 3> minByCol {{ +inf, +inf, +inf }};
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 3; ++j) {
                maxByRow[i] = std::max(maxByRow[i], A[i][j]);
                minByCol[j] = std::min(minByCol[j], A[i][j]);
            }
        }
        auto minValueIt = std::max_element(minByCol.begin(), minByCol.end());
        auto maxValueIt = std::min_element(maxByRow.begin(), maxByRow.end());
        if(*maxValueIt != *minValueIt) {
            return solveBad(A);
        }
        ++(*pureSolves);
        Point a {0, 0, 0};
        Point b {0, 0, 0};
        a.p[std::distance(maxByRow.begin(), maxValueIt)] = 1;
        b.p[std::distance(minByCol.begin(), minValueIt)] = 1;
        return StrategyPoint { *minValueIt, a, b };
    }

};

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

[[maybe_unused]] static inline void printCostMatrix(const std::array<std::array<double, 3>, 3>& A) {
    fmt::print("{} {} {}\n", A[0][0], A[0][1], A[0][2]);
    fmt::print("{} {} {}\n", A[1][0], A[1][1], A[1][2]);
    fmt::print("{} {} {}\n", A[2][0], A[2][1], A[2][2]);
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
        size_t pureSolves = 0;
        for(size_t i = 0; i < g.states.size(); ++i) {
            auto A = formCostMatrix(g, v, i);
            auto solution = BilinearMinMax::solve(A, &pureSolves);
            vNext[i] = solution;
            
            if(false) { //iter > 130 && (ssize_t)(v[i] - vNext[i]) != 0) {
                fmt::print("i={} v[i]={} vNext[i]={}\n", i, v[i].value, vNext[i].value);
                auto s = g.states[i];
                const auto& sa = s.stateA();
                const auto& sb = s.stateB();
                fmt::print("{} {}/{}/{}  {}/{}/{}\n", i, sa.lives(), sa.bullets(), sa.remainingShields(), sb.lives(), sb.bullets(), sb.remainingShields());
                fmt::print("{} {} {}\n", A[0][0], A[0][1], A[0][2]);
                fmt::print("{} {} {}\n", A[1][0], A[1][1], A[1][2]);
                fmt::print("{} {} {}\n", A[2][0], A[2][1], A[2][2]);
                fmt::print("Solution value: {}\n", solution.value);
                fmt::print("\n");
            }
        }
        size_t diffSize = 0;
        size_t diffInf = 0;
        size_t finiteMagn = 0;
        for(size_t i = 0; i < v.size(); ++i) {
            diffSize += (ssize_t)(v[i].value - vNext[i].value) != 0;
            diffInf += (std::isinf(vNext[i].value) != std::isinf(v[i].value));
            if(!std::isinf(vNext[i].value) && !std::isinf(v[i].value)) finiteMagn += std::abs(vNext[i].value - v[i].value);
        }
        // double d = distance(v, vNext);
        // fmt::print("Iter #{:4}  DiffNz={} DiffInfNz={} diffMagn={} pureSolves={} |v-v+|={}\n", iter, diffSize, diffInf, finiteMagn, pureSolves, d);
        // if(diffSize == 0) break;
        // break;
        v.swap(vNext);
    }

    std::for_each(v.begin(), v.end(), [&](auto& e) { e.value /= iter; });

    return v;
}

Shapley::Shapley(int seed) : rand_(seed) {
    gameGraph_ = make_graph();
    if(!gameGraph_) return;
    meanPayoff_ = approximateMeanPayoff(*gameGraph_);
}

Shapley::~Shapley() = default;

Action Shapley::nextAction(const PlayerState& stateA, const PlayerState& stateB) {
    std::pair<PlayerState, PlayerState> p { stateA, stateB };
    auto it = std::lower_bound(gameGraph_->states.begin(), gameGraph_->states.end(), p, StateComparator2{});
    size_t pos = std::distance(gameGraph_->states.begin(), it);
    const auto& payoff = meanPayoff_[pos];
    // auto A = formCostMatrix(*gameGraph_, meanPayoff_, pos);
    // printCostMatrix(A);
    // fmt::print("{}/{} {}   Astrat={}/{}/{} Bstrat={}/{}/{}\n", pos, gameGraph_->states.size(), payoff.value, payoff.a.p[0], payoff.a.p[1], payoff.a.p[2], payoff.b.p[0], payoff.b.p[1], payoff.b.p[2]);
    int toss = rand_.pick(101);
    if(toss <= 100*payoff.a.p[0])
        return Action::Reload;
    else if(toss <= 100*(payoff.a.p[0]+payoff.a.p[1]))
        return Action::Shield;
    else
        return Action::Shoot;
}

void Shapley::learnFromGame(const GameRecording&) {

}