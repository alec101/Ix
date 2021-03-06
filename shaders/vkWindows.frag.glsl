#version 450

// the ixWindow shader is designed to work on pixels, so every coordinate or thickness or anything is in pixels
//   but ofc the unit can be anything

// in/out

layout(location= 0) in vec2 in_UV;
layout(location= 0) out vec4 out_color;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec2 vp;                  // viewport position on the virtual desktop
} glb;

layout(set= 1, binding= 0) uniform sampler2D texSampler;
 
// push constants

layout(push_constant) uniform PConsts {
  vec4 color;               // color
  vec4 pos;                 // position/origin
  int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled
} p;

// ************************************************************************************

void main() {
  if((p.flags& 0x0004)> 0)
    out_color= texture(texSampler, in_UV).rgba;
  else
    out_color= vec4(1, 1, 1, 1);

  out_color*= p.color;

  // disabled color
  if((p.flags& 0x0008)> 0) {
    float n= (out_color.r+ out_color.g+ out_color.b)/ 3;
    out_color= vec4(n, n, n, out_color.a);
  }
}
