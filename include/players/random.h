#ifndef RANDOM_H
#define RANDOM_H

#include "player.h"
#include "rand.h"

class RandomPlayer : public Player {
public:
    explicit RandomPlayer(const Rules& rules, int seed) : Player(rules), rand(seed) { }

    Action nextAction(const PlayerState& myState, const PlayerState&) override {
        return myState.randomAllowedAction(&rand, rules_);
    }

    void learnFromGame(const GameRecording&) override { }

private:
    mutable Rand rand;
};

#endif