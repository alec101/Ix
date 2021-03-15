// exotic exts used:
// - GL_EXT_texture_filter_anisotropic

#include "ix/ix.h"


/* TODO:

*/



/*
    so the high res would be a imageView that is used if the object is closer to the camera?
    or a high res would be required by the gpu, maybe...
    i dono...
    i think it's the closer to camera thing, simple.
    i already thout of this... close to camera, medium distance and far distance... low med high imageView's
    */




///----------------///
// TEXTURE resource //
///================///

ixTexture::ixTexture(Ix *in_ix, uint32 in_texID): ixResource(in_ix), vkd(this) {

  texLibID= in_texID;

  fileType= 0;
  flags.setDown(0x01);
  flags.setDown(0x02);
  flags.setDown(0x04);
  data= null;
  
  #ifdef IX_USE_OPENGL
  glData.id= 0;
  glData.target= GL_TEXTURE_2D;
  glData.parent= this;
  #endif

  #ifdef IX_USE_VULKAN
  vkd.parent= this;
  vkd.img= null;
  vkd.imgView= null;
  vkd.sampler= null;
  vkd.customSampler= null;
  vkd.set= null;           // either own set from staticPool, or points to the set of the stream

  vkd.flags.setDown(0x01);   // own sampler
  vkd.flags.setUp(0x02);     // create the set
  #endif

  stream= null;
  segment= null;
  layer= null;

  delData();
}


void ixTexture::delData() {
  _ix->res.tex.unload(this);           /// unload from graphics card

  fileName.delData();
  
  flags.setDown(0x01);
  if(data) { delete data; data= null; }
}


void ixTexture::Vulkan::delData() {
  Ix *_ix= parent->_ix;

  // static texture
  if(parent->affinity< 2) {
    if(img)
      delete img;

    if(imgView)
      _ix->vk.DestroyImageView(_ix->vk, imgView, _ix->vk);

    if(sampler) {
      if(flags.isUp(0x01))
        _ix->vk.objects.delSampler(sampler);
      else
        sampler->nrTextures--;
    }

    if(flags.isUp(0x02))
      if(set) _ix->res.tex.vkd.staticPool->delSet(set);

  // stream texture
  } else if(parent->affinity== 64) {
    if(img)
      if(parent->stream)
        parent->stream->releaseSpot(parent);

    if(sampler && flags.isUp(0x01))
      _ix->vk.objects.delSampler(sampler);
  } /// types of textures

  img= null;
  imgView= null;
  sampler= null;
  set= null;
}








/*
cchar * ixTexture::_getError() {
  if(err== 22)  return "setting texture parameters FAIL";
  if(err== 23)  return "texture download from gpu FAIL";
  if(err== 24)  return "OpenGL error";
  if(err== 25)  return "texture sizes are not power of two";
  if(err== 26)  return "image format is not compatible";
  if(err== 27)  return "image convert fail";
  return ixResource::_getError();
}



bool ixTexture::check() {
  error.makeme(__FUNCTION__);
  return false;
}
*/













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








///================--------------------///
// VULKAN specific ==================== //
///================--------------------///

#ifdef IX_USE_VULKAN

#endif // IX_USE_VULKAN












