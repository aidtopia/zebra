// The Zebra puzzle
#include "solver_lib/constraints.h"
#include "solver_lib/solver.h"

#include <array>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <utility>

enum House {
    house1, house2, house3, house4, house5,
    house_count
};

constexpr std::array<House, house_count> houses = {
    house1, house2, house3, house4, house5
};

std::string_view HouseName(House h) {
    constexpr std::array<std::string_view, house_count> names = {
        "first house", "second house", "middle house", "fourth house", "last house"
    };
    return names[h];
}

enum Item {
    English,        Japanese,   Norwegian,  Spanish,    Ukrainian,
    blue,           green,      ivory,      red,        yellow,
    dog,            fox,        horse,      snail,      zebra,
    coffee,         juice,      milk,       tea,        water,
    Chesterfields,  Kools,      LuckyStrike,OldGold,    Parliaments,
    item_count
};

std::string_view ItemName(Item item) {
    constexpr std::string_view names[item_count] = {
        "Englishman", "Japanese man", "Norwegian", "Spaniard", "Ukrainian",
        "blue", "green", "ivory", "red", "yellow",
        "dog", "fox", "horse", "snail", "zebra",
        "coffee", "juice", "milk", "tea", "water",
        "Chesterfields", "Kools", "Lucky Strike", "Old Gold", "Parliaments"
    };
    return names[item];
}

auto constexpr solution_size =
    static_cast<std::size_t>(house_count) *
    static_cast<std::size_t>(item_count);

constexpr Index IndexOf(House house, Item item) {
    return static_cast<Index>(house)*item_count + item;
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
    dog, fox, horse, snail, zebra
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
    out << separator;
    return out;
}

// Returns the house associated with the item. This assumes a valid solution.
House HouseWith(Item item, Solution const &s) {
    for (auto const h : houses) {
        if (s[IndexOf(h, item)] == YES) return h;
    }
    return house_count;
}

Item ItemOf(Category cat, House h, Solution const &s) {
    for (auto const item : categories[cat].second) {
        if (s[IndexOf(h, item)] == YES) return item;
    }
    return item_count;
}

Item WhoHas(Item item, Solution const &s) {
    auto const house = HouseWith(item, s);
    if (house == house_count) return item_count;
    return ItemOf(nationality, house, s);
}

int main() {
    Puzzle puzzle(solution_size);
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
    puzzle.Constrain<Fixed>(
        "The green house cannot be first and to the right of the ivory house.",
        IndexOf(house1, green), NO);
    puzzle.Constrain<IfPThenQ>(
        "The green house is immediately to the right of the ivory house.",
        IndexOf(house2, green), IndexOf(house1, ivory));
    puzzle.Constrain<IfPThenQ>(
        "The green house is immediately to the right of the ivory house.",
        IndexOf(house3, green), IndexOf(house2, ivory));
    puzzle.Constrain<IfPThenQ>(
        "The green house is immediately to the right of the ivory house.",
        IndexOf(house4, green), IndexOf(house3, ivory));
    puzzle.Constrain<IfPThenQ>(
        "The green house is immediately to the right of the ivory house.",
        IndexOf(house5, green), IndexOf(house4, ivory));
    // clue 7
    puzzle.Constrain<Identical>(
        "The Old Gold smoker owns a snail.",
        Row(OldGold), Row(snail));
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
        puzzle.Constrain<IfPThenOneOrMoreOfQ>(
            "Chesterfields are smoked in the house next to the house with the fox.",
            IndexOf(h, Chesterfields), Neighbors(h, fox));
    }
    // clue 12
    for (auto h : houses) {
        puzzle.Constrain<IfPThenOneOrMoreOfQ>(
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
        puzzle.Constrain<IfPThenOneOrMoreOfQ>(
            "The Norwegian lives next to the blue house.",
            IndexOf(h, Norwegian), Neighbors(h, blue));
    }

    const auto solutions = puzzle.Solve();
    for (const auto &s : solutions) {
        // Show the full solution table.
        std::cout << s << '\n';
        // And answer the specific questions.
        std::cout << "The " << ItemName(WhoHas(water, s))
                  << " drinks " << ItemName(water) << ".\n";
        std::cout << "The " << ItemName(WhoHas(zebra, s))
                  << " has the pet " << ItemName(zebra) << ".\n\n";

        // Summary
        for (auto const h : houses) {
            std::cout << "The " << ItemName(ItemOf(color, h, s))
                      << " house is occupied by the "
                      << ItemName(ItemOf(nationality, h, s))
                      << ", who drinks " << ItemName(ItemOf(beverage, h, s))
                      << ", smokes " << ItemName(ItemOf(cigarette, h, s))
                      << ", and has a pet " << ItemName(ItemOf(pet, h, s))
                      << ".\n";
        }
    }

    return 0;
}
