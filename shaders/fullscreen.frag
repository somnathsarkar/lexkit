#version 310 es
precision highp float;

layout(location = 0) in vec2 i_uv;
layout(location = 0) out vec4 o_rgba;

layout(binding = 0) uniform sampler2D smp_text;

void main()
{
  float samp_tex = texture(smp_text, i_uv).r;
  o_rgba = vec4(samp_tex, 0.0, 0.0, 1.0);
}
