#version 450

layout(set = 1, binding = 0) uniform Emiter 
{
	vec3 color;
} emiter;

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;

void main()
{
    outColor0 = vec4(emiter.color, 1.0f);
    outColor1 = vec4(emiter.color, 1.0f);
}
