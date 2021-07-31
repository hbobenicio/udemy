#ifndef FA_STATE_H
#define FA_STATE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <optional>

namespace fa
{
    class State;

    using States = std::vector<std::shared_ptr<State>>;

    class State {
    protected:
        bool accepting;
        std::unordered_map<std::string_view, States> transitions;

    public:
        State(bool accepting = false) noexcept;

        void add_transition_for_symbol(std::string_view symbol, std::shared_ptr<State> state);

        std::optional<States> get_transitions_for_symbol(std::string_view symbol) const;
        bool is_accepting() const;

        friend std::ostream& operator<<(std::ostream& os, const State& state);
    };
}

#endif
