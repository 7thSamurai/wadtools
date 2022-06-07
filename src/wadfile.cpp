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
#include <cassert>
#include <cstring>

WadFile::WadFile(const std::string &path, Mode mode) : mode_(mode) {
    // Default directory
    dirs.push_back({"", {}});

    if (mode == Mode::Open)
        open(path);
    else
        create(path);
}

WadFile::~WadFile() {
    // Only for saving
    if (mode_ == Mode::Open)
        return;

    // Create the lumps
    std::vector<LumpEntry> lumps;
    for (const auto &dir : dirs) {
        LumpEntry marker;
        marker.offset = marker.size = 0;
        std::copy(dir.name.begin(), dir.name.end(), marker.name);

        if (is_map_lump(marker.name)) {
            lumps.push_back(marker);

            for (const auto &lump : dir.lumps)
                lumps.push_back(lump);
        }

        else if (dir.name.size()) {
            // Start marker
            std::copy_n("_START", 6, marker.name+dir.name.size());
            lumps.push_back(marker);

            for (const auto &lump : dir.lumps)
                lumps.push_back(lump);

            // End marker
            std::copy_n("_END", 4, marker.name+dir.name.size());
            lumps.push_back(marker);
        }

        else {
            for (const auto &lump : dir.lumps)
                lumps.push_back(lump);
        }
    }

    // Create the header
    Header header;
    header.id[0] = (mode_ == Mode::CreateIWAD) ? 'I' : 'P';
    header.id[1] = 'W'; header.id[2] = 'A'; header.id[3] = 'D';
    header.size   = Common::little32(lumps.size());
    header.offset = Common::little32(file.tellp());

    // Save the lump entries
    for (auto lump : lumps) {
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

std::size_t WadFile::num_dirs() const {
    return dirs.size();
}

std::string WadFile::dir_name(std::size_t index) const {
    assert(index < dirs.size());

    return dirs[index].name;
}

std::size_t WadFile::dir_size(std::size_t index) const {
    assert(index < dirs.size());

    return dirs[index].lumps.size();
}

std::string WadFile::lump_name(std::size_t dir, std::size_t index) const {
    assert(dir < dirs.size());
    assert(index < dirs[dir].lumps.size());

    return lump_name(dirs[dir].lumps[index].name);
}

std::size_t WadFile::lump_size(std::size_t dir, std::size_t index) const {
    assert(dir < dirs.size());
    assert(index < dirs[dir].lumps.size());

    return dirs[dir].lumps[index].size;
}

bool WadFile::valid() {
    if (mode_ != Mode::Open)
        return false;

    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();

    for (const auto &dir : dirs) {
        for (const auto &lump : dir.lumps) {
            // Make sure that the entry is contained inside
            if (lump.offset + lump.size > size)
                return false;
        }
    }

    return true;
}

Lump WadFile::read_lump(std::size_t dir, std::size_t index) {
    assert(dir < dirs.size());
    assert(index < dirs[dir].lumps.size());

    if (mode_ != Mode::Open)
        return Lump();

    file.seekg(dirs[dir].lumps[index].offset, std::ios::beg);

    return Lump(file, dirs[dir].lumps[index].size);
}

bool WadFile::write_lump(const std::string &dir, const std::string &name, Lump lump) {
    // Including null-terminator
    if (mode_ == Mode::Open || name.size() >= sizeof(LumpEntry::name))
        return false;

    // Make sure that the directory name isn't to long
    if (dir.size() > 2)
        return false;

    // Create the entry
    LumpEntry entry;
    entry.offset = file.tellp();
    entry.size   = lump.size();

    // Copy the name
    std::fill_n(entry.name, sizeof(entry.name), 0x00);
    std::copy_n(&name[0], name.size(), entry.name);

    // First try to find the directory
    unsigned int index;
    for (index = 0; index < dirs.size(); index++) {
        if (dirs[index].name == name)
            break;
    }

    // Otherwise create it
    if (index == dirs.size())
        dirs.push_back({name, {}});

    // Add the entry
    dirs[index].lumps.push_back(entry);

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
    std::vector<LumpEntry> lumps(header.size);

    // Read the lump entries
    for (auto &lump : lumps) {
        file.read(reinterpret_cast<char*>(&lump), sizeof(LumpEntry));

        lump.offset = Common::little32(lump.offset);
        lump.size   = Common::little32(lump.size);
    }

    // Create the directories
    create_dirs(lumps);
}

void WadFile::create(const std::string &path) {
    // Create the file
    file = std::fstream(path, std::ios::out | std::ios::binary);
    if (!file.good())
        throw std::runtime_error("Unable to create file " + path);

    // Skip the space where the header goes (We'll fill it in later)
    file.seekg(sizeof(Header), std::ios::beg);
}

void WadFile::create_dirs(const std::vector<LumpEntry> &lumps) {
    // Find the directories
    for (auto i = 0; i < lumps.size(); i++) {
        // We are only looking for virtual lumps
        if (lumps[i].size) {
            dirs[0].lumps.push_back(lumps[i]);
            continue;
        }

        // Check if this is a map marker
        auto name = lump_name(lumps[i].name);
        if (is_map_marker(lumps[i].name)) {
            // Find the end of the lumps
            int j;
            for (j = i+1; j < std::min<int>(i+10, lumps.size()); j++) {
                if (!is_map_lump(lumps[j].name))
                    break;
            }

            // Create the directory
            dirs.push_back({name, {}});

            // Add the lumps to the directory
            for (int k = i+1; k < j; k++, i++)
                dirs.back().lumps.push_back(lumps[k]);

            continue;
        }

        // Make sure that this is a starting marker
        if (!Common::ends_with(name, "_START")) {
            dirs[0].lumps.push_back(lumps[i]);
            continue;
        }

        // Strip the "_START" from the name
        name = name.substr(0, name.size() - 6);

        // Search for the end marker
        int j;
        for (j = i+1; j < lumps.size(); j++) {
            if (lumps[i].size)
                continue;

            // Check if this is the end marker
            if (lump_name(lumps[j].name) == (name+"_END"))
                break;
        }

        // Couldn't find the end marker, just add the start one as a lump
        if (i != lumps.size()) {
            dirs[0].lumps.push_back(lumps[i]);
            continue;
        }

        // Create the directory
        dirs.push_back({name, {}});

        // Add the lumps to the directory
        for (int k = i+1; k < j-1; k++, i++)
            dirs.back().lumps.push_back(lumps[k]);
    }
}

std::string WadFile::lump_name(const char name[8]) const {
    // Find first occurence of a null byte
    int i;
    for (i = 7; i >= 0; i--) {
        if (!name[i])
            break;
    }

    if (i >= 0)
        return std::string(name, i+1);

    return std::string(name, 8);
}

bool WadFile::is_map_marker(const char name[8]) const {
    if (name[0] == 'E' && name[2] == 'M') {
        if (!std::isdigit(name[1])) return false;
        if (!std::isdigit(name[3])) return false;
        if (name[4]) return false;

        return true;
    }

    else if (name[0] == 'M' && name[1] == 'A' && name[3] == 'P') {
        if (!std::isdigit(name[3])) return false;
        if (!std::isdigit(name[4])) return false;
        if (name[5]) return false;

        return true;
    }

    return false;
}

bool WadFile::is_map_lump(const char name[8]) const {
    if (strncmp(name, "THINGS",   8) == 0 ||
        strncmp(name, "LINEDEFS", 8) == 0 ||
        strncmp(name, "VERTEXES", 8) == 0 ||
        strncmp(name, "SEGS",     8) == 0 ||
        strncmp(name, "SSECTORS", 8) == 0 ||
        strncmp(name, "NODES",    8) == 0 ||
        strncmp(name, "SECTORS",  8) == 0 ||
        strncmp(name, "REJECT",   8) == 0 ||
        strncmp(name, "BLOCKMAP", 8) == 0)
        return true;

    return false;
}
