#version 310 es

layout(location = 0) in vec2 i_xy_clip_position;
layout(location = 0) out vec2 o_uv;

void main()
{
  gl_Position = vec4(i_xy_clip_position, 0.0, 1.0);
  o_uv = vec2((i_xy_clip_position.x + 1.0) / 2.0, (i_xy_clip_position.y + 1.0) / 2.0);
}
