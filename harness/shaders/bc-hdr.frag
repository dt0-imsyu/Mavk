#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 colour;
layout(set=0,binding=1) uniform sampler2D image;
layout(push_constant) uniform State { vec4 value; } state;
void main() {
    vec3 v=textureLod(image,uv,state.value.x).rgb;
    colour=vec4(v/(vec3(1.0)+abs(v))*0.5+0.5,1.0);
}
