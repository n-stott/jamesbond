#ifndef QLEARNER_H
#define QLEARNER_H

#include "player.h"
#include "rand.h"
#include <vector>

class QLearner : public Player {
public:
    explicit QLearner(int seed = 0) : rand_(seed) { }
    Action nextAction(const PlayerState& myState, const PlayerState& opponentState);
    void learnFromGame(const GameState& gameState);

private:
    struct QState {
        // Single player state can be encoded 216 values
        // 6 (lives) * 6 (bullets) * 6 (shields) = 216 < 256

        // Whole game state takes 216*216 = 46656 entries
        // Multiply by 3 for each player choice (only 1 side)
        struct Score {
            double score = 0.0;
            int confidence = 0;

            void add(double prize) {
                ++confidence;
                score += prize;
            }
        };
        std::vector<Score> qReload;
        std::vector<Score> qShield;
        std::vector<Score> qShoot;

        std::vector<Score>* qLookup[3];

        QState() {
            qReload.resize(216*216);
            qShield.resize(216*216);
            qShoot.resize(216*216);
            qLookup[0] = &qReload;
            qLookup[1] = &qShield;
            qLookup[2] = &qShoot;
        }

        static int configToIndex(const PlayerState& me, const PlayerState& opponent);
    } state_;
    Rand rand_;
};

#endif