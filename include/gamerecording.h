#ifndef GAMERECORDING_H
#define GAMERECORDING_H

#include "player.h"
#include "gamestate.h"
#include <vector>

class GameRecording {
public:
    GameRecording(const Player* a, const Player* b) : a_(a), b_(b), winner_(nullptr) { }

    void clear() {
        actionsA_.clear();
        actionsB_.clear();
    }

    void record(Action a, Action b) {
        actionsA_.push_back(a);
        actionsB_.push_back(b);
    }

    void recordWinner(const Player* winner) {
        winner_ = winner;
    }

    template<typename Callback>
    void replay(Callback&& callback) const {
        assert(a_->rules() == b_->rules());
        GameState replayState;
        size_t turn = 0;
        while(!replayState.gameOver() && turn < actionsA_.size()) {
            Action actionA = actionsA_[turn];
            Action actionB = actionsB_[turn];
            ++turn;
            GameStateSnapshot before = replayState.snap();
            replayState.resolve(actionA, actionB, a_->rules());
            GameStateSnapshot after = replayState.snap();
            callback(before, after, actionA, actionB);
        }
    }

    const Player* winner() const { return winner_; }
    const Player* playerA() const { return a_; }
    const Player* playerB() const { return b_; }

private:
    const Player* a_;
    const Player* b_;
    const Player* winner_;
    std::vector<Action> actionsA_;
    std::vector<Action> actionsB_;
};

#endif