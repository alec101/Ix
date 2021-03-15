#pragma once

class gloShader: public chainData {
public:
  uint32 id;
  operator GLuint() { return id; }

  //int u_camera, u_color;          // standard uniforms that should be present in all shaders
  //int u_viewportPos;              // viewport position on virtual desktop, used in frag program, usually
  str8 vertFile, fragFile;        // program file names - manly used to find duplicate shaders / no allocs for same shader program

  // cfg funcs


  inline void load(cchar *in_vert, cchar *in_frag) { vertFile= in_vert, fragFile= in_frag; }
  inline void loadVert(cchar *in_vert) { vertFile= in_vert; }
  inline void loadFrag(cchar *in_frag) { fragFile= in_frag; }

  // when program is configured, use build to compile it and make it use ready

  bool build();


  //virtual void initUniforms();    // inits uniforms, does not set the current program (base func inits the base uniforms)
  //void updateViewportPos();

  inline void useProgram() { glUseProgram(id); }

  // constructor/destructor

  gloShader(); 
  virtual ~gloShader();
  virtual void delData();

private:
  friend class ixShaderSys;
  friend class Ix;
};















