#include "players/qlearner.h"

int QLearner::QState::configToIndex(const PlayerState& me, const PlayerState& opponent) {
    return me.lives() + 6*me.bullets() + 6*6*me.remainingShields()
         + 6*6*6*(opponent.lives() + 6*opponent.bullets() + 6*6*opponent.remainingShields());
}

Action QLearner::nextAction(const PlayerState& myState, const PlayerState& opponentState) {
    int index = QState::configToIndex(myState, opponentState);
    QState::Score scoreReload = state_.qReload[index];
    QState::Score scoreShield = state_.qShield[index];
    QState::Score scoreShoot  = state_.qShoot[index];
    QState::Score bestScore = scoreReload;
    QState::Score worstScore = scoreReload;
    Action action = Action::Reload;
    if(scoreShield.score > bestScore.score) {
        bestScore = scoreShield;
        action = Action::Shield;
    }
    if(scoreShoot.score > bestScore.score) {
        bestScore = scoreShoot;
        action = Action::Shoot;
    }
    if(scoreShield.score < worstScore.score) worstScore = scoreShield;
    if(scoreShoot.score < worstScore.score) worstScore = scoreShoot;
    if(bestScore.confidence < 5 || worstScore.confidence < 5 || std::abs(bestScore.score - worstScore.score) < 0.1) {
        return myState.randomAllowedAction(&rand_);
    } else {
        return action;
    }
}

void QLearner::learnFromGame(const GameState& state) {
    Player* winner = state.winner();
    double prize = 0.0;
    if(winner == this) {
        prize = 0.5;
    } else if (winner == nullptr) {
        prize = -0.1;
    } else {
        prize = -0.5;
    }
    state.replay([&](const GameState& s, Action a, Action b) {
        if(s.playerA() == this) {
            int index = QState::configToIndex(s.stateA(), s.stateB());
            (state_.qLookup[(int)a])->operator[](index).add(prize);
        } else {
            int index = QState::configToIndex(s.stateB(), s.stateA());
            (state_.qLookup[(int)b])->operator[](index).add(prize);
        }
    });
}