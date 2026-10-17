#version 450
layout(location=0) in vec2 position;
layout(push_constant) uniform Parameters { vec4 state; } parameters;
void main(){gl_Position=vec4(position+parameters.state.xy,0,1);}
