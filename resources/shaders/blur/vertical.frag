#version 450

layout(set = 0, binding = 0) uniform sampler2D frameTexture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const float weight[15] = float[] (
    0.00884695, 0.01821591, 0.0335624,  0.05533504, 0.08163802, 0.10777793,
    0.12732458, 0.13459835, 0.12732458, 0.10777793, 0.08163802, 0.05533504,
    0.0335624,  0.01821591, 0.00884695
);

void main()
{
	vec2 texOffset = 1.0 / textureSize(frameTexture, 0);
    vec3 result = texture(frameTexture, fragTexCoord).rgb * weight[0];

    for (int i = 1; i < 15; ++i) {
        result += texture(frameTexture, fragTexCoord + vec2(0.0, texOffset.y * i)).rgb * weight[i];
        result += texture(frameTexture, fragTexCoord - vec2(0.0, texOffset.y * i)).rgb * weight[i];
    }

    outColor = vec4(result, 1.0);
}