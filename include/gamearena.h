#ifndef GAMEARENA_H
#define GAMEARENA_H

#include "player.h"

class GameArena {
public:
    GameArena();
    virtual ~GameArena() = default;
    
    Player* play(Player* a, Player* b, bool replayable);

    void replay() const;

protected:
    GameState state_;
};

#endif