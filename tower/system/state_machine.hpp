#pragma once

#include <tower/system/state.hpp>

#include <unordered_map>

namespace tower {
class StateMachine {
public:
    void add_state(std::unique_ptr<State> state);
    void set_initial_state(std::string_view state_name);

    bool try_transition(std::string_view new_state_name);

private:
    State* _current_state {};
    std::unordered_map<std::string_view, std::unique_ptr<State>> _states {};
};


inline void StateMachine::add_state(std::unique_ptr<State> state) {
    const std::string_view name {state->get_name()};
    _states[name] = std::move(state);
}

inline void StateMachine::set_initial_state(std::string_view state_name) {
    if (const auto initial_state_iter {_states.find(state_name)}; initial_state_iter == _states.end()) return;
    else _current_state = initial_state_iter->second.get();
}

inline bool StateMachine::try_transition(std::string_view new_state_name) {
    if (!_current_state) return false;
    if (!_current_state->can_transition(new_state_name)) return false;

    State* new_state;
    if (const auto new_state_iter {_states.find(new_state_name)}; new_state_iter == _states.end()) return false;
    else new_state = new_state_iter->second.get();

    _current_state->exit();
    new_state->enter();
    _current_state = new_state;

    return true;
}
}
