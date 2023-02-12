#include "gamearena.h"
#include "gamerecording.h"
#include "fmt/core.h"
#include <string>

#define MAX_TURNS 100

GameArena::GameArena() : state_(nullptr, nullptr) { }

const Player* GameArena::play(Player* a, Player* b, GameRecording* recording) {
    if(!(a->rules() == b->rules())) return nullptr;
    const Rules& rules = a->rules();
    state_ = GameState(a, b);
    int turns = 0;
    if(recording) recording->clear();
    while(!state_.gameOver() && turns < rules.maxTurns) {
        ++turns;
        Action actionA = a->nextAction(state_.stateA(), state_.stateB());
        Action actionB = b->nextAction(state_.stateB(), state_.stateA());
        if(recording) recording->record(actionA, actionB);
        state_.resolve(actionA, actionB, rules);
    }
	const Player* winner = state_.winner();
	if(recording) recording->recordWinner(winner);
    return winner;
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

void GameArena::replay(const GameRecording& recording) const {
    recording.replay([](const GameStateSnapshot& s, const GameStateSnapshot&, Action a, Action b) {
        fmt::print("{}  A={}  B={}  {}\n", toString(s.stateA), toString(a), toString(b), toString(s.stateB));
    });
}
