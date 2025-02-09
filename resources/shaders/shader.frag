#version 450

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(set = 2, binding = 0) uniform Light {
    vec3 position;
    vec3 viewPosition;
    vec3 color;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} light;

layout(set = 3, binding = 0) uniform Material 
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 color;
	float shininess;
} material;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    // ambient
    vec3 ambient = light.ambient * material.ambient;

    // diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(light.position - fragPosition);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);
    
    // specular
    vec3 viewDir = normalize(light.viewPosition - fragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);  

    // result
    vec3 result = (ambient + diffuse + specular) * material.color;
    outColor = vec4(result, 1.0f);
}

//outColor = texture(texSampler, fragTexCoord);