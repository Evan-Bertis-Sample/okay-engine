#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;

uniform vec3 u_color;
out vec3 f_color;

void main()
{
   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);

   f_color = u_color;
}