#version 450

// in/out
layout(location= 0) out vec3 out_UV;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec2 vp;                  // viewport position on the virtual desktop
} glb;

// push constants

layout(push_constant) uniform PConsts {
  vec4 color;                   // color
  float x0, y0, z;              // position start coordinates
  float xe, ye;                 // position end coordinates
  float tx0, ty0, tDepth;       // tex coords start (z= depth)
  float txe, tye;               // tex coords end
  float hollow;                 // negative= no hollowing;
  int flags;                    // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture
} p;

// ************************************************************************************

void main() {
  /*
  if(gl_VertexIndex== 0)
    gl_Position= vec4(p.x0,  p.ye,  p.z, 1),
    out_UV=      vec3(p.tx0, p.ty0, p.tDepth);

  else if(gl_VertexIndex== 1)
    gl_Position= vec4(p.xe,  p.ye,  p.z, 1),
    out_UV=      vec3(p.txe, p.ty0, p.tDepth);

  else if(gl_VertexIndex== 2)
    gl_Position= vec4(p.x0,  p.y0,  p.z, 1),
    out_UV=      vec3(p.tx0, p.tye, p.tDepth);

  else if(gl_VertexIndex== 3)
    gl_Position= vec4(p.xe,  p.y0,  p.z, 1),
    out_UV=      vec3(p.txe, p.tye, p.tDepth);
    */

  if(gl_VertexIndex== 0)
    gl_Position= vec4(p.x0,  p.ye,  p.z, 1),
    out_UV=      vec3(p.tx0, p.tye, p.tDepth);

  else if(gl_VertexIndex== 1)
    gl_Position= vec4(p.xe,  p.ye,  p.z, 1),
    out_UV=      vec3(p.txe, p.tye, p.tDepth);

  else if(gl_VertexIndex== 2)
    gl_Position= vec4(p.x0,  p.y0,  p.z, 1),
    out_UV=      vec3(p.tx0, p.ty0, p.tDepth);

  else if(gl_VertexIndex== 3)
    gl_Position= vec4(p.xe,  p.y0,  p.z, 1),
    out_UV=      vec3(p.txe, p.ty0, p.tDepth);



  if((p.flags& 0x0001)> 0)
    gl_Position= glb.cameraPersp* gl_Position;
  else
    gl_Position= glb.cameraOrtho* gl_Position;
}
