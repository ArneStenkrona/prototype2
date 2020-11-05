#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

void Texture::load(char const * path) {
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        printf("failed to load texture: %s\n", path);
        assert(false && "failed to load texture image!");
    }

    size_t bufferSize = texWidth * texHeight * 4;
    pixelBuffer.resize(bufferSize);
    for (size_t i = 0; i < pixelBuffer.size(); i++) {
        pixelBuffer[i] = pixels[i];
    }

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    stbi_image_free(pixels);
}

Texture* Texture::defaultTexture() {
    static Texture texture = {{0,0,0,1},1,1,4,1};
    return &texture;
}
