#ifndef COMPONENT_H
#define COMPONENT_H

typedef uint32_t ModelIndex;
struct Model {
    ModelIndex index;
};

struct Transform {
    struct Position { float x, y, z; } position;
    struct Rotation { float x, y, z, w; } rotation;
    struct Scale { float x, y, z; } scale;
};

#endif