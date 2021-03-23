#include "ix/ix.h"

#ifndef max
#define max(a, b) ((a)> (b)? (a): (b))
#endif




/*
HIGH RESOLUTION / LOW RESOLUTION
 - there has to be 2 levels for each texture... low and high res
 - switching between the two, happens depending on the distance to the camera.
 - low res should be plenty, and fast to load from disk.
 - as you move the camera closer, the high res textures must load, if not already loaded
 - texFile must load extreem fast. like zomg. the header probly need to change, to something fast to read, 2-3-4 bytes long max, not that whole text
 - it would be possible to have even a 3rd level of detail, but it would start to complicate things... the highest def image tho, can incorporate so much
 */






#ifdef IX_USE_VULKAN



bool ixTexSys::assignSpecificStream(ixTexture *out_t, ixTexSys::Stream *in_stream) {
  return in_stream->assignSpot(out_t);
}

bool ixTexSys::assignStream(ixTexture *out_t, uint32 in_dx, uint32 in_dy, uint32 in_dz) {
  const char *err= null;
  int errL= 0;

  if(out_t->stream)
    out_t->stream->releaseSpot(out_t);
  
  if((!in_dx) || (!in_dy)) {
    if(out_t->data== null) IXERR("no size specified for the stream, no data avaible in the texture. Pick one or the other");
    if((!out_t->data->dx) || (!out_t->data->dy)) IXERR("no size specified for the stream, the size in texture data is also 0");

    in_dx= out_t->data->dx;
    in_dy= out_t->data->dy;
    in_dz= out_t->data->depth;
  }

  // search for a stream and assign it to the texture
  for(Stream *s= (Stream *)streams.first; s; s= (Stream *)s->next)
    if(s->dx== in_dx && s->dy== in_dy && s->dz== in_dz)
      return s->assignSpot(out_t);

Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);
    return false;
  } else
    return true;      // can't reach this point
}





#endif // VULKAN specific



///===========================-----------------------------///
// constructors / destructors ============================= //
///===========================-----------------------------///

ixTexSys::Stream::Stream(Ix *in_ix): ixClass(ixClassT::TEXSTREAM), _ix(in_ix) {
  cluster= null;
  //setPoolSegmentSize= 10;

  dx= dy= dz= 0;
  levels= 0;
  segmentLayers= 0;
  format= ImgFormat::UNDEFINED;
  swizzle= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

  //setLayout= null;
  //setPool= null;
  sampler= null;
  _customObjects= 0;

  /// sampler default values
  samplerInfo.sType= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.pNext= nullptr;
  samplerInfo.flags= 0;
  
  samplerInfo.magFilter=  (VkFilter)_ix->res.tex.cfg.magFilter;
  samplerInfo.minFilter=  (VkFilter)_ix->res.tex.cfg.minFilter;
  samplerInfo.mipmapMode= (VkSamplerMipmapMode)_ix->res.tex.cfg.mipmapFilter;
  samplerInfo.minLod=     0;
  samplerInfo.maxLod=     -1;       // if left negative, it will auto-compute from the cfg
  samplerInfo.mipLodBias= 0;        // this can lower the resolution by just increasing it (cuts textures in half for a +1)

  samplerInfo.addressModeU= VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV= VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW= VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.borderColor=  VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

  samplerInfo.anisotropyEnable= (_ix->res.tex.cfg.anisotropy> 1);
  samplerInfo.maxAnisotropy=    (float)_ix->res.tex.cfg.anisotropy;

  samplerInfo.compareEnable= VK_FALSE;
  samplerInfo.compareOp=     VK_COMPARE_OP_ALWAYS;
  samplerInfo.unnormalizedCoordinates= VK_FALSE;

  _ix->res.tex.streams.add(this);
}


ixTexSys::Stream::~Stream() {
  delData();
}


void ixTexSys::Stream::delData() {
  while(segments.last)
    _delSegment((Segment *)segments.last);

  if(cluster) { delete cluster; cluster= null; }

  //if(!_customObjects.isUp(0x01)) if(setPool) { _ix->vk.objects.delDynamicSetPool(setPool); setPool= null; }
  if(!_customObjects.isUp(0x02)) if(sampler) { _ix->vk.objects.delSampler(sampler);        sampler= null; }
  //if(!_customObjects.isUp(0x04)) setLayout= null;
}


ixTexSys::Stream::Segment::Segment() {
  freeSpacePeak= 0;

  image= null;
  //view= VK_NULL_HANDLE;
  
  //set= null;

  freeSpace= null;  // mem alloc
  layersMem= null;  // mem alloc
}


ixTexSys::Stream::Segment::~Segment() {
  layers.nrNodes= 0;
  layers.first= layers.last= null;
  if(freeSpace) { delete[] freeSpace; freeSpace= null; }
  if(layersMem) { delete[] layersMem; layersMem= null; }

}




///======================--------------------------------------------------------
// configuration / build ========================================================
///======================--------------------------------------------------------

void ixTexSys::Stream::cfgSize(uint32 in_dx, uint32 in_dy, uint32 in_dz) {
  dx= in_dx;
  dy= in_dy;
  dz= in_dz;
}


void ixTexSys::Stream::cfgSegment(uint32 in_segmentLayers, uint32 in_maxSegments) {
  segmentLayers= in_segmentLayers;
  maxSegmentLayers= in_maxSegments;
}


void ixTexSys::Stream::cfgFormat(ImgFormat in_format) {
  format= in_format;
}

void ixTexSys::Stream::cfgSwizzle(VkComponentSwizzle in_r, VkComponentSwizzle in_g, VkComponentSwizzle in_b, VkComponentSwizzle in_a) {
  swizzle.r= in_r, swizzle.g= in_g, swizzle.b= in_b, swizzle.a= in_a;
}


void ixTexSys::Stream::cfgLevels(uint32 in_n) {
  if(in_n== 0) {
    levels= 1;
    for(uint32 b= MAX(MAX(dx, dy), dz); b!= 1; b/= 2)
      levels++;
  } else
    levels= in_n;
}

/*
void ixTexSys::Stream::cfgSetPoolSegmentSize(uint16 in_n) {
  setPoolSegmentSize= in_n;
}

void ixTexSys::Stream::cfgSetLayout(VkoDescriptorSetLayout *in_p) {
  setLayout= in_p;
  _customObjects.setUp(0x04);
}

void ixTexSys::Stream::cfgSetPool(VkoDynamicSetPool *in_p) {
  setPool= in_p;
  _customObjects.setUp(0x01);
}
*/

void ixTexSys::Stream::cfgSampler(ixvkSampler *in_p) {
  sampler= in_p;
  _customObjects.setUp(0x02);
}


bool ixTexSys::Stream::build() {
  const char *err= nullptr;
  uint32 errL= 0;
  //ixvkImage *img= nullptr;
  //Segment *seg= nullptr;

  /// check to see if configured right
  if(format== ImgFormat::UNDEFINED)           IXERR("Format not configured");
  if((!dx) || (!dy) || (!dz))                 IXERR("Sizes not configured");
  if((!segmentLayers))                        IXERR("Segment size not configured");
  
  /// cluster cfg
  cluster= new ixvkResCluster(_ix);
  cluster->cfg(0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  // CLUSTER SIZE LEFT 0, the ixvkImage will populate it, after it knows how big a segment must be
  
  /// segment cfg
  _addSegment();


Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);
    return false;
  }

  return true;
}





// funcs =========================================================


bool ixTexSys::Stream::assignSpot(ixTexture *out_tex) {
  out_tex->stream= null;
  out_tex->segment= null;
  out_tex->layer= null;

  // search for a segment with free space
  Segment *s= (Segment *)segments.first;
  while(s) {
    if(s->freeSpacePeak) break;       // found
    s= (Segment *)s-> next;
  }

  // no free space in any segment, add a segment
  if(!s) {
    _addSegment();
    s= (Segment *)segments.last;
    if(!s->freeSpacePeak)             // could not add segment due max segments reached
      return false;
  }

  _linkTexture(out_tex, this, s, (Segment::Layer *)s->freeSpace[s->freeSpacePeak- 1]);

  s->freeSpacePeak--;

  return true;
}


void ixTexSys::Stream::releaseSpot(ixTexture *out_tex) {
  out_tex->segment->freeSpace[out_tex->segment->freeSpacePeak]= out_tex->layer;
  out_tex->segment->freeSpacePeak++;
  _unlinkTexture(out_tex);
}




















void ixTexSys::Stream::_addSegment() {
  // check for maximum segments reached
  if(maxSegmentLayers)
    if(segments.nrNodes+ 1> maxSegmentLayers)
      return;

  Segment *s= new Segment;

  /// mem alloc / freespace handling
  s->freeSpace= new void *[segmentLayers];
  s->layersMem= new Segment::Layer[segmentLayers];

  for(uint a= 0; a< segmentLayers; a++) {
    s->freeSpace[a]= (int8 *)s->layersMem+ a* sizeof(Segment::Layer);
    /// layers initialization
    s->layersMem[a].index= a;
    s->layersMem[a].tex= nullptr;
    
    s->layers.add(&s->layersMem[a]);
  }
  s->freeSpacePeak= segmentLayers;

  /// vulkan image cfg/build
  s->image= new ixvkImage(cluster);
  s->image->cfgFormat((VkFormat)format);
  s->image->cfgSize(VkExtent3D{dx, dy, dz}, segmentLayers);
  s->image->cfgUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT| VK_IMAGE_USAGE_SAMPLED_BIT);

  if(!s->image->build()) { cluster->delResource(s->image); delete s; return; }

  /// set layout of whole image to shader read
  _ix->vki.cmdTool->pool->reset();
  _ix->vki.cmdTool->startRecording();
  s->image->barrierRange(_ix, *_ix->vki.cmdTool, 0, segmentLayers, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             VK_ACCESS_MEMORY_WRITE_BIT,         VK_ACCESS_MEMORY_WRITE_BIT,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
  _ix->vki.cmdTool->endRecording();
  _ix->vki.cmdTool->submit(*_ix->vki.qTool);
  _ix->vk.QueueWaitIdle(*_ix->vki.qTool);

  /// set the access struct
  for(uint a= 0; a< segmentLayers; a++) {
    s->image->access[a].layout= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    s->image->access[a].qFamily= _ix->vki.q1->family;
  }

  /// vulkan image view cfg/build
  s->image->viewInfo.viewType= VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  s->image->viewInfo.format= (VkFormat)format;
  s->image->viewInfo.components= swizzle;
  s->image->viewInfo.subresourceRange.aspectMask= Img::vkGetAspectFromFormat(format);
  s->image->viewInfo.subresourceRange.baseMipLevel= 0;
  if(levels== 0)
    cfgLevels(0);
  s->image->viewInfo.subresourceRange.levelCount= levels;
  s->image->viewInfo.subresourceRange.baseArrayLayer= 0;
  s->image->viewInfo.subresourceRange.layerCount= segmentLayers;
  s->image->createView();


  /*
  VkImageViewCreateInfo viewInfo;
    viewInfo.sType= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext= nullptr;      // usage is inherited but can be inserted here with a VkImageViewUsageCreateInfo
    viewInfo.flags= 0;

    viewInfo.image=    s->image->handle->image;
    viewInfo.viewType= VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format=   (VkFormat)format;
    viewInfo.components= swizzle;

    viewInfo.subresourceRange.aspectMask=     Img::vkGetAspectFromFormat(format);
    viewInfo.subresourceRange.baseMipLevel=   0;

    if(levels== 0)
      cfgLevels(0);
    viewInfo.subresourceRange.levelCount=     levels;

    viewInfo.subresourceRange.baseArrayLayer= 0;
    viewInfo.subresourceRange.layerCount=     segmentLayers;
  if(!error.vkCheck(_ix->vk.CreateImageView(_ix->vk, &viewInfo, _ix->vk, &s->view))) { error.simple("vkCreateImageView failed."); return; }
  */


  /* HANDLED BY MATERIAL
  /// vulkan set layout - point this to a custom created one if needed
  if(setLayout== null)
    setLayout= _ix->res.tex.vkData.standard4mapsLayout;   // standard 4 channel

  /// vulkan set pool - point this to a custom created one if needed
  if(setPool== null) {
    setPool= _ix->vk.objects.addDynamicSetPool();
    setPool->configure(setLayout, setPoolSegmentSize);
    if(!setPool->build()) { error.detail(_ix->vk.errorText, __FUNCTION__, __LINE__); }
  }
  */
  /// vulkan sampler - point this to a custom created one if needed
  if(sampler== null) {
    sampler= new ixvkSampler(&_ix->vk);
    _ix->vk.objects.addCustomSampler(sampler);
    
    if(samplerInfo.maxLod< 0)
      samplerInfo.maxLod= (float)(levels- 1);

    sampler->createInfo= samplerInfo;
    if(!sampler->build()) error.detail("VKO own sampler build failed.", __FUNCTION__, __LINE__);
  }

  segments.add(s);  // all done
}






void ixTexSys::Stream::_delSegment(Segment *out_s) {

  for(Segment::Layer *l= (Segment::Layer *)out_s->layers.first; l; l= (Segment::Layer *)l->next) {
    if(l->tex)
      _unlinkTexture(l->tex);
  }

  // MOVED TO ixvkImage (DESTRUCTOR)
  //if(out_s->view!= VK_NULL_HANDLE) {
  //  _ix->vk.DestroyImageView(_ix->vk, out_s->view, _ix->vk);
  //  out_s->view= VK_NULL_HANDLE;
  //}

  if(cluster) {
    if(out_s->image) {
      
      cluster->delResource(out_s->image);
      out_s->image= null;
    }
  }
  
  out_s->layers.nrNodes= 0;
  out_s->layers.first= out_s->layers.last= null;
  if(out_s->freeSpace) { delete[] out_s->freeSpace; out_s->freeSpace= null; }
  if(out_s->layersMem) { delete[] out_s->layersMem; out_s->layersMem= null; }

  segments.del(out_s);
}



void ixTexSys::Stream::_linkTexture(ixTexture *out_t, ixTexSys::Stream *in_p1, ixTexSys::Stream::Segment *in_p2, ixTexSys::Stream::Segment::Layer *out_p3) {
  if(!out_t) return;
  
  out_p3->tex= out_t;

  out_t->stream= in_p1;
  out_t->segment= in_p2;
  out_t->layer= out_p3;
  out_t->vkd.img= in_p2->image;
  //out_t->vkd.imgView= in_p2->view;
  if(!out_t->vkd.flags.isUp(0x01))
    if(!out_t->vkd.sampler)
      out_t->vkd.sampler= in_p1->sampler;
}


void ixTexSys::Stream::_unlinkTexture(ixTexture *out_t) {
  if(out_t->stream!= this) { error.detail("texture is not part of this stream", __FUNCTION__); return; }

  ixTexSys::Stream                 *stream=  out_t->stream;
  //ixTexSys::Stream::Segment        *segment= out_t->segment;
  ixTexSys::Stream::Segment::Layer *layer=   out_t->layer;

  out_t->segment= null;
  out_t->layer= null;
  out_t->stream= null;    // <<< TO BE OR NOT TO BE - maybe this can be a more static thing, that won't change

  out_t->vkd.img= null;
  //out_t->vkd.imgView= null;

  layer->tex= null;
  
  //layer->tex->vkd.img= null;
  //layer->tex->vkd.imgView= VK_NULL_HANDLE;

  if(!out_t->vkd.flags.isUp(0x01))
    if(out_t->vkd.sampler== stream->sampler)
      out_t->vkd.sampler= null;
}







