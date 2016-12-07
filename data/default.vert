#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 uv;

uniform mat4 mvp;

void
main()
{
	gl_Position = mvp * vec4(in_position, 1.0);
	uv = vec2(in_uv.x, 1-in_uv.t);
}