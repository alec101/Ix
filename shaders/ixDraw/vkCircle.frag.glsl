#version 450

// in/out

layout(location= 0) in vec3 in_UV;
layout(location= 0) out vec4 out_color;

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;       // perspective camera matrix
  mat4 cameraUI;          // UI orthographic camera matrix
  vec3 cameraPos;         // camera position (eye) in the world
  vec2 vp;                // [scaled] UI viewport position on the virtual desktop
  vec2 vs;                // [scaled] UI viewport size
  float UIscale;

  vec3 sunPos;
  vec3 sunColor;
  float sunAmbientStr;
  float sunSpecularStr;
} glb;

layout(set= 1, binding= 0) uniform sampler2D texSampler;

// push constants

layout(push_constant) uniform PConsts {
  vec4 color;                   // color
  float x, y, z, radius;        // position (centre), radius
  //float xe, ye;                 // ending position
  float tx0, ty0, tDepth;       // tex coords start (z= depth)
  float txe, tye;               // tex coords end
  float hollow;                 // negative= no hollowing;
  int flags;                    // each byte meaning: 1= persp camera, 2= UI camera, 3= enable texture, 4= disabled color (greyscale)
} p;


// ************************************************************************************
void main() {
  vec4 center= vec4(p.x, p.y, p.z, 1);
  float radius= p.radius;
  float alpha, d;

  if((p.flags& 0x0001)> 0) {
    center= glb.cameraPersp* center;
    center.xyz= center.xyz/ center.w;
    center.xy= (center.xy* .5+ .5)* glb.vs;

    radius= (glb.cameraPersp* vec4(radius, 0, 0, 1)).x;
    radius= (radius* .5)* glb.vs.x;
  } else {
    center.x-= glb.vp.x;
    center.y-= glb.vp.y;
  }
  
  d= distance(gl_FragCoord, center);

  // filled circle
  if(p.hollow< 0.0) {
    alpha= radius- d;

  // hollow circle
  } else {
    /// anything outside or inside the perimeter, has 0 alpha
    float halfThick= 0.5* (p.hollow- 1);     /// this trick will make the circle thickness, not pretty, but it works good.
    alpha= 1+ halfThick- abs(d- (radius- (1+ halfThick)));
  }
   
  if(alpha< 0) alpha= 0;
  //if(alpha< 0.3) alpha= 0.3;   // DEBUG, this can show the circle rectangle that is used to draw it
  out_color= vec4(1, 1, 1, alpha);

  // textured circle.... dono if it's worth any more code for this
  if((p.flags& 0x0004)> 0)
    out_color*= texture(texSampler, vec2(in_UV)).rgba;

  // update color with print class's color
  out_color*= p.color;

  // disabled color
  if((p.flags& 0x0008)> 0) {
    float n= (out_color.r+ out_color.g+ out_color.b)/ 3;
    out_color= vec4(n, n, n, out_color.a);
  }
}






