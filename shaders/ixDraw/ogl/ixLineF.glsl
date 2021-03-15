#version 330 core

// input vertex shader texture UV
in vec3 UV;

// output color
out vec4 color;
 
// standard Ix uniforms
uniform vec2 u_viewportPos; // viewport position in pixels on the virtual desktop (osi/ix use virtual desktop coords, always)
uniform vec4 u_color;

// uniforms

uniform bool u_clip;    // 0= disable, 1= enable
uniform vec2 u_clip0;   // clipping: 0= start / x0, y0
uniform vec2 u_clipE;   // clipping: e= end / xe, ye

void main() {
  // clipping - done with gl_FragCoord "https://www.opengl.org/wiki/Built-in_Variable_(GLSL)#Fragment_shader_inputs"
  if(u_clip)
    if((gl_FragCoord.x< u_clip0.x) || (gl_FragCoord.x> u_clipE.x) ||
       (gl_FragCoord.y< u_clip0.y) || (gl_FragCoord.y> u_clipE.y)) {
      color= vec4(0.0f, 0.0f, 0.0f, 0.0f);           // sets color to 0 if outside the clipping
      return;
    }




  // MAKEME






  // Output color = color of the texture at the specified UV
  if(u_useTexture)
    color= texture(texSampler, vec2(UV)).rgba;
  else
    color= vec4(1, 1, 1, 1);

  /// update color with print class's color
  color*= u_color;
}