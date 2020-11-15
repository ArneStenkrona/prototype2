#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput accumulation;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput revealage;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 accum = subpassLoad(accumulation);
    float reveal = subpassLoad(revealage).r;

    // Blend Func: GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA
    outColor = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
    // outColor = reveal == 0 ? vec4(1,0,0,0) : vec4(0,1,0,0);
}

