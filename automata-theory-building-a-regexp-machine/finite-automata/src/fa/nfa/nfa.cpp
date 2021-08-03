#include "nfa.h"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <errno.h>
#include <cstring>
#include <cassert>
#include <sstream>
#include <string_view>

using namespace std;

namespace fa::nfa
{
    GraphDumpVisitor::GraphDumpVisitor(string_view title)
        : title(title)
    {
        //noop
    }

    void GraphDumpVisitor::visitNFA(NFA nfa)
    {
        (void) nfa;
        //noop
    }

    void GraphDumpVisitor::visitState(const State* state)
    {
        std::string state_label = "S" + to_string(this->state_count);

        this->state_labels[state] = state_label;
        this->state_count++;
    }

    void GraphDumpVisitor::visitTransition(const State* from, const std::string& symbol, const State* to)
    {
        std::string symbol_label = (symbol == EPSILON) ? "ε" : symbol;

        auto from_label = this->state_labels.find(from);
        assert(from_label != this->state_labels.end());

        auto to_label = this->state_labels.find(to);
        assert(to_label != this->state_labels.end());
        
        ostringstream ss;
        ss << "  " << from_label->second << " -> " << to_label->second << " [label=\"" << symbol_label << "\"]";

        this->transitions.push_back(ss.str());
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

    NFA kleene(NFA a)
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
        return a + kleene(a);
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

    void NFA::accept(Visitor& visitor) const
    {
        set<const State*> visited_states;
        // std::set<std::tuple<std::shared_ptr<State>, const std::string&, std::shared_ptr<State>>> visited_transitions;

        visitor.visitNFA(*this);
        this->in->accept(visitor, visited_states/*, visited_transitions*/);
        this->in->accept(visitor, visited_states/*, visited_transitions*/);
    }
}
