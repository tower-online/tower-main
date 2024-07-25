#include <catch2/catch_test_macros.hpp>
#include <tower/system/event.hpp>

#include <string>

using namespace tower;

TEST_CASE("event notifies to event listeners", "[Event]") {
    Event<int> event {};

    int num = 0;
    const auto _listener1 = std::make_shared<EventListener<int>>([&num](const int add) {
        num += add;
    });
    const auto _listener2 = std::make_shared<EventListener<int>>([&num](const int add) {
        num += add;
    });

    event.subscribe(_listener1->shared_from_this());
    event.subscribe(_listener2->shared_from_this());

    event.notify(42);
    REQUIRE(num == 84);
}