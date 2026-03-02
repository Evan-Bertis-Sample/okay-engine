#version 300 es
precision highp float;

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec2 a_uv;

/* Transforms */
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

/* Material */
uniform vec3 u_color;

/* Camera */
uniform vec3 u_cameraPosition;

out vec3 v_color;
out vec3 v_worldPos;
out vec3 v_worldNormal;
out vec2 v_uv;

mat3 normalMatrix(mat4 m) {
    return mat3(transpose(inverse(m)));
}

void main() {
    vec4 worldPos4 = u_modelMatrix * vec4(a_pos, 1.0f);
    v_worldPos = worldPos4.xyz;

    v_worldNormal = vec3(u_modelMatrix * vec4(a_normal, 0.0f));

    v_uv = a_uv;
    v_color = u_color * a_color.rgb;

    gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(a_pos, 1.0f);
}