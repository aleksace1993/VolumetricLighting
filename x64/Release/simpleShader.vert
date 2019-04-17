#version 330
layout (location = 0) in vec3 vertex;

uniform mat4 LightsMVP;

void main()
{
	gl_Position= LightsMVP*vec4(vertex,1.0);
}
