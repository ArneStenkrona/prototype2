#version 450
#extension GL_ARB_separate_shader_objects : enable
// Huge thanks to: https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/ for
// a fantastic grid rendering technique

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    float nearPlane;
    float farPlane;
} ubo;

layout(location = 0) in VS_OUT {
    vec3 nearPoint;
    vec3 farPoint;
} fs_in;

layout(location = 0) out vec4 accumColor;
layout(location = 1) out float revealColor;

vec4 grid(vec3 fragPos3D, float scale, bool drawAxis) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;
    return color;
}
float computeDepth(vec3 pos) {
    vec4 viewPos = ubo.view * vec4(pos.xyz, 1.0);
    viewPos.z -= 1; // small bias
    vec4 clip_space_pos = ubo.proj * viewPos;
    return (clip_space_pos.z / clip_space_pos.w);
}
// float computeLinearDepth(vec3 pos) {
//     vec4 clip_space_pos = ubo.proj * ubo.view * vec4(pos.xyz, 1.0);
//     float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
//     float linearDepth = (2.0 * ubo.nearPlane * ubo.farPlane) / (ubo.farPlane + ubo.nearPlane - clip_space_depth * (ubo.farPlane - ubo.nearPlane)); // get linear value between 0.01 and 100
//     return linearDepth / ubo.farPlane; // normalize
// }

float linearizeDepth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));      
}

void main() {
    float t = -fs_in.nearPoint.y / (fs_in.farPoint.y - fs_in.nearPoint.y);
    if (t <= 0) discard;
    vec3 fragPos3D = fs_in.nearPoint + t * (fs_in.farPoint - fs_in.nearPoint);

    float depth = computeDepth(fragPos3D);
    gl_FragDepth = depth;

    float fading =1.0 - 2.0 * t;

    vec4 color = (grid(fragPos3D, 1, true) + grid(fragPos3D, 0.1, true))* float(t > 0); // adding multiple resolution for the grid
    color.a *= fading;

    // // Insert your favorite weighting function here. The color-based factor
    // // avoids color pollution from the edges of wispy clouds. The z-based
    // // factor gives precedence to nearer surfaces.
    
    float weight = 
            max(min(1.0, max( max(color.r, color.g), color.b ) * color.a ) , color.a) *
            clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);

    // Blend Func: GL_ONE, GL_ONE
    // Switch to premultiplied alpha and weight
    accumColor = vec4(color.rgb * color.a, color.a) * weight;

    // Blend Func: GL_ZERO, GL_ONE_MINUS_SRC_ALPHA
    revealColor = color.a;
}