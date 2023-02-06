#include "capi.h"
#include "player.h"
#include "players/random.h"
#include "players/qlearner.h"
#include "players/shapley.h"

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

    JBPlayer* jb_createPlayer(JBPlayerType type, int seed) {
        std::unique_ptr<Player> p;
        switch(type) {
            case JBPlayerType::RANDOM: p = std::make_unique<RandomPlayer>(seed); break;
            case JBPlayerType::QLEARNER: p = std::make_unique<QLearner>(seed); break;
            case JBPlayerType::SHAPLEY: p = std::make_unique<Shapley>(seed); break;
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

    int jb_lives(JBPlayerState* state) {
        if(!state) return -1;
        return state->lives;
    }

    int jb_bullets(JBPlayerState* state) {
        if(!state) return -1;
        return state->bullets;
    }
    
    int jb_remainingShields(JBPlayerState* state) {
        if(!state) return -1;
        return state->remainingShields;
    }

    static JBAction toJBAction(Action a) {
        switch(a) {
            case Action::Reload: return JBAction::RELOAD;
            case Action::Shield: return JBAction::SHIELD;
            case Action::Shoot: return JBAction::SHOOT;
        }
        return JBAction::SHOOT;
    }

    static Action fromJBAction(JBAction a) {
        switch(a) {
            case JBAction::RELOAD: return Action::Reload;
            case JBAction::SHIELD: return Action::Shield;
            case JBAction::SHOOT: return Action::Shoot;
        }
        return Action::Shoot;
    }

    JBAction jb_play(JBPlayer* player, JBPlayerState* ownState, JBPlayerState* opponentState) {
        if(!player) return JBAction::SHOOT;
        if(!ownState) return JBAction::SHOOT;
        if(!opponentState) return JBAction::SHOOT;
        PlayerState mine = PlayerState::from(ownState->lives, ownState->bullets, ownState->remainingShields);
        PlayerState theirs = PlayerState::from(opponentState->lives, opponentState->bullets, opponentState->remainingShields);
        Action a = player->playerHandle->nextAction(mine, theirs);
        return toJBAction(a);
    }

    int jb_applyActions(JBPlayer* playerA, JBPlayer* playerB, JBPlayerState* stateA, JBPlayerState* stateB, JBAction actionA, JBAction actionB) {
        if(!playerA) return -1;
        if(!playerB) return -1;
        if(!stateA) return -1;
        if(!stateB) return -1;
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
        return 0;
    }
}