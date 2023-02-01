#include "gamearena.h"
#include <string>
#include <fmt/core.h>

#define MAX_TURNS 100

GameArena::GameArena() : state_(nullptr, nullptr) { }

Player* GameArena::play(Player* a, Player* b, bool replayable) {
    state_ = GameState(a, b, replayable);
    int turns = 0;
    while(!state_.gameOver() && turns < MAX_TURNS) {
        ++turns;
        Action actionA = a->nextAction(state_.stateA(), state_.stateB());
        Action actionB = b->nextAction(state_.stateB(), state_.stateA());
        state_.resolve(actionA, actionB);
    }
    a->learnFromGame(state_);
    b->learnFromGame(state_);
    return state_.winner();
}

static std::string toString(Action action) {
    switch(action) {
        case Action::Reload: return "reload";
        case Action::Shield: return "shield";
        case Action::Shoot:  return "shoot ";
    }
    return "";
}

static std::string toString(const PlayerState& playerState) {
    return fmt::format("lives={} bullets={} shields={}", playerState.lives(), playerState.bullets(), playerState.remainingShields());
}

void GameArena::replay() const {
    state_.replay([](const GameState& s, Action a, Action b) {
        fmt::print("{}  A={}  B={}  {}\n", toString(s.stateA()), toString(a), toString(b), toString(s.stateB()));
    });
    fmt::print("{}  A={}  B={}  {}\n", toString(state_.stateA()), "      ", "      ", toString(state_.stateB()));
}