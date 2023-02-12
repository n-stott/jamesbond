#ifndef CAPI_H
#define CAPI_H

extern "C" {

    struct JBRules;
    struct JBPlayer;
    struct JBPlayerState;

    enum JBPlayerType : int {
        RANDOM,
        QLEARNER,
        SHAPLEY,
    };

    enum JBAction : int {
        RELOAD,
        SHIELD,
        SHOOT,
    };

    enum JBError : int {
        NONE = 0,
        INVALID_RULES = -1,
        INVALID_PLAYER = -2,
        INVALID_STATE = -3,
        INVALID_ACTION = -4,
    };

    JBRules* jb_createRules(int startLives, int maxBullets, int maxShields);
    void jb_destroyRules(JBRules* rules);

    JBPlayer* jb_createPlayer(JBPlayerType type, JBRules* rules, int seed);
    void jb_destroyPlayer(JBPlayer* player);

    JBPlayerState* jb_createState(int lives, int bullets, int remainingShields);
    void jb_destroyState(JBPlayerState* state);

    JBError jb_lives(JBPlayerState* state, int* lives);
    JBError jb_bullets(JBPlayerState* state, int* bullets);
    JBError jb_remainingShields(JBPlayerState* state, int* remainingShields);

    JBError jb_play(JBPlayer* player, JBPlayerState* ownState, JBPlayerState* opponentState, JBRules* rules, JBAction* action);

    JBError jb_applyActions(JBPlayer* playerA, JBPlayer* playerB, JBPlayerState* stateA, JBPlayerState* stateB, JBAction actionA, JBAction actionB);
    

}

#endif