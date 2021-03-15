// exotic exts used:
// - GL_EXT_texture_filter_anisotropic

#include "ix/ix.h"

#ifndef max
#define max(a, b) ((a)> (b)? (a): (b))
#endif

/* TODO:
 - when program exit happens, there might be no renderer selected, or the window gets destroyed before a proper close of the renderer
   this means that destroying textures is not possible. there has to be some kind of check... when closing the winConsole, program crashes
  - a func that loads a base Img class with all different file formats

  THINK ABOUT:
  glTexture / vkTexture? for fastest
and texture would check wich one to call...
it might be the fastest way
vk in front of all funcs or gl...
and the main one would have slightest overblabla



*/



/*
HIGH RESOLUTION / LOW RESOLUTION
 - there has to be 2 levels for each texture... low and high res
 - switching between the two, happens depending on the distance to the camera.
 - low res should be plenty, and fast to load from disk.
 - as you move the camera closer, the high res textures must load, if not already loaded
 - texFile must load extreem fast. like zomg. the header probly need to change, to something fast to read, 2-3-4 bytes long max, not that whole text
 - it would be possible to have even a 3rd level of detail, but it would start to complicate things... the highest def image tho, can incorporate so much



*/








/*
TEXTURES
-mkay, the texture will have the seglist type of memory
  there will be memory allocation for x 256 textures, for y 512 textures, for z 128 textures, etc.
  the size of the texture will be key
  
  there should be mechanisms to enlarge or reduce this number, but ofc such mechanisms must not be called often



  THEORY CRAFTING:
-a texture is used, then is put to the front of a chain list. When you need memory, you release the mem of the last texture in that list
 if all are used, well you ran out of memory....
 but this, in theory would be the way to know what texture to unload when needed

 ofc, this is more problematic to unload the highest level of that texture...

 the issue would be... you have many textures, but maybe you need the top level of only so many
 

 vvvvvvvvv
 WHAT IF, HIGHER RES TEXTURES, COULD OCCUPY MORE SLOTS? LIKE A NORMAL SLOTx4 ???
 you'd need to alocate a continous memory for that texture
 ^^^^^^^^^^ in your dreams... vulkan knows nothing





 {
UNLOAD/DOWNLOAD, EVERYTHING SHOULD BE IN THE SYSTEM. THE TEXTURE MUST CALL THE PARENT WITH INLINES
this is getting better
texSys must handle all build operations, due things won't be that easy to just load/unload/blabla, directly to the texture
maybe it will be only 1 build func, but there will be alot of automations to handle the texture streaming and such
so i dono but things will be way bigger
so texSys.cpp will handle all textures
texture.cpp, will be more of a container, not that big, and most ixTexture funcs should be inlines to the texSys
}

*/

/*
MESHES

  vertex buffers have no patterned length
  i am guessing... chunks of memory get allocated

  static vertex buffers should be known to be static, and kinda there, all the time, and destroyed when needed

  but there should be dynamic vertex buffers that come and go too...
  this sounds very familliar with what open gl had, probably... with the hints of what the vertex buffer will be used for...
  i am rebuilding opengl here


  there is a way...
  i make 3 types of clusters, each different size, maybe more for very small meshes

  each big object would be comprised of many clusters
  the challenge would be to properly cut it into clusters
  that would happen with the obj/3ds/max to my mesh file
  it can be a triangle fan/strip/blabla
  so for each type of mesh composition, the mesh object must know to cut it right

  ok all this could be awesome

  the problem i foresee... memory segment 1 is 10% filled

                           memory segment 2 is 10% filled too, and that darn mesh is heavily used
                          
  so...
   there has to be some type of defragmentation...
  right?
  but the defragmentation knows how big to have a 3rd "glass" to be filled for the copy
  well... why a 3rd glass... direct copy... into the empty segment 1


  i am guessing the main issues would be...
  -some garbage , can be reduced with many segment sizes
  -the big question... how slow would it be to draw from a ton of places, everywhere... randomly...
   would having a nice organized stuff be drawned faster?!
  but such organisation can be created by the user, in a way.........



  this would be an awesome engine
  but... am i gonna use something right now with it? i mean, an alien game would have set meshes and textures... not much...
  */






//class _ixTexture {
//  static bool glBuildTexture(Img *in_img, ixTexture *in_texture, bool in_mipmap, bool in_compress);
//  static void glPopulateTexture(Tex *in_src, ixTexture *out_dst, int in_magFilter, int in_minFilter, int8 in_anisotropy);
//};









/*

// SHARING funcs

void ixTexture::_loadAllShares(cchar *in_fn) {
  _ixResShare *p= (_ixResShare *)_share->next;
  while(p!= _share) {
    p->res->load(in_fn);
    p= (_ixResShare *)p->next;
  }
}


void ixTexture::_delAllSharesButThis() {
  _ixResShare *p= (_ixResShare *)_share->next;
  while(p!= _share) {
    p= (_ixResShare *)p->next;
    ((_ixResShare *)p->prev)->res->_share= null;
    p->delPrev();
  }
}


bool ixTexture::_isLastShare() {
  return (_share->next== _share);
}

// breaks this texture from the share list; if there's only 2 nodes in the list, it breaks the other share too, so _share= null
void ixTexture::_breakShare() {
  if(_share->next->next== _share->next) {
    ((_ixResShare *)_share->next)->res->_share= null;
    _share->next->del();
  }
  _share->del();
  _share= null;
}

// breaks / releases all the share list, the textures will not be linked in any way
void ixTexture::_breakAllShares() {
  while(_share->next!= _share) {
    ((_ixResShare *)_share->next)->res->_share= null;
    _share->next->del();
  }
  _share->del();
}

*/












///--------======================--------///
// ======== TEXTURE SYSTEM class ======== //
///--------======================--------///

ixTexSys::ixTexSys(Ix *in_ix): _ix(in_ix), add(this) {

  // TEXTURES DEFAULT settings
  cfg.magFilter=   ixFilter::LINEAR;
  cfg.minFilter=   ixFilter::LINEAR;
  cfg.mipmapFilter= ixMipmapFilter::LINEAR;
  cfg.anisotropy=  8;
  cfg.compressRule= 0;        // up to the texture (file) if it is compressed or not

  #ifdef IX_USE_VULKAN
  //vkd.parent= this;
  vkd._ix= this->_ix;

  /// static textures 
  vkd.staticPool= null;
  vkd.staticLayout= null;
  
  vkd.standard4mapsLayout= null;
  #endif // IX_USE_VULKAN

  #ifdef IX_USE_OPENGL
  glData.parent= this;
  #endif // IX_USE_OPENGL
}


void ixTexSys::delData() {
  while(data.first)
    data.del(data.first);

  while(streams.last)
    streams.del(streams.last);
  
}






bool ixTexSys::load(ixTexture *out_t, cchar *in_fname) {
  if(!loadMem(out_t, in_fname)) return false;
  return upload(out_t);
}


bool ixTexSys::loadMem(ixTexture *out_t, cchar *in_fname) {
  const char *err= null;
  int errL;

  if(out_t->data) delete out_t->data;
  //out_t->err= 0;

  out_t->data= new Tex; if(!out_t->data) IXERR("memory alloc failed.");

  if(!out_t->data->load(in_fname))
    goto Exit;
  
  /// populate whatever is left to fill in
  out_t->fileName= out_t->data->fileName;
  out_t->fileType= out_t->data->fileType;
  out_t->type= out_t->data->type;

Exit:
  if(err) {
    error.detail(str8("[")+ out_t->fileName+ "] " + err, __FUNCTION__, errL);

    if(out_t->data) delete out_t->data;
    out_t->data= null;
    return false;
  }

  return true; // success
}


bool ixTexSys::reload(ixTexture *inout_t) {
  unload(inout_t);
  return load(inout_t, inout_t->fileName);
}


bool ixTexSys::upload(ixTexture *out_t) {
  bool ret= false;
  #ifdef IX_USE_VULKAN
  if(_ix->renVulkan()) ret= vkd.upload(out_t);
  #endif
  #ifdef IX_USE_OPENGL
  if(_ix->renOpenGL()) ret= glData.upload(out_t);
  #endif

  // success
  if(ret) {
    if(!out_t->flags.isUp(0x02)) { delete out_t->data; out_t->data= null; }
    if(!out_t->flags.isUp(0x04)) if(out_t->data) { delete out_t->data->bitmap; out_t->data->bitmap= null; }

  // failed
  } else {
    error.detail(str8("[")+ out_t->fileName+ "] upload failed", __FUNCTION__);
  }

  return ret;
}


bool ixTexSys::download(ixTexture *out_t) {
  #ifdef IX_USE_VULKAN
  if(_ix->renVulkan()) return vkd.download(out_t);
  #endif
  #ifdef IX_USE_OPENGL
  if(_ix->renOpenGL()) return glData.download(out_t);
  #endif
  return false;
}


bool ixTexSys::unload(ixTexture *out_t) {
  #ifdef IX_USE_VULKAN
  if(_ix->renVulkan()) return vkd.unload(out_t);
  #endif
  #ifdef IX_USE_OPENGL
  if(_ix->renOpenGL()) { glData.unload(out_t); return true; }
  #endif
  return false;
}


/*
/// checks validity of all textures
bool ixTexSys::checkAllTextures() {
  return true;    // DISABLED<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  bool ret= true;                       /// start with true
  
  /// pass thru all textures
  ixTexture *p= (ixTexture *)data.first;
  while(p) {
    if(!p->check()) ret= false;    /// mark false if any tex is not valid
    p= (ixTexture *)p->next;
  }
  return ret;
}
*/


/*
/// reloads all textures that are not valid
bool ixTexSys::reloadAllInvalid() {
  bool ret= true;                       /// start with true

  /// pass thru all textures
  ixTexture *p= (ixTexture *)data.first;
  while(p) {
    if(!p->check())
      if(!reload(p)) ret= false;      /// mark false if 1 did not reload
    p= (ixTexture *)p->next;
  }

  return ret;
}
*/



// TEXTURE creation -------------------------------------------------
///==================================================================

ixTexture *ixTexSys::Add::ixStaticTexture() {
  ixTexture *t= new ixTexture(_ix);
  _ix->res.tex.data.add(t);

  t->affinity= 0;
  
  return t;
}


ixTexture *ixTexSys::Add::staticTexture() {
  ixTexture *t= new ixTexture(_ix);
  _ix->res.tex.data.add(t);

  t->affinity= 1;
  
  return t;
}


ixTexture *ixTexSys::Add::streamTexture(ixTexSys::Stream *in_stream) {
  ixTexture *t= new ixTexture(_ix);
  _ix->res.tex.data.add(t);

  t->affinity= 64;
  _ix->res.tex.assignSpecificStream(t, in_stream);
  
  return t;
}


ixTexture *ixTexSys::Add::unconfiguredTexture() {
  ixTexture *t= new ixTexture(_ix);
  _ix->res.tex.data.add(t);
  return t;
}





void ixTexSys::delTexture(ixTexture *t) {
  if(t) data.del(t);
  t= null;
}



ixTexture *ixTexSys::search(cchar *in_fname, uint32 in_id) {
  if((in_fname== null) && (in_id== 0)) { error.detail("both <in_fname> and <in_id> are null/0", __FUNCTION__); return null; }

  if(in_id) {
    for(ixTexture *p= (ixTexture *)data.first; p; p= (ixTexture *)p->next)
      if(p->texLibID== in_id) return p;
  } else {
    for(ixTexture *p= (ixTexture *)data.first; p; p= (ixTexture *)p->next)
      if(p->fileName== in_fname)
        return p;
  }
  return null;
}
































///=================================///
// OPENGL specific ----------------- //
///=================================///

#ifdef IX_USE_OPENGL
/*
//void ixTexture::OpenGL::populate(Tex *in_src, ixFilter magFilter, ixFilter minFilter, int8 anisotropy) {
void ixTexSys::OpenGL::populate(ixTexture *out_texture, Tex *in_src) {
  /// populate texture vals
  out_texture->fileName= in_src->fileName;
  out_texture->fileType= in_src->fileType;
  out_texture->type= in_src->type;
  //target= in_src->target;
  
  //out_texture->magFilter=  magFilter;
  //out_texture->minFilter=  minFilter;
  //out_texture->anisotropy= settings.anisotropy;

  /// no mipmapping for 1D textures
//  if(parent->type== Img::Type::T_1D) {
//    if(parent->minFilter== ixFilter::  GL_LINEAR_MIPMAP_LINEAR)  out_dst->minFilter= GL_LINEAR;
    //if(parent->minFilter== GL_LINEAR_MIPMAP_NEAREST) out_dst->minFilter= GL_NEAREST;
  //}

  /*
  /// set default swizzle for grayscale images
  if(parent->fileType!= 0) {
    if(in_src->type== ImgFormat:: IMG_GREY) {
      out_dst->data->swizzG= GL_RED;
      out_dst->data->swizzB= GL_RED;
    } else if(in_src->type== IMG_GREY_ALPHA) {
      out_dst->data->swizzG= GL_RED;
      out_dst->data->swizzB= GL_RED;
      out_dst->data->swizzA= GL_GREEN;
    }
  }*/
//}

// main build texture function - AVOID TO HAVE MULTIPLE FUNCS LIKE THIS- ANY CHANGE WILL NEED TO BE MADE IN MULTIPLE PLACES
bool ixTexSys::OpenGL::upload(ixTexture *out_t) {
  const char *err= nullptr;
  bool ret= false;
  Tex *t= out_t->data;

  uint32 sx= t->dx, sy= t->dy, sd= t->depth;
  GLenum target= Img::glGetGlType(out_t->type);
  GLenum inFormat= Img::glGetGlFormat(t->format);
  GLenum inData= Img::glGetDataType(t->format);
  vec4 brd;
  GLenum filter;
  GLenum outFormat= 0;
  bool compress= false;

  if(!t)         { error.detail("texture data class is null", __FUNCTION__, __LINE__); return false; } 
  if(!t->bitmap) { error.detail("texture bitmap is null", __FUNCTION__, __LINE__); return false; }
  if(!t->areSizesPowerOfTwo()) { err= "Texture sizes are not power of 2"; goto Exit; }
  if(out_t->glData.id) if(out_t->check()) { error.detail("there is already a texture uploaded in this class", __FUNCTION__, __LINE__); return false; }
  /// try to convert the image if it's not OGL compatible - 3D textures should be already compatible (wuss, convert your images!!!)
  if(!t->glIsCompatible()) {
    if(out_t->type== Img::Type::T_3D) { err= "Image Format is NOT compatible"; goto Exit; }
    if(!t->glConvertCompatible())  { err= "Image convert FAILED"; goto Exit; }
  }

  // cubemap research: 
  // https://www.opengl.org/sdk/docs/man3/xhtml/glTexImage2D.xml - the texImage2D func handles cube maps
  // GL_CUBE_MAP - > 6 sides - target= GL_TEXTURE_CUBE_MAP_POSITIVE_X GL_TEXTURE_CUBE_MAP_NEGATIVE_X etc
  //if(dst->type== GL_TEXTURE_CUBE_MAP) {
  //  6 glTexImage2D for each face
  //}

  

  out_t->glData.target= target;



  // determine the output format

       if(parent->cfg.compressRule== 0) compress= t->compressed;
  else if(parent->cfg.compressRule== 1) compress= false;
  else if(parent->cfg.compressRule== 2) compress= true;

  /// input compression differs from the output compression
  if(compress!= t->compressed) {
    /// use the Img function to determine the output format
    if(!compress)
      outFormat= Img::glGetGlFormat(Img::compressedToUncompressed(t->format));
    /// let OpenGL decide the best compression technique
    else {
      if(t->nchannels== 1) outFormat= GL_COMPRESSED_RED;
      else if(t->nchannels== 2) outFormat= GL_COMPRESSED_RG;
      else if(t->nchannels== 3) outFormat= GL_COMPRESSED_RGB;
      else if(t->nchannels== 4) outFormat= GL_COMPRESSED_RGBA;
    }
  /// same format
  } else
    outFormat= inFormat;

  error.glFlushErrors();
  glGenTextures(1, &out_t->glData.id);
  glBindTexture(target, out_t->glData.id);

  /// mipmap
  if(t->nrLevels> 1) {
    for(int a= 0; a< t->nrLevels; a++) {  /// for each mipmap level
      if(compress) {                      /// compressed
        if(target== GL_TEXTURE_2D) glCompressedTexImage2D(GL_TEXTURE_2D, a, inFormat, sx, sy,     0, t->levSize[a], (uint8 *)t->bitmap+ t->levFrom[a]);
        if(target== GL_TEXTURE_3D) glCompressedTexImage3D(GL_TEXTURE_3D, a, inFormat, sx, sy, sd, 0, t->levSize[a], (uint8 *)t->bitmap+ t->levFrom[a]);
      } else {                            /// not compressed
        if(target== GL_TEXTURE_2D) glTexImage2D(GL_TEXTURE_2D, a, outFormat, sx, sy,     0, inFormat, inData, (uchar*)t->bitmap+ t->levFrom[a]);
        if(target== GL_TEXTURE_3D) glTexImage3D(GL_TEXTURE_3D, a, outFormat, sx, sy, sd, 0, inFormat, inData, (uchar*)t->bitmap+ t->levFrom[a]);
      }

      //mipmap levels: sx>> 1 would divide by 2 i think

      sx= max(sx>> 1, 1); // sx= max(sx/ 2, 1);
      sy= max(sy>> 1, 1); // sy= max(sy/ 2, 1);
      sd= max(sd>> 1, 1); // sd= max(sd/ 2, 1);
    } /// for each mipmap level
    glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, t->nrLevels- 1);

  /// no mipmap
  } else {                          
    if(compress) {             /// compressed
      if(target== GL_TEXTURE_1D) glCompressedTexImage1D(GL_TEXTURE_1D, 0, inFormat, t->dx,                  0, t->levSize[0], t->bitmap);
      if(target== GL_TEXTURE_2D) glCompressedTexImage2D(GL_TEXTURE_2D, 0, inFormat, t->dx, t->dy,           0, t->levSize[0], t->bitmap);
      if(target== GL_TEXTURE_3D) glCompressedTexImage3D(GL_TEXTURE_3D, 0, inFormat, t->dx, t->dy, t->depth, 0, t->levSize[0], t->bitmap);
    } else {                        /// not compressed
      if(target== GL_TEXTURE_1D) glTexImage1D(GL_TEXTURE_1D, 0, outFormat, t->dx,                  0, inFormat, inData, t->bitmap);
      if(target== GL_TEXTURE_2D) glTexImage2D(GL_TEXTURE_2D, 0, outFormat, t->dx, t->dy,           0, inFormat, inData, t->bitmap);
      if(target== GL_TEXTURE_3D) glTexImage3D(GL_TEXTURE_3D, 0, outFormat, t->dx, t->dy, t->depth, 0, inFormat, inData, t->bitmap);
    }
    glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
  }

  /// set all texture parameters
  if(t->nrLevels> 1) {
    if(parent->cfg.magFilter== ixFilter::LINEAR) filter= GL_LINEAR;
    else if(parent->cfg.magFilter== ixFilter::NEAREST) filter= GL_NEAREST;
  } else {
    if(parent->cfg.magFilter== ixFilter::LINEAR && parent->cfg.mipmapFilter== ixMipmapFilter::LINEAR) filter= GL_LINEAR_MIPMAP_LINEAR;
    else if(parent->cfg.magFilter== ixFilter::LINEAR && parent->cfg.mipmapFilter== ixMipmapFilter::NEAREST) filter= GL_LINEAR_MIPMAP_NEAREST;
    else if(parent->cfg.magFilter== ixFilter::NEAREST && parent->cfg.mipmapFilter== ixMipmapFilter::LINEAR) filter= GL_NEAREST_MIPMAP_LINEAR;
    else if(parent->cfg.magFilter== ixFilter::NEAREST && parent->cfg.mipmapFilter== ixMipmapFilter::NEAREST) filter= GL_NEAREST_MIPMAP_NEAREST;
  }
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
  if(t->nrLevels> 1) {
    if(parent->cfg.minFilter== ixFilter::LINEAR) filter= GL_LINEAR;
    else if(parent->cfg.minFilter== ixFilter::NEAREST) filter= GL_NEAREST;
  } else {
    if(parent->cfg.minFilter== ixFilter::LINEAR && parent->cfg.mipmapFilter== ixMipmapFilter::LINEAR) filter= GL_LINEAR_MIPMAP_LINEAR;
    else if(parent->cfg.minFilter== ixFilter::LINEAR && parent->cfg.mipmapFilter== ixMipmapFilter::NEAREST) filter= GL_LINEAR_MIPMAP_NEAREST;
    else if(parent->cfg.minFilter== ixFilter::NEAREST && parent->cfg.mipmapFilter== ixMipmapFilter::LINEAR) filter= GL_NEAREST_MIPMAP_LINEAR;
    else if(parent->cfg.minFilter== ixFilter::NEAREST && parent->cfg.mipmapFilter== ixMipmapFilter::NEAREST) filter= GL_NEAREST_MIPMAP_NEAREST;
  }
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLint)parent->cfg.anisotropy);
  glTexParameteri(target, GL_TEXTURE_WRAP_S,        t->glGetGlWrap(t->wrapU));
  glTexParameteri(target, GL_TEXTURE_WRAP_T,        t->glGetGlWrap(t->wrapV));
  glTexParameteri(target, GL_TEXTURE_WRAP_R,        t->glGetGlWrap(t->wrapW));
  brd= 0.0f;
  if(t->border== ixBorderColor::FLOAT_OPAQUE_BLACK)  brd.a= 1.0f;
  else if(t->border== ixBorderColor::FLOAT_OPAQUE_WHITE) brd[0]= brd[1]= brd[2]= brd[3]= 1.0f;
  glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, brd);
  glTexParameteri(target, GL_TEXTURE_SWIZZLE_R,     t->glGetGlSwizzle(t->swizzR, GL_RED));
  glTexParameteri(target, GL_TEXTURE_SWIZZLE_G,     t->glGetGlSwizzle(t->swizzG, GL_GREEN));
  glTexParameteri(target, GL_TEXTURE_SWIZZLE_B,     t->glGetGlSwizzle(t->swizzB, GL_BLUE));
  glTexParameteri(target, GL_TEXTURE_SWIZZLE_A,     t->glGetGlSwizzle(t->swizzA, GL_ALPHA));
  //glTexParameteri(target, GL_TEXTURE_COMPARE_MODE,  data->compareMode);
  //glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC,  data->compareFunc);


  /// check for gl errors
  if(error.glError()) { err= "OpenGL error"; goto Exit; }

  ret= out_t->check();

Exit:
  
  if(!err) {

  // error handling
  } else {
    error.detail(err, __FUNCTION__);
  }

  return ret;
}


bool ixTexSys::OpenGL::download(ixTexture *out_t) {
  if(!out_t->data)
    out_t->data= new Tex;
  return out_t->data->glGetID(out_t->glData.id, out_t->glData.target);
}


void ixTexSys::OpenGL::unload(ixTexture *out_t) {
  if(out_t->glData.id)
    if(parent)
      if(parent->_ix)
        if(parent->_ix->renOpenGL())
          if(parent->_ix->glIsActive())
            glDeleteTextures(1, &out_t->glData.id);
  out_t->glData.id= 0;
  out_t->isValid= 0;
}



// anisotropy - https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt
// GL_TEXTURE_MAX_ANISOTROPY_EXT, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT

int ixTexSys::OpenGL::getMaxAnisotropy() {
  int ret;
  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ret);
  return ret;
}

int ixTexSys::OpenGL::getAnisotropy(int in_tgt) {
  int ret;
  glGetTexParameteriv(in_tgt, GL_TEXTURE_MAX_ANISOTROPY_EXT, &ret);
  return ret;
}

void ixTexSys::OpenGL::setAnisotropy(int in_tgt, int in_level) {
  glTexParameteri(in_tgt, GL_TEXTURE_MAX_ANISOTROPY_EXT, in_level);
}

#endif // IX_USE_OPENGL















///================------------------///
// VULKAN specific ================== //
///================------------------///



#ifdef IX_USE_VULKAN


void ixTexSys::Vulkan::init() {

  // static textures handling

  staticLayout= _ix->vk.objects.addDescriptorSetLayout();
  staticLayout->cfgAddDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
  staticLayout->build();

  staticPool= _ix->vk.objects.addDynamicSetPool();
  staticPool->configure(staticLayout, _ix->cfg.vk.staticTexturesDynamicSetSegmentSize);
  staticPool->build();

  // standard 4 slots material layout

  standard4mapsLayout= _ix->vk.objects.addDescriptorSetLayout();
  standard4mapsLayout->cfgAddDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
  standard4mapsLayout->cfgAddDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
  standard4mapsLayout->cfgAddDescriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
  standard4mapsLayout->cfgAddDescriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
  standard4mapsLayout->build();

  // noTexture creation

  _ix->vki.noTexture= _ix->res.tex.add.ixStaticTexture();
  Tex dat;

  ixFilter saveMinFilter= _ix->res.tex.cfg.minFilter;
  ixFilter saveMagFilter= _ix->res.tex.cfg.magFilter;

  _ix->res.tex.cfg.magFilter= ixFilter::NEAREST;
  _ix->res.tex.cfg.minFilter= ixFilter::NEAREST;

  dat.dx= 2;
  dat.dy= 2;
  dat.depth= 1;
  dat.nrLevels= 1;
  dat.type= Img::Type::T_2D;
  
  dat.format= ImgFormat::R8G8B8A8_UNORM;
  dat.computePixelInfo();
  
  dat.size= dat.dx* dat.dy* (dat.bpp/ 8);
  dat.levFrom[0]= 0;
  dat.levSize[0]= dat.size;
  
  dat.bitmap= new uint8[dat.size];

  // fill a checkerboard of pink+black
  struct _rgba { uint8 r, g, b, a; } *p;
  p= (_rgba *)dat.bitmap;
  p[0]= { 255, 127, 255, 255 };
  p[1]= {   3,  10,  20, 255 };
  p[2]= {   3,  10,  20, 255 };
  p[3]= { 255, 127, 255, 255 };
  
  _ix->vki.noTexture->data= &dat;
  _ix->vki.noTexture->flags.setUp(0x02);    // keep params
  _ix->vki.noTexture->flags.setUp(0x04);    // keep data

  _ix->res.tex.upload(_ix->vki.noTexture);
  _ix->vki.noTexture->data= null;
  _ix->vki.noTexture->flags.setDown(0x02);
  _ix->vki.noTexture->flags.setDown(0x04);

  /// restore configured filters
  _ix->res.tex.cfg.magFilter= saveMagFilter;
  _ix->res.tex.cfg.minFilter= saveMinFilter;
}














































//void ixTexSys::Vulkan::buildTexture(Tex *in_tex, ixTexture *out_texture) {
bool ixTexSys::Vulkan::upload(ixTexture *out_tex) {
  // A SET CAN BE ASSIGNED FROM HERE, IF THERE WILL BE A STANDARD TO MAKE 1 SET DEDICATED TO THE TEXTURES

  //bool ret= false;
  const char *err= null;
  int32 errL= -1;
  vkObject *in_vk= &_ix->vk;
  Tex *t= out_tex->data;
  //void *map;
  //ixvkBuffer *stage;
  //uint sx, sy, sd;

  /// safety checks
  if(out_tex== nullptr) IXERR("out_texture is null")
  if(t== nullptr) IXERR("texture data is null, load a texture first")
  if(!t->areSizesPowerOfTwo()) IXERR("texture sizes are not power of two")
    
  // CUBE, TEXTURE ARRAYS MUST HAPPEN <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    

  // create image / assign stream
  if(out_tex->affinity< 2) {
    /// check to destroy current vulkan image or not
    if(out_tex->vkd.img)
      if((out_tex->vkd.img->handle->createInfo.extent.height!= t->dy) ||
         (out_tex->vkd.img->handle->createInfo.extent.width!=  t->dx) ||
         (out_tex->vkd.img->handle->createInfo.extent.depth!=  t->depth) ||
         (out_tex->vkd.img->handle->createInfo.mipLevels!=     t->nrLevels) ||
         (out_tex->vkd.img->handle->createInfo.format!= (VkFormat)t->format)) {

        out_tex->vkd.img->cluster->delResource(out_tex->vkd.img);

        if(out_tex->vkd.imgView)
          _ix->vk.DestroyImageView(_ix->vk, out_tex->vkd.imgView, _ix->vk);

        out_tex->vkd.img= null;
        out_tex->vkd.imgView= null;
      }

    // create an ixvkImage
    if(out_tex->vkd.img== null) {
    
      if(out_tex->affinity== 0) 
        out_tex->vkd.img= new ixvkImage(_ix->vki.clusterIxDevice);
      else if(out_tex->affinity== 1)
        out_tex->vkd.img= new ixvkImage(_ix->vki.clusterDevice);

      //out_tex->vkd.img->currentQueue= _ix->vki.qTransfer;      /// initial queue will be the one that first uses the image

      // static image
      if(out_tex->affinity< 2) {
        // -If it is VK_IMAGE_LAYOUT_PREINITIALIZED, then the image data can be preinitialized by the host while using this layout,
        //  and the transition away from this layout will preserve that data.
        // -If it is VK_IMAGE_LAYOUT_UNDEFINED, then the contents of the data are considered to be undefined,
        //  and the transition away from this layout is not guaranteed to preserve that data
        out_tex->vkd.img->handle->cfgInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
        out_tex->vkd.img->handle->cfgUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        out_tex->vkd.img->cfgFormat((VkFormat)out_tex->data->format);
        out_tex->vkd.img->cfgSize(VkExtent3D()= { out_tex->data->dx, out_tex->data->dy, out_tex->data->depth}, 1, out_tex->data->nrLevels);

        out_tex->vkd.img->build();

      } else IXERR("Unknown Texture Affinity");


    } /// create Image

  } /// affinity 0 & 1
  else if(out_tex->affinity== 64) {                   // stream
    /// assign a stream if required
    if(out_tex->stream== null)
      _ix->res.tex.assignStream(out_tex);
    out_tex->vkd.img->access[out_tex->layer->index].layout= VK_IMAGE_LAYOUT_UNDEFINED;

    if(out_tex->stream== null) IXERR("Could not assign a stream to texture");
  }

  // upload to device
  if(out_tex->affinity< 2) {
    out_tex->vkd.img->upload(out_tex->data, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  } else if(out_tex->affinity== 64) {
    out_tex->vkd.img->upload(out_tex->data, out_tex->layer->index, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
  

  // image view
  /// image view will be destroyed at start, if vkImage needs to be created / uploaded tex differs from current
  if((out_tex->vkd.imgView== VK_NULL_HANDLE) && (out_tex->affinity< 2)) {
    /// Vulkan Image View
    VkImageViewCreateInfo viewInfo;
      viewInfo.sType= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewInfo.pNext= nullptr;      // usage is inherited but can be inserted here with a VkImageViewUsageCreateInfo
      viewInfo.flags= 0;

      viewInfo.image=    out_tex->vkd.img->handle->image;
      viewInfo.viewType= (VkImageViewType)t->type;
      viewInfo.format=   (VkFormat)t->format;

      viewInfo.components.r= (VkComponentSwizzle)t->swizzR;
      viewInfo.components.g= (VkComponentSwizzle)t->swizzG;
      viewInfo.components.b= (VkComponentSwizzle)t->swizzB;
      viewInfo.components.a= (VkComponentSwizzle)t->swizzA;

      viewInfo.subresourceRange.aspectMask=     Img::vkGetAspectFromFormat(t->format);
      viewInfo.subresourceRange.baseMipLevel=   0;
      viewInfo.subresourceRange.levelCount=     t->nrLevels;
      viewInfo.subresourceRange.baseArrayLayer= 0;
      viewInfo.subresourceRange.layerCount=     1;
    if(!error.vkCheck(_ix->vk.CreateImageView(_ix->vk, &viewInfo, _ix->vk, &out_tex->vkd.imgView))) IXERR("vkCreateImageView failed.");

  } else if(out_tex->affinity== 64) {
    out_tex->vkd.imgView= out_tex->segment->view;

  }

  // Vulkan sampler
  assignSampler(out_tex);


  /// create a set for the texture
  if(out_tex->vkd.flags.isUp(0x02) && out_tex->affinity!= 64) {  // create set flag
    /// static texture set
    if(out_tex->affinity<= 1) {

      if(out_tex->vkd.set== null)
        out_tex->vkd.set= staticPool->addSet();

      VkDescriptorImageInfo imgInfo;
        imgInfo.imageView=   out_tex->vkd.imgView;
        imgInfo.sampler=     *out_tex->vkd.sampler;
        imgInfo.imageLayout= out_tex->vkd.img->access[0].layout;

      VkWriteDescriptorSet update;
        update.sType= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        update.pNext= null;
        update.dstSet= out_tex->vkd.set->set;
        update.dstBinding= 0;
        update.dstArrayElement= 0;
        update.descriptorCount= 1;
        update.descriptorType= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        update.pImageInfo= &imgInfo;
        update.pBufferInfo= null;
        update.pTexelBufferView= null;
      _ix->vk.UpdateDescriptorSets(_ix->vk, 1, &update, 0, null);
    }

    // <<<<<<<<<<  other afinity sets <<<<<<<<<<<<<
  }


Exit:
  if(err) {
    out_tex->vkd.delData();

    if(in_vk->errorText)
      error.detail(in_vk->errorText, __FUNCTION__);
    error.detail(err, __FUNCTION__, errL);
    return false; // fail

  } else
    return true;  // success
}























































void ixTexSys::Vulkan::assignSampler(ixTexture *out_t) {
  
  // SAMPLER assignment/create

  /* SPECS NOTE:

    "magFilter values of VK_FILTER_NEAREST and VK_FILTER_LINEAR directly correspond to GL_NEAREST and GL_LINEAR magnification filters.
    minFilter and mipmapMode combine to correspond to the similarly named OpenGL minification filter of GL_minFilter_MIPMAP_mipmapMode
    (e.g. minFilter of VK_FILTER_LINEAR and mipmapMode of VK_SAMPLER_MIPMAP_MODE_NEAREST correspond to GL_LINEAR_MIPMAP_NEAREST).

    There are no Vulkan filter modes that directly correspond to OpenGL minification filters of GL_LINEAR or GL_NEAREST,
    but they can be emulated using VK_SAMPLER_MIPMAP_MODE_NEAREST, minLod = 0, and maxLod = 0.25,
    and using minFilter = VK_FILTER_LINEAR or minFilter = VK_FILTER_NEAREST, respectively.

    Note that using a maxLod of zero would cause magnification to always be performed, 
    and the magFilter to always be used. This is valid, just not an exact match for OpenGL behavior.
    Clamping the maximum LOD to 0.25 allows the λ value to be non-zero and minification to be performed,
    while still always rounding down to the base level. If the minFilter and magFilter are equal, then using a maxLod of zero also works."

  MKAY.... 
    mag filter is directly to opengl
    min filter is tricky, as it seems vulkan always wants to use mipmapping.
    therefore, to actually use a min filter but use only level 0 (no mipmap), you'd set maxLOD to 0.25 and mipmap to nearest
  OR
    if min filter == mag filter, setting magLOD to 0 would always use mag filter, min filter would not care.


    LOD bias, forces vulkan to use a lower resolution (higher level, lower resolution)
    minLOD would be base level, maxLOD would be maximum mipmaping level
  */


  if(out_t->affinity< 2) {

    // https://www.khronos.org/registry/vulkan/specs/1.2-khr-extensions/html/chap12.html#VkSamplerCreateInfo
    VkSamplerCreateInfo samplerInfo;
    if(out_t->vkd.customSampler)
      samplerInfo= *out_t->vkd.customSampler;
    else {
      samplerInfo.sType= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      samplerInfo.pNext= nullptr;
      samplerInfo.flags= 0;
    
      samplerInfo.magFilter=  (VkFilter)_ix->res.tex.cfg.magFilter;
      samplerInfo.minFilter=  (VkFilter)_ix->res.tex.cfg.minFilter;
      samplerInfo.mipmapMode= (VkSamplerMipmapMode)_ix->res.tex.cfg.mipmapFilter;
      samplerInfo.minLod=     0;
      samplerInfo.maxLod=     (float)(out_t->data->nrLevels- 1);
      samplerInfo.mipLodBias= 0;        // this can lower the resolution by just increasing it (cuts textures in half for a +1)

      samplerInfo.addressModeU= (VkSamplerAddressMode)out_t->data->wrapU;
      samplerInfo.addressModeV= (VkSamplerAddressMode)out_t->data->wrapV;
      samplerInfo.addressModeW= (VkSamplerAddressMode)out_t->data->wrapW;
      samplerInfo.borderColor=  (VkBorderColor)out_t->data->border;

      samplerInfo.anisotropyEnable= (_ix->res.tex.cfg.anisotropy> 1);
      samplerInfo.maxAnisotropy=    (float)_ix->res.tex.cfg.anisotropy;

      samplerInfo.compareEnable= VK_FALSE;
      samplerInfo.compareOp=     VK_COMPARE_OP_ALWAYS;
      samplerInfo.unnormalizedCoordinates= VK_FALSE;
    }

    /// create an own sampler for this texture, if REQUESTED
    out_t->vkd.sampler= null;
    if(out_t->vkd.flags.isUp(0x01)) {  // own sampler flag
      ixvkSampler *p= new ixvkSampler(&_ix->vk);
      _ix->vk.objects.addCustomSampler(p);
      out_t->vkd.sampler= p;
      p->nrTextures++;
      p->privateSampler= true;
      p->createInfo= samplerInfo;
      if(!p->build()) error.detail("VKO own sampler build failed.", __FUNCTION__, __LINE__);

    } else {
      /// search for an already created sampler to assign, values must match
      for(ixvkSampler *p= (ixvkSampler *)_ix->vk.objects.samplers.first; p; p= (ixvkSampler *)p->next) {
        if(p->privateSampler) continue;
        VkSamplerCreateInfo *s= &p->createInfo;
        if(s->maxLod== samplerInfo.maxLod)
        if(s->minLod== samplerInfo.minLod)
        if(s->mipLodBias== samplerInfo.mipLodBias)
        if(s->magFilter== samplerInfo.magFilter)
        if(s->minFilter== samplerInfo.minFilter)
        if(s->mipmapMode== samplerInfo.mipmapMode)
        if(s->addressModeU== samplerInfo.addressModeU)
        if(s->addressModeV== samplerInfo.addressModeV)
        if(s->addressModeW== samplerInfo.addressModeW)
        if(s->borderColor== samplerInfo.borderColor)
        if(s->anisotropyEnable== samplerInfo.anisotropyEnable)
        if(s->maxAnisotropy== samplerInfo.maxAnisotropy)
        if(s->compareEnable== samplerInfo.compareEnable)
        if(s->compareOp== samplerInfo.compareOp)
        if(s->unnormalizedCoordinates== samplerInfo.unnormalizedCoordinates) {
          out_t->vkd.sampler= p;
          p->nrTextures++;
        }
      }

      /// no sampler found, one is created
      if(out_t->vkd.sampler== null) {
        ixvkSampler *p= new ixvkSampler(&_ix->vk);
        _ix->vk.objects.addCustomSampler(p);
        out_t->vkd.sampler= p;
        p->nrTextures++;
        p->createInfo= samplerInfo;
        if(!p->build()) error.detail("VKO sampler build failed.", __FUNCTION__, __LINE__);
      }
    }

  } else if(out_t->affinity== 64) {
    if(!out_t->vkd.flags.isUp(0x01)) // own sampler flag
      out_t->vkd.sampler= out_t->stream->sampler;
    else {
      ixvkSampler *p= new ixvkSampler(&_ix->vk);
      _ix->vk.objects.addCustomSampler(p);
      out_t->vkd.sampler= p;
      p->nrTextures++;
      p->privateSampler= true;
      
      if(out_t->vkd.customSampler)
        p->createInfo= *out_t->vkd.customSampler;
      else {
        p->createInfo.sType= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        p->createInfo.pNext= nullptr;
        p->createInfo.flags= 0;
    
        p->createInfo.magFilter=  (VkFilter)_ix->res.tex.cfg.magFilter;
        p->createInfo.minFilter=  (VkFilter)_ix->res.tex.cfg.minFilter;
        p->createInfo.mipmapMode= (VkSamplerMipmapMode)_ix->res.tex.cfg.mipmapFilter;
        p->createInfo.minLod=     0;
        p->createInfo.maxLod=     (float)(out_t->data->nrLevels- 1);
        p->createInfo.mipLodBias= 0;        // this can lower the resolution by just increasing it (cuts textures in half for a +1)

        p->createInfo.addressModeU= (VkSamplerAddressMode)out_t->data->wrapU;
        p->createInfo.addressModeV= (VkSamplerAddressMode)out_t->data->wrapV;
        p->createInfo.addressModeW= (VkSamplerAddressMode)out_t->data->wrapW;
        p->createInfo.borderColor=  (VkBorderColor)out_t->data->border;

        p->createInfo.anisotropyEnable= (_ix->res.tex.cfg.anisotropy> 1);
        p->createInfo.maxAnisotropy=    (float)_ix->res.tex.cfg.anisotropy;

        p->createInfo.compareEnable= VK_FALSE;
        p->createInfo.compareOp=     VK_COMPARE_OP_ALWAYS;
        p->createInfo.unnormalizedCoordinates= VK_FALSE;
      }
      if(!p->build()) error.detail("VKO own sampler build failed.", __FUNCTION__, __LINE__);
    }

  }
}



bool ixTexSys::Vulkan::download(ixTexture *in_t) {
  const char *err= null;
  int errL= -1;
  if(in_t== null) IXERR("<in_t> is null");

  if(in_t->data== null)
    in_t->data= new Tex;

  if(in_t->affinity< 2) {
    if(!in_t->vkd.img->download(in_t->data, 0)) IXERR("Vulkan download failed");
  } else if(in_t->affinity== 64) {
    if(!in_t->vkd.img->download(in_t->data, in_t->layer->index)) IXERR("Vulkan download failed");
  }

  in_t->data->border= (ixBorderColor)in_t->vkd.sampler->createInfo.borderColor;
  
  in_t->data->wrapU= (Img::Wrap)in_t->vkd.sampler->createInfo.addressModeU;
  in_t->data->wrapV= (Img::Wrap)in_t->vkd.sampler->createInfo.addressModeV;
  in_t->data->wrapW= (Img::Wrap)in_t->vkd.sampler->createInfo.addressModeW;

  //in_t->data->swizzA; swizzle is in imgView, but that's the only data not saved... a class that has just the createInfo can happen

Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);

    if(in_t)
      if(in_t->data)
        in_t->data->delData();

    return false;

  } else
    return true;
}


bool ixTexSys::Vulkan::unload(ixTexture *out_t) {
  out_t->vkd.delData();
  
  return true;
}

#endif // VULKAN specific








