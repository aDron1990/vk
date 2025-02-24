#version 450
//?#extension GL_KHR_vulkan_glsl: enable

layout(set = 1, binding = 0) uniform samplerCube skybox;

layout(location = 0) in vec4 fragPosition;

layout(location = 0) out vec4 outColor0;

void main()
{
    outColor0 = vec4(texture(skybox, fragPosition.xyz).rgb, 1.0);
}
