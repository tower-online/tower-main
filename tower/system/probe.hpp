#pragma once

#include <iostream>

namespace tower {
class Probe final {
public:
    Probe() {
        std::cout << "[Probe] Constructed" << std::endl;
    }

    ~Probe() {
        std::cout << "[Probe] Destructed" << std::endl;
    }

    Probe(const Probe& other) {
        std::cout << "[Probe] Copy constructed" << std::endl;
    }

    Probe& operator=(const Probe& other) {
        std::cout << "[Probe] Copy assigned" << std::endl;
        return *this;
    }

    Probe(Probe&& other) noexcept {
        std::cout << "[Probe] Move constructed" << std::endl;
    }

    Probe& operator=(Probe&& other) noexcept {
        std::cout << "[Probe] Move assigned" << std::endl;
        return *this;
    }
};
}
