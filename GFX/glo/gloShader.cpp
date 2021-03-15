#include "ix/ix.h"

#ifdef IX_USE_OPENGL

#include "osi/include/util/fileOp.h"


// online shader compiler & preview http://shdr.bkcore.com




// gloShader class =====-----
///=========================
///=========================



gloShader::gloShader(): id(0) {

}


gloShader::~gloShader() {
  delData();
}


void gloShader::delData() {
  if(_glr)
    if(osi.glr== _glr)
      if(glIsProgram(id))
        glDeleteProgram(id);
  
  id= 0;
  //u_camera= u_color= u_viewportPos= -1;
}

/*
void gloShader::initUniforms() {
  glUseProgram(id);
  
}
*/





bool gloShader::build() {
  bool ret= false;
  bool chatty= true;

  str8 s;
  GLint result= GL_FALSE;
  int infoLogLength;
  GLuint vertID= ~0, fragID= ~0;

  id= ~0;

  // vertex module
  if(vertFile.nrUnicodes) {
    vertID= glCreateShader(GL_VERTEX_SHADER);
    
    /// read the Vertex Shader code from the file
    if(!secureRead8(vertFile, &s)) goto Exit;
  
    // Compile Vertex Shader
    glShaderSource(vertID, 1, (GLchar * const *)&s.d, NULL);
    glCompileShader(vertID);
  
    /// Check Vertex Shader
    glGetShaderiv(vertID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength) {
      s.delData();
      s.d= new char[infoLogLength];
      glGetShaderInfoLog(vertID, infoLogLength, NULL, (GLchar *)s.d);
      s.updateLen();
      if(s.nrUnicodes) {
        error.console(str8().f("Shader[%s] ERROR:", vertFile.d));
        error.console(s);
      }
    }

    if(!result) goto Exit;
  }

  // fragment module
  if(fragFile.nrUnicodes) {
    fragID= glCreateShader(GL_FRAGMENT_SHADER);

    /// read the Fragment Shader code from the file
    if(!secureRead8(fragFile, &s)) goto Exit;
 
    // compile Fragment Shader
    glShaderSource(fragID, 1, (GLchar * const *)&s.d, NULL);
    glCompileShader(fragID);
 
    /// check Fragment Shader
    glGetShaderiv(fragID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength) {
      s.delData();
      s.d= new char[infoLogLength];
      glGetShaderInfoLog(fragID, infoLogLength, NULL, (GLchar *)s.d);
      s.updateLen();
      if(s.nrUnicodes) { 
        error.console(str8().f("Shader[%s] ERROR:", fragFile.d));
        error.console(s);
      }
    }
    if(!result) goto Exit;
  }


  // Link the program
  id= glCreateProgram();
  if(vertID!= ~0) glAttachShader(id, vertID);
  if(fragID!= ~0) glAttachShader(id, fragID);
  // <<<< ADD MODULE TYPES HERE
  glLinkProgram(id);
 
  /// check the program
  glGetProgramiv(id, GL_LINK_STATUS, &result);
  glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
  if(infoLogLength) {
    s.delData();
    s.d= new char[infoLogLength];
    glGetProgramInfoLog(id, infoLogLength, NULL, (GLchar *)s.d);
    s.updateLen();
    if(s.nrUnicodes) { 
      error.console("Linking ERROR:");
      error.console(s);
    }
  }
  if(!result) goto Exit;

  // success
  if(chatty) error.console(str8().f("Shader OK [%s][%s]", vertFile.d, fragFile.d));
  ret= true;

Exit:
  if(vertID!= ~0) glDeleteShader(vertID);
  if(fragID!= ~0) glDeleteShader(fragID);
  if(!ret) if(id!= ~0) glDeleteProgram(id);

  return ret;
}



























/* OLD LOAD FUNCS ------------------------

// either compiles the shader or if it already exists, just points out_shader to it
void gloShader::loadShader(gloShader **out_shader, cchar *in_vertex, cchar *in_fragment) {
  /// search thru already existing shaders
  *out_shader= ixShaders.getShader(in_vertex, in_fragment);

  /// create the shader if not already existing
  if(!*out_shader) {
    *out_shader= new gloShader;
    (*out_shader)->id= ixShaders.compileShader(in_vertex, in_fragment);
    if(!((*out_shader)->id)) {
      delete *out_shader;
      *out_shader= null;
      return;
    }
    (*out_shader)->vertexFile= in_vertex;
    (*out_shader)->fragmentFile= in_fragment;
  }
}
*/

/*
// either compiles the shader or if it already exists, just points out_shader to it
template <class ixShaderSys::TixShader>
void ixShaderSys::loadShader(ixShaderSys::TixShader **out_shader, cchar *in_vertex, cchar *in_fragment) {
  /// search thru already existing shaders
  *out_shader= (ixShaderSys::TixShader *)ixShaders.getShader(in_vertex, in_fragment);

  /// create the shader if not already existing
  if(!*out_shader) {
    *out_shader= new ixShaderSys::TixShader;
    (*out_shader)->id= ixShaders.compileShader(in_vertex, in_fragment);
    if(!((*out_shader)->id)) {
      delete *out_shader;
      *out_shader= null;
      return;
    }
    (*out_shader)->vertexFile= in_vertex;
    (*out_shader)->fragmentFile= in_fragment;
    shaders.add(*out_shader);
  }
}
*/

/*
template <class TixShader>
TixShader *ixShaderSys::loadShader(size_t dummy, cchar *in_vertex, cchar *in_fragment) {
  /// search thru already existing shaders
  TixShader *out_shader= (TixShader*)getShader(in_vertex, in_fragment);

  /// create the shader if not already existing
  if(!out_shader) {
    out_shader= new TixShader;
    out_shader->id= compileShader(in_vertex, in_fragment);
    if(!out_shader->id) {
      delete out_shader;
      out_shader= null;
      return null;
    }
    out_shader->vertexFile= in_vertex;
    out_shader->fragmentFile= in_fragment;
  }
  return out_shader;
}
*/

/*
gloShader *ixShaderSys::loadShader2(size_t in_sizeofDerived, cchar *in_vertex, cchar *in_fragment) {
  /// search thru already existing shaders
  gloShader *out_shader= null;
  out_shader= getShader(in_vertex, in_fragment);

  /// create the shader if not already existing
  if(!out_shader) {
    out_shader= (gloShader *)new int8[in_sizeofDerived+8];
    //for(int a= in_sizeofDerived- 1; a>= 0; a--)
      // *((int8 *)out_shader+ a)= 0;
    Str::memclr(out_shader, in_sizeofDerived);    // clean the class, constructors cannot be called with this alloc method

    out_shader->id= compileShader(in_vertex, in_fragment);
    if(!(out_shader->id)) {
      out_shader->~gloShader();
      delete[] (int8 *)out_shader;
      return null;
    }
    out_shader->vertexFile= in_vertex;
    out_shader->fragmentFile= in_fragment;
    out_shader->_ix= _parent;
    shaders.add(out_shader);
  }
  return out_shader;
}
*/






#endif /// IX_USE_OPENGL



