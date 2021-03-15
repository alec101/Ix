#include "ix/ix.h"

#ifdef IX_USE_OPENGL

glObject::glObject(): draw(this) {
  ren= null;
}


glObject::~glObject() {

}


void glObject::delData() {

}


void glObject::init(osiGlRenderer *in_r) {
  ren= in_r;
  draw.init();
}




// returns the shader class that has these source files; if no shader is already loaded and compiled, returns null
gloShader *glObject::getShader(cchar *vertFile, cchar *fragFile) {
  for(gloShader *p= (gloShader *)shaders.first; p; p= (gloShader *)p->next)
    if((p->vertFile== vertFile) && (p->fragFile== fragFile))
      return p;
  return null;
}



















/*
// on a window move / resize, all shaders should update viewportPos uniform
void ixShaderSys::updateAllViewportPos() {
  for(gloShader *p= (gloShader *)shaders.first; p; p= (gloShader *)p->next) {
    glUseProgram(p->id);
    p->updateViewportPos();
  }
}
*/




/* FROM THE TIME I ALLOCATED WITH int8's
bool ixShaderSys::delShader(uint32 id) {
  for(gloShader *p= (gloShader *)shaders.first; p; p= (gloShader *)p->next)
    if(p->id== id) {
      shaders.release(p);
      p->~gloShader();
      delete[] (int8 *)p;
      return true;
    }

  return false;
}


void ixShaderSys::delShader(gloShader *p) {
  shaders.release(p);
  p->~gloShader();
  delete[] (int8 *)p;
}
*/




#endif /// IX_USE_OPENGL
















