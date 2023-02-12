#ifndef TOURNEY_H
#define TOURNEY_H

#include "gamearena.h"
#include "gamerecording.h"
#include "player.h"

class Tourney {
public:
    struct Result {
        int winsA = 0;
        int winsB = 0;
        int ties = 0;
    };

    struct Params {
        bool allowLearningA = true;
        bool allowLearningB = true;
    };

    static Result run(int rounds, Player* a, Player* b, const Params& params) {
        Result result;
        GameRecording recording(a, b);
        bool withRecording = params.allowLearningA || params.allowLearningB;
        for(int round = 0; round < rounds; ++round) {
            GameArena arena;
            const Player* winner = arena.play(a, b, withRecording ? &recording : nullptr);
            if(params.allowLearningA) a->learnFromGame(recording);
            if(params.allowLearningB) b->learnFromGame(recording);
            if(!winner) ++result.ties;
            if(winner == a) ++result.winsA;
            if(winner == b) ++result.winsB;
        }
        return result;
    }

};

#endif