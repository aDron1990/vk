#version 450
//?#extension GL_KHR_vulkan_glsl: enable
#extension GL_EXT_nonuniform_qualifier: enable // for textureArray[] unstead textureArray[128]

layout(set = 4, binding = 0) uniform sampler2D textureArray[];

layout(location = 0) in vec3 fragPosition;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor0;

layout(std140, set = 1, binding = 0) uniform Matetial
{
	vec3 diffuse;
	vec3 specular;
	float shininess;
	int diffuseIndex;
	int specularIndex;
} material;

layout(std140, set = 2, binding = 0) uniform View
{
	vec3 position;
} view;

layout(std140, set = 3, binding = 0) uniform DirLight
{
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
} dirLight;

void main()
{
	vec3 diffuseColor = material.diffuseIndex == -1 ? material.diffuse : texture(textureArray[material.diffuseIndex], fragTexCoord).rgb;
	vec3 specularColor = material.specularIndex == -1 ? material.specular : texture(textureArray[material.specularIndex], fragTexCoord).rgb;

	vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(-dirLight.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    
	vec3 viewDir = normalize(view.position - fragPosition);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

	vec3 ambient = dirLight.ambient * diffuseColor;
	vec3 diffuse = diff * dirLight.diffuse * diffuseColor;
	vec3 specular = spec * dirLight.specular * specularColor;

    outColor0 = vec4(diffuse + ambient + specular, 1.0);
}
