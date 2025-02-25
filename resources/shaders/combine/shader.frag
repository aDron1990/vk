#version 450
//?#extension GL_KHR_vulkan_glsl: enable

layout(set = 0, binding = 0) uniform sampler2D frameTexture;
layout(set = 1, binding = 0) uniform Global
{
    float gamma;
} global;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = texture(frameTexture, fragTexCoord);
    outColor = vec4(pow(color.rgb, vec3(1.0 / global.gamma)), color.a);
}
