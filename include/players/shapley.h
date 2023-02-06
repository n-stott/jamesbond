#ifndef SHAPLEY_H
#define SHAPLEY_H

#include "player.h"
#include <memory>

struct GameGraph;

struct Point { double p[3]; };

struct StrategyPoint {
    double value = 0.0;
    Point a {};
    Point b {};
};

class Shapley : public Player {
public:
    explicit Shapley(int seed = 0);
    ~Shapley();
    Action nextAction(const PlayerState& myState, const PlayerState& opponentState);
    void learnFromGame(const GameRecording& recording);

private:
    std::unique_ptr<GameGraph> gameGraph_;
    std::vector<StrategyPoint> meanPayoff_;
    Rand rand_;
};

#endif