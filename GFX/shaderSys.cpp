#include "ix/ix.h"


// online shader compiler & preview http://shdr.bkcore.com



/*
* -SHADERS could be able to be saved and loaded, with an asembly like structure in the file
*  ATM, materials will have a certain number of shaders, known
*  dynamic shaders that can be loaded from a file, is in the future, IF even
* 
* 
* 
* 
*/





//   ####    ##    ##    ####    ######    ########  ######
// ##        ##    ##  ##    ##  ##    ##  ##        ##    ##
//   ####    ########  ########  ##    ##  ######    ######
//       ##  ##    ##  ##    ##  ##    ##  ##        ##    ##
//  #####    ##    ##  ##    ##  ######    ########  ##    ##

///==========================================================///


ixShader::ixShader(Ix *in_ix, uint32 in_matID): ixClass(ixClassT::SHADER), _ix(in_ix), matID(in_matID) {

  // VULKAN shader
  #ifdef IX_USE_VULKAN
  vk= null;
  if(_ix->renVulkan())
    vk= _ix->vk.objects.addShader();
  #endif

  // OPENGL shader
  #ifdef IX_USE_OPENGL
  gl= null;
  if(_ix->renOpenGL())
    _ix->glo.createShader(&this->gl);
  #endif

  if(in_matID) {
    _ix->res.shader.shadersMaterialList.add(this);
  } else
    _ix->res.shader.shadersGenericList.add(this);

  // THE MATERIALLIST IS SHADY STUFF, IT GOES DOWN THE RABBIT HOLE TOO FAR
  // I THINK IT'S GENERALIZING STUFF TOO MUCH
  // I THINK ONLY ONE LIST WILL REMAIN IN THE END

}


ixShader::~ixShader() {
  chainData::~chainData();
  #ifdef IX_USE_VULKAN
  if(vk) { _ix->vk.objects.delShader(vk); vk= null; }
  #endif
  #ifdef IX_USE_OPENGL
  if(gl) { _ix->glMakeCurrent(); _ix->glo.delShader(gl); gl= null; }
  #endif
}




// SHADER

//   ####    ##    ##    ####    ########  ########  ##    ##
// ##         ##  ##   ##           ##     ##        ###  ###
//   ####      ####      ####       ##     ######    ## ## ##
//       ##     ##           ##     ##     ##        ##    ##
//  #####       ##      #####       ##     ########  ##    ##

///==========================================================///


///=======================================///
// -----===== ixShaderSys class =====----- //
///=======================================///


ixShaderSys::ixShaderSys(Ix *in_ix): _ix(in_ix) {
}


ixShaderSys::~ixShaderSys() {
  delData();
}


void ixShaderSys::delData() {
  while(shadersGenericList.first)
    shadersGenericList.del(shadersGenericList.last);
  while(shadersMaterialList.first)
    shadersMaterialList.del(shadersMaterialList.last);
}


void ixShaderSys::delShader(ixShader *out_s) {
  if(out_s->matID== 0)
    shadersGenericList.del(out_s);
  else
    shadersMaterialList.del(out_s);
}













