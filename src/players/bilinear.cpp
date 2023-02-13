#include "players/bilinear.h"
#include "bilinearminmax.h"
#include <array>

static double playerStateValue(const Rules& rules, const PlayerState& s) {
    return (rules.maxShields+1)*((rules.maxBullets+1)*s.lives() + s.bullets()) + s.remainingShields();
}

// The quantity to be minimized by A
static double gameStateValue(const Rules& rules, const GameState& s) {
    return playerStateValue(rules, s.stateB()) - playerStateValue(rules, s.stateA());
}

Action BilinearPlayer::nextAction(const PlayerState& myState, const PlayerState& opponentState) {
    GameState s = GameState::from(myState, opponentState);
    const Player* opponent = this+0x100;
    std::array<std::array<double, 3>, 3> payoff;
    for(int a = 0; a < 3; ++a) {
        for(int b = 0; b < 3; ++b) {
            GameState t = s;
            t.resolve((Action)a, (Action)b, rules_);
            if(t.gameOver()) {
                const Player* winner = t.winner(this, opponent);
                if(winner == this) {
                    payoff[a][b] = -1000;
                } else if(winner == opponent) {
                    payoff[a][b] = +1000;
                } else {
                    payoff[a][b] = 0.0;
                }
            } else {
                payoff[a][b] = gameStateValue(rules_, t) - gameStateValue(rules_, s);
            }
        }
    }
    auto strategy = BilinearMinMax::solve(payoff);
    Action preferredAction = actionWithBias(rand_, strategy.p.p[0], strategy.p.p[1], strategy.p.p[2]);
    if(myState.isLegalAction(preferredAction, rules_)) return preferredAction;
    return myState.randomAllowedAction(&rand_, rules_);

}