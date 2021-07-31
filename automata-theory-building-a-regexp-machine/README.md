# Automata Theory - Building a Regex Machine

Personal notes about the course.

## Link

https://www.udemy.com/course/automata-theory-building-a-regexp-machine

## Notes

### Section 1: Formal grammars and Finite automata

#### Formal Grammar Definition

> G = (N, T, P, S)

- N: Nonterminals ( {S, X} )
- T: Terminals ( {a, b} )
- P: Productions ( {S->aX, X->b} )
- S: Starting Symbol (S)

#### (Grammar) Hierarchy of Chomsky

- 0. Unrestricted (used to define natural languages)
- 1. Context-sensitive (used to define programming languages)
- 2. Context-free (used to define programming languages)
- 3. Regular (RegExp) (weakest one)

>  The lower the language is, the less strings it can handle.

RegExp notation is just syntatic notation for Regular Grammars, which can also be represented with BNF notation (with some restrictions)

RegExp cannot handle nesting (which needs state tracking), but can handle recursion!

#### Finite Automata (aka FSM: Finite State Machines)

Are just directed graphs. Nodes are states and edges are transitions.

Regular Grammars have direct translation to State Machines

Types of FA:

- WITH Output
  - Moore Machine
  - Mealy Machine
- WITHOUT Output (used to implement Regular Expressions)
  - DFA (Deterministic Finite Automata)
    - Forbids epsilon transitions and multiple transitions on the same symbol
  - NFA (Nondeterministic Finite Automata)
    - Allows transitions on the same symbol to different states (we don't know for sure where to go from one state)
  - e-NFA (epsilon NFA)
    - NFA with epsilon transitions

Easiest route: `RegExp -> e-NFA -> ... -> DFA`

Formal Definition

> NFA = (Q, SIGMA, DELTA, q0, F)

- Q: All possibe states - {A, B}
- SIGMA: Alphabet - {1, 0}
- transition function - Q x SIGMA -> 2^Q
- q0 - Starting state - A
- F - Set of accepting states - {B}
