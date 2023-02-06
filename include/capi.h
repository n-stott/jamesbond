#ifndef CAPI_H
#define CAPI_H

extern "C" {

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

    JBPlayer* jb_createPlayer(JBPlayerType type, int seed);
    void jb_destroyPlayer(JBPlayer* player);

    JBPlayerState* jb_createState(int lives, int bullets, int remainingShields);
    void jb_destroyState(JBPlayerState* state);

    int jb_lives(JBPlayerState* state);
    int jb_bullets(JBPlayerState* state);
    int jb_remainingShields(JBPlayerState* state);

    JBAction jb_play(JBPlayer* player, JBPlayerState* ownState, JBPlayerState* opponentState);
    int jb_applyActions(JBPlayer* playerA, JBPlayer* playerB, JBPlayerState* stateA, JBPlayerState* stateB, JBAction actionA, JBAction actionB);
    

}

#endif