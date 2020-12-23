#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform texture2D textures[8];
layout(set = 0, binding = 2) uniform sampler samp;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 positions[200];
    vec4 up_vectors[200];
    vec4 right_vectors[200];
    vec4 colors[200];
    mat4 view;
    mat4 projection;
} ubo;

layout(push_constant) uniform PER_OBJECT {
    layout(offset = 0) vec4 billboardSize;
    layout(offset = 16) int albedoIndex;
    layout(offset = 20) int positionIndex;
} pc;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec2 fragTexCoord;
} fs_in;

layout(location = 0) out vec4 accumColor;
layout(location = 1) out float revealColor;

void main() {
    // get albedo
    vec4 color = pc.albedoIndex < 0 ? ubo.colors[pc.positionIndex] :
                                       (texture(sampler2D(textures[pc.albedoIndex], samp), fs_in.fragTexCoord)) * ubo.colors[pc.positionIndex];
    float weight = 
        max(min(1.0, max( max(color.r, color.g), color.b ) * color.a ) , color.a) *
        clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);

    // Blend Func: GL_ONE, GL_ONE
    // Switch to premultiplied alpha and weight
    accumColor = color * weight;

    // Blend Func: GL_ZERO, GL_ONE_MINUS_SRC_ALPHA
    revealColor = color.a;
}
