#version 450

// in/out

layout(location= 0) out vec4 out_color;

// sets

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
  int flags;                // each byte meaning: 1= persp camera, 2= ortho camera, 3= enable texture, 4= disabled color (greyscale), 5= smoothing
} p;


// ************************************************************************************
void main() {

  // smoothing with alpha, based on distance of current pixel from line
  if((p.flags& 0x0010)> 0) {
    // distance(p1, p2, (x0, y0))= | (y2- y1)* x0- (x2- x1)* y0 + x2* y1- y2* x1 |  /  sqrt((y2- y1)^2 + (x2- x1)^2)

    vec2 s= p.pos[0].xy,    // start position
         e= p.pos[1].xy;    // end position

    if((p.flags& 0x0002)> 0)
      s.x+= 0.5, s.y+= 0.5, e.x+= 0.5, e.y+= 0.5;

    float dx= e.x- s.x;
    float dy= e.y- s.y;
    float x0= gl_FragCoord.x+ glb.vp.x;
    float y0= gl_FragCoord.y+ glb.vp.y;

    float distance= abs((dy* x0)- (dx* y0)+ (e.x* s.y)- (e.y* s.x)) / sqrt((dy* dy)+ (dx* dx));
    float alpha= (0.5* (p.size+ 1))- distance;

    if(alpha< 0) alpha= 0;
    if(alpha> 1) alpha= 1;

    out_color= p.color;
    out_color.a*= alpha;

  // vulkan simple line (they could've at least put the slightest effort into the default line)
  } else {
    out_color= p.color;
  }

  // disabled color
  if((p.flags& 0x0008)> 0) {
    float n= (out_color.r+ out_color.g+ out_color.b)/ 3;
    out_color= vec4(n, n, n, out_color.a);
  }
}
