#version 450
#ifdef MAVK_CULL_USER_BLOCK
#ifdef MAVK_CULL_BLOCK_LOCATION
layout(location=4) in Payload { vec4 color; flat int tag; vec2 values[2]; } payload;
#else
in Payload { layout(location=0) vec4 color; layout(location=1) flat int tag; layout(location=2) vec2 values[2]; } payload;
#endif
#define color payload.color
#else
layout(location=0) in vec4 color;
#endif
layout(location=0) out vec4 outputColor;
void main(){outputColor=color;
#ifdef MAVK_CULL_USER_BLOCK
if(payload.tag!=7||payload.values[0]!=vec2(2,3)||payload.values[1]!=vec2(4,5))outputColor=vec4(0,1,0,1);
#endif
#ifdef MAVK_CULL_PRIMITIVE_ID
if(gl_PrimitiveID<0||gl_PrimitiveID>2)outputColor=vec4(1,1,0,1);
else outputColor=vec4(vec3(gl_PrimitiveID==0,gl_PrimitiveID==1,gl_PrimitiveID==2)*color.a,1);
#endif
}
