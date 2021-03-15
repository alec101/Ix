#pragma once


class gloDraw;
class glObject;

class gloPoint {
public:

  void init() {}
  
  gloPoint(): glo(null), parent(null) {}


private:
  glObject *glo;
  gloDraw *parent;
  friend class gloDraw;
  friend class glObject;
};


class gloLine {
public:
  // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
  // distance(p1, p2, (x0, y0))=  |(y2- y1)x0- (x2- x1)y0 + x2y1- y2x1|  /  sqrt((y2- y1)^2 + (x2- x1)^2)
  // probly i will set each pixel's color based on the distance
  // either that, or a very thin quad 
  void init() {}



private:
  glObject *glo;
  gloDraw *parent;
  friend class gloDraw;
};



// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ gloCircle class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

class gloCircle {
public:

  inline void useProgram()                                    { glUseProgram(_sl->id); }
  inline void setCamera(ixCamera *c)                          { glUniformMatrix4fv(u.camera, 1, GL_FALSE, c->cameraMat); }
  inline void setColor(float r, float g, float b, float a)    { glUniform4f(u.color, r, g, b, a); }
  inline void setColorui(uint8 r, uint8 g, uint8 b, uint8 a)  { glUniform4f(u.color, ((float)r)/ 255.0f, ((float)g)/ 255.0f, ((float)b)/ 255.0f, ((float)a)/ 255.0f); }
  inline void setColorv(float *f)                             { glUniform4fv(u.color, 1, f); }

  inline void setTexture(ixTexture *t)                        { glBindTexture(t->glData.target, t->glData.id); glUniform1i(u.useTexture, 1); }
  inline void disableTexture()                                { glUniform1i(u.useTexture, 0); }
  inline void setTexCoords(float x0= 0.0f, float y0= 0.0f, float xe= 1.0f, float ye= 1.0f, float depth= 0.0f) { glUniform3f(u.tex0, x0, y0, depth); glUniform2f(u.texE, xe, ye); }

  inline void setCoordsD(float x0, float y0, float dx, float dy, float z= 0.0f) {
    glUniform3f(u.pos, x0, y0, z);
    glUniform2f(u.delta, dx, dy);
    glUniform1f(u.radius, MIN(dx, dy)/ 2.0f);
    glUniform2f(u.centre, x0+ (dx/ 2.0f), y0+ (dy/ 2.0f));
  }

  inline void setCoordsRadius(float x, float y, float radius, float z= 0.0f) {
    float delta= radius* 2;
    glUniform3f(u.pos, x- radius, y- radius, z);
    glUniform2f(u.delta, delta, delta);
    glUniform1f(u.radius, radius);
    glUniform2f(u.centre, x, y);
  }

  inline void setFilled(bool b) { glUniform1i(u.filled, b); }
  inline void setThick(float t) { glUniform1f(u.thick, t); }
  
  void setClipPlane(int32 x0, int32 y0, int32 xe, int32 ye);
  void setClipPlaneD(int32 x0, int32 y0, int32 dx, int32 dy);
  void setClipPlaneR(const recti &r);
  inline void delClipPlane() { glUniform1i(u.clip, 0); }

  inline void render() { glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); }

  // constructors / destructors

  gloCircle(): glo(null), parent(null), _sl(null) {}
  ~gloCircle() {}



private:


  void _init();
  gloShader *_sl;
  struct Uniforms {
    int camera;
    int color;
    
    int pos;        // triangle vertices (3 vec3)
    int delta;      // texture coordinates (3 vec3)
    int useTexture; // bool - use texture or not
    int tex0;       // tex coord start position    tex0.z= depth
    int texE;       // tex coord end position
    int clip;
    int clip0, clipE;
    int filled;     // set true to fill the circle
    int centre;     // circle centre
    int radius;     // circle radius
    int thick;      // circle thickness

    Uniforms(): camera(-1), color(-1), pos(-1), delta(-1), useTexture(-1), tex0(-1), texE(-1),
                           filled(-1), centre(-1), radius(-1), thick(-1), clip(-1),
                           clip0(-1), clipE(-1)  {}
    void initUniforms(gloShader *);
  } u;

  glObject *glo;
  gloDraw *parent;
  friend class gloDraw;
};







// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ gloTriangle class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

class gloTriangle {
public:

  /*
  vec3 vert[3];
  vec3 tex[3];
  vec4 color;
  ixTexture *texture;     // set this to null to not use any texture

  inline void setColor  (float in_r, float in_g, float in_b, float in_a) { color.set(in_r, in_g, in_b, in_a); }
  inline void setColorui(uint8 in_r, uint8 in_g, uint8 in_b, uint8 in_a) { color.set(((float)in_r)/ 255.0f, ((float)in_g)/ 255.0f, ((float)in_b)/ 255.0f, ((float)in_a)/ 255.0f); }

  inline void setVertexnv(int n, vec3 &in_v) { vert[n]= in_v; }
  inline void setVertexnf(int n, float in_x, float in_y, float in_z) { vert[n].x= in_x, vert[n].y= in_y, vert[n].z= in_z; }
  inline void setVertexni(int n, int32 in_x, int32 in_y, int32 in_z) { vert[n].x= (float)in_x, vert[n].y= (float)in_y, vert[n].z= (float)in_z; }
  inline void setVertexv(vec3 *in_v) { vert[0]= in_v[0], vert[1]= in_v[1], vert[2]= in_v[2]; }
  inline void setVertexf(float *in_f) { vert[0]= in_f; vert[1]= in_f+ 3; vert[2]= in_f+ 4; }

  inline void setTexnv(int n, vec3 &in_v) { tex[n]= in_v; }
  inline void setTexnf(int n, float in_u, float in_v, float in_w) { tex[n].x= in_u, tex[n].y= in_v, tex[n].z= in_w; }
  inline void setTexv(vec3 *in_v) { tex[0]= in_v[0], tex[1]= in_v[1], tex[2]= in_v[2]; }
  inline void setTexf(float *in_f) { tex[0]= in_f, tex[1]= in_f+ 3, tex[2]= in_f+ 6; }
  inline void setTexture(ixTexture *in_t) { texture= in_t; }
  inline void disableTexture() { texture= null; }


  void render();
  */

  inline void useProgram() { glUseProgram(_sl->id); }
  inline void setCamera(ixCamera *c) { glUniformMatrix4fv(u.camera, 1, GL_FALSE, c->cameraMat); }
  inline void setColor(float r, float g, float b, float a)   { glUniform4f(u.color, r, g, b, a); }
  inline void setColorv(float *f) { glUniform4fv(u.color, 1, f); }
  inline void setColorui(uint8 r, uint8 g, uint8 b, uint8 a) { glUniform4f(u.color, ((float)r)/ 255.0f, ((float)g)/ 255.0f, ((float)b)/ 255.0f, ((float)a)/ 255.0f); }

  inline void setCoords(vec3 *v) { glUniform3fv(u.vert, 3, v->v); }
  inline void setCoords1(int32 vertex, vec3 &v) { glUniform3fv(u.vert+ vertex, 1, v.v); }
  
  inline void setTexture(ixTexture *t) { glBindTexture(t->glData.target, t->glData.id); glUniform1i(u.useTexture, 1); }
  inline void setTexCoords(vec3 *tex) { glUniform3fv(u.tex, 3, tex->v); }
  inline void setTexCoords1(int32 vertex, vec3 &tex) { glUniform3fv(u.tex+ vertex, 1, tex.v); }
  inline void disableTexture() { glUniform1i(u.useTexture, 0); }
  
  void setClipPlane(int32 x0, int32 y0, int32 xe, int32 ye);
  void setClipPlaneD(int32 x0, int32 y0, int32 dx, int32 dy);
  void setClipPlaneR(const recti &r);
  inline void delClipPlane() { glUniform1i(u.clip, 0); }

  inline void render() { glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); }



  // constructor / destructor

  gloTriangle(): glo(null), parent(null) {}
  ~gloTriangle() {}
  void delData() {}

private:

  void init();

  gloShader *_sl;
  struct Uniforms {
    int camera, color;
    int vert;       // triangle vertices (3 vec3)
    int tex;        // texture coordinates (3 vec3)
    int useTexture; // bool - use texture or not
    int clip;
    int clip0, clipE;

    Uniforms(): camera(-1), color(-1), vert(-1), tex(-1), useTexture(-1), clip(-1), clip0(-1), clipE(-1)  {}
    void initUniforms(gloShader *);
  } u;

  glObject *glo;
  gloDraw *parent;
  friend class gloDraw;
};


///=====================///
// QUAD class =====----- //
///=====================///
class gloQuad {
public:
  /*
  rectf pos;
  vec4 color;
  ixTexture *texture;     // set this to null to not use any texture
  rectf texCoords;        // default is 0,0 -> 1,1, to show all texture, but this can be changed to show parts of the texture
  float hollow;           // [def:-1.0f] the quad is hollow or not. if negative, it's disabled. 
  */


  // funcs
  /*
  inline void setColor  (float in_r, float in_g, float in_b, float in_a) { color.set(in_r, in_g, in_b, in_a); }
  inline void setColorui(uint8 in_r, uint8 in_g, uint8 in_b, uint8 in_a) { color.set(((float)in_r)/ 255.0f, ((float)in_g)/ 255.0f, ((float)in_b)/ 255.0f, ((float)in_a)/ 255.0f); }

  inline void setCoords (float in_x0, float in_y0, float in_xe, float in_ye) { pos.set(in_x0, in_xe, in_y0, in_ye); }
  inline void setCoordsD(float in_x0, float in_y0, float in_dx, float in_dy) { pos.setD(in_x0, in_y0, in_dx, in_dy); }
  inline void setCoordsDi(int in_x0, int in_y0, int in_dx, int in_dy) { pos.setD((float)in_x0, (float)in_y0, (float)in_dx, (float)in_dy); }
  inline void setTexture(ixTexture *in_t) { texture= in_t; }
  inline void setTexCoords(float x0, float y0, float xe, float ye) { texCoords.set(x0, xe, y0, ye); }
  inline void disableTexture() { texture= null; }
  inline void setHollow(float in_f) { hollow= in_f; }
  
  //void render();// THIS MUST USE THE ix.gl.useProgram() - ANOTHER INLINE THAT SHOULD JUST CHECK IF CHANGING PROGRAMS IS NEEDED WITH A SIMPLE IF

  // imediate funcs - they set and transmit to the program the changes, renderImediate will not set any uniforms, will only render
  // ALL THESE MUST BE INLINE TOO
  // THE PURPOSE OF imediate IS TO RENDER MULTIPLE (LOTS) OF QUADS WITHOUT SENDING MANY UNIFORMS, ONLY THE BARE MINIMUMS
  */


  inline void useProgram()                                        { glUseProgram(_sl->id); }
  inline void setCamera(ixCamera *c)                              { glUniformMatrix4fv(u.camera, 1, GL_FALSE, c->cameraMat); }
  inline void setColor(float r, float g, float b, float a)        { glUniform4f(u.color, r, g, b, a); }
  inline void setColorv(float *f)                                 { glUniform4fv(u.color, 1, f); }
  inline void setColorui(uint8 r, uint8 g, uint8 b, uint8 a)      { glUniform4f(u.color, ((float)r)/ 255.0f, ((float)g)/ 255.0f, ((float)b)/ 255.0f, ((float)a)/ 255.0f); }
  inline void setCoords(float x0, float y0, float xe, float ye, float z= 0.0f)   { glUniform3f(u.pos, x0, y0, z); glUniform2f(u.delta, xe- x0, ye- y0); }
  inline void setCoordsD(float x0, float y0, float dx, float dy, float z= 0.0f)  { glUniform3f(u.pos, x0, y0, z); glUniform2f(u.delta, dx, dy); }
  inline void setCoordsDi(int32 x0, int32 y0, int32 dx, int32 dy, float z= 0.0f) { glUniform3f(u.pos, (float)x0, (float)y0, z); glUniform2f(u.delta, (float)dx, (float)dy); }
  inline void setCoordsr(const rectf &r, float z= 0.0f)                          { glUniform3f(u.pos, r.x0, r.y0, z); glUniform2f(u.delta, r.dx, r.dy); }
  inline void setCoordsri(const recti &r, float z= 0.0f)                         { glUniform3f(u.pos, (float)r.x0, (float)r.y0, z); glUniform2f(u.delta, (float)r.dx, (float)r.dy); }
  inline void setTexture(ixTexture *t)                            { glBindTexture(t->glData.target, t->glData.id); glUniform1i(u.useTexture, 1); }
  inline void setTexCoords(float x0= 0.0f, float y0= 0.0f, float xe= 1.0f, float ye= 1.0f, float depth= 0.0f) { glUniform3f(u.tex0, x0, y0, depth); glUniform2f(u.texE, xe, ye); }
  inline void disableTexture()                                    { glUniform1i(u.useTexture, 0); }
  inline void setHollow(float w= -1.0f)                           { if(w< 0.0f) { glUniform1ui(u.hollow, 0); } else { glUniform1ui(u.hollow, 1); glUniform1f(u.hollowWidth, w); } }
  
  void setClipPlane(int32 x0, int32 y0, int32 xe, int32 ye);
  void setClipPlaneD(int32 x0, int32 y0, int32 dx, int32 dy);
  void setClipPlaneR(const recti &r);
  inline void delClipPlane() { glUniform1i(u.clip, 0); }

  inline void render() { glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); }

  // constructors / destructors

  gloQuad(): parent(null), glo(null) {}
  ~gloQuad() {}
  //void delData() { texCoords.set(0.0f, 1.0f, 0.0f, 1.0f); color.set(1.0f, 1.0f, 1.0f, 1.0f); pos.setD(0.0f, 0.0f, 1.0f, 1.0f); texture= null; }

private:

  void init();

  gloShader *_sl;

  struct Uniforms {
    int camera, color;
    int pos;            // quad start position
    int delta;          // quad dx and dy
    int tex0;           // texture coordonates start position
    int texE;           // texture coordonates end position (no need for the 3rd cooronate!!)
    int useTexture;     // bool - use texture or not
    int clip;           // bool - use clipping plane
    int clip0, clipE;   // clipping plane coords
    int hollow;         // bool - quad is hollow
    int hollowWidth;    // hollow line width

    Uniforms(): camera(-1), color(-1), pos(-1), delta(-1), tex0(-1), texE(-1), useTexture(-1), clip(-1), clip0(-1), clipE(-1), hollow(-1), hollowWidth(-1) {}
    void initUniforms(gloShader *);
  }u;

  glObject *glo;          // master
  gloDraw *parent;        // parent
  friend class gloDraw;
};





///====================================///
// DRAW main class ==========---------- //
///====================================///

class gloDraw {
public:

  osiGlRenderer *ren;
  glObject *parent;

  gloPoint point;
  gloLine line;
  gloTriangle triangle;
  gloQuad quad;
  gloCircle circle;

  void init();

  gloDraw(glObject *in_parent);
  ~gloDraw();
  void delData();

  
private:
  

  friend class gloTriangle;
  friend class gloQuad;
};




















