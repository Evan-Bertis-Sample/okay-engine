#version 300 es
precision highp float;

out vec4 FragColor;

in vec4 v_color;
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

   vec2 r = clamp(radius, vec2(0.0f), vec2(0.5f));

   if(r.x <= 0.0001f && r.y <= 0.0001f) {
      vec2 q = abs(p) - vec2(0.5f);
      return max(q.x, q.y);
   }

   vec2 scale = vec2(1.0f) / max(r, vec2(0.0001f));
   vec2 ps = p * scale;
   vec2 halfSize = vec2(0.5f) * scale - vec2(1.0f);

   vec2 q = abs(ps) - halfSize;

   return length(max(q, vec2(0.0f))) + min(max(q.x, q.y), 0.0f) - 1.0f;
}

void main() {
    // v_position is already in NDC space.
    /*
    if (v_position.x < u_clipSpaceTL.x || v_position.x > u_clipSpaceBR.x ||
        v_position.y > u_clipSpaceTL.y || v_position.y < u_clipSpaceBR.y) {
        discard;
    }
    */

   vec2 radius = clamp(u_borderRadius, vec2(0.0f), vec2(0.5f));
   vec2 borderWidth = max(u_borderWidth, vec2(0.0f));

   float dist = roundedRectSDF(v_uv, radius);

    // Anti-alias width in screen space.
   float aa = max(fwidth(dist), 0.00001f);

    // Shape alpha: 1 inside, 0 outside, smooth at edge.
   float shapeAlpha = 1.0f - smoothstep(0.0f, aa, dist);

   vec4 texColor = texture(u_albedo, v_uv);
   vec4 fillColor = v_color * texColor;

   vec2 safeRadius = max(radius, vec2(0.0001f));
   float borderDist = max(borderWidth.x / safeRadius.x, borderWidth.y / safeRadius.y);

   float borderMask = 0.0f;

   if(borderDist > 0.0f) {
        // dist == 0 is outer edge.
        // dist == -borderDist is inner border edge.
      float innerEdge = -borderDist;

        // 1 near the outer edge, 0 past the inner edge.
      borderMask = smoothstep(innerEdge - aa, innerEdge + aa, dist);
   }

   vec4 color = mix(fillColor, u_borderColor, borderMask);

   color.a *= shapeAlpha;

   if(color.a <= 0.001f) {
      discard;
   }

   FragColor = color;
}