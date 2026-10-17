#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 color;
layout(location=0) out vec3 vertexColor;
layout(push_constant) uniform Parameters { mat4 transform; } parameters;
void main(){gl_Position=parameters.transform*vec4(position,1);vertexColor=color;}
