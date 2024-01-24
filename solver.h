// A framework for solving logic puzzles.
#ifndef SOLVER_H
#define SOLVER_H

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

enum Truth { NO = -1, MAYBE = 0, YES = 1 };
constexpr Truth operator!(Truth t) { return static_cast<Truth>(-t); }

using Index = std::size_t;
using IndexList = std::vector<Index>;

enum class Result { CONFLICT = -1, NO_CHANGE = 0, PROGRESS = 1 };

class Solution {
    public:
        explicit Solution(std::size_t slots) : m_table(slots, MAYBE) {}

        Truth operator[](Index index) const { return m_table[index]; }

        std::size_t size() const { return m_table.size(); }

        Index FirstMaybe() const;

        Result Set(Index index, Truth value);

        std::size_t Count(const IndexList &indexes, Truth value) const;

    private:
        std::vector<Truth> m_table;
};

class Puzzle {
    public:
        explicit Puzzle(std::size_t slots) : m_slot_count(slots) {}

        std::vector<Solution> Solve() const;

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
        Result ApplyConstraints(Solution &candidate) const;

        std::size_t m_slot_count;
        std::vector<std::unique_ptr<BasicConstraint>> m_constraints;
};

#endif
