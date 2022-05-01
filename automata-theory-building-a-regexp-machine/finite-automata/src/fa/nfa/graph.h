#ifndef FA_NFA_GRAPH_H
#define FA_NFA_GRAPH_H

#include <list>
#include <memory>

namespace fa::nfa
{
    class State;

    class Graph {
    protected:
        std::list<std::unique_ptr<State>> states;

    public:
        State* create_state(bool accepting = false);
    };
};

#endif
