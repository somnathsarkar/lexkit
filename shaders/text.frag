#version 310 es
precision highp float;

layout(location = 0) in vec2 i_uv;
layout(location = 0) out vec4 o_rgba;

layout(binding = 0) uniform sampler2D u_smp_text;

void main()
{
  float samp = texture(u_smp_text, i_uv).r;
  o_rgba = vec4(1.0, 1.0, 1.0, samp);
}
