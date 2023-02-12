#ifndef PLAYER_H
#define PLAYER_H

#include "gamestate.h"

class GameRecording;

class Player {
public:
    explicit Player(const Rules& rules) : rules_(rules) { }
    virtual ~Player() = default;
    
    virtual Action nextAction(const PlayerState& myState, const PlayerState& opponentState) = 0;
    virtual void learnFromGame(const GameRecording& recording) = 0;

    const Rules& rules() const { return rules_; }

protected:
    Rules rules_;
};

#endif