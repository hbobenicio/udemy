#include "state.h"

#include <map>
#include <iostream>

#include "nfa.h"

using namespace std;

namespace fa::nfa
{
    State::State(bool accepting) noexcept
        : accepting(accepting)
    {
    }

    void State::add_transition(const string& symbol, std::shared_ptr<State> state)
    {
        // auto states = this->transitions.find(symbol);
        // if (states == this->transitions.end()) {
        //     vector<shared_ptr<State>> new_states = { state };
        //     this->transitions[symbol] = new_states;
        // } else {
        //     states->second.emplace_back(state);
        // }
        this->transitions[symbol].push_back(state);
    }

    optional<States> State::get_transitions(const string& symbol) const
    {
        const auto states = this->transitions.find(symbol);
        if (states == this->transitions.end()) {
            return nullopt;
        }

        return optional{ states->second };
    }

    void State::accept(
        Visitor& visitor,
        std::set<const State*>& visited_states
    ) const
    {
        if (visited_states.find(this) != visited_states.end()) {
            return;
        }

        visitor.visitState(this);
        visited_states.insert(this);

        // We visit all states first then the transitions just because
        // It's useful (at least for the GraphDumpVisitor) to know visited
        // states before dealing with visited transitions
        for (const auto& [symbol, states]: this->transitions) {
            for (auto state: states) {
                state->accept(visitor, visited_states);
            }
        }
        for (const auto& [symbol, states]: this->transitions) {
            for (auto state: states) {
                visitor.visitTransition(this, symbol, state.get());
            }
        }
    }

    bool State::matches(set<const State*>& visited_states, string_view input) const
    {
        if (visited_states.find(this) != visited_states.end()) {
            return false;
        }
        visited_states.insert(this);

        if (input.empty()) {
            // no more input and we're at an accepting state. it matches!
            if (this->accepting) {
                return true;
            }
            // no more input but there may be epsilon transitions from this state that
            // may lead to an accepting state. follow those epsilon transitions and try
            // to match them.
            if (auto next_states = this->get_transitions(EPSILON); next_states) {
                for (auto next_state: *next_states) {
                    // if we found an accepting state from epsilon transitions recursively,
                    // then it's a match!
                    if (next_state->matches(visited_states, "")) {
                        return true;
                    }
                }
            }
            // no more input but we didn't found any accepting state. it's not a match!
            return false;
        }

        // there is input to be consumed.
        // slice the input for symbols
        char c = input.front();
        string symbol{c};

        string_view rest = input;
        rest.remove_prefix(1);

        // search for transitions using this symbol
        if (auto next_states = this->get_transitions(symbol); next_states) {
            // we should only share visited states between Epsilon transitions!
            visited_states.clear();
            for (auto next_state: *next_states) {
                if (next_state->matches(visited_states, rest)) {
                    return true;
                }
            }
        }
        // there still may be epsilon transitions for us to check
        if (auto next_states = this->get_transitions(EPSILON); next_states) {
            for (auto next_state: *next_states) {
                if (next_state->matches(visited_states, input)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool State::is_accepting() const
    {
        return this->accepting;
    }

    void State::set_accepting(bool accepting)
    {
        this->accepting = accepting;
    }
}
