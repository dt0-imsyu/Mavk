#version 450
layout(location=0) in vec2 position;
layout(push_constant) uniform Parameters { mat4 transform; } parameters;
void main(){gl_Position=parameters.transform*vec4(position,0,1);}
