#ifndef FA_NFA_H
#define FA_NFA_H

#include <memory>
#include <set>
#include <map>
#include <string_view>

#include "state.h"

namespace fa::nfa
{
    #define EPSILON ""

    class NFA;

    class Visitor
    {
    protected:
        virtual ~Visitor() = default;
    public:
        // TODO These methods could be improved. For instance, they could return a bool, indicating if
        //      the traversal should stop or continue.
        virtual bool visitState(const State* state) = 0;
        virtual bool visitTransition(const State* from, const std::string& symbol, const State* to) = 0;
        virtual bool visitNFA(NFA nfa) = 0;
    };

    class GraphDumpVisitor: public Visitor
    {
    protected:
        std::string_view title;
        size_t state_count = 0;
        std::map<const State*, std::string> state_labels;
        std::vector<std::string> transitions;

    public:
        GraphDumpVisitor(std::string_view title);

        bool visitNFA(NFA nfa) override;
        bool visitState(const State* state) override;
        bool visitTransition(const State* from, const std::string& symbol, const State* to) override;

        void dump_graph(const std::string& file_path) const;
    };

    struct TransitionsTable {
        const State* starting;
        std::map<const State*, std::map<std::string, std::vector<const State*>>> table;
        std::map<const State*, std::vector<const State*>> epsilon_closures;
    };

    std::ostream& operator<<(std::ostream& os, const TransitionsTable& transitions_table);

    class TransitionsTableVisitor: public Visitor
    {
    protected:
        TransitionsTable transitions_table;
        size_t state_count = 0;
        std::map<const State*, std::string> state_labels;

        bool visitNFA(NFA nfa) override;
        bool visitState(const State* state) override;
        bool visitTransition(const State* from, const std::string& symbol, const State* to) override;

    public:
        const TransitionsTable& get_transitions_table() const;
    };

    /**
     * NFA Fragment.
     * 
     * The basic building block for creating Nondeterministic Finite Automatas (NFA).
     * 
     * A NFA Fragment models only one input state and an output state.
     * 
     * TODO rename this to Fragment maybe. Analyze later if this makes sense...
     */
    class NFA {
    public:
        std::shared_ptr<State> in;
        std::shared_ptr<State> out;
        /**
         * Standard generic constructor.
         * 
         * Just creates a new NFA with the given input and output states.
         */
        NFA(std::shared_ptr<State> in, std::shared_ptr<State> out);

        /**
         * Single character (byte) constructor.
         * 
         * Constructs a NFA fragment with a single transition from
         * the input state to the output state for the given char as
         * the transition symbol.
         * 
         * In other words, this NFA Fragment accepts the given char.
         */
        NFA(char c);

        /**
         * Epsilon (empty) constructor.
         * 
         * Constructs a NFA fragment with a single transition from
         * the input state to the output state for the EPSILON symbol.
         * 
         * The epsilon symbol represents the empty string.
         * 
         * In practical terms, this allows the machine to change states
         * without reading any input.
         */ 
        NFA();

        /**
         * Verifies if the given input matches this NFA.
         */
        [[nodiscard]]
        bool match(std::string_view input);

        /**
         * Concatenation composition operator
         */
        [[nodiscard]]
        NFA operator+(NFA other);

        /**
         * union composition operator
         */
        [[nodiscard]]
        NFA operator|(NFA other);

        /**
         * Kleene (star) unary operator.
         * 
         * "Loops" the given machine zero or more times.
         */
        friend NFA kleene_naive(NFA a);

        /**
         * Optimal kleene operator.
         * 
         * Just adds two epsilon transitions:
         *    A.in  -e-> A.out , and
         *    A.out -e-> A.in
         */
        friend NFA zeroOrMore(NFA a);

        /**
         * Optimal plus '+' operator.
         * 
         * Just adds an epsilon transition from A.out to A.in.
         */
        friend NFA oneOrMore(NFA a);

        /**
         * Optimal question mark '?' (optional) operator.
         * 
         * Just adds an epsilon transition from A.in to A.out.
         */
        friend NFA opt(NFA a);

        void accept(Visitor& visitor) const;

        bool matches(std::string_view input) const;
    };

    /**
     * Fold left all NFA's using the concatenation '+' operator.
     */
    template <typename... NFAArgs>
    NFA concat(NFAArgs... nfas) {
        return (... + nfas);
    }

    /**
     * Fold left all NFA's using the union '|' operator.
     */
    template <typename... NFAArgs>
    NFA disjoint(NFAArgs... nfas) {
        return (... | nfas);
    }

    /**
     * Naive approach to a+.
     * 
     * "Loops" the given machine one or more times.
     * 
     * Naively equivalent to aa*
     */
    NFA plus_naive(NFA a);

    /**
     * Naive approach to a?.
     * 
     * Naively equivalent to a|e
     */
    NFA question_mark_naive(NFA a);

    /**
     * Naive approach to digits character class [0-9] <=> (0|1|...|9).
     */
    NFA digit_naive();

    /**
     * Naive approach to general character class [from-to] <=> (from|...|to).
     */
    NFA char_range_naive(char from, char to);

    /**
     * Optimal character class (range) pattern.
     * 
     * Just union these sanely with only transitions, without more states.
     */
    NFA range(char from, char to);
}

#endif
