#ifndef PRT_TEXTURE_H
#define PRT_TEXTURE_H

#include "src/container/vector.h"

struct Texture {
    prt::vector<unsigned char> pixelBuffer;
    int texWidth, texHeight, texChannels;
    uint32_t mipLevels;

    void load(char const * path);

    inline unsigned char* sample(float x, float y) {
        int sx = static_cast<int>(float(texWidth - 1) * x + 0.5f);
        int sy = static_cast<int>(float(texHeight - 1) * y + 0.5f);
        int si = texChannels * (sy * texWidth + sx);
        return &pixelBuffer[si];
    }
    static Texture* defaultTexture();
};

#endif