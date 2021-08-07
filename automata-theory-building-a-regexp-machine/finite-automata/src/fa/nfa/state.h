#ifndef FA_STATE_H
#define FA_STATE_H

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <optional>
#include <set>
#include <tuple>

namespace fa::nfa
{
    class State;
    class Visitor;

    using States = std::vector<std::shared_ptr<State>>;

    class State {
    protected:
        bool accepting;
        std::map<std::string, States> transitions;

    public:
        State(bool accepting = false) noexcept;

        void add_transition(const std::string& symbol, std::shared_ptr<State> state);

        [[nodiscard]]
        std::optional<States> get_transitions(const std::string& symbol) const;

        void accept(
            Visitor& visitor,
            std::set<const State*>& visited_states
        ) const;

        bool matches(std::set<const State*>& visited_states, std::string_view input) const;
        
        [[nodiscard]]
        bool is_accepting() const;
        void set_accepting(bool accepting);
    };
}

#endif
