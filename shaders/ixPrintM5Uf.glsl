 #version 330 core

 
// Interpolated values from the vertex shaders
in vec2 UV;
// Ouput data
out vec4 color;
 
// Values that stay constant for the whole mesh.
uniform sampler2D texSampler;

uniform vec4 u_color;
//uniform int u_bgType;


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


  // Output color = color of the texture at the specified UV
  color= texture(texSampler, UV).rgba;

  /// update color with print class's color
  color*= u_color;
}