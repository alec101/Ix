#version 450

// in/out

layout(location= 0) out vec4 out_color;

layout(location= 1) in vec4 in_vert[2];

// sets

layout(set= 0, binding= 0) uniform GlobalUniforms {
  mat4 cameraPersp;         // perspective camera matrix
  mat4 cameraUI;            // UI orthographic camera matrix
  vec3 cameraPos;
  vec2 vp;                  // UI viewport position on the virtual desktop
  vec2 vs;                  // UI viewport size
  float UIscale;
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
    float dx= in_vert[1].x- in_vert[0].x;
    float dy= in_vert[1].y- in_vert[0].y;
    float distance= abs((dy* gl_FragCoord.x)- (dx* gl_FragCoord.y)+ (in_vert[1].x* in_vert[0].y)- (in_vert[1].y* in_vert[0].x)) / sqrt((dy* dy)+ (dx* dx));

    float alpha= (0.5* (p.size+ 1))- distance;    // seems best
    //float alpha= (0.5* (p.size+ .5))- distance; // not quite
    //float alpha= (p.size* 0.5)- distance;       // this would work if there were the rule that line width should always be +1, when smoothing
    
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
