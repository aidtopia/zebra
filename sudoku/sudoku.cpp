#include "solver_lib/constraints.h"
#include "solver_lib/solver.h"

#include <iostream>

constexpr Index IndexOf(int row, int col, int val) {
    return (row-1)*81 + (col-1)*9 + val-1;
}

IndexList Row(int row, int val) {
    IndexList result;
    for (int col = 1; col < 10; ++col) {
        result.push_back(IndexOf(row, col, val));
    }
    return result;
}

IndexList Col(int col, int val) {
    IndexList result;
    for (int row = 1; row < 10; ++row) {
        result.push_back(IndexOf(row, col, val));
    }
    return result;
}

IndexList Cell(int row, int col) {
    IndexList result;
    for (int val = 1; val < 10; ++val) {
        result.push_back(IndexOf(row, col, val));
    }
    return result;
}

IndexList Box(int box, int val) {
    IndexList result;
    int const row0 = 3 * ((box-1)/3) + 1;
    int const row1 = row0 + 3;
    int const col0 = 3 * ((box-1)%3) + 1;
    int const col1 = col0 + 3;
    for (int row = row0; row < row1; ++row) {
        for (int col = col0; col < col1; ++col) {
            result.push_back(IndexOf(row, col, val));
        }
    }
    return result;
}

std::ostream &operator<<(std::ostream &out, Solution const &s) {
    for (int row = 1; row < 10; ++row) {
        for (int col = 1; col < 10; ++col) {
            for (int val = 1; val < 10; ++val) {
                if (s[IndexOf(row, col, val)] == YES) {
                    out << val;
                    break;
                }
            }
            out.put(' ');
        }
        out.put('\n');
    }
    return out;
}

int main() {
    Puzzle puzzle(9*9*9);

    // Basic Sudoku rules:
    for (int i = 1; i < 10; ++i) {
        for (int j = 1; j < 10; ++j) {
            puzzle.Constrain<ExactlyNOf>("Cell has exactly 1 digit.", 1, Cell(i, j));
            puzzle.Constrain<ExactlyNOf>("Digit appears exactly once in row.", 1, Row(i, j));
            puzzle.Constrain<ExactlyNOf>("Digit appears exactly once in column.", 1, Col(i, j));
            puzzle.Constrain<ExactlyNOf>("Digit appears exactly once in box.", 1, Box(i, j));
        }
    }

    // Pre-filled cells in the puzzle.
    // I found this "easy" example online.  It turns out that the solution can
    // be deduced without the solver guessing at all.  I should probably find a
    // more difficult example for testing.
    puzzle.Constrain<Fixed>("Fixed", IndexOf(2, 6, 3));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(2, 8, 8));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(2, 9, 5));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(3, 3, 1));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(3, 5, 2));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(4, 4, 5));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(4, 6, 7));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(5, 3, 4));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(5, 7, 1));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(6, 2, 9));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(7, 1, 5));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(7, 8, 7));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(7, 9, 3));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(8, 3, 2));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(8, 5, 1));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(9, 5, 4));
    puzzle.Constrain<Fixed>("Fixed", IndexOf(9, 9, 9));

    const auto solutions = puzzle.Solve();
    for (const auto &solution : solutions) {
        std::cout << solution << '\n';
    }
    return 0;
}
