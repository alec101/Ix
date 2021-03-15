#version 450
 
layout(location= 0) in vec2 in_UV;
layout(location= 1) out vec4 out_color;

layout(set= 1, binding= 1) uniform sampler2D texSampler;

// push constants
layout(push_constant) uniform PConst {
  vec4 pos;               // changed frequently
  vec4 color;
  vec2 clip0;             // clipping: 0= start / x0, y0
  vec2 clipE;             // clipping: e= end / xe, ye
  int flags;              // each byte meaning: 0= persp camera, 1= ortho camera, 2= clip enable
} p;


void main() {
  // clipping - done with gl_FragCoord "https://www.opengl.org/wiki/Built-in_Variable_(GLSL)#Fragment_shader_inputs"
  if(p.flags& 0x0004)
  //if(p.clip)
    if((gl_FragCoord.x< p.clip0.x) || (gl_FragCoord.x> p.clipE.x) ||
       (gl_FragCoord.y< p.clip0.y) || (gl_FragCoord.y> p.clipE.y)) {
      out_color= vec4(0.0f, 0.0f, 0.0f, 0.0f);           // sets color to 0 if outside the clipping
      return;
    }

  // Output color = color of the texture at the specified UV
  out_color= texture(texSampler, UV).rgba;

  /// update color with print class's color
  out_color*= p.color;
}