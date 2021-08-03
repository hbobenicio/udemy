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
        auto states = this->transitions.find(symbol);
        if (states == this->transitions.end()) {
            vector<shared_ptr<State>> new_states = { state };
            this->transitions[symbol] = new_states;
        } else {
            states->second.emplace_back(state);
        }
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
        // auto this_state = make_shared<State>(this);
        // shared_ptr<State> this_state{ &this };
        if (visited_states.find(/*this_state*/this) != visited_states.end()) {
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

    bool State::is_accepting() const
    {
        return this->accepting;
    }

    void State::set_accepting(bool accepting)
    {
        this->accepting = accepting;
    }
}
