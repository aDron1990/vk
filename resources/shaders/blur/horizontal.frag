#version 450

layout(set = 0, binding = 0) uniform sampler2D frameTexture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
	vec2 texOffset = 5.0 / textureSize(frameTexture, 0); // ������ ������ �������
    vec3 result = texture(frameTexture, fragTexCoord).rgb * weight[0]; // ����������� �������

    // ������ �� �������� �������� (��������������)
    for (int i = 1; i < 5; ++i) {
        result += texture(frameTexture, fragTexCoord + vec2(texOffset.x * i, 0.0)).rgb * weight[i];
        result += texture(frameTexture, fragTexCoord - vec2(texOffset.x * i, 0.0)).rgb * weight[i];
    }


    outColor = vec4(result, 1.0);
}
