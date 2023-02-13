#ifndef BILINEAR_H
#define BILINEAR_H

#include "player.h"
#include "rand.h"

class BilinearPlayer : public Player {
public:
    explicit BilinearPlayer(const Rules& rules, int seed) : Player(rules), rand_(seed) { }

    Action nextAction(const PlayerState& myState, const PlayerState& opponentState) override;

    void learnFromGame(const GameRecording&) override { }

private:
    mutable Rand rand_;
};

#endif