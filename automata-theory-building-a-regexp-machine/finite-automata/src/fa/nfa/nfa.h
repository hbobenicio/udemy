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
        virtual void visitState(const State* state) = 0;
        virtual void visitTransition(const State* from, const std::string& symbol, const State* to) = 0;
        virtual void visitNFA(NFA nfa) = 0;
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

        void visitNFA(NFA nfa) override;
        void visitState(const State* state) override;
        void visitTransition(const State* from, const std::string& symbol, const State* to) override;

        void dump_graph(const std::string& file_path) const;
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
    protected:
        std::shared_ptr<State> in;
        std::shared_ptr<State> out;

    public:
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
        friend NFA kleene(NFA a);

        void accept(Visitor& visitor) const;
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
}

#endif
