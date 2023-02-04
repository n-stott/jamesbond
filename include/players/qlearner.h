#ifndef QLEARNER_H
#define QLEARNER_H

#include "player.h"
#include "rand.h"
#include "fmt/core.h"
#include <algorithm>
#include <vector>

class QLearner : public Player {
public:
    explicit QLearner(int seed = 0) : rand_(seed) { }
    Action nextAction(const PlayerState& myState, const PlayerState& opponentState);
    void learnFromGame(const GameRecording& recording);

    double confidence() const {
        double c = 0;
        size_t total = 0;
        std::for_each(state_.qReload.begin(), state_.qReload.end(), [&](const QState::Score& s) { c += (s.confidence >= 5); ++ total; });
        std::for_each(state_.qShield.begin(), state_.qShield.end(), [&](const QState::Score& s) { c += (s.confidence >= 5); ++ total; });
        std::for_each(state_.qShoot.begin(), state_.qShoot.end(), [&](const QState::Score& s) { c += (s.confidence >= 5); ++ total; });
        return 10*c/total;
    }

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

        std::vector<Score>& lookupAction(Action a) {
            return *qLookup[(int)a];
        }

        void update(int beforeIndex, int afterIndex, Action a, double prize) {
            static const double learningRate = 0.1;
            static const double discountFactor = 0.1;
            Score& updatee = lookupAction(a)[beforeIndex];
            double estimates[3] = { lookupAction(Action::Reload)[afterIndex].score, lookupAction(Action::Shield)[afterIndex].score, lookupAction(Action::Shoot)[afterIndex].score };
            double estimate = *std::max_element(estimates, estimates+3);
            ++updatee.confidence;
            double delta = learningRate * (prize + discountFactor * estimate - updatee.score);
            updatee.score += delta;
        }

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