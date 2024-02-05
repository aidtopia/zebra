// A library of constraints for setting up Puzzles.
#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "solver.h"

#include <algorithm>
#include <cassert>

// The value at a specific index in a solution is fixed.
class Fixed : public Puzzle::BasicConstraint {
    public:
        Fixed(const std::string &name, Index index, Truth value = YES) :
            BasicConstraint(name), m_index(index), m_value(value)
        {
            assert(m_value != MAYBE);
        }

        Result Evaluate(Solution &s) const override {
            return s.Set(m_index, m_value);
        }

    private:
        Index m_index;
        Truth m_value;
};

// If the value at index P is YES, then the value at Q must be YES.
// Converse might not apply, but the contrapositive does.
class IfPThenQ : public Puzzle::BasicConstraint {
    public:
        IfPThenQ(std::string const &name, Index P, Index Q) :
            BasicConstraint(name), m_p(P), m_q(Q) {}

        Result Evaluate(Solution &s) const override {
            if (s[m_p] == YES && s[m_q] == NO) return Result::CONFLICT;
            if (s[m_p] == YES && s[m_q] == MAYBE) return s.Set(m_q, YES);
            if (s[m_q] == NO  && s[m_p] == MAYBE) return s.Set(m_p, NO);
            return Result::NO_CHANGE;
        }

    private:
        Index m_p, m_q;
};

// The values at two specific indices into a solution must match.
class Identical : public Puzzle::BasicConstraint {
    public:
        Identical(const std::string &name, Index index1, Index index2) :
            BasicConstraint(name), m_indexes1{index1}, m_indexes2{index2} {}

        Identical(const std::string &name, IndexList &&indexes1, IndexList &&indexes2) :
            BasicConstraint(name),
            m_indexes1(std::move(indexes1)),
            m_indexes2(std::move(indexes2)) {}

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

// Exactly n of a specific subset of values in a solution must be a specific
// value, which implies that the others in that subset must have the opposite
// value.
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

// If P is YES, then at least one of Q is YES.
class IfPThenOneOrMoreOfQ : public Puzzle::BasicConstraint {
    public:
        IfPThenOneOrMoreOfQ(const std::string &name, Index P, IndexList &&Q) :
            BasicConstraint(name), m_p(P), m_q(std::move(Q)) {}

        Result Evaluate(Solution &s) const override {
            auto const P = s[m_p];
            auto const Q = QCount(s);
            if (P == YES) {
                if (Q.yeses == 0) {
                    if (Q.maybes == 0) return Result::CONFLICT;
                    if (Q.maybes == 1) {
                        for (auto i : m_q) {
                            if (s[i] == MAYBE) return s.Set(i, YES);
                        }
                        assert(false && "couldn't find the one MAYBE");
                    }
                }
            }
            if (P == MAYBE && Q.yeses == 0 && Q.maybes == 0) {
                return s.Set(m_p, NO);
            }
            return Result::NO_CHANGE;
        }

    private:
        struct Counts { std::size_t noes = 0, maybes = 0, yeses = 0; };
        Counts QCount(Solution const &s) const {
            Counts counts;
            for (auto const i : m_q) {
                switch (s[i]) {
                    case YES:   counts.yeses  += 1; break;
                    case MAYBE: counts.maybes += 1; break;
                    case NO:    counts.noes   += 1; break;
                }
            }
            return counts;
        }
        Index m_p;
        IndexList m_q;
};

#endif
