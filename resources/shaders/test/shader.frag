#version 450
//?#extension GL_KHR_vulkan_glsl: enable

layout(set = 1, binding = 0) uniform sampler2D diffuseMap;

layout(location = 0) in vec4 fragPosition;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor0;

void main()
{
    outColor0 = texture(diffuseMap, fragTexCoord);
}
