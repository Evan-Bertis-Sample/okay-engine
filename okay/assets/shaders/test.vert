#version 300 es
precision highp float;

// position, normal, color, uv

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec2 a_uv;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform vec3 u_color;

out vec3 f_color;

void main() {
   gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(a_pos, 1.0f);
   // set the color to a_uv
   f_color = vec3(a_uv.x, a_uv.y, 1.0f);
}