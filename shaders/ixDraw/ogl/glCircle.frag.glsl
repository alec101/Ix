#version 330 core

// input vertex shader texture UV
in vec3 UV;

// output color
out vec4 color;
 
// standard Ix uniforms
uniform vec2 u_viewportPos; // viewport position in pixels on the virtual desktop (osi/ix use virtual desktop coords, always)
uniform vec4 u_color;

// clipping plane
uniform bool u_clip;    // 0= disable, 1= enable
uniform vec2 u_clip0;   // clipping: 0= start / x0, y0
uniform vec2 u_clipE;   // clipping: e= end / xe, ye

// circle parameters
uniform bool  u_filled;       // set true to fill the circle
uniform vec2  u_centre;       // circle centre
uniform float u_radius;       // circle radius
uniform float u_thick;        // circle thickness

uniform bool  u_useTexture;
uniform sampler2D texSampler;



void main() {
  // clipping - done with gl_FragCoord "https://www.opengl.org/wiki/Built-in_Variable_(GLSL)#Fragment_shader_inputs"
  if(u_clip)
    if((gl_FragCoord.x< u_clip0.x) || (gl_FragCoord.x> u_clipE.x) ||
       (gl_FragCoord.y< u_clip0.y) || (gl_FragCoord.y> u_clipE.y)) {
      color= vec4(0.0f, 0.0f, 0.0f, 0.0f);           // sets color to 0 if outside the clipping
      return;
    }



  // CIRCLE drawing
  float dx= gl_FragCoord.x- (u_centre.x- u_viewportPos.x);
  float dy= gl_FragCoord.y- (u_centre.y- u_viewportPos.y);
  float distance= sqrt((dx* dx)+ (dy* dy));   // current pixel distance from the circle origin
  float alpha;

  // filled
  if(u_filled) {
    alpha= u_radius- distance;

  // not filled
  } else {
    /// anything outside or inside the perimeter, has 0 alpha
    float halfThick= 0.5* (u_thick- 1);     /// this trick will make the circle thickness, not pretty, but it works good.
    alpha= 1+ halfThick- abs(distance- (u_radius- (1+ halfThick)));
  }

  if(alpha< 0) alpha= 0;
  // if(alpha< 0.3) alpha= 0.3;   // DEBUG, this can show the circle rectangle that is used to draw it
  color= vec4(1, 1, 1, alpha);


  // textured circle.... dono if it's worth any more code for this
  if(u_useTexture)
    color*= texture(texSampler, vec2(UV)).rgba;

  /// update color with print class's color
  color*= u_color;
}