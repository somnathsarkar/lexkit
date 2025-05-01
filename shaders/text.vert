#version 310 es

layout(location = 0) in vec4 i_xywh;
layout(location = 1) in vec4 i_uv_minmax;
layout(location = 0) out vec2 o_uv;

const vec2 quad[4] = vec2[4](
  vec2(1.0, 0.0),
  vec2(1.0, 1.0),
  vec2(0.0, 0.0),
  vec2(0.0, 1.0)
);

void main()
{
  vec2 pos_local = mix(i_xywh.xy, i_xywh.xy + i_xywh.zw, quad[gl_VertexID]);
  gl_Position = vec4(
    mix(-1.0, 1.0, pos_local.x),
    mix(1.0, -1.0, pos_local.y),
    0.0,
    1.0
  );
  o_uv = mix(i_uv_minmax.xy, i_uv_minmax.zw, quad[gl_VertexID]);
}
