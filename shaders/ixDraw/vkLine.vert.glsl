#version 450

// in/out

// global uniform buffer
layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec2 vp;                  // viewport position on the virtual desktop
} glb;

// push constants
layout(push_constant) uniform PConsts {
  vec4 color;               // color
  vec4 pos[2];              // line start and end point positions
  float size;               // point size
  int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= greyscale, 5= smoothing
} p;


// ************************************************************************************
void main() {
  gl_Position= p.pos[gl_VertexIndex];

  if((p.flags& 0x0001)> 0)
    // probly i'll send the updated gl_Position to fragment, so you get a x, y for perspective, but i dono ATM
    gl_Position= glb.cameraPersp* gl_Position;
  else {
    gl_Position= glb.cameraOrtho* gl_Position;
  }
}
