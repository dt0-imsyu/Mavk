#version 450
layout(push_constant) uniform Parameters { vec4 state; } parameters;
layout(location=0) out vec4 color;
void main(){color=vec4(1,parameters.state.zw,1);}
