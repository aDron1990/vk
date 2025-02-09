#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 2, binding = 0) uniform Light {
    vec3 position;
    vec3 viewPosition;
    vec3 color;
} light;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

float ambientStrength = 0.1;
float specularStrength = 0.5;

void main()
{
    vec3 ambient = ambientStrength * light.color;
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(light.position - fragPosition);

    vec3 viewDir = normalize(light.viewPosition - fragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * light.color;  

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color;

    vec3 result = (ambient + diffuse + specular) * fragColor;
    outColor = vec4(result, 1.0f);
}

//outColor = texture(texSampler, fragTexCoord);