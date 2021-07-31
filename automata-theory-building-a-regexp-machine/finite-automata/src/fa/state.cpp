#include "state.h"

#include <map>
#include <iostream>

using namespace std;

namespace fa
{
    State::State(bool accepting) noexcept
        : accepting(accepting)
    {
    }

    void State::add_transition_for_symbol(std::string_view symbol, std::shared_ptr<State> state)
    {
        auto states = this->transitions.find(symbol);
        if (states == this->transitions.end()) {
            vector<shared_ptr<State>> new_states = { state };
            this->transitions[symbol] = new_states;
        } else {
            states->second.emplace_back(state);
        }
    }

    optional<States> State::get_transitions_for_symbol(std::string_view symbol) const
    {
        const auto states = this->transitions.find(symbol);
        if (states == this->transitions.end()) {
            return nullopt;
        }

        return optional{ states->second };
    }

    bool State::is_accepting() const
    {
        return this->accepting;
    }


    ostream& operator<<(ostream& os, const State& state)
    {
        if (state.accepting) {
            os << "State{e}";
            return os;
        }

        os << "State{ ";
        for (const auto& [symbol, states]: state.transitions) {
            os << symbol << " -> [ ";
            for (size_t i = 0; i < states.size(); i++) {
                if (i != 0) os << ", ";
                os << *states[i];
            }
            os << " ]";
        }
        os << " }";

        return os;
    }
}
