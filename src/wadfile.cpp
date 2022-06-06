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

#include "wadfile.hpp"
#include "common.hpp"
#include <algorithm>

WadFile::WadFile(const std::string &path, Mode mode) : mode_(mode) {
    if (mode == Mode::Open)
        open(path);
    else
        create(path);
}

WadFile::~WadFile() {
    // Only for saving
    if (mode_ == Mode::Open)
        return;

    // Create the header
    Header header;
    header.id[0] = (mode_ == Mode::CreateIWAD) ? 'I' : 'P';
    header.id[1] = 'W'; header.id[2] = 'A'; header.id[3] = 'D';
    header.size   = Common::little32(lumps.size());
    header.offset = Common::little32(file.tellp());

    // Save the lump entries
    for (auto i = 0; i < lumps.size(); i++) {
        LumpEntry lump = lumps[i];

        // Endian swap the lump entry
        lump.offset = Common::little32(lump.offset);
        lump.size   = Common::little32(lump.size);

        file.write(reinterpret_cast<char*>(&lump), sizeof(LumpEntry));
    }

    // Now save the header
    file.seekg(0, std::ios::beg);
    file.write(reinterpret_cast<char*>(&header), sizeof(Header));
    file.close();
}

std::size_t WadFile::num_lumps() const {
    return lumps.size();
}

std::string WadFile::lump_name(std::size_t index) const {
    if (index >= lumps.size())
        return "";

    auto name = lumps[index].name;

    // Find first occurence of a null byte
    int i;
    for (i = 7; i >= 0; i--) {
        if (!name[i])
            break;
    }

    if (i >= 0)
        return std::string(lumps[index].name, i+1);

    return std::string(lumps[index].name, 8);
}

std::size_t WadFile::lump_size(std::size_t index) const {
    if (index >= lumps.size())
        return 0;

    return lumps[index].size;
}

bool WadFile::valid() {
    if (mode_ != Mode::Open)
        return false;

    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();

    for (const auto &lump : lumps) {
        // Make sure that the entry is contained inside
        if (lump.offset + lump.size > size)
            return false;
    }

    return true;
}

Lump WadFile::read_lump(std::size_t index) {
    if (mode_ != Mode::Open || index >= lumps.size())
        return Lump();

    file.seekg(lumps[index].offset, std::ios::beg);

    return Lump(file, lumps[index].size);
}

bool WadFile::write_lump(const std::string &name, Lump lump) {
    // Including null-terminator
    if (mode_ == Mode::Open || name.size() >= sizeof(LumpEntry::name))
        return false;

    // Create the entry
    LumpEntry entry;
    entry.offset = file.tellp();
    entry.size   = lump.size();

    // Copy the name
    std::fill_n(entry.name, sizeof(entry.name), 0x00);
    std::copy_n(&name[0], name.size(), entry.name);
    lumps.push_back(entry);

    // Write the data
    lump.write(file);

    return true;
}

void WadFile::open(const std::string &path) {
    // Read the header
    file = std::fstream(path, std::ios::in | std::ios::binary);
    if (!file.good())
        throw std::runtime_error("Unable to open file " + path);

    Header header;
    file.read(reinterpret_cast<char*>(&header), sizeof(Header));

    // Make sure that the file is a WAD
    if (std::string(header.id, 4) != "IWAD" && std::string(header.id, 4) != "PWAD")
        throw std::runtime_error("File " + path + " is not a WAD");

    header.offset = Common::little32(header.offset);
    header.size   = Common::little32(header.size);

    file.seekg(header.offset);
    lumps.resize(header.size);

    // Read the lump entries
    for (auto &lump : lumps) {
        file.read(reinterpret_cast<char*>(&lump), sizeof(LumpEntry));

        lump.offset = Common::little32(lump.offset);
        lump.size   = Common::little32(lump.size);
    }
}

void WadFile::create(const std::string &path) {
    // Create the file
    file = std::fstream(path, std::ios::out | std::ios::binary);
    if (!file.good())
        throw std::runtime_error("Unable to create file " + path);

    // Skip the space where the header goes (We'll fill it in later)
    file.seekg(sizeof(Header), std::ios::beg);
}
