#version 450

// in/out

//layout(location= 0) out vec3 out_UV;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms { // global uniform buffer
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec3 cameraPos;
  vec2 vp;                  // viewport position on the virtual desktop
} glb;

// push constants

layout(push_constant) uniform PConsts {
  vec4 color;               // color
  vec4 pos;                 // point position
  float size;               // point size
  int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture
} p;


// ************************************************************************************
void main() {
  gl_Position= p.pos;
  gl_PointSize= p.size;

  if((p.flags& 0x0001)> 0)
    gl_Position= glb.cameraPersp* gl_Position;
  else
    gl_Position= glb.cameraOrtho* gl_Position;
}
