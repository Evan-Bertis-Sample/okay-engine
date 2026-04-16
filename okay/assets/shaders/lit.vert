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
out mat3 v_worldToTangent;

void main() {
    vec4 worldPos4 = u_modelMatrix * vec4(a_pos, 1.0f);
    v_worldPos = worldPos4.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(u_modelMatrix)));
    v_worldNormal = normalize(normalMatrix * a_normal);

    v_uv = a_uv;
    v_color = u_color * a_color.rgb;

    vec3 ref = vec3(0.0f, 1.0f, 0.0f);
    if (abs(dot(v_worldNormal, ref)) > 0.999) ref = vec3(1.0f, 0.0f, 0.0f);
    vec3 v_tangent = normalize(ref - dot(ref, v_worldNormal) * v_worldNormal);
    vec3 v_bitangent = normalize(cross(v_tangent, v_worldNormal));

    mat3 TBN = mat3(v_tangent, v_worldNormal, v_bitangent);
    v_worldToTangent = transpose(TBN);
    gl_Position = u_projectionMatrix * u_viewMatrix * worldPos4;
}
