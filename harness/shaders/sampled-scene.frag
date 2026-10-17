#version 450
layout(location=0) in vec2 uv;
layout(location=1) in float lod;
layout(set=0,binding=1) uniform sampler2D image;
layout(location=0) out vec4 colour;
void main(){colour=textureLod(image,uv,lod);}
