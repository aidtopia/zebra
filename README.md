# Zebra

A programming challenge to make a general solver for constraint-based puzzles.

Here's the puzzle that inspired it.

1. There are five houses, each of a different color and inhabited by men of different nationalities, with different pets, drinks and cigarettes.
2. The Englishman lives in the red house.
3. The Spaniard owns the dog.
4. Coffee is drunk in the green house.
5. The Ukrainian drinks tea.
6. The green house is immediately to the right (your right) of the ivory house.
7. The Old Gold smoker owns snails.
8. Kools are smoked in the yellow house.
9. Milk is drunk in the middle house.
10. The Norwegian lives in the first house on the left.
11. The man who smokes Chesterfields lives in the house next to the man with the fox.
12. Kools are smoked in the house next to the house where the horse is kept.
13. The Lucky Strike smoker drinks orange juice.
14. The Japanese smokes Parliaments.
15. The Norwegian lives next to the blue house.

Who drinks water?
Who owns the zebra?

This is sometimes known as The Zebra Puzzle or Einstein's Riddle.  Different sources have slightly different versions, but those are mostly just label changes.

There’s a Rosetta Code page to see various solutions to a version of the Zebra puzzle.  Most of those solutions use “generate and prune,” which generates solutions and tests them.

But the generators hard-code some of the constraints.  For example, rule 1 tells you that there’s one of each type in each house and that no item exists in more than one house.  This is hard-coded into the generator rather than just one more constraint to the problem.  Without that, the search space would be intractable.

And the pruners re-order the clues in order to prune huge chunks of the search space early on.  There’s a lot of human intervention baked into the problem solving process.

I wanted to make a framework for solving this type of problem generally--one that could be re-used to solve similar types of logic puzzles.  My priority was the generality of the solver, with efficiency only a secondary concern.

## The Solver

I implemented the general solver in solver.h and solver.cpp, which has no specific knowledge about any particular puzzle.

Possible solutions are represented by an instance of a Solution, which is a vector of Truth values.  Truths can be YES, NO, or MAYBE.  As long as there are any MAYBEs in the vector, then the solution is not complete.

To use the solver for a particular puzzle, you decide how to represent a solution as an array of Truth values, create an instance of Puzzle, and add constraints to it by calling Puzzle::Constrain.

I’ve implemented several kinds of constraints I've found useful in constraints.h.  If a particular puzzle calls for another kind, you can create one without any modification to the puzzle framework.

The solver begins by creating a candidate solution with all values set to MAYBE and pushes it onto a stack.

Each pass through the main loop takes pops the solution from the stack, and treats it as the current candidate solution.

The constraints are evaluated to figure out as much of the solution as possible.  When the constraints can no longer infer anything more, the solver resorts to systematic guessing.  It makes two copies of the current candidate, and replaces one of the MAYBEs in them—YES for one guess, and NO for the other.  Both of the guesses are pushed onto the stack.  If a conflict arises, the guess was bad, and it’s thrown out.

The solver keeps track of every solution that satisfies all the constraints.  The Zebra puzzle has exactly one solution.  An under-constrained puzzle may have many.  And an over-constrained puzzle will have zero solutions.

If necessary, the solver will explore the solution space exhaustively.  But for puzzles with sufficient constraints, most of that space will be pruned quickly.

In the case of the Zebra puzzle, more than half of the solution can be determined just by applying the constraints.  Then it makes about three guesses, before it has enough information to find the first solution.  It continues exploration but quickly discovers that there are no other solutions.

### Custom Constraints

To create a custom constraint, derive from Puzzle::BasicConstraint and provide an implementation of Evaluate.  Evaluate will be called with a reference to a candidate Solution.

If that solution violates the constraint, Evaluate must return CONFLICT.

Otherwise, Evaluate must determine whether it has enough information to replace any of the MAYBEs in the solution.  If it can, it must makes those changes and return PROGRESS.  Evaluate must never guess—it make only the changes that the constraint demands.

If there's no violation of the constraint but also no progress, Evaluate must return NO_CHANGE.

## The Zebra Puzzle

Let's use the zebra puzzle to illustrate how you might set up the framework to solve a particular puzzle.

Recall that there are five houses, five house colors, five occupant nationalities, five beverage preferences, five cigarette brands, and five types of animals.  We need to represent how these things are associated using a vector of Truths.

I used a two-dimensional array with the five houses as the columns, and all of the 25 "items" as the rows.  So our solution will have 125 Truth values.  If a particular item _i_ is associated with house _h_, then the value at (_h_, _i_) will be YES.

```
   h1    h2    h3    h4    h5
+-----+-----+-----+-----+-----+
|     |     |     |     |     | Englishman
|     |     |     |     |     | Japanese man
|     |     |     |     |     | Norwegian
|     |     |     |     |     | Spaniard
|     |     |     |     |     | Ukrainian
+-----+-----+-----+-----+-----+
|     |     |     |     |     | blue
|     |     |     |     |     | green
|     |     |     |     |     | ivory
|     |     |     |     |     | red
|     |     |     |     |     | yellow
+-----+-----+-----+-----+-----+
|     |     |     |     |     | dog
|     |     |     |     |     | fox
|     |     |     |     |     | horse
|     |     |     |     |     | snails
|     |     |     |     |     | zebra
+-----+-----+-----+-----+-----+
|     |     |     |     |     | coffee
|     |     |     |     |     | juice
|     |     |     |     |     | milk
|     |     |     |     |     | tea
|     |     |     |     |     | water
+-----+-----+-----+-----+-----+
|     |     |     |     |     | Chesterfields
|     |     |     |     |     | Kools
|     |     |     |     |     | Lucky Strike
|     |     |     |     |     | Old Gold
|     |     |     |     |     | Parliaments
+-----+-----+-----+-----+-----+
```

Note that this represention allows for invalid solutions.  For example, a particular house could be associated with all five colors and no animals.  This is not a problem, since the rules that require 1-to-1 associations will be encoded as constraints.

I ordered the rows so that items in the same category are consecutive.  For example, the first five rows are the nationalities, the next five are the colors, etc.  In a sense, our solution is a 3-dimensional array indexed by (house, category, item), but that would be more difficult to visualize, so I'm treating categories as subsets of rows.

Most of zebra.cpp defines this mapping from (house, item) to indexes within the solution.  To make it easy to express constraints, visualize solutions, and iterate through our enumerations, we also define tables of strings and arrays of indexes.  That's a lot of code, but it's all just conveniences.

In main() we instantiate the Puzzle, add our constraints, and call Puzzle::Solve which returns a vector of solutions.  Constrain is a method template that takes the type of constraint as a template parameter and the arguments for the constraint’s constructor.

For example:

```C++
    // clue 9
    puzzle.Constrain<Fixed>(
        "Milk is drunk in the middle house.",
        IndexOf(house3, milk), YES);
```

Fixed is a constraint that requires a specific index into the solution to have a specific value (YES or NO).  house3 and milk are enumerator values defined by the Zebra-specific code, and IndexOf maps them to a specific index in a Solution per the mapping I chose for this problem.

By itself, that inference is not super interesting.  But when the program also applies constraints from clue 1, the solver can infer a lot of information.  Here’s the most complex clue in the puzzle:

```C++
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
```

We use several constraints to encode clue 1.  They’re all of the type ExactlyNOf.  These say that exactly one value in each row and one in each subcolumn must be YES.  (You could also write it as exactly 4 must be NO.)  Once we know that house3 has the milk, we know that no other beverage is drunk in house3 and that no other house has milk.  So the ExactlyNOf constraints corresponding to the portion of the column representing beverages for house3 and the row for milk can deduce that all the other values are NO.

All the constraints have a textual description, which the solver prints whenever the constraint is able to infer more information.  It provides a trace of how it solved the puzzle.

Here’s the output from a run, which starts with a trace of what the solver does and ends with a display of the solution:

```
Progress: The green house can't be first because it's to the right of the ivory.
Progress: Milk is drunk in the middle house.
Progress: The Norwegian lives in the first house.
Progress: The Norwegian lives next to the blue house.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 house has the Norwegian.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 house has the blue.
Progress: Exactly 1 beverage in each house.
Progress: Exactly 1 house has the milk.
Progress: The English man lives in the red house.
Progress: The Spanish man owns the dog.
Progress: Coffee is drunk in the green house.
Progress: The Ukrainian man drinks tea.
Progress: The green house is immediately to the right of the ivory house.
Progress: Kools are smoked in the yellow house.
Progress: The Lucky Strike smoker drinks orange juice.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 house has the yellow.
Progress: Kools are smoked in the yellow house.
Progress: Kools are smoked in the house next to the house where the horse is kept.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 house has the horse.
Progress: Exactly 1 cigarette in each house.
Progress: The Spanish man owns the dog.
Progress: The Old Gold smoker owns snails.
Progress: The Lucky Strike smoker drinks orange juice.
Progress: Exactly 1 beverage in each house.
Progress: Exactly 1 house has the water.
Guessing: Index 11.  [Here it has inferred everything it can from the constraints, so it starts guessing.]
Progress: Exactly 1 pet in each house.  [And then sees if the constraints can infer more in light of the guess.]
Progress: Exactly 1 house has the fox.
Progress: Chesterfields are smoked in the house next to the house with the fox.
Progress: Chesterfields are smoked in the house next to the house with the fox.
Progress: Chesterfields are smoked in the house next to the house with the fox.
Progress: Exactly 1 house has the Chesterfields.
Progress: Exactly 1 cigarette in each house.
Progress: The Lucky Strike smoker drinks orange juice.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 house has the Ukrainian.
Progress: Exactly 1 beverage in each house.
Progress: Exactly 1 house has the tea.
Guessing: Index 50.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 house has the English.
Progress: The English man lives in the red house.
Progress: The Spanish man owns the dog.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 house has the green.
Progress: Exactly 1 house has the ivory.
Progress: Exactly 1 cigarette in each house.
Progress: Exactly 1 house has the Old Gold.
Progress: Coffee is drunk in the green house.
Progress: The Old Gold smoker owns snails.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 beverage in each house.
Progress: Exactly 1 beverage in each house.
Progress: The Lucky Strike smoker drinks orange juice.
Progress: Exactly 1 cigarette in each house.
Progress: Exactly 1 cigarette in each house.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 nationality in each house.
Progress: The Spanish man owns the dog.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 pet in each house.
Solution!  [Note that it found the solution here but keeps investigating to see if there are more.]
Progress: The English man lives in the red house.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 house has the ivory.
Progress: The green house is immediately to the right of the ivory house.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 color in each house.
Progress: The English man lives in the red house.
Progress: Coffee is drunk in the green house.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 beverage in each house.
Progress: Exactly 1 beverage in each house.
Progress: The Spanish man owns the dog.
Progress: The Lucky Strike smoker drinks orange juice.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 cigarette in each house.
Progress: The Old Gold smoker owns snails.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 house has the zebra.
Guessing: Index 51.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 house has the Japanese.
Progress: Exactly 1 house has the Spanish.
Progress: The Spanish man owns the dog.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 cigarette in each house.
Progress: Exactly 1 cigarette in each house.
Conflict: The Old Gold smoker owns snails.  [Shows which constraint detected the inconsistency in the solution.]
Pruning: Candidate is not consistent.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 house has the Japanese.
Progress: Exactly 1 house has the Spanish.
Progress: The Spanish man owns the dog.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 cigarette in each house.
Progress: Exactly 1 cigarette in each house.
Conflict: The Old Gold smoker owns snails.
Pruning: Candidate is not consistent.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 house has the zebra.
Guessing: Index 26.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 house has the Japanese.
Progress: The Ukrainian man drinks tea.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 beverage in each house.
Progress: Exactly 1 house has the juice.
Progress: Exactly 1 cigarette in each house.
Conflict: The Lucky Strike smoker drinks orange juice.
Pruning: Candidate is not consistent.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 house has the Ukrainian.
Progress: The Ukrainian man drinks tea.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 beverage in each house.
Progress: The Lucky Strike smoker drinks orange juice.
Progress: Exactly 1 cigarette in each house.
Progress: Exactly 1 house has the Chesterfields.
Progress: Chesterfields are smoked in the house next to the house with the fox.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 house has the fox.
Progress: The Spanish man owns the dog.
Progress: The Old Gold smoker owns snails.
Progress: Exactly 1 cigarette in each house.
Progress: Exactly 1 house has the Parliaments.
Progress: The Japanese man smokes Parliaments.
Progress: Exactly 1 nationality in each house.
Progress: The English man lives in the red house.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 house has the ivory.
Progress: The green house is immediately to the right of the ivory house.
Progress: Exactly 1 color in each house.
Progress: Exactly 1 color in each house.
Progress: The English man lives in the red house.
Progress: Coffee is drunk in the green house.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 nationality in each house.
Progress: Exactly 1 beverage in each house.
Progress: Exactly 1 beverage in each house.
Progress: The Spanish man owns the dog.
Progress: The Lucky Strike smoker drinks orange juice.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 pet in each house.
Progress: Exactly 1 cigarette in each house.
Progress: Exactly 1 cigarette in each house.
Conflict: The Old Gold smoker owns snails.
Pruning: Candidate is not consistent.
+-----+-----+-----+-----+-----+
| no  | no  | YES | no  | no  | English
| no  | no  | no  | no  | YES | Japanese
| YES | no  | no  | no  | no  | Norwegian
| no  | no  | no  | YES | no  | Spanish
| no  | YES | no  | no  | no  | Ukrainian
+-----+-----+-----+-----+-----+
| no  | YES | no  | no  | no  | blue
| no  | no  | no  | no  | YES | green
| no  | no  | no  | YES | no  | ivory
| no  | no  | YES | no  | no  | red
| YES | no  | no  | no  | no  | yellow
+-----+-----+-----+-----+-----+
| no  | no  | no  | YES | no  | dog
| YES | no  | no  | no  | no  | fox
| no  | YES | no  | no  | no  | horse
| no  | no  | YES | no  | no  | snails
| no  | no  | no  | no  | YES | zebra
+-----+-----+-----+-----+-----+
| no  | no  | no  | no  | YES | coffee
| no  | no  | no  | YES | no  | juice
| no  | no  | YES | no  | no  | milk
| no  | YES | no  | no  | no  | tea
| YES | no  | no  | no  | no  | water
+-----+-----+-----+-----+-----+
| no  | YES | no  | no  | no  | Chesterfields
| YES | no  | no  | no  | no  | Kools
| no  | no  | no  | YES | no  | Lucky Strike
| no  | no  | YES | no  | no  | Old Gold
| no  | no  | no  | no  | YES | Parliaments
+-----+-----+-----+-----+-----+
```

## A Sudoku Puzzle

I used the same engine without changes to solve a Sudoku puzzle (see sudoku.cpp).  Obviously, a custom Sudoku solver would be more efficient, but I believe this demonstrates that other types of constraint-based logic puzzles can be solved by the general framework without modification.

Mapping Sudoku is actually easier.  The puzzle has 81 cells, and each cell can have one of 9 possible values, so we'll use 9 Truth values for each cell.

The basic rules of the puzzle are all of the ExactlyNOf form.  A cell has exactly one YES (corresponding to the digit).  Each digit apperas exactly onces in each row, each column, and each box.

To solve a particular puzzle, you add a Fixed constraint for each of the pre-filled cells.

