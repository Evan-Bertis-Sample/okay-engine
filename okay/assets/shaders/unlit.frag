#version 300 es
precision highp float;

out vec4 FragColor;

in vec3 v_color;
in vec3 v_normal;
in vec3 v_position;
in vec2 v_uv;
in vec3 v_cameraPosition;
in vec3 v_cameraDirection;

uniform sampler2D u_albedo;  // optional, can be ignored if not used

void main()
{
   vec3 texture = texture(u_albedo, v_uv).rgb;
   vec3 baseColor = v_color * texture;
   FragColor = vec4(baseColor, 1.0);
}