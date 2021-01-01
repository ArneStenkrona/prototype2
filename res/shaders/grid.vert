#version 450
#extension GL_ARB_separate_shader_objects : enable
// Huge thanks to: http://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/ for
// a fantastic grid rendering technique

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    float nearPlane;
    float farPlane;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out VS_OUT {
    vec3 nearPoint;
    vec3 farPoint;
} vs_out;

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec3 p = inPosition;
    vs_out.nearPoint = UnprojectPoint(p.x, p.y, 0.0, ubo.view, ubo.proj).xyz; // unprojecting on the near plane
    vs_out.farPoint = UnprojectPoint(p.x, p.y, 1.0, ubo.view, ubo.proj).xyz; // unprojecting on the far plane

    gl_Position = vec4(p, 1.0); // using directly the clipped coordinates
}
