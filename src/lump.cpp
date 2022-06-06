// Copyright (C) 2022 Zach Collins <zcollins4@proton.me>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "lump.hpp"
#include <fstream>

Lump::Lump() : size_(0) {
}

Lump::Lump(std::fstream &file, std::size_t size) : size_(size) {
    data_ = std::make_unique<std::uint8_t[]>(size_);
    file.read(reinterpret_cast<char*>(data_.get()), size_);
}

Lump::Lump(const std::string &path) {
    // Read the raw binary data from the file
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.good())
        throw std::runtime_error("Error opening file " + path);

    size_ = file.tellg();
    data_ = std::make_unique<std::uint8_t[]>(size_);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data_.get()), size_);
    file.close();
}

void Lump::write(std::fstream &file) {
    file.write(reinterpret_cast<char*>(data_.get()), size_);
}
