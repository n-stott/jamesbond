#include "capi.h"
#include "fmt/core.h"

int main() {
    JBPlayer* p0 = jb_createPlayer(JBPlayerType::RANDOM, 0);
    JBPlayer* p1 = jb_createPlayer(JBPlayerType::QLEARNER, 1);

    JBPlayerState* s0 = jb_createState(5, 0, 0);
    JBPlayerState* s1 = jb_createState(5, 0, 0);

    JBRules* rules = jb_createRules(5, 5, 5);

    auto onReturn = [&]() {
        jb_destroyPlayer(p0);
        jb_destroyPlayer(p1);
        jb_destroyState(s0);
        jb_destroyState(s1);
        jb_destroyRules(rules);
    };

    JBAction a0;
    JBAction a1;

    if(jb_play(p0, s0, s1, rules, &a0) < 0) {
        onReturn();
        return 1;
    }
    if(jb_play(p1, s1, s0, rules, &a1) < 0) {
        onReturn();
        return 1;
    }

    auto JBActionToString = [](JBAction a) -> const char* {
        switch(a) {
            case RELOAD: return "reload";
            case SHIELD: return "shield";
            case SHOOT: return "shoot";
        }
        return "";
    };

    auto printState = [](const char* player, JBPlayerState* state) {
        int lives = 0;
        int bullets = 0;
        int remainingShields = 0;
        if(jb_lives(state, &lives) < 0) return;
        if(jb_bullets(state, &bullets) < 0) return;
        if(jb_remainingShields(state, &remainingShields) < 0) return;
        fmt::print("Player {} has {} lives, {} bullets and {} remaining shields\n", 
                player, lives, bullets, remainingShields);
    };

    printState("0", s0);
    printState("1", s1);

    fmt::print("Player 0 plays \"{}\"\n", JBActionToString(a0));
    fmt::print("Player 1 plays \"{}\"\n", JBActionToString(a1));

    if(jb_applyActions(p0, p1, s0, s1, a0, a1) < 0) {
        fmt::print("play failed\n");
    }

    printState("0", s0);
    printState("1", s1);

    return 0;
}