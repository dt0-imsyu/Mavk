#version 450
layout(location=0) in vec2 position;
layout(location=1) in vec2 inUV;
layout(set=0,binding=0) uniform State { mat4 transform; vec4 data; } state;
layout(location=0) out vec2 uv;
layout(location=1) out float lod;
void main(){gl_Position=state.transform*vec4(position,0,1);uv=inUV;lod=state.data.x;}
