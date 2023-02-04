#ifndef GAMEARENA_H
#define GAMEARENA_H

#include "player.h"
#include "gamestate.h"

class GameArena {
public:
    GameArena();
    virtual ~GameArena() = default;
    
    const Player* play(Player* a, Player* b, GameRecording* recording);

    void replay(const GameRecording& recording) const;

protected:
    GameState state_;
};

#endif