#pragma once

#include <string>
#include <unordered_map>

namespace tower {
class State {
public:
    virtual ~State() = default;
    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual std::string_view get_name() const = 0;

    void add_transition(std::string_view state_name, std::function<bool()> condition = [] { return true; });
    bool can_transition(std::string_view next_state_name);

private:
    std::unordered_map<std::string_view, std::function<bool()>> _transitions;
};


inline void State::add_transition(std::string_view state_name, std::function<bool()> condition) {
    _transitions[state_name] = std::move(condition);
}

inline bool State::can_transition(std::string_view next_state_name) {
    if (const auto next_iter {_transitions.find(next_state_name)}; next_iter == _transitions.end()) {
        return false;
    } else {
        const auto& condition {next_iter->second};
        return condition();
    }
}
}
