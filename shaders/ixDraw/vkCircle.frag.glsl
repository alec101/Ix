#version 450

// in/out

layout(location= 0) in vec3 in_UV;
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
  vec4 color;                   // color
  float x, y, z, radius;        // position (centre), radius
  //float xe, ye;                 // ending position
  float tx0, ty0, tDepth;       // tex coords start (z= depth)
  float txe, tye;               // tex coords end
  float hollow;                 // negative= no hollowing;
  int flags;                    // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled color (greyscale)

} p;


// ************************************************************************************
void main() {
  
  // CIRCLE drawing
  //float dx= glb.vp.x+ gl_FragCoord.x- (p.x0+ (p.dx/ 2));
  //float dy= glb.vp.y+ gl_FragCoord.y- (p.y0+ (p.dy/ 2));
  float dx= glb.vp.x+ gl_FragCoord.x- p.x;
  float dy= glb.vp.y+ gl_FragCoord.y- p.y;

  float distance= sqrt((dx* dx)+ (dy* dy));   // current pixel distance from the circle origin

  float alpha;
  //float radius;

  //if(p.dx< p.dy) radius= p.dx/ 2;
  //else           radius= p.dy/ 2;

  // filled circle
  if(p.hollow< 0.0) {
    alpha= p.radius- distance;

  // hollow circle
  } else {
    /// anything outside or inside the perimeter, has 0 alpha
    float halfThick= 0.5* (p.hollow- 1);     /// this trick will make the circle thickness, not pretty, but it works good.
    alpha= 1+ halfThick- abs(distance- (p.radius- (1+ halfThick)));
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
