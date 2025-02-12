#version 450

layout(set = 0, binding = 0) uniform sampler2D frameTexture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 color = texture(frameTexture, fragTexCoord);
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    outColor = vec4(vec3(brightness), 1.0);
    if(brightness > 0.8)
        outColor = vec4(color.rgb, 1.0);
    else
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
}
