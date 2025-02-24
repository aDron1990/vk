#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 fragPosition;

void main() {
    vec4 pos = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    gl_Position = pos;
    fragPosition = vec4(inPosition, 1.0);
}