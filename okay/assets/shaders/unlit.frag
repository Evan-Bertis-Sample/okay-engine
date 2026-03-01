#version 300 es
precision highp float;

out vec4 FragColor;

in vec3 v_color;
in vec3 v_normal;
in vec3 v_position;
in vec2 v_uv;
in vec3 v_cameraPosition;
in vec3 v_cameraDirection;

void main()
{
   FragColor = vec4(v_color, 1.0);
}