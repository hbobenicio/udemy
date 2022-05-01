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

        void get_epsilon_states(std::set<const State*>& visited_states, std::vector<const State*>& epsilon_states) const;

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
        std::vector<const State*> get_epsilon_closure() const;
        
        // GETTERS

        [[nodiscard]]
        bool is_accepting() const;

        [[nodiscard]]
        const std::map<std::string, States>& get_transitions() const;

        // SETTERS
        void set_accepting(bool accepting);
    };
}

#endif
