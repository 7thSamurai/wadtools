#pragma once

#include "lump.hpp"
#include "color.hpp"

class Palette : public Lump
{
public:
    Palette() : Lump() {
    }

    Palette(std::fstream &file, std::size_t size) : Lump(file, size) {
    }

    Palette(const std::string &path) : Lump(path) {
    }

    Color operator [] (std::size_t index) {
        assert(index < size_/3);
        return Color(data_[index*3+0], data_[index*3+1], data_[index*3+2], 255);
    }

};
