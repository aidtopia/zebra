#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// A framework for solving logic puzzles.

enum Truth { NO = -1, MAYBE = 0, YES = 1 };
Truth operator!(Truth t) { return static_cast<Truth>(-t); }

using Index = std::size_t;
using IndexList = std::vector<Index>;

enum class Result { CONFLICT = -1, NO_CHANGE = 0, PROGRESS = 1 };

class Solution {
    public:
        explicit Solution(std::size_t slots) : m_table(slots, MAYBE) {}

        Truth operator[](Index index) const { return m_table[index]; }

        std::size_t size() const { return m_table.size(); }

        Index FirstMaybe() const {
            const auto it =
                std::find(m_table.begin(), m_table.end(), MAYBE);
            return std::distance(m_table.begin(), it);
        }

        Result Set(Index index, Truth value) {
            assert(index < m_table.size());
            assert(value != MAYBE);
            if (m_table[index] == value) return Result::NO_CHANGE;
            if (m_table[index] != MAYBE) return Result::CONFLICT;
            m_table[index] = value;
            return Result::PROGRESS;
        }

        std::size_t Count(const IndexList &indexes, Truth value) const {
            return std::count_if(indexes.begin(), indexes.end(),
                                 [&](Index i) { return m_table[i] == value; });
        }

    private:
        std::vector<Truth> m_table;
};

class Puzzle {
    public:
        explicit Puzzle(std::size_t slots) : m_slot_count(slots) {}

        std::vector<Solution> Solve() {
            std::vector<Solution> solutions;
            std::stack<Solution> candidates;
            candidates.emplace(Solution(m_slot_count));
            while (!candidates.empty()) {
                // Deduce as much as we can.
                Solution &candidate = candidates.top();
                Result result;
                do {
                    result = ApplyConstraints(candidate);
                } while (result == Result::PROGRESS);

                if (result == Result::CONFLICT) {
                    // This candidate is a dead end.
                    candidates.pop();
                    std::cout << "Pruning: Candidate is not consistent.\n";
                    continue;
                }
                
                const Index first_maybe = candidate.FirstMaybe();
                if (first_maybe == candidate.size()) {
                    // No MAYBEs left, so the candidate is an actual solution.
                    solutions.push_back(std::move(candidate));
                    candidates.pop();
                    std::cout << "Solution!\n";
                    continue;
                }
                
                // Replace current candidate with two guesses.
                Solution guess1 = candidate;  // copy
                Solution guess2 = candidate;  // copy
                candidates.pop();
                guess1.Set(first_maybe, NO);
                candidates.push(std::move(guess1));
                guess2.Set(first_maybe, YES);
                candidates.push(std::move(guess2));
                std::cout << "Guessing: Index " << first_maybe << ".\n";
            }
            return solutions;
        }

        class BasicConstraint {
            public:
                explicit BasicConstraint(const std::string &name) :
                    m_name(name) {}
                virtual ~BasicConstraint() = default;
                virtual Result Evaluate(Solution &s) const = 0;
                const std::string &GetName() const { return m_name; }
            private:
                std::string m_name;
        };

        template <typename T, typename... Args>
        void Constrain(Args... args) {
            m_constraints.push_back(
                std::make_unique<T>(std::forward<Args>(args)...)
            );
        }

    private:
        Result ApplyConstraints(Solution &candidate) const {
            Result result = Result::NO_CHANGE;
            for (const auto &c : m_constraints) {
                switch (c->Evaluate(candidate)) {
                    case Result::CONFLICT:
                        std::cout << "Conflict: " << c->GetName() << '\n';
                        return Result::CONFLICT;
                    case Result::NO_CHANGE:
                        break;
                    case Result::PROGRESS:
                        std::cout << "Progress: " << c->GetName() << '\n';
                        result = Result::PROGRESS;
                        break;
                }
            }
            return result;
        }

        std::size_t m_slot_count;
        std::vector<std::unique_ptr<BasicConstraint>> m_constraints;
};

class Fixed : public Puzzle::BasicConstraint {
    public:
        Fixed(const std::string &name, Index index, Truth value = YES) :
            BasicConstraint(name), m_index(index), m_value(value) {}

        Result Evaluate(Solution &s) const override {
            return s.Set(m_index, m_value);
        }

    private:
        Index m_index;
        Truth m_value;
};

class Identical : public Puzzle::BasicConstraint {
    public:
        Identical(const std::string &name, Index index1, Index index2) :
            BasicConstraint(name)
        {
            m_indexes1.push_back(index1);
            m_indexes2.push_back(index2);
        }

        Identical(const std::string &name, IndexList &&indexes1, IndexList &&indexes2) :
            BasicConstraint(name), m_indexes1(std::move(indexes1)), m_indexes2(std::move(indexes2)) {}

        Result Evaluate(Solution &s) const override {
            assert(m_indexes1.size() == m_indexes2.size());
            Result result = Result::NO_CHANGE;
            for (Index i = 0; i < m_indexes1.size(); ++i) {
                if (s[m_indexes1[i]] == YES && s[m_indexes2[i]] == NO) return Result::CONFLICT;
                if (s[m_indexes2[i]] == YES && s[m_indexes1[i]] == NO) return Result::CONFLICT;
                if (s[m_indexes1[i]] == MAYBE && s[m_indexes2[i]] != MAYBE) {
                    s.Set(m_indexes1[i], s[m_indexes2[i]]);
                    result = Result::PROGRESS;
                }
                if (s[m_indexes2[i]] == MAYBE && s[m_indexes1[i]] != MAYBE) {
                    s.Set(m_indexes2[i], s[m_indexes1[i]]);
                    result = Result::PROGRESS;
                }
            }
            return result;
        }

    private:
        IndexList m_indexes1;
        IndexList m_indexes2;
};

class ExactlyNOf : public Puzzle::BasicConstraint {
    public:
        ExactlyNOf(const std::string &name, Index n, IndexList &&indexes, Truth value = YES) :
            BasicConstraint(name),
            m_number(n), m_indexes(std::move(indexes)), m_value(value) {}

        Result Evaluate(Solution &s) const override {
            const std::size_t matches = s.Count(m_indexes, m_value);
            const std::size_t maybes = s.Count(m_indexes, MAYBE);
            if (maybes < m_number - matches) return Result::CONFLICT;
            if (matches > m_number) return Result::CONFLICT;
            if (maybes > 0) {
                if (matches == m_number) {
                    for (Index index : m_indexes) {
                        if (s[index] == MAYBE) s.Set(index, !m_value);
                    }
                    return Result::PROGRESS;
                }
                if (maybes == m_number - matches) {
                    for (Index index : m_indexes) {
                        if (s[index] == MAYBE) s.Set(index, m_value);
                    }
                    return Result::PROGRESS;
                }
            }
            return Result::NO_CHANGE;
        }

    private:
        std::size_t m_number;
        IndexList m_indexes;
        Truth m_value;
};

class OneIfAny : public Puzzle::BasicConstraint {
    public:
        OneIfAny(const std::string &name, Index one, IndexList &&any) :
            BasicConstraint(name), m_one(one), m_any(std::move(any)) {}

        Result Evaluate(Solution &s) const override {
            const std::size_t noes = std::count_if(m_any.begin(), m_any.end(), [&](Index i) {
                return s[i] == NO;
            });
            if (noes == m_any.size()) return s.Set(m_one, NO);
            if (s[m_one] == YES) {
                const std::size_t maybes = std::count_if(m_any.begin(), m_any.end(), [&](Index i) {
                    return s[i] == MAYBE;
                });
                if (maybes == 1 && noes + 1 == m_any.size()) {
                    for (auto i : m_any) {
                        if (s[i] == MAYBE) return s.Set(i, YES);
                    }
                }
            }
            return Result::NO_CHANGE;
        }

    private:
        Index m_one;
        IndexList m_any;
};


// The Zebra puzzle

enum House {
    house1, house2, house3, house4, house5,
    house_count
};

constexpr std::array<House, house_count> houses = {
    house1, house2, house3, house4, house5
};

enum Item {
    English,        Japanese,   Norwegian,  Spanish,    Ukrainian,
    blue,           green,      ivory,      red,        yellow,
    dog,            fox,        horse,      snails,     zebra,
    coffee,         juice,      milk,       tea,        water,
    Chesterfields,  Kools,      LuckyStrike,OldGold,    Parliaments,
    item_count
};

std::string_view ItemName(Item item) {
    constexpr std::string_view names[item_count] = {
        "Englishman", "Japanese man", "Norwegian", "Spaniard", "Ukrainian",
        "blue", "green", "ivory", "red", "yellow",
        "dog", "fox", "horse", "snails", "zebra",
        "coffee", "juice", "milk", "tea", "water",
        "Chesterfields", "Kools", "Lucky Strike", "Old Gold", "Parliaments"
    };
    return names[item];
}

enum Category {
    nationality, color, pet, beverage, cigarette,
    category_count
};

std::string_view CatName(Category cat) {
    constexpr std::string_view names[category_count] = {
        "nationality", "color", "pet", "beverage", "cigarette brand"
    };
    return names[cat];
}

constexpr std::array<Item, 5> nationalities = {
    English, Japanese, Norwegian, Spanish, Ukrainian
};

constexpr std::array<Item, 5> colors = {
    blue, green, ivory, red, yellow
};

constexpr std::array<Item, 5> pets = {
    dog, fox, horse, snails, zebra
};

constexpr std::array<Item, 5> beverages = {
    coffee, juice, milk, tea, water
};

constexpr std::array<Item, 5> cigarettes = {
    Chesterfields, Kools, LuckyStrike, OldGold, Parliaments
};

constexpr std::array<std::pair<Category, std::array<Item, 5>>, 5> categories = {
    std::make_pair(nationality, nationalities),
    std::make_pair(color, colors),
    std::make_pair(pet, pets),
    std::make_pair(beverage, beverages),
    std::make_pair(cigarette, cigarettes)
};

constexpr Index IndexOf(House house, Item item) {
    return static_cast<Index>(house)*item_count + item;
}

IndexList Row(Item item) {
    IndexList row;
    for (auto house : houses) {
        row.push_back(IndexOf(house, item));
    }
    return row;
}

IndexList Col(House house, Category cat) {
    IndexList col;
    const auto &items = categories[cat].second;
    for (const auto item : items) {
        col.push_back(IndexOf(house, item));
    }
    return col;
}

IndexList Neighbors(House house, Item item) {
    IndexList neighbors;
    if (house1 < house)
        neighbors.push_back(IndexOf(static_cast<House>(house - 1), item));
    if (house < house5)
        neighbors.push_back(IndexOf(static_cast<House>(house + 1), item));
    return neighbors;
}

std::ostream &operator<<(std::ostream &out, const Solution &s) {
    constexpr std::string_view separator =
        "+-----+-----+-----+-----+-----+\n";
    for (const auto &[category, items] : categories) {
        out << separator;
        for (const auto &item : items) {
            out << '|';
            for (const auto house : houses) {
                switch (s[IndexOf(house, item)]) {
                    case YES:   out << " YES "; break;
                    case MAYBE: out << "     "; break;
                    case NO:    out << " no  "; break;
                }
                out << '|';
            }
            out << ' ' << ItemName(item) << '\n';
        }
    }
    return out << separator;
}

int main() {
    Puzzle puzzle(static_cast<std::size_t>(house_count) * item_count);
    // clue 1
    for (const auto &[category, items] : categories) {
        for (const auto house : houses) {
            std::stringstream ss;
            ss << "Exactly 1 " << CatName(category) << " in each house.";
            puzzle.Constrain<ExactlyNOf>(ss.str(), 1, Col(house, category));
        }
        for (const auto &item : items) {
            std::stringstream ss;
            ss << "Exactly 1 house has the " << ItemName(item) << '.';
            puzzle.Constrain<ExactlyNOf>(ss.str(), 1, Row(item));
        }
    }
    // clue 2
    puzzle.Constrain<Identical>(
        "The Englishman lives in the red house.",
        Row(English), Row(red));
    // clue 3
    puzzle.Constrain<Identical>(
        "The Spaniard owns the dog.",
        Row(Spanish), Row(dog));
    // clue 4
    puzzle.Constrain<Identical>(
        "Coffee is drunk in the green house.",
        Row(coffee), Row(green));
    // clue 5
    puzzle.Constrain<Identical>(
        "The Ukrainian drinks tea.",
        Row(Ukrainian), Row(tea));
    // clue 6
    IndexList greens = Row(green);
    std::rotate(greens.begin(), greens.begin() + 1, greens.end());
    puzzle.Constrain<Identical>(
        "The green house is immediately to the right of the ivory house.",
        Row(ivory), std::move(greens));
    puzzle.Constrain<Fixed>(
        "The green house can't be first because it's to the right of the ivory.", 
        IndexOf(house1, green), NO);
    // clue 7
    puzzle.Constrain<Identical>(
        "The Old Gold smoker owns snails.",
        Row(OldGold), Row(snails));
    // clue 8
    puzzle.Constrain<Identical>(
        "Kools are smoked in the yellow house.",
        Row(Kools), Row(yellow));
    // clue 9
    puzzle.Constrain<Fixed>(
        "Milk is drunk in the middle house.",
        IndexOf(house3, milk), YES);
    // clue 10
    puzzle.Constrain<Fixed>(
        "The Norwegian lives in the first house.",
        IndexOf(house1, Norwegian), YES);
    // clue 11
    for (auto h : houses) {
        puzzle.Constrain<OneIfAny>(
            "Chesterfields are smoked in the house next to the house with the fox.",
            IndexOf(h, Chesterfields), Neighbors(h, fox));
    }
    // clue 12
    for (auto h : houses) {
        puzzle.Constrain<OneIfAny>(
            "Kools are smoked in the house next to the house where the horse is kept.",
            IndexOf(h, Kools), Neighbors(h, horse));
    }
    // clue 13
    puzzle.Constrain<Identical>(
        "The Lucky Strike smoker drinks orange juice.",
        Row(LuckyStrike), Row(juice));
    // clue 14
    puzzle.Constrain<Identical>(
        "The Japanese man smokes Parliaments.",
        Row(Japanese), Row(Parliaments));
    // clue 15
    for (auto h : houses) {
        puzzle.Constrain<OneIfAny>(
            "The Norwegian lives next to the blue house.",
            IndexOf(h, Norwegian), Neighbors(h, blue));
    }

    const auto solutions = puzzle.Solve();
    for (const auto &solution : solutions) {
        std::cout << solution << '\n';
    }
    return 0;
}
