#pragma once

#include <boost/asio.hpp>
#include <tower/system/state.hpp>

#include <chrono>
#include <unordered_map>

namespace tower {
class StateMachine {
public:
    ~StateMachine();

    void add_state(std::unique_ptr<State> state);
    void set_initial_state(std::string_view state_name);

    bool can_transition(std::string_view new_state_name);
    bool try_transition(std::string_view new_state_name);
    void transition_delayed(std::string new_state_name, boost::asio::strand<boost::asio::any_io_executor>& strand,
        std::chrono::steady_clock::duration delay, bool override = true);

    void abort();

private:
    State* _current_state {};
    std::unordered_map<std::string_view, std::unique_ptr<State>> _states {};
    std::shared_ptr<boost::asio::steady_timer> _delay_timer;
};


inline StateMachine::~StateMachine() {
    abort();
}

inline void StateMachine::add_state(std::unique_ptr<State> state) {
    const std::string_view name {state->get_name()};
    _states[name] = std::move(state);
}

inline void StateMachine::set_initial_state(const std::string_view state_name) {
    if (const auto initial_state_iter {_states.find(state_name)}; initial_state_iter == _states.end()) return;
    else _current_state = initial_state_iter->second.get();
}

inline bool StateMachine::can_transition(std::string_view new_state_name) {
    if (!_current_state) return false;
    return _current_state->can_transition(new_state_name);
}

inline bool StateMachine::try_transition(const std::string_view new_state_name) {
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

inline void StateMachine::transition_delayed(std::string new_state_name, boost::asio::strand<boost::asio::any_io_executor>& strand,
    const std::chrono::steady_clock::duration delay, const bool override) {
    if (override) abort();
    _delay_timer = std::make_shared<boost::asio::steady_timer>(strand);
    _delay_timer->expires_after(delay);

    co_spawn(strand, [this, timer = _delay_timer, new_state_name = std::move(new_state_name)]->boost::asio::awaitable<void> {
        if (const auto [ec] {co_await timer->async_wait(as_tuple(boost::asio::use_awaitable))}; ec) {
            co_return;
        }

        try_transition(new_state_name);
    }, boost::asio::detached);
}

inline void StateMachine::abort() {
    if (_delay_timer) {
        _delay_timer->cancel();
        _delay_timer.reset();
    }
}
}
