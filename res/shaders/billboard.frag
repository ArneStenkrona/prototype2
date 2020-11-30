#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform texture2D textures[8];
layout(set = 0, binding = 2) uniform sampler samp;


layout(push_constant) uniform PER_OBJECT {
    layout(offset = 0) vec4 baseColor;
    layout(offset = 16) vec2 billboardSize;
    layout(offset = 24) int albedoIndex;
    layout(offset = 28) int positionIndex;
} pc;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec2 fragTexCoord;
} fs_in;

layout(location = 0) out vec4 accumColor;
layout(location = 1) out float revealColor;

void main() {
    // get albedo
    vec4 color = 0 < 0 ? pc.baseColor :
                                       (texture(sampler2D(textures[0], samp), fs_in.fragTexCoord)) * pc.baseColor;
    float weight = 
        max(min(1.0, max( max(color.r, color.g), color.b ) * color.a ) , color.a) *
        clamp(0.03 / (1e-5 + pow(fs_in.fragPos.z / 200, 4.0)), 1e-2, 3e3);

    // Blend Func: GL_ONE, GL_ONE
    // Switch to premultiplied alpha and weight
    accumColor = color * weight;

    // Blend Func: GL_ZERO, GL_ONE_MINUS_SRC_ALPHA
    revealColor = color.a;
}
