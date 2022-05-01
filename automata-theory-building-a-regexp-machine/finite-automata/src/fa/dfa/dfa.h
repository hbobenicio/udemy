#ifndef FA_DFA_H
#define FA_DFA_H

#include <map>
#include <vector>

#include <fa/nfa/nfa.h>

namespace fa::dfa
{
    class Table
    {
    protected:
        // TODO define the dfa State structure

    public:
        Table(fa::nfa::NFA nfa);
    };
}

#endif
