#version 300 es
precision highp float;

out vec4 FragColor;

in vec3 v_color;
in vec3 v_normal;
in vec3 v_position;
in vec2 v_uv;
in vec3 v_cameraPosition;
in vec3 v_cameraDirection;

uniform sampler2D u_albedo;
uniform vec2 u_borderRadius; // UV units per axis
uniform vec2 u_borderWidth;  // UV units per axis
uniform vec4 u_borderColor;
uniform vec2 u_clipSpaceTL;
uniform vec2 u_clipSpaceBR;

float roundedRectSDF(vec2 uv, vec2 radius) {
   vec2 p = uv - vec2(0.5f);

    // Scale into an isotropic-ish space where radius.x/radius.y can differ.
   vec2 r = clamp(radius, vec2(0.0f), vec2(0.5f));
   vec2 scale = vec2(1.0f) / max(r, vec2(0.0001f));

   vec2 ps = p * scale;
   vec2 halfSize = vec2(0.5f) * scale - vec2(1.0f);

   vec2 q = abs(ps) - halfSize;

    // Distance is in "radius units"; 0 is the outer rounded rect edge.
   return length(max(q, vec2(0.0f))) + min(max(q.x, q.y), 0.0f) - 1.0f;
}

void main() {
    // v_position is already in NDC space.
   if(v_position.x < u_clipSpaceTL.x || v_position.x > u_clipSpaceBR.x ||
      v_position.y > u_clipSpaceTL.y || v_position.y < u_clipSpaceBR.y) {
      discard;
   }

   vec2 radius = clamp(u_borderRadius, vec2(0.0f), vec2(0.5f));
   vec2 borderWidth = max(u_borderWidth, vec2(0.0f));

   float dist = roundedRectSDF(v_uv, radius);

   if(dist > 0.0f) {
      discard;
   }

   vec4 texColor = texture(u_albedo, v_uv);
   vec4 fillColor = vec4(v_color, 1.0f) * texColor;

    // Convert border width into the same scaled distance space.
   vec2 safeRadius = max(radius, vec2(0.0001f));
   float borderDist = max(borderWidth.x / safeRadius.x, borderWidth.y / safeRadius.y);

   bool isBorder = borderDist > 0.0f && dist > -borderDist;

   FragColor = isBorder ? u_borderColor : fillColor;
}