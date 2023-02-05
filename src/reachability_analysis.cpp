#include "gamestate.h"
#include "fmt/core.h"
#include <deque>
#include <set>

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

int main() {
    std::set<GameState, StateComparator> visitedStates;
    std::deque<GameState> stateQueue;
    stateQueue.push_back(GameState{nullptr, nullptr});
    std::array<Action, 3> actions { Action::Reload, Action::Shield, Action::Shoot };
    while(!stateQueue.empty()) {
        fmt::print("{} - {}\n", stateQueue.size(), visitedStates.size());
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

}