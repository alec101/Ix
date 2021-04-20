#version 450

// in/out

layout(location= 1) out vec4 out_vert[2];
//layout(location= 1) out vec4 out_v2;

// global uniform buffer
layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraOrtho;         // orthographic camera matrix
  vec2 vp;                  // viewport position on the virtual desktop
  vec2 vs;                  // viewport size
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
  
  /* https://stackoverflow.com/questions/42118665/world-position-to-screen-position-in-fragment-shader-glsl
  vec4 worldSpace = vec4(1., 2., 3., 1.);
  // get homogeneous clip space coordinates
  vec4 clipSpace = projectionMatrix * ( modelViewMatrix * worldCoords );
  // apply perspective divide to get normalized device coordinates
  vec3 ndc = clipSpace.xyz / clipSpace.w;
  // do viewport transform
  vec2 screenSpace = (ndc.xy * .5 + .5) * vec2(uCanvasWidth, uCanvasHeight);
  screenSpace.y = uCanvasHeight - screenSpace.y;
  */

  // better to compute out_vert in the vert shader, due it's gonna happen twice, than doing it on every pixel in fragment
  if((p.flags& 0x0001)> 0) {
    gl_Position= glb.cameraPersp* p.pos[gl_VertexIndex];
    out_vert[0]= glb.cameraPersp* p.pos[0],
    out_vert[1]= glb.cameraPersp* p.pos[1];
  } else {
    gl_Position= glb.cameraOrtho* p.pos[gl_VertexIndex];
    out_vert[0]= glb.cameraOrtho* p.pos[0],
    out_vert[1]= glb.cameraOrtho* p.pos[1];
  }

  out_vert[0].xyz= out_vert[0].xyz/ out_vert[0].w;
  out_vert[0].xy= (out_vert[0].xy* .5+ .5)* glb.vs;

  out_vert[1].xyz= out_vert[1].xyz/ out_vert[1].w;
  out_vert[1].xy= (out_vert[1].xy* .5+ .5)* glb.vs;
}
