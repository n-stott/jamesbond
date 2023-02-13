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
        JBPlayerState(int lives, int bullets, int remainingShields) : state(PlayerState::from(lives, bullets, remainingShields)) { }
        PlayerState state;
    };

    struct JBRules {
        JBRules(int startLives, int maxBullets, int maxShields, int maxTurns) : rules{startLives, maxBullets, maxShields, maxTurns} { }
        Rules rules;
    };

    JBRules* jb_createRules(int startLives, int maxBullets, int maxShields, int maxTurns) {
        std::unique_ptr<JBRules> rules = std::make_unique<JBRules>(startLives, maxBullets, maxShields, maxTurns);
        return rules.release();
    }

    void jb_destroyRules(JBRules* rules) {
        if(!rules) return;
        delete rules;
    }

    JBPlayer* jb_createPlayer(JBPlayerType type, JBRules* rules, int seed) {
        if(!rules) return nullptr;
        std::unique_ptr<Player> p;
        switch(type) {
            case JBPlayerType::RANDOM: {
                p = std::make_unique<RandomPlayer>(rules->rules, seed);
                break;
            }
            case JBPlayerType::QLEARNER: {
                p = QLearner::tryCreate(rules->rules, seed);
                if(!p) return nullptr;
                RandomPlayer r(!!rules ? rules->rules : Rules{}, seed+1);
                Tourney::Params semiB{false, true};
                Tourney::play2v2(100000, &r, p.get(), semiB);
                break;
            }
            case JBPlayerType::SHAPLEY: {
                p = ShapleyPlayer::tryCreate(rules->rules, seed);
                if(!p) return nullptr;
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
        *lives = state->state.lives();
        return JBError::NONE;
    }

    JBError jb_bullets(JBPlayerState* state, int* bullets) {
        if(!state) return JBError::INVALID_STATE;
        *bullets = state->state.bullets();
        return JBError::NONE;
    }
    
    JBError jb_remainingShields(JBPlayerState* state, int* remainingShields) {
        if(!state) return JBError::INVALID_STATE;
        *remainingShields = state->state.remainingShields();
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
        if(state.state.lives() < 0 || state.state.lives() > rules.rules.startLives) return false;
        if(state.state.bullets() < 0 || state.state.bullets() > rules.rules.maxBullets) return false;
        if(state.state.remainingShields() < 0 || state.state.remainingShields() > rules.rules.maxShields) return false;
        return true;
    }

    JBError jb_play(JBPlayer* player, JBPlayerState* ownState, JBPlayerState* opponentState, JBRules* rules, JBAction* action) {
        if(!player) return JBError::INVALID_PLAYER;
        if(!ownState) return JBError::INVALID_STATE;
        if(!opponentState) return JBError::INVALID_STATE;
        if(!rules) return JBError::INVALID_RULES;
        if(!isStateValid(*ownState, *rules)) return JBError::INVALID_STATE;
        if(!isStateValid(*opponentState, *rules)) return JBError::INVALID_STATE;
        PlayerState mine = ownState->state;
        PlayerState theirs = opponentState->state;
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
        if(!(playerA->playerHandle->rules() == playerB->playerHandle->rules())) return JBError::INVALID_RULES;
        PlayerState sa = stateA->state;
        PlayerState sb = stateB->state;
        GameState gs = GameState::from(sa, sb);
        const Rules& rules = playerA->playerHandle->rules();
        gs.resolve(fromJBAction(actionA), fromJBAction(actionB), rules);
        stateA->state = gs.stateA();
        stateB->state = gs.stateB();
        return JBError::NONE;
    }
}