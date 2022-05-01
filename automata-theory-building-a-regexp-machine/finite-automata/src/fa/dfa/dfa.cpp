#include "dfa.h"

#include <cassert>

using namespace std;
using namespace fa;

namespace fa::dfa
{
    Table::Table(nfa::NFA nfa)
    {
        // TODO this could be encapsulated inside a NFA method... need to think better about the lifetime of
        // these objects...
        nfa::TransitionsTableVisitor visitor;
        nfa.accept(visitor);
        const nfa::TransitionsTable& nfa_transitions_table = visitor.get_transitions_table();

        // struct TransitionsTable {
        //     const State* starting;
        //     std::map<const State*, std::map<std::string, std::vector<const State*>>> table;
        //     std::map<const State*, std::vector<const State*>> epsilon_closures;
        // };
        // TODO think better for the item type of this vector. We probably need some kind of dfa::State/TableItem struct.
        //      if we have state labels here, we could concat those labels to form the string that would represent the
        //      resulting dfa state.
        // for every state from the nfa-transitions table, we need to keep track of new created states.
        // we break the loop when no more states were created from this traversal.
        vector<size_t> new_dfa_states;
        bool done = false;
        while (!done) {
            for (const auto& [from_state, transitions]: nfa_transitions_table.table) {
                // if there are no symbol-transitions from this state, the dfa resulting state is
                // the nfa state's epsilon closure
                if (transitions.empty()) {
                    auto it = nfa_transitions_table.epsilon_closures.find(from_state);
                    // every nfa-transitions table need to have a non-empty epsilon_closure starting from this state
                    assert(it != nfa_transitions_table.epsilon_closures.end());
                    vector<const nfa::State*> epsilon_closure_states = it->second;
                    // TODO merge somehow the new dfa state from these nfa::states and then
                    //      add it the the new_dfa_states vector
                }
                for (const auto& [symbol, next_states]: transitions) {

                }
            }
        }
    }
}
