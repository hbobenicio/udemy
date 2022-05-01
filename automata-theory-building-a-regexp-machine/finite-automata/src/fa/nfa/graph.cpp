#include "graph.h"

#include <memory>

#include "state.h"

using namespace std;

namespace fa::nfa
{
    State* Graph::create_state(bool accepting)
    {
        // Trust me I'm an Engineer!
        State* state = new State(accepting);
        this->states.emplace_back(state);
        return state;
    }
}
