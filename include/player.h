#ifndef PLAYER_H
#define PLAYER_H

#include "gamestate.h"

class Player {
public:
    virtual ~Player() = default;
    virtual Action nextAction(const PlayerState& myState, const PlayerState& opponentState) = 0;
    virtual void learnFromGame(const GameRecording& recording) = 0;

};

#endif