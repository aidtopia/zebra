#include "solver.h"

#include <cassert>
#include <iostream>
#include <stack>

Index Solution::FirstMaybe() const {
    const auto it =
        std::find(m_table.begin(), m_table.end(), MAYBE);
    return std::distance(m_table.begin(), it);
}

Result Solution::Set(Index index, Truth value) {
    assert(index < m_table.size());
    assert(value != MAYBE);
    if (m_table[index] == value) return Result::NO_CHANGE;
    if (m_table[index] != MAYBE) return Result::CONFLICT;
    m_table[index] = value;
    return Result::PROGRESS;
}

std::size_t Solution::Count(const IndexList &indexes, Truth value) const {
    return std::count_if(indexes.begin(), indexes.end(),
                            [&](Index i) { return m_table[i] == value; });
}


std::vector<Solution> Puzzle::Solve() const {
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

Result Puzzle::ApplyConstraints(Solution &candidate) const {
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
