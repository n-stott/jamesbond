#include "capi.h"
#include "fmt/core.h"

int main() {
    JBPlayer* p0 = jb_createPlayer(JBPlayerType::RANDOM, 0);
    JBPlayer* p1 = jb_createPlayer(JBPlayerType::QLEARNER, 1);

    JBPlayerState* s0 = jb_createState(5, 0, 5);
    JBPlayerState* s1 = jb_createState(5, 0, 5);

    JBAction a0 = jb_play(p0, s0, s1);
    JBAction a1 = jb_play(p1, s1, s0);

    auto JBActionToString = [](JBAction a) -> const char* {
        switch(a) {
            case RELOAD: return "reload";
            case SHIELD: return "shield";
            case SHOOT: return "shoot";
        }
        return "";
    };

    auto printState = [](const char* player, JBPlayerState* state) {
        fmt::print("Player {} has {} lives, {} bullets and {} remaining shields\n", 
                player,
                jb_lives(state),
                jb_bullets(state),
                jb_remainingShields(state));
    };

    printState("0", s0);
    printState("1", s1);

    fmt::print("Player 0 plays \"{}\"\n", JBActionToString(a0));
    fmt::print("Player 1 plays \"{}\"\n", JBActionToString(a1));

    int rc = jb_applyActions(p0, p1, s0, s1, a0, a1);
    if(rc < 0) {
        fmt::print("play failed\n");
        goto fail;
    }

    printState("0", s0);
    printState("1", s1);

    fail:
    jb_destroyPlayer(p0);
    jb_destroyPlayer(p1);
    jb_destroyState(s0);
    jb_destroyState(s1);

    return 0;
}