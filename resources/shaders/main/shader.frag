#version 450
//?#extension GL_KHR_vulkan_glsl: enable

layout(std140, set = 1, binding = 0) uniform Light
{
    vec3 direction;
    vec3 viewPosition;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} light;

layout(std140, set = 6, binding = 0) uniform LigthSpace
{
	mat4 space;
} lightSpace;

layout(std140, set = 2, binding = 0) uniform Material 
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 color;
	float shininess;
} material;

layout(set = 3, binding = 0) uniform sampler2D diffuseMap;
layout(set = 4, binding = 0) uniform sampler2D specularMap;
layout(set = 5, binding = 0) uniform sampler2D shadowMap;
layout(set = 7, binding = 0) uniform samplerCube skybox;

layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor0;
//layout(location = 1) out vec4 outColor1;
 
float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = vec3(projCoords.xy * 0.5 + 0.5, projCoords.z);
    if(projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    float shadow = currentDepth - 0.005 > closestDepth ? 1.0 : 0.0;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    // ambient
    vec3 ambient = light.ambient * vec3(texture(diffuseMap, fragTexCoord));

    // diffuse
    vec3 norm = fragNormal;
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(diffuseMap, fragTexCoord).rgb;
    
    // skybox
    vec3 I = normalize(vec3(fragPosition) - light.viewPosition);
    vec3 R = reflect(I, normalize(fragNormal));
    vec3 env = texture(skybox, R).rgb;

    // specular
    vec3 viewDir = normalize(light.viewPosition - vec3(fragPosition));
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = env * spec * vec3(texture(specularMap, fragTexCoord));  

    // result
    float shadow = ShadowCalculation(lightSpace.space * fragPosition);
    if (shadow == 1.0) {specular = vec3(1.0);}
    vec4 result = vec4((ambient + (1.0 - shadow) * (diffuse + specular * (1.0 - shadow))), 1.0);
    
    outColor0 = result;
    //outColor0 = vec4(texture(skybox, R).rgb, 1.0);
}
