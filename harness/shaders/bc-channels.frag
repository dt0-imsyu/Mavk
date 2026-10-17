#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 colour;
layout(set=0,binding=1) uniform sampler2D image;
layout(push_constant) uniform State { vec4 value; } state;
void main() {
    vec4 sampleValue=textureLod(image,uv,state.value.x);
    // Remap signed channels into visible UNORM readback. Not a texture rewrite.
    colour=vec4(sampleValue.rg*0.5+0.5,0.25,1.0);
}
