#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <memory>

struct Card {
    int rank; // from 6 to 14 (Jack - 11, Lady - 12, King - 13, Ace - 14)
};

class GameState {
public:
    explicit GameState(std::vector<std::vector<Card>> &&decks, int number_of_decks, int cards_in_deck, int bottom_rank);

    [[nodiscard]] std::string GetHash() const;

    // To estimate potential of game state
    [[nodiscard]] int GetPriority() const;

    bool operator<(const GameState& other_state) const;

    [[nodiscard]] bool IsFinal() const {
        return final_;
    }

    [[nodiscard]] int GetTopCard(int ind) const {
        if (decks_[ind].empty()) return 2000000000;
        return decks_[ind].back().rank;
    }

    [[nodiscard]] bool IsDeckEmpty(int ind) const {
        return decks_[ind].empty();
    }

    [[nodiscard]] std::vector<std::vector<Card>> GetCopyOfDecks() const {
        return decks_;
    }

    [[nodiscard]] bool CanDeckBeCollapsed(int ind) {
        if (decks_[ind].size() != cards_in_deck_ || decks_[ind][0].rank != bottom_rank_) {
            return false;
        }

        for (int i = 1; i < cards_in_deck_; ++i) {
            if ((decks_[ind][i].rank - decks_[ind][i - 1].rank) != -1) {
                return false;
            }
        }

        return true;
    }

    bool operator==(const GameState &other_state) const;
private:
    void Validate();

    int number_of_decks_;
    int cards_in_deck_;
    int bottom_rank_;
    std::vector<std::vector<Card>> decks_;
    std::string hash_string_;
    bool final_;
};

GameState::GameState(std::vector<std::vector<Card>> &&decks, int number_of_decks, int cards_in_deck, int bottom_rank) {
    number_of_decks_ = number_of_decks;
    cards_in_deck_ = cards_in_deck;
    bottom_rank_ = bottom_rank;
    final_ = true;
    decks_ = decks;

    // collapse decks if can
    Validate();

    for (auto & deck : decks) {
        if (deck.empty()) {
            for (int i = 0; i < cards_in_deck; ++i) {
                hash_string_ += "00";
            }
        } else {
            final_ = false;
            for (auto & val : deck) {
                std::string num = std::to_string(val.rank);
                if (val.rank < 10) {
                    num = "0" + std::to_string(val.rank);
                }
                hash_string_ += num;
            }
            for (int i = deck.size(); i < cards_in_deck; ++i) {
                hash_string_ += "00";
            }
        }
    }

}

std::string GameState::GetHash() const {
    return hash_string_;
}

int GameState::GetPriority() const {
    if (this->final_) {
        return 2000000000; // Very big number
    }

    int potential = 0;

    for (auto & deck : decks_) {
        if (deck.empty()) continue;

        for (int j = 1; j < decks_.size(); ++j) {
            potential += ((deck[j].rank - deck[j - 1].rank) == 1) * 20;
        }
    }

    for (auto & deck : decks_) {
        potential += deck.empty() * 50;
    }

    for (int i = 0; i < decks_.size(); ++i) {
        if (decks_[i].empty()) continue;
        for (int j = 0; j < decks_.size(); ++j) {
            if (i == j) break;

            if (decks_[j].empty() || (decks_[i].back().rank - decks_[j].back().rank) == -1) {
                potential += 5000 * (decks_[j].size() + 1);
            }
        }
    }

    return potential;
}

bool GameState::operator<(const GameState &other_state) const {
    return this->GetPriority() < other_state.GetPriority();
}

bool GameState::operator==(const GameState &other_state) const {
    return this->GetPriority() == other_state.GetPriority();
}

void GameState::Validate() {
    for (int i = 0; i < decks_.size(); ++i) {
        if (CanDeckBeCollapsed(i)) {
            decks_[i].resize(0);
        }
    }
}

class SolitaireGame {
public:
    SolitaireGame(int _number_of_decks, int _cards_in_deck, int bottom_rank) {
        number_of_decks_ = _number_of_decks;
        cards_in_deck_ = _cards_in_deck;
        bottom_rank_ = bottom_rank;

        ReadDecks();
    }

    void ReadDecks() {
        std::vector<std::vector<Card>> init(number_of_decks_, std::vector<Card>(cards_in_deck_));

        // Reading cards from bottom to the top in each deck
        for (int i = 0; i < number_of_decks_; ++i) {
            for (int j = 0; j < cards_in_deck_; ++j) {
                std::cin >> init[i][j].rank;
            }
        }

        GameState init_state(std::move(init), number_of_decks_, cards_in_deck_, bottom_rank_);
        hashes_processed_.insert({init_state.GetHash(), 0});
        process_queue_.emplace(init_state);
    }

    bool Solve() {
        bool ans = false;

        while (!process_queue_.empty()) {
            ans = Step();
            if (ans) break;
        }

        return ans;
    }

private:
    bool Step() {
        GameState state = process_queue_.top();
        process_queue_.pop();

//        std::cout << state.GetPriority() << std::endl;

        if (state.IsFinal()) {
            return true;
        }

        std::string cur_hash = state.GetHash();

        if (hashes_processed_.find(cur_hash) != hashes_processed_.end() &&
            hashes_processed_[cur_hash] == 1) {
            return false;
        }

        hashes_processed_[cur_hash] = 1;
        SpawnNewStates(std::move(state));

        return false;
    }

    void SpawnNewStates(GameState&& state) {
        for (int i = 0; i < number_of_decks_; ++i) {
            if (state.IsDeckEmpty(i)) continue;

            // Move top card from deck i to another deck
            for (int j = 0; j < number_of_decks_; ++j) {
                if (i == j) {
                    continue;
                }

                if (state.GetTopCard(i) < state.GetTopCard(j)) {
                    std::vector<std::vector<Card>> copy = state.GetCopyOfDecks();
                    Card moving_card = copy[i].back();
                    copy[i].pop_back();
                    copy[j].emplace_back(moving_card);

                    GameState new_state(std::move(copy), number_of_decks_, cards_in_deck_, bottom_rank_);
                    std::string new_hash = new_state.GetHash();

                    if (hashes_processed_.find(new_hash) != hashes_processed_.end()) {
                        continue;
                    } else {
                        process_queue_.emplace(new_state);
                        hashes_processed_[new_hash] = 0;
                    }
                }
            }
        }

        hashes_processed_[state.GetHash()] = 1;
    }

    int number_of_decks_;
    int cards_in_deck_;
    int bottom_rank_;
    std::map<std::string, int> hashes_processed_; // to avoid cycling in Step()
    std::priority_queue<GameState, std::vector<GameState>> process_queue_;
};

int main() {
    const int kNumberOfDecks = 8;
    const int kCardsInDeck = 9;

    SolitaireGame new_game(kNumberOfDecks, kCardsInDeck, 14);
    if (new_game.Solve()) {
        std::cout << "Можно убрать все карты.";
    } else std::cout << "Нельзя убрать все карты.";

    std::cout << std::endl;

    return 0;
}
