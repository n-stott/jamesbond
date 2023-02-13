#ifndef BIASEDRANDOM_H
#define BIASEDRANDOM_H

#include "player.h"
#include "rand.h"

class BiasedRandomPlayer : public Player {
public:
    explicit BiasedRandomPlayer(const Rules& rules, int seed, int biasReload, int biasShield, int biasShoot) :
            Player(rules),
            rand(seed),
            biasReload_(biasReload),
            biasShield_(biasShield),
            biasShoot_(biasShoot) {
        if(biasReload_ <= 0) biasReload_ = 1;
        if(biasShield_ <= 0) biasShield_ = 1;
        if(biasShoot_ <= 0) biasShoot_ = 1;
    }

    Action nextAction(const PlayerState& myState, const PlayerState&) override {
        return myState.randomAllowedActionWithBias(&rand, rules_, biasReload_, biasShield_, biasShoot_);
    }

    void learnFromGame(const GameRecording&) override { }

private:
    mutable Rand rand;
    int biasReload_;
    int biasShield_;
    int biasShoot_;
};

#endif