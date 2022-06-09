#include "common.hpp"
#include "color.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

namespace Common {

bool save_image(const std::string &path, const Color *image, unsigned int width, unsigned int height) {
    int result = stbi_write_png(path.c_str(), width, height, 4, image, width*4);
    return result != 0;
}

std::tuple<std::unique_ptr<Color[]>, int, int> load_image(const std::string &path) {
    // Load the image
    int x, y, n = 4;
    auto pixels = stbi_load(path.c_str(), &x, &y, &n, n);

    // Copy the data
    auto data = std::make_unique<Color[]>(x * y);
    std::copy(pixels, pixels + x*y*n, reinterpret_cast<char*>(data.get()));
    stbi_image_free(pixels);

    return std::make_tuple(std::move(data), x, y);
}

}
