#version 450

// in/out

//layout(location= 0) in vec3 in_UV;
layout(location= 0) out vec4 out_color;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraUI;            // UI orthographic camera matrix
  vec3 cameraPos;
  vec2 vp;                  // viewport position on the virtual desktop
  vec2 vs;                  // [scaled] viewport size
  float UIscale;            //

} glb;

//layout(set= 1, binding= 0) uniform sampler2D texSampler;

// push constants

layout(push_constant) uniform PConsts {
  vec4 color;               // color
  vec4 pos;                 // point position
  float size;               // point size
  int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled color (greyscale)
} p;


// ************************************************************************************
void main() {
  out_color= p.color;

  // disabled color
  if((p.flags& 0x0008)> 0) {
    float n= (out_color.r+ out_color.g+ out_color.b)/ 3;
    out_color= vec4(n, n, n, out_color.a);
  }
}
