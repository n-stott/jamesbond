#include "players/qlearner.h"
#include "gamerecording.h"

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
    if(bestScore.confidence < 5 || worstScore.confidence < 5 || std::abs(bestScore.score - worstScore.score) < 1) {
        return myState.randomAllowedAction(&rand_, rules_);
    } else {
        if(myState.isLegalAction(action, rules_)) return action;
        return myState.randomAllowedAction(&rand_, rules_);
    }
}

void QLearner::learnFromGame(const GameRecording& recording) {
    const Player* winner = recording.winner();
    double prize = 0.0;
    if(winner == this) {
        prize = 10;
    } else if (winner == nullptr) {
        prize = -1;
    } else {
        prize = -10;
    }
    recording.replay([&](const GameStateSnapshot& before, const GameStateSnapshot& after, Action a, Action b) {
        if(recording.playerA() == this) {
            int beforeIndex = QState::configToIndex(before.stateA, before.stateB);
            int afterIndex = QState::configToIndex(after.stateA, after.stateB);
            state_.update(beforeIndex, afterIndex, a, prize);
        } else {
            int beforeIndex = QState::configToIndex(before.stateB, before.stateA);
            int afterIndex = QState::configToIndex(after.stateB, after.stateA);
            state_.update(beforeIndex, afterIndex, b, prize);
        }
    });
}