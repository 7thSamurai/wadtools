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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fstream>

#include "lump.hpp"

class WadFile
{
public:
    enum Mode {
        Open,
        CreateIWAD,
        CreatePWAD
    };

    WadFile(const std::string &path, Mode mode);
    ~WadFile();

    std::size_t num_dirs() const;
    std::string dir_name(std::size_t index) const;
    std::size_t dir_size(std::size_t index) const;

    std::string lump_name(std::size_t dir, std::size_t index) const;
    std::size_t lump_size(std::size_t dir, std::size_t index) const;

    bool valid();

    Lump read_lump(std::size_t dir, std::size_t index);
    bool write_lump(const std::string &dir, const std::string &name, Lump lump);

private:
    struct Header {
        char id[4];
        std::uint32_t size;
        std::uint32_t offset;
    };

    struct LumpEntry {
        std::uint32_t offset;
        std::uint32_t size;
        char name[8];
    };

    struct Dir {
        std::string name;
        std::vector<LumpEntry> lumps;
    };

    void open(const std::string &path);
    void create(const std::string &path);
    void create_dirs(const std::vector<LumpEntry> &lumps);

    std::string lump_name(const char name[8]) const;
    bool is_map_marker(const char name[8]) const;
    bool is_map_lump(const char name[8]) const;

    Mode mode_;

    std::vector<Dir> dirs;
    std::fstream file;
};
