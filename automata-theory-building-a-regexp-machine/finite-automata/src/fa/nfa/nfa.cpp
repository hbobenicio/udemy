#include "nfa.h"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <errno.h>
#include <cstring>
#include <cassert>
#include <sstream>
#include <string_view>
#include <iomanip>

using namespace std;

static string string_from_symbol(const std::string& symbol)
{
    return (symbol == EPSILON) ? "ε" : symbol;
}

namespace fa::nfa
{
    GraphDumpVisitor::GraphDumpVisitor(string_view title)
        : title(title)
    {
        //noop
    }

    bool GraphDumpVisitor::visitNFA(NFA nfa)
    {
        (void) nfa;
        return true;
    }

    bool GraphDumpVisitor::visitState(const State* state)
    {
        std::string state_label = "S" + to_string(this->state_count);

        this->state_labels[state] = state_label;
        this->state_count++;

        return true;
    }

    bool GraphDumpVisitor::visitTransition(const State* from, const std::string& symbol, const State* to)
    {
        std::string symbol_label = string_from_symbol(symbol);

        auto from_label = this->state_labels.find(from);
        assert(from_label != this->state_labels.end());

        auto to_label = this->state_labels.find(to);
        assert(to_label != this->state_labels.end());
        
        ostringstream ss;
        ss << "  " << from_label->second << " -> " << to_label->second << " [label=\"" << symbol_label << "\"]";

        this->transitions.push_back(ss.str());

        return true;
    }

    void GraphDumpVisitor::dump_graph(const std::string& file_path) const
    {
        ofstream file{ file_path };
        if (!file.is_open()) {
            int error_code = errno;
            fprintf(
                stderr,
                "error: could not open file %s: [%d] %s\n",
                file_path.c_str(),
                error_code,
                strerror(error_code)
            );
            exit(1);
        }

        file << "digraph NFA {\n";
        file << "  labelloc=\"t\";\n";
        file << "  label=\"" << this->title << "\";\n";
        file << "  rankdir=LR;\n";
        for (const auto& [state_ptr, state_label]: this->state_labels) {
            string lbl = state_label + (state_ptr->is_accepting() ? "[shape=\"doublecircle\"]" : "");
            file << "  " << lbl << ";\n";
        }
        for (const std::string& transition_text: this->transitions) {
            file << transition_text << ";\n";
        }
        file << "}\n";
    }

    bool TransitionsTableVisitor::visitNFA(NFA nfa)
    {
        this->transitions_table.starting = nfa.in.get();
        return true;
    }

    bool TransitionsTableVisitor::visitState(const State* state)
    {
        // setting the state's label
        std::string state_label = "S" + to_string(this->state_count);
        this->state_labels[state] = state_label;
        this->state_count++;

        cerr << "[DEBUG] [TransitionsTableVisitor] visiting state ";
        cerr << state_label << " at " << state << '\n';

        // this will initialize the table entry for this state.
        // it may not have any transition (so visitTransition will never get called for this).
        this->transitions_table.table[state];
        // saving epsilon closure for this state
        this->transitions_table.epsilon_closures[state] = state->get_epsilon_closure();
        return true;
    }

    bool TransitionsTableVisitor::visitTransition(const State* from, const std::string& symbol, const State* to)
    {
        cerr << "[DEBUG] [TransitionsTableVisitor] visiting transition ";
        cerr << "{ from: " << this->state_labels[from];
        cerr << ", symbol: " << string_from_symbol(symbol);
        cerr << ", to: " << this->state_labels[to] << "}\n";

        this->transitions_table.table[from][symbol].push_back(to);
        return true;
    }

    const TransitionsTable& TransitionsTableVisitor::get_transitions_table() const
    {
        return transitions_table;
    }

    std::ostream& operator<<(std::ostream& os, const TransitionsTable& transitions_table)
    {
        for (const auto& [state, transitions]: transitions_table.table) {
            if (state == transitions_table.starting) {
                os << " >S(";
            } else if (state->is_accepting()) {
                os << " .S(";
            } else {
                os << "  S(";
            }
            os << state << "): [";
            for (const auto& [symbol, next_states]: transitions) {
                os << string_from_symbol(symbol);
                os << " -> {";
                for (const auto& next_state: next_states) {
                    os << "S(" << next_state << "), ";
                }
                os << "}";
            } 
            os << "]\n";
        }
        return os;
    }

    NFA::NFA(std::shared_ptr<State> in, std::shared_ptr<State> out)
        : in(in), out(out)
    {
    }

    NFA::NFA(char c)
        : in(make_shared<State>(false))
        , out(make_shared<State>(true))
    {
        in->add_transition(string{c}, out);
    }

    NFA::NFA()
        : in(make_shared<State>(false))
        , out(make_shared<State>(true))
    {
        in->add_transition(EPSILON, out);
    }

    bool NFA::match(std::string_view input)
    {
        (void)input;
        // TODO return this->in->match(input);
        return false;
    }

    /**
     * The concat operator.
     * 
     * Concats this with another NFA fragment.
     * 
     * AB <=> A -> e -> B
     * where A = this
     */
    NFA NFA::operator+(NFA other)
    {
        this->out->add_transition(EPSILON, other.in);

        this->out->set_accepting(false);
        other.out->set_accepting(true);

        return NFA{this->in, other.out};
    }

    /**
     * The union operator.
     * 
     * Creates a NFA that can change state to either A or B.
     * 
     * A|B <=> e -> A --+-> e
     *         +--> B --+
     * where A = this
     */
    NFA NFA::operator|(NFA other)
    {
        auto starting_state = make_shared<State>(false);
        auto accepting_state = make_shared<State>(true);
        
        starting_state->add_transition(EPSILON, this->in);
        starting_state->add_transition(EPSILON, other.in);

        this->out->add_transition(EPSILON, accepting_state);
        other.out->add_transition(EPSILON, accepting_state);

        this->out->set_accepting(false);
        other.out->set_accepting(false);

        return NFA{ starting_state,  accepting_state};
    }

    NFA kleene_naive(NFA a)
    {
        // epsilon machine, with in=A, out=B and only transition A -e-> B
        NFA resulting;

        resulting.in->add_transition(EPSILON, a.in);
        a.out->add_transition(EPSILON, resulting.out);

        a.out->set_accepting(false);
        resulting.out->set_accepting(true);
        
        // loop
        resulting.out->add_transition(EPSILON, a.in);

        return resulting;
    }

    NFA plus_naive(NFA a)
    {
        return a + kleene_naive(a);
    }

    NFA question_mark_naive(NFA a)
    {
        return disjoint(a, NFA{});
    }

    NFA digit_naive()
    {
        return disjoint(
            NFA{'0'},
            NFA{'1'},
            NFA{'2'},
            NFA{'3'},
            NFA{'4'},
            NFA{'5'},
            NFA{'6'},
            NFA{'7'},
            NFA{'8'},
            NFA{'9'}
        );
    }

    NFA char_range_naive(char from, char to)
    {
        assert(from <= to);

        NFA resulting{from};
        for (char c = from + 1; c <= to; c++) {
            resulting = resulting | NFA{c};
        }

        return resulting;
    }

    NFA zeroOrMore(NFA a)
    {
        a.in->add_transition(EPSILON, a.out);
        a.out->add_transition(EPSILON, a.in);

        return a;
    }

    NFA oneOrMore(NFA a)
    {
        a.out->add_transition(EPSILON, a.in);

        return a;
    }

    NFA opt(NFA a)
    {
        a.in->add_transition(EPSILON, a.out);

        return a;
    }

    NFA range(char from, char to)
    {
        assert(from <= to);

        NFA resulting{ from };
        for (char c = from + 1; c <= to; c++) {
            resulting.in->add_transition(string{c}, resulting.out);
        }

        return resulting;
    }

    void NFA::accept(Visitor& visitor) const
    {
        set<const State*> visited_states;

        visitor.visitNFA(*this);
        this->in->accept(visitor, visited_states);
        this->out->accept(visitor, visited_states);
    }

    bool NFA::matches(string_view input) const
    {
        set<const State*> visited_states;

        return this->in->matches(visited_states, input);
    }
}
