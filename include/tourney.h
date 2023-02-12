#ifndef TOURNEY_H
#define TOURNEY_H

#include "gamearena.h"
#include "gamerecording.h"
#include "player.h"
#include "fmt/core.h"
#include <algorithm>
#include <vector>
#include <memory>
#include <string>

class Tourney {
public:
    struct Result {
        int winsA = 0;
        int winsB = 0;
        int ties = 0;
    };

    struct Params {
        bool allowLearningA = true;
        bool allowLearningB = true;
    };

    void addPlayer(const std::string& name, Player* player) {
        if(!player) return;
        playerNames_.push_back(name);
        players_.push_back(player);
    }

    static Result play2v2(int rounds, Player* a, Player* b, const Params& params) {
        return playMatch(rounds, a, b, params);
    }

    void run(int roundsPerMatch = 1000) {
        Params noLearning { false, false };
        std::vector<double> playerScores(players_.size(), 0);
        for(size_t i = 0; i < players_.size(); ++i) {
            fmt::print("{:20}", playerNames_[i]);
            for(size_t j = 0; j < players_.size(); ++j) {
                if(i == j) {
                    fmt::print("      ");
                    continue;
                }
                auto result = playMatch(roundsPerMatch, players_[i], players_[j], noLearning);
                playerScores[i] += result.winsA;
                playerScores[j] += result.winsB;
                fmt::print("{:.2f}  ", 1.0*result.winsA/roundsPerMatch);
                if(result.winsA > result.winsB) {
                    playerScores[i] += 500;
                } else if (result.winsB > result.winsA) {
                    playerScores[j] += 500;
                } else {
                    playerScores[i] += 100;
                    playerScores[j] += 100;
                }
            }
            fmt::print("\n");
        }
        std::vector<std::pair<double, std::string>> scoredPlayers;
        scoredPlayers.reserve(players_.size());
        for(size_t i = 0; i < players_.size(); ++i) {
            scoredPlayers.emplace_back(playerScores[i],playerNames_[i]);
        }
        std::sort(scoredPlayers.begin(), scoredPlayers.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        for(const auto& sp : scoredPlayers) {
            fmt::print("{:20} : {:8}\n", sp.second, sp.first);
        }
    }

private:
    static Result playMatch(int rounds, Player* a, Player* b, const Params& params) {
        Result result;
        GameRecording recording(a, b);
        bool withRecording = params.allowLearningA || params.allowLearningB;
        for(int round = 0; round < rounds; ++round) {
            GameArena arena;
            const Player* winner = arena.play(a, b, withRecording ? &recording : nullptr);
            if(params.allowLearningA) a->learnFromGame(recording);
            if(params.allowLearningB) b->learnFromGame(recording);
            if(!winner) ++result.ties;
            if(winner == a) ++result.winsA;
            if(winner == b) ++result.winsB;
        }
        return result;
    }

    std::vector<std::string> playerNames_;
    std::vector<Player*> players_;
};

#endif