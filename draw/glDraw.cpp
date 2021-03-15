#include "ix/ix.h"

#ifdef IX_USE_OPENGL

/*

LINE:
https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
distance(p1, p2, (x0, y0))= |(y2- y1)x0- (x2- x1)y0 + x2y1- y2x1| / sqrt((y2- y1)^2 + (x2- x1)^2)
*/


gloDraw::gloDraw(glObject *in_parent) {
  parent= in_parent;
  point.parent= this,    point.glo= in_parent;
  line.parent= this,     line.glo= in_parent;
  triangle.parent= this, triangle.glo= in_parent;
  quad.parent= this,     quad.glo= in_parent;
  circle.parent= this,   circle.glo= in_parent;

}


gloDraw::~gloDraw() {

}


void gloDraw::delData() {

}


void gloDraw::init() {
  ren= parent->ren;
  
  point.init();
  line.init();
  triangle.init();
  quad.init();
  circle._init();
}


//void gloDraw::_setIxEngine(Ix *in_ix) {
//  point._ix= line._ix= triangle._ix= quad._ix= circle._ix= in_ix;
//}










// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ gloCIRCLE class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀




void gloCircle::_init() {
  glo->createShader(&_sl);
  _sl->load(Ix::Config::shaderDIR()+ "gloDraw/ixCircleV.glsl", Ix::Config::shaderDIR()+ "gloDraw/ixCircleF.glsl");
  if(_sl->build())
    u.initUniforms(_sl);
}



void gloCircle::Uniforms::initUniforms(gloShader *in_sl) {
  glUseProgram(in_sl->id);
  camera=     glGetUniformLocation(in_sl->id, "u_camera");
  color=      glGetUniformLocation(in_sl->id, "u_color");

  pos=         glGetUniformLocation(in_sl->id, "u_pos");
  delta=       glGetUniformLocation(in_sl->id, "u_delta");
  useTexture=  glGetUniformLocation(in_sl->id, "u_useTexture");
  tex0=        glGetUniformLocation(in_sl->id, "u_tex0");
  texE=        glGetUniformLocation(in_sl->id, "u_texE");
  
  clip=        glGetUniformLocation(in_sl->id, "u_clip");
  clip0=       glGetUniformLocation(in_sl->id, "u_clip0");
  clipE=       glGetUniformLocation(in_sl->id, "u_clipE");

  filled=      glGetUniformLocation(in_sl->id, "u_filled");
  centre=      glGetUniformLocation(in_sl->id, "u_centre");
  radius=      glGetUniformLocation(in_sl->id, "u_radius");
  thick=       glGetUniformLocation(in_sl->id, "u_thick");

  glUniform4f(color, 1.0f, 1.0f, 1.0f, 1.0f);
}




void gloCircle::setClipPlane(int32 x0, int32 y0, int32 xe, int32 ye) {
  /// fragment shader works with viewport coords
  //x0-= _ix->win->x0;
  //y0-= _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x0, (float)y0);
  glUniform2f(u.clipE, (float)xe, (float)ye);
}


void gloCircle::setClipPlaneD(int32 x0, int32 y0, int32 dx, int32 dy) {
  /// fragment shader works with viewport coords
  //x0-= _ix->win->x0;
  //y0-= _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x0, (float)y0);
  glUniform2f(u.clipE, (float)(x0+ dx), (float)(y0+ dy));
}


void gloCircle::setClipPlaneR(const recti &r) {
  /// fragment shader works with viewport coords
  int32 x= r.x0;//- _ix->win->x0;
  int32 y= r.y0;//- _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x, (float)y);
  glUniform2f(u.clipE, (float)(x+ r.dx), (float)(y+ r.dy));
}













///===========///
// IX TRIANGLE //
///===========///




void gloTriangle::init() {
  glo->createShader(&_sl);
  _sl->load(Ix::Config::shaderDIR()+ "gloDraw/ixTriangleV.glsl", Ix::Config::shaderDIR()+ "gloDraw/ixTriangleF.glsl");
  if(_sl->build()) {
    u.initUniforms(_sl);
  }
}


void gloTriangle::Uniforms::initUniforms(gloShader *in_sl) {
  glUseProgram(in_sl->id);
  camera=     glGetUniformLocation(in_sl->id, "u_camera");
  color=      glGetUniformLocation(in_sl->id, "u_color");
  vert=       glGetUniformLocation(in_sl->id, "u_vert");
  tex=        glGetUniformLocation(in_sl->id, "u_tex");
  useTexture= glGetUniformLocation(in_sl->id, "u_useTexture");
  clip=       glGetUniformLocation(in_sl->id, "u_clip");
  clip0=      glGetUniformLocation(in_sl->id, "u_clip0");
  clipE=      glGetUniformLocation(in_sl->id, "u_clipE");
}



void gloTriangle::setClipPlane(int32 x0, int32 y0, int32 xe, int32 ye) {
  /// fragment shader works with viewport coords
  //x0-= _ix->win->x0;
  //y0-= _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x0, (float)y0);
  glUniform2f(u.clipE, (float)xe, (float)ye);
}


void gloTriangle::setClipPlaneD(int32 x0, int32 y0, int32 dx, int32 dy) {
  /// fragment shader works with viewport coords
  //x0-= _ix->win->x0;
  //y0-= _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x0, (float)y0);
  glUniform2f(u.clipE, (float)(x0+ dx), (float)(y0+ dy));
}


void gloTriangle::setClipPlaneR(const recti &r) {
  /// fragment shader works with viewport coords
  int32 x= r.x0; //- _ix->win->x0;
  int32 y= r.y0; //- _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x, (float)y);
  glUniform2f(u.clipE, (float)(x+ r.dx), (float)(y+ r.dy));
}




/*
void gloTriangle::render() {
  if(!_sl) return;
  glUseProgram(_sl->id);

  glUniformMatrix4fv(_sl->u_camera, 1, GL_FALSE, _parent->_parent->camera->cameraMat);

  if(texture) {
    glEnable(texture->target);
    glBindTexture(texture->target, texture->id);
    glUniform1i(_sl->u_useTexture, 1);
    glUniform3fv(_sl->u_tex,    1, tex[0].v);
    glUniform3fv(_sl->u_tex+ 1, 1, tex[1].v);
    glUniform3fv(_sl->u_tex+ 2, 1, tex[2].v);
    
  } else {
    glDisable(GL_TEXTURE_2D);
    glUniform1i(_sl->u_useTexture, 0);
  }

  glUniformMatrix4fv(_sl->u_camera, 1, GL_FALSE,  _ix->camera->cameraMat);
  glUniform4fv(_sl->u_color, 1, color.v);
  
  glUniform3fv(_sl->u_vert,    1, vert[0].v);
  glUniform3fv(_sl->u_vert+ 1, 1, vert[1].v);
  glUniform3fv(_sl->u_vert+ 2, 1, vert[2].v);

  glDrawArrays(GL_TRIANGLES, 0, 3);   // draw the triangle

  /// return the gl to the default states
  if(!texture)
    glEnable(GL_TEXTURE_2D);
  else if(texture->target!= GL_TEXTURE_2D)
    glDisable(texture->target);
}
*/








///=======///
// IX QUAD //
///=======///


void gloQuad::init() {
  glo->createShader(&_sl);
  _sl->load(Ix::Config::shaderDIR()+ "gloDraw/ixQuadV.glsl", Ix::Config::shaderDIR()+ "gloDraw/ixQuadF.glsl");
  if(_sl->build()) {
    glUseProgram(*_sl);
    u.initUniforms(_sl);
  }
}


void gloQuad::Uniforms::initUniforms(gloShader *in_sl) {
  glUseProgram(*in_sl);
  camera=     glGetUniformLocation(*in_sl, "u_camera");
  color=      glGetUniformLocation(*in_sl, "u_color");

  pos=         glGetUniformLocation(*in_sl, "u_pos");
  delta=       glGetUniformLocation(*in_sl, "u_delta");
  tex0=        glGetUniformLocation(*in_sl, "u_tex0");
  texE=        glGetUniformLocation(*in_sl, "u_texE");
  useTexture=  glGetUniformLocation(*in_sl, "u_useTexture");
  clip=        glGetUniformLocation(*in_sl, "u_clip");
  clip0=       glGetUniformLocation(*in_sl, "u_clip0");
  clipE=       glGetUniformLocation(*in_sl, "u_clipE");
  hollow=      glGetUniformLocation(*in_sl, "u_hollow");
  hollowWidth= glGetUniformLocation(*in_sl, "u_hollowWidth");
}

/*
void gloQuad::setColor(float in_r, float in_g, float in_b, float in_a) {
  glUniform4f(_sl->u_useTexture, in_r, in_g, in_b, in_a);
}

void gloQuad::setColorui(uint8 in_r, uint8 in_g, uint8 in_b, uint8 in_a) {
  glUniform4f(_sl->u_useTexture, ((float)in_r)/ 255.0f, ((float)in_g)/ 255.0f, ((float)in_b)/ 255.0f, ((float)in_a)/ 255.0f);
}

void gloQuad::setCoords(float in_x0, float in_y0, float in_xe, float in_ye) {
  glUniform2f(_sl->u_sPos, in_x0, in_y0);
  glUniform2f(_sl->u_delta, in_xe- in_x0, in_ye- in_y0);
}

void gloQuad::setCoordsD(float in_x0, float in_y0, float in_dx, float in_dy) {
  glUniform2f(_sl->u_sPos, in_x0, in_y0);
  glUniform2f(_sl->u_delta, in_dx, in_dy);
}
*/


void gloQuad::setClipPlane(int32 x0, int32 y0, int32 xe, int32 ye) {
  /// fragment shader works with viewport coords
  //x0-= _ix->win->x0;
  //y0-= _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x0, (float)y0);
  glUniform2f(u.clipE, (float)xe, (float)ye);
}


void gloQuad::setClipPlaneD(int32 x0, int32 y0, int32 dx, int32 dy) {
  /// fragment shader works with viewport coords
  //x0-= _ix->win->x0;
  //y0-= _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x0, (float)y0);
  glUniform2f(u.clipE, (float)(x0+ dx), (float)(y0+ dy));
}


void gloQuad::setClipPlaneR(const recti &r) {
  /// fragment shader works with viewport coords
  int32 x= r.x0;//- _ix->win->x0;
  int32 y= r.y0;//- _ix->win->y0;

  glUniform1i(u.clip, 1);
  glUniform2f(u.clip0, (float)x, (float)y);
  glUniform2f(u.clipE, (float)(x+ r.dx), (float)(y+ r.dy));
}




/*
void gloQuad::render() {
  if(!_sl) return;
  glUseProgram(_sl->id);
  bool texEnabled= (bool)glIsEnabled((texture? texture->target: GL_TEXTURE_2D));

  glUniformMatrix4fv(_sl->u_camera, 1, GL_FALSE, _parent->_parent->camera->cameraMat);

  if(texture) {
    glEnable(texture->target);
    glBindTexture(texture->target, texture->id);
    glUniform1i(_sl->u_useTexture, 1);
    glUniform3f(_sl->u_sTex, texCoords.x0, texCoords.y0, 0.0f);
    glUniform2f(_sl->u_eTex, texCoords.xe, texCoords.ye);
  } else {
    glDisable(GL_TEXTURE_2D);
    glUniform1i(_sl->u_useTexture, 0);
  }

  glUniformMatrix4fv(_sl->u_camera, 1, GL_FALSE,  _ix->camera->cameraMat);
  glUniform4fv(_sl->u_color, 1, color.v);
  
  glUniform4f(_sl->u_sPos, pos.x0, pos.y0, 0.0f, 1.0f);
  glUniform2f(_sl->u_delta, pos.dx, pos.dy);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);   // draw the quad

  /// return the gl to the default states
  if(!texEnabled)
    glDisable(texture? texture->target: GL_TEXTURE_2D);
  else
    glEnable(texture? texture->target: GL_TEXTURE_2D);

}
*/











#endif /// IX_USE_OPENGL






