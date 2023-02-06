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

    static PlayerState from(int lives, int bullets, int remainingShields) {
        PlayerState s;
        s.lives_ = lives;
        s.bullets_ = bullets;
        s.remainingShields_ = remainingShields;
        return s;
    }

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

    void die() { lives_ = 0; }

    bool isLegalAction(Action a) const {
        switch(a) {
            case Action::Reload: return bullets_ < MAX_BULLETS;
            case Action::Shield: return remainingShields_ > 0;
            case Action::Shoot: return bullets_ > 0;
        }
    }

    void resolveOwnAction(Action a) {
        assert(isLegalAction(a));
        switch(a) {
            case Action::Reload: {
                ++bullets_;
                remainingShields_ = MAX_SHIELDS;
                return;
            }
            case Action::Shield: {
                --remainingShields_;
                return;
            }
            case Action::Shoot: {
                --bullets_;
                remainingShields_ = MAX_SHIELDS;
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

    void resolve(Action actionA, Action actionB) {
        if(!stateA_.isLegalAction(actionA)) stateA_.die();
        if(!stateB_.isLegalAction(actionB)) stateB_.die();
        if(gameOver()) return;
        stateA_.resolveOwnAction(actionA);
        stateB_.resolveOwnAction(actionB);
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
        GameState replayState(a_, b_);
        size_t turn = 0;
        while(!replayState.gameOver() && turn < actionsA_.size()) {
            Action actionA = actionsA_[turn];
            Action actionB = actionsB_[turn];
            ++turn;
            GameStateSnapshot before = replayState.snap();
            replayState.resolve(actionA, actionB);
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