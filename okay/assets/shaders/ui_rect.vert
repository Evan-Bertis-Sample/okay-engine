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
uniform vec3 u_cameraPosition;
uniform vec3 u_cameraDirection;
uniform vec3 u_color;

out vec3 v_color;
out vec3 v_normal;
out vec3 v_position;
out vec2 v_uv;
out vec3 v_cameraPosition;
out vec3 v_cameraDirection;


void main() {
   mat4 mvp = u_projectionMatrix * u_viewMatrix * u_modelMatrix;
   vec4 pos = mvp * vec4(a_pos, 1.0f);
   gl_Position = pos;

   v_color = u_color;
   v_normal = a_normal;
   v_position = a_pos;
   v_uv = a_uv;
   v_cameraPosition = u_cameraPosition;
   v_cameraDirection = u_cameraDirection;
}