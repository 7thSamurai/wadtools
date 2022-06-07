#pragma once

#include <cstdint>
#include <cassert>

class Color
{
public:
    Color() : r(0), g(0), b(0), a(0) {
    }

    Color(std::uint8_t r0, std::uint8_t g0, std::uint8_t b0) : r(g0), g(g0), b(b0) {
    }

    Color(std::uint8_t r0, std::uint8_t g0, std::uint8_t b0, std::uint8_t a0) : r(r0), g(g0), b(b0), a(a0) {
    }

    std::uint8_t operator [] (std::size_t i) const {
        assert(i < 4);
        return reinterpret_cast<const std::uint8_t*>(this)[i];
    }

    std::uint8_t &operator [] (std::size_t i) {
        assert(i < 4);
        return reinterpret_cast<std::uint8_t*>(this)[i];
    }

    std::uint8_t r, g, b, a;
};
