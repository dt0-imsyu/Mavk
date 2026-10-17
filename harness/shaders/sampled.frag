#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 colour;
layout(set=0,binding=1) uniform sampler2D image;
layout(push_constant) uniform State { vec4 value; } state;
void main() { colour=textureLod(image,uv,state.value.x); }
