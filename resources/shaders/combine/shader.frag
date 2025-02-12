#version 450

layout(set = 0, binding = 0) uniform sampler2D frameTexture;
layout(set = 1, binding = 0) uniform sampler2D blurTexture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = texture(frameTexture, fragTexCoord);
    vec4 blurColor = texture(blurTexture, fragTexCoord);
    
    outColor = color + blurColor;
}
