#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "rand.h"
#include <array>
#include <vector>

class Player;

enum class Action {
    Reload,
    Shield,
    Shoot,
};

struct Rules {
    int startLives = 5;
    int maxBullets = 5;
    int maxShields = 5;
    int maxTurns = 1000;

    friend bool operator==(const Rules& a, const Rules& b) {
        return a.startLives == b.startLives
            && a.maxBullets == b.maxBullets
            && a.maxShields == b.maxShields
            && a.maxTurns == b.maxTurns;
    }
};

class PlayerState {
public:
    explicit PlayerState() : PlayerState(Rules{}) { }
    explicit PlayerState(const Rules& rules) : lives_(rules.startLives), bullets_(0), remainingShields_(rules.maxShields) { }

    static PlayerState from(int lives, int bullets, int remainingShields) {
        PlayerState s;
        s.lives_ = lives;
        s.bullets_ = bullets;
        s.remainingShields_ = remainingShields;
        return s;
    }

    Action randomAllowedAction(Rand* rand, const Rules& rules) const {
        std::array<Action, 3> availableActions;
        int nbAvailableActions = 0;
        if(bullets_ < rules.maxBullets) availableActions[nbAvailableActions++] = Action::Reload;
        if(bullets_ > 0) availableActions[nbAvailableActions++] = Action::Shoot;
        if(remainingShields_ > 0) availableActions[nbAvailableActions++] = Action::Shield;
        return availableActions[rand->pick(nbAvailableActions)];
    }

    int lives() const { return lives_; }
    int bullets() const { return bullets_; }
    int remainingShields() const { return remainingShields_; }

    void die() { lives_ = 0; }

    bool isLegalAction(Action a, const Rules& rules) const {
        switch(a) {
            case Action::Reload: return bullets_ < rules.maxBullets;
            case Action::Shield: return remainingShields_ > 0;
            case Action::Shoot: return bullets_ > 0;
        }
    }

    void resolveOwnAction(Action a, const Rules& rules) {
        assert(isLegalAction(a));
        switch(a) {
            case Action::Reload: {
                ++bullets_;
                remainingShields_ = rules.maxShields;
                return;
            }
            case Action::Shield: {
                --remainingShields_;
                return;
            }
            case Action::Shoot: {
                --bullets_;
                remainingShields_ = rules.maxShields;
                return;
            }
        }
    }

    void resolveOpponentAction(Action myAction, Action opponentAction) {
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
    GameState(const Player* a, const Player* b) : playerA_(a), playerB_(b) { }
    virtual ~GameState() = default;

    static GameState from(const Player* a, const Player* b, const PlayerState& sa, const PlayerState& sb) {
        GameState gs(a, b);
        gs.stateA_ = sa;
        gs.stateB_ = sb;
        return gs;
    }

    bool gameOver() const {
        return stateA_.lives() <= 0 || stateB_.lives() <= 0;
    }

    void resolve(Action actionA, Action actionB, const Rules& rules) {
        if(!stateA_.isLegalAction(actionA, rules)) stateA_.die();
        if(!stateB_.isLegalAction(actionB, rules)) stateB_.die();
        if(gameOver()) return;
        stateA_.resolveOwnAction(actionA, rules);
        stateB_.resolveOwnAction(actionB, rules);
        stateA_.resolveOpponentAction(actionA, actionB);
        stateB_.resolveOpponentAction(actionB, actionA);
    }

    GameStateSnapshot snap() const {
        return GameStateSnapshot{stateA(), stateB()};
    }

    const Player* winner() const {
        if(!gameOver()) return breakTie();
        if(stateA_.lives() > 0) return playerA_;
        if(stateB_.lives() > 0) return playerB_;
        return nullptr;
    }

    const Player* breakTie() const {
        if(stateA_.lives() > stateB_.lives()) return playerA_;
        if(stateB_.lives() > stateA_.lives()) return playerB_;
        if(stateA_.bullets() > stateB_.bullets()) return playerA_;
        if(stateB_.bullets() > stateA_.bullets()) return playerB_;
        if(stateA_.remainingShields() > stateB_.remainingShields()) return playerA_;
        if(stateB_.remainingShields() > stateA_.remainingShields()) return playerB_;
        return nullptr;
    }

    const PlayerState& stateA() const { return stateA_; }
    const PlayerState& stateB() const { return stateB_; }

    const Player* playerA() const { return playerA_; }
    const Player* playerB() const { return playerB_; }

private:
    const Player* playerA_;
    const Player* playerB_;

    PlayerState stateA_;
    PlayerState stateB_;

};

#endif