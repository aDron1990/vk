#version 450

layout(set = 1, binding = 0) uniform Light {
    vec3 direction;
    vec3 viewPosition;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} light;

layout(set = 2, binding = 0) uniform Material 
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 color;
	float shininess;
} material;

layout(set = 3, binding = 0) uniform sampler2D diffuseMap;
layout(set = 4, binding = 0) uniform sampler2D specularMap;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;

void main()
{
    // ambient
    vec3 ambient = light.ambient * vec3(texture(diffuseMap, fragTexCoord));

    // diffuse
    vec3 norm = fragNormal;
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseMap, fragTexCoord));
    
    // specular
    vec3 viewDir = normalize(light.viewPosition - fragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(specularMap, fragTexCoord));  

    // result
    vec4 result = vec4((ambient + diffuse + specular), 1.0);
    outColor0 = vec4(result);
    outColor1 = vec4(result);
}
