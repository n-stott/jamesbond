#include "capi.h"
#include "player.h"
#include "players/random.h"
#include "players/qlearner.h"
#include "players/shapley.h"
#include "tourney.h"

#include <memory>

extern "C" {

    struct JBPlayer {
        JBPlayer(std::unique_ptr<Player> ph) : playerHandle(std::move(ph)) { }
        std::unique_ptr<Player> playerHandle;
    };

    struct JBPlayerState {
        JBPlayerState(int lives, int bullets, int remainingShields) : lives(lives), bullets(bullets), remainingShields(remainingShields) { }
        int lives;
        int bullets;
        int remainingShields;
    };

    struct JBRules {
        JBRules(int startLives, int maxBullets, int maxShields) :startLives(startLives), maxBullets(maxBullets), maxShields(maxShields) { }
        int startLives;
        int maxBullets;
        int maxShields;
    };

    JBRules* jb_createRules(int startLives, int maxBullets, int maxShields) {
        std::unique_ptr<JBRules> rules = std::make_unique<JBRules>(startLives, maxBullets, maxShields);
        return rules.release();
    }

    void jb_destroyRules(JBRules* rules) {
        if(!rules) return;
        delete rules;
    }

    JBPlayer* jb_createPlayer(JBPlayerType type, int seed) {
        std::unique_ptr<Player> p;
        switch(type) {
            case JBPlayerType::RANDOM: {
                p = std::make_unique<RandomPlayer>(seed);
                break;
            }
            case JBPlayerType::QLEARNER: {
                p = std::make_unique<QLearner>(seed);
                RandomPlayer r(seed+1);
                Tourney::Params semiB{false, true};
                Tourney::run(100000, &r, p.get(), semiB);
                break;
            }
            case JBPlayerType::SHAPLEY: {
                p = std::make_unique<Shapley>(seed);
                break;
            }
            default: break;
        }
        if(!p) return nullptr;
        std::unique_ptr<JBPlayer> jbp = std::make_unique<JBPlayer>(std::move(p));
        return jbp.release();
    }

    void jb_destroyPlayer(JBPlayer* player) {
        if(!player) return;
        delete player;
    }

    JBPlayerState* jb_createState(int lives, int bullets, int remainingShields) {
        std::unique_ptr<JBPlayerState> s = std::make_unique<JBPlayerState>(lives, bullets, remainingShields);
        return s.release();
    }

    void jb_destroyState(JBPlayerState* state) {
        if(!state) return;
        delete state;
    }

    JBError jb_lives(JBPlayerState* state, int* lives) {
        if(!state) return JBError::INVALID_STATE;
        *lives = state->lives;
        return JBError::NONE;
    }

    JBError jb_bullets(JBPlayerState* state, int* bullets) {
        if(!state) return JBError::INVALID_STATE;
        *bullets = state->bullets;
        return JBError::NONE;
    }
    
    JBError jb_remainingShields(JBPlayerState* state, int* remainingShields) {
        if(!state) return JBError::INVALID_STATE;
        *remainingShields = state->remainingShields;
        return JBError::NONE;
    }

    static Action fromJBAction(JBAction a) {
        switch(a) {
            case JBAction::RELOAD: return Action::Reload;
            case JBAction::SHIELD: return Action::Shield;
            case JBAction::SHOOT: return Action::Shoot;
        }
        return Action::Shoot;
    }

    static bool isStateValid(const JBPlayerState& state, const JBRules& rules) {
        if(state.lives < 0 || state.lives > rules.startLives) return false;
        if(state.bullets < 0 || state.bullets > rules.maxBullets) return false;
        if(state.remainingShields < 0 || state.remainingShields > rules.maxShields) return false;
        return true;
    }

    JBError jb_play(JBPlayer* player, JBPlayerState* ownState, JBPlayerState* opponentState, JBRules* rules, JBAction* action) {
        if(!player) return JBError::INVALID_PLAYER;
        if(!ownState) return JBError::INVALID_STATE;
        if(!opponentState) return JBError::INVALID_STATE;
        if(!rules) return JBError::INVALID_RULES;
        if(!isStateValid(*ownState, *rules)) return JBError::INVALID_STATE;
        if(!isStateValid(*opponentState, *rules)) return JBError::INVALID_STATE;
        PlayerState mine = PlayerState::from(ownState->lives, ownState->bullets, ownState->remainingShields);
        PlayerState theirs = PlayerState::from(opponentState->lives, opponentState->bullets, opponentState->remainingShields);
        Action a = player->playerHandle->nextAction(mine, theirs);
        switch(a) {
            case Action::Reload: *action = JBAction::RELOAD; return JBError::NONE;
            case Action::Shield: *action = JBAction::SHIELD; return JBError::NONE;
            case Action::Shoot: *action = JBAction::SHOOT; return JBError::NONE;
        }
        return JBError::INVALID_ACTION;
    }

    JBError jb_applyActions(JBPlayer* playerA, JBPlayer* playerB, JBPlayerState* stateA, JBPlayerState* stateB, JBAction actionA, JBAction actionB) {
        if(!playerA) return JBError::INVALID_PLAYER;
        if(!playerB) return JBError::INVALID_PLAYER;
        if(!stateA) return JBError::INVALID_STATE;
        if(!stateB) return JBError::INVALID_STATE;
        PlayerState sa = PlayerState::from(stateA->lives, stateA->bullets, stateA->remainingShields);
        PlayerState sb = PlayerState::from(stateB->lives, stateB->bullets, stateB->remainingShields);
        GameState gs = GameState::from(playerA->playerHandle.get(),
                                       playerB->playerHandle.get(),
                                       sa,
                                       sb);
        gs.resolve(fromJBAction(actionA), fromJBAction(actionB));
        stateA->lives = gs.stateA().lives();
        stateB->lives = gs.stateB().lives();
        stateA->bullets = gs.stateA().bullets();
        stateB->bullets = gs.stateB().bullets();
        stateA->remainingShields = gs.stateA().remainingShields();
        stateB->remainingShields = gs.stateB().remainingShields();
        return JBError::NONE;
    }
}