#pragma once


class Ix;
class ixShader;
#ifdef IX_USE_OPENGL
class gloShader;
#endif

class ixShaderSys {
  Ix *_ix;
public:

  void delShader(ixShader *out_shader);
  
  chainList shadersGenericList;     // shaders that are not part of the material library
  chainList shadersMaterialList;    // shaders in the material library      <<< WILL PROBLY GO, BUT ATM IT'S FINE

  // constructor / destructor

  ixShaderSys(Ix *parent);
  ~ixShaderSys();
  void delData();

private:
  friend class Ix;
  friend class ixglPrintShader;
};






class ixShader: public chainData {
public:
  Ix *_ix;

  uint32 matID;                                   // shaders used for mats have unique ID's; 0 is special, and MUST NOT be used for materials

  #ifdef IX_USE_OPENGL
  gloShader *gl;
  void glScissor();
  #endif

  #ifdef IX_USE_VULKAN
  VkoShader *vk;
  #endif


  // constructor / destructor

  ixShader(Ix *in_ix, uint32 in_materialID= 0);      // leaving matID 0 will signal it's not used for materials; MUST NOT be used for mats in that case
  virtual ~ixShader();
private:

  friend class ixShaderSys;
};









