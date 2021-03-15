#version 330 core

// the ixWindow shader is designed to work on pixels, so every coordinate or thickness or anything is in pixels
//   but ofc the unit can be anything



// TODO / TO THINK
// gl_FragCoord have 0, 0 origin, that is NOT THE VDESKTOP, that everything uses. it's the viewport 0, 0
// clip could have another variable, that would substract from the u_camera coords somehow, and everything sent to glsl would have the right coords
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^




// input from vertex program
in vec2 UV;

// output color
out vec4 color;
 
// uniforms
uniform vec2 u_viewportPos;   // viewport position in pixels on the virtual desktop (osi/ix use virtual desktop coords, always)

uniform sampler2D texSampler;
uniform bool u_useTexture;
uniform vec4 u_color;
uniform bool u_disabled;      // window is disabled, must compute the disabled color

uniform vec2 u_quadPos0;      // custom vertex position x0, y0
uniform vec2 u_quadPosE;      // custom vertex position xe, ye

uniform bool u_clip;          // 0= disable, 1= enable
uniform vec2 u_clip0;         // clipping: 0= start / x0, y0
uniform vec2 u_clipE;         // clipping: e= end / xe, ye

/*
uniform bool u_circle;        // set true to draw a circle
uniform bool u_circleFilled;  // set true to fill the circle
uniform vec2 u_circleCentre;  // circle centre
uniform float u_circleRadius; // circle radius
uniform float u_circleThick;  // circle thickness
*/

/*
uniform bool u_hollowRect;    // set true to draw a hollow rectangle
uniform float u_hollowRectWidth;// hollow rectangle lines width in pixels
*/

void main() {
  // clipping - done with gl_FragCoord "https://www.opengl.org/wiki/Built-in_Variable_(GLSL)#Fragment_shader_inputs"
  if(u_clip)
    if((gl_FragCoord.x< u_clip0.x) || (gl_FragCoord.x> u_clipE.x) ||
       (gl_FragCoord.y< u_clip0.y) || (gl_FragCoord.y> u_clipE.y)) {
      color= vec4(0.0f, 0.0f, 0.0f, 0.0f);           // sets color to 0 if outside the clipping
      return;
    }
/*
  // CIRCLE drawing
  if(u_circle) {
    float dx= gl_FragCoord.x- u_circleCentre.x;
    float dy= gl_FragCoord.y- u_circleCentre.y;
    float distance= sqrt((dx* dx)+ (dy* dy));   // current pixel distance from the circle origin
    float alpha;

    // filled circle
    if(u_circleFilled) {
      alpha= u_circleRadius- distance;

    // not filled circle
    } else {
      /// anything outside or inside the perimeter, has 0 alpha
      float halfThick= 0.5* (u_circleThick- 1);     /// this trick will make the circle thickness, not pretty, but it works good.
      alpha= 1+ halfThick- abs(distance- (u_circleRadius- (1+ halfThick)));
    }
    if(alpha< 0) alpha= 0;
    // if(alpha< 0.3) alpha= 0.3;   // DEBUG, this can show the circle rectangle that is used to draw it
    color= vec4(1, 1, 1, alpha);
    
  // hollow RECTANGLE drawing
  } else if(u_hollowRect) {
    if( (gl_FragCoord.x+ u_viewportPos.x<= (u_quadPos0.x+ u_hollowRectWidth)) || 
        (gl_FragCoord.x+ u_viewportPos.x>= (u_quadPosE.x- u_hollowRectWidth)) || 
        (gl_FragCoord.y+ u_viewportPos.y<= (u_quadPos0.y+ u_hollowRectWidth)) || 
        (gl_FragCoord.y+ u_viewportPos.y>= (u_quadPosE.y- u_hollowRectWidth)) )
      color= vec4(1, 1, 1, 1);
    else
      color= vec4(0, 0, 0, 0);

  // normal rectangle drawing, textured or not
  } else {
    */
    if(u_useTexture)
      color= texture(texSampler, UV).rgba;
    else
      color= vec4(1, 1, 1, 1);

  //}
  
  color*= u_color;

  // DISABLED COLOR
  if(u_disabled) {
    float n= (color.r+ color.g+ color.b)/ 3;
    color= vec4(n, n, n, color.a);
  }


}




