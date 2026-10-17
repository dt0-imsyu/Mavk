#version 450
layout(location=0) out vec4 color;
// Different from the host diagnostic's orange triangle: verifies app shader use.
void main(){color=vec4(0,.25,1,1);}
