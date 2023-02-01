#ifndef TOURNEY_H
#define TOURNEY_H

#include "gamearena.h"
#include "player.h"

class Tourney {
public:
    struct Result {
        int winsA = 0;
        int winsB = 0;
        int ties = 0;
    };

    static Result run(int rounds, Player* a, Player* b, bool replayable = true) {
        Result result;
        for(int round = 0; round < rounds; ++round) {
            GameArena arena;
            Player* winner = arena.play(a, b, replayable);
            if(!winner) ++result.ties;
            if(winner == a) ++result.winsA;
            if(winner == b) ++result.winsB;
        }
        return result;
    }

};

#endif