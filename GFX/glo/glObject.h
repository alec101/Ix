#pragma once


class glObject {
public:

  osiGlRenderer *ren;
  gloDraw draw;
  chainList shaders;

  //template <class T>
  inline void createShader(gloShader **out_shader) { *out_shader= new gloShader; shaders.add(*out_shader); }
  inline void delShader(gloShader *in_s) { shaders.del(in_s); }
  inline void delAllShaders() { while(shaders.first) delShader(shaders.first); }

  gloShader *getShader(cchar *vertFile, cchar *fragFile);

  void init(osiGlRenderer *in_r);


  /*
class Gl, part of ix
  ix.gl.enableTexture(target)
  ix.gl.disableTexture(target)
  the class has internal flags so glEnable(GL_TARGET) is not called if not needed
  so it will be easy to call ix.gl.enable(GL_TARGET)

  and a ix.gl.defaultState() = all flags are checked, and set to the default
  this could be called after a big drawing func, that changes many things
  not to be used that frequently cuz it is kinda CPU-expensive if many many states are checked

  maybe something clever can be done with only enable(enabledthing), but it must be like with an enum with ogl

  */
  void func();
  /*
  inline void enableTexture1D() { if(!_tex1D) glEnable(GL_TEXURE_1D); }
  inline void enableTexture2D() { if(!_tex2D) glEnable(GL_TEXURE_2D); }
  inline void enableTexture3D() { if(!_tex3D) glEnable(GL_TEXURE_3D); }
  inline void disableTexture1D() { if(_tex1D) glDisable(GL_TEXURE_1D); }
  inline void disableTexture2D() { if(_tex2D) glDisable(GL_TEXURE_2D); }
  inline void disableTexture3D() { if(_tex3D) glDisable(GL_TEXURE_3D); }
  inline void useProgram(int id) { if(id!= _currentProgram) { glUseProgram(id); _currentProgram= id; } }
  */


  glObject();
  ~glObject();
  void delData();

private:
  bool _tex1D, _tex2D, _tex3D, _texCubic, _texMuchoGrande;
  int _currentProgram;
};












