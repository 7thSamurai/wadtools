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

#include <iostream>
#include <fstream>
#include <filesystem>
#include "wadfile.hpp"

void process_dir(const std::string &base_path, WadFile &wad, std::size_t dir_index) {
    auto dir = wad.get_dir(dir_index);
    auto dir_name = dir.name;

    if (dir_name.size()) {
        std::filesystem::create_directories(std::filesystem::path(base_path + dir_name));
        dir_name += "/";
    }

    // Extract the lumps
    for (auto i : dir.lumps) {
        auto name = wad.lump_name(i);
        auto lump = wad.read_lump(dir_index, i);

        // Write the entry
        std::fstream file(base_path + dir_name + name, std::ios::out | std::ios::binary);
        if (!file.good())
            throw std::runtime_error("Unable to create file " + name);

        lump->write(file);
        file.close();
    }

    // Do the other directories
    for (auto i : dir.dirs)
        process_dir(base_path + dir_name, wad, i);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " [WAD PATHS...]" << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        std::string path = argv[i];
        std::cout << "Extracting " << path << "..." << std::endl;

        try {
            WadFile wad(path, WadFile::Open);

            // Create the base directory
            auto base_path = std::filesystem::path(path).stem().string() + "/";
            std::filesystem::create_directory(std::filesystem::path(base_path));

            process_dir(base_path, wad, 0);
        }
        catch (const std::exception &ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            return 1;
        }
    }

    return 0;
}
