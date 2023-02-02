#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "rand.h"
#include <array>
#include <vector>

#define START_LIVES 5
#define MAX_BULLETS 5
#define MAX_SHIELDS 5 

class Player;

enum class Action {
    Reload,
    Shield,
    Shoot,
};

class PlayerState {
public:
    PlayerState() : lives_(START_LIVES), bullets_(0), remainingShields_(MAX_SHIELDS) { }
    explicit PlayerState(int lives) : PlayerState() { lives_ = lives; }

    Action randomAllowedAction(Rand* rand) const {
        std::array<Action, 3> availableActions;
        int nbAvailableActions = 0;
        if(bullets_ < MAX_BULLETS) availableActions[nbAvailableActions++] = Action::Reload;
        if(bullets_ > 0) availableActions[nbAvailableActions++] = Action::Shoot;
        if(remainingShields_ > 0) availableActions[nbAvailableActions++] = Action::Shield;
        return availableActions[rand->pick(nbAvailableActions)];
    }

    int lives() const { return lives_; }
    int bullets() const { return bullets_; }
    int remainingShields() const { return remainingShields_; }

    void resolve(Action myAction, Action opponentAction) {
        if(myAction == Action::Reload) {
            if(bullets_ >= MAX_BULLETS) {
                lives_ = 0;
            } else {
                ++bullets_;
                remainingShields_ = MAX_SHIELDS;
            }
        }
        if(myAction == Action::Shield) {
            if(remainingShields_ <= 0) {
                lives_ = 0;
            } else {
                --remainingShields_;
            }
        }
        if(myAction == Action::Shoot) {
            if(bullets_ <= 0) {
                lives_ = 0;
            } else {
                --bullets_;
                remainingShields_ = MAX_SHIELDS;
            }
        }
        if(opponentAction == Action::Shoot && myAction != Action::Shield) {
            lives_ = std::max(0, lives_-1);
        }
    }

private:
    int lives_;
    int bullets_;
    int remainingShields_;
};

struct GameStateSnapshot {
    PlayerState stateA;
    PlayerState stateB;
};

class GameState {
public:
    GameState(Player* a, Player* b, bool replayable = false) : playerA_(a), playerB_(b), replayable_(replayable) { }

    bool gameOver() const {
        return stateA_.lives() <= 0 || stateB_.lives() <= 0;
    }

    void resolve(Action actionA, Action actionB) {
        if(replayable_) {
            actionsA_.push_back(actionA);
            actionsB_.push_back(actionB);
        }
        stateA_.resolve(actionA, actionB);
        stateB_.resolve(actionB, actionA);
    }

    GameStateSnapshot snap() const {
        return GameStateSnapshot{stateA(), stateB()};
    }

    Player* winner() const {
        if(!gameOver()) return nullptr;
        if(stateA_.lives() > 0) return playerA_;
        if(stateB_.lives() > 0) return playerB_;
        return nullptr;
    }

    template<typename Callback>
    void replay(Callback&& callback) const {
        if(!replayable_) return;
        GameState replayState(playerA_, playerB_);
        size_t turn = 0;
        while(!replayState.gameOver()) {
            Action actionA = actionsA_[turn];
            Action actionB = actionsB_[turn];
            ++turn;
            GameStateSnapshot before = replayState.snap();
            replayState.resolve(actionA, actionB);
            GameStateSnapshot after = replayState.snap();
            callback(before, after, actionA, actionB);
        }
    }

    const PlayerState& stateA() const { return stateA_; }
    const PlayerState& stateB() const { return stateB_; }

    const Player* playerA() const { return playerA_; }
    const Player* playerB() const { return playerB_; }

private:
    Player* playerA_;
    Player* playerB_;

    PlayerState stateA_;
    PlayerState stateB_;

    bool replayable_ = false;
    std::vector<Action> actionsA_;
    std::vector<Action> actionsB_;

};

#endif