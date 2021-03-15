#pragma once

using namespace mlib;
class ixTexSys;
class ixvkSampler;
class ixvkImage;



///=============///
// TEXTURE class //
///=============///

class ixTexture: public ixResource {
public:
  uint32 texLibID;    // special case 0: not in texture library file

  uint8 fileType;     // 0= TEX, 1= TGA, 2= PNG
  Img::Type type;     // 1D/2D/3D

  // [def:1] texture affinity, defining how the texture will work, internally.
  //  0: ix static texture, using ix's first cluster;
  //  1: static texture using static cluster;
  // 64: using texture stream sys;
  uint8 affinity;

  // 0x01 [def:down] is valid
  // 0x02 [def:down] keep parameters -  *data
  // 0x04 [def:down] keep bitmap data - *data::*bitmap
  ixFlags8 flags;

  Tex *data;

  ixTexSys::Stream *stream;                  // stream that is part of (if it is)
  ixTexSys::Stream::Segment *segment;        // segment of the stream sys, this is part of
  ixTexSys::Stream::Segment::Layer *layer;   // array layer of the segment

  #ifdef IX_USE_OPENGL
  struct OpenGL {
    uint id;            // OpenGL texture ID
    uint target;        // OpenGL target (GL_TEXTURE_2D / etc)
    ixTexture *parent;
  } glData;
  #endif // IX_USE_OPENGL


  #ifdef IX_USE_VULKAN
  struct Vulkan {
    ixTexture *parent;

    VkImageView   imgView;
    ixvkImage     *img;
    ixvkSampler   *sampler;
    VkSamplerCreateInfo *customSampler; // point this to a samplerCreateInfo for another way to customize the sampler
    VkoDynamicSet *set;                 // affinity 64: this is always null, the material must handle it

    // 0x01 [def:down] own sampler - [true: the sampler is unique to this texture] [false: sampler can be shared with multiple textures]
    // 0x02 [def:up] create set - ignored for affinity64
    ixFlags8 flags;

    inline Vulkan(ixTexture *in_parent): parent(in_parent) {}
    void delData();
  } vkd;
  #endif // IX_USE_VULKAN


  // funcs

  //inline void init(Ix *i) { vkData.init(i); } // if manually using a texture, it must know what Ix engine is gonna work on

  /*
  inline bool load(cchar *in_fn)    { return _ix->res.tex.load(this, in_fn); }
  inline bool loadMem(cchar *in_fn) { return _ix->res.tex.loadMem(this, in_fn); }
  inline bool reload()   { return _ix->res.tex.reload(this); }        // reloads texture from disk (uses fileName as the file)
  inline bool upload()   { return _ix->res.tex.upload(this); }
  inline bool download() { return _ix->res.tex.download(this); }
  inline bool unload()   { return _ix->res.tex.unload(this); }
  */

  /*
   i think these could be implemented, if you know what to load, there's no file extension check.
  bool loadTEX();
  bool loadTGA();
  bool loadPNG();
  */

  //bool load(cchar *);   /// loads texture from file

  //bool check();           // checks with the GPU engine if the texture is valid


  // constructor / destructor

  ixTexture(Ix *in_ix, uint32 in_textureLibraryID= 0);    // id[0] means no id in the lib file
  void delData();
  ~ixTexture() { delData(); }

protected:
  friend class ixTexSys;
  friend class ixTexSys::Stream;
  //cchar *_getError();

  

  /*
  void _loadAllShares(cchar *name);
  void _delAllSharesButThis();
  bool _isLastShare();
  void _breakShare();             // breaks this texture from the share list; if there's only 2 nodes in the list, it breaks the other share too, so _share= null
  void _breakAllShares();         // breaks / releases all the share list, the textures will not be linked in any way
  */
  friend class ixMaterial;
};










/// sub texture struct; a part of a texture: it's size in pixels and S & T texCoords
struct ixSubTex {
  uint16 dx, dy;          /// size in pixels 
  float s0, t0, se, te;   /// texture coordonates

  ixSubTex() { delData(); }
  void delData() { dx= dy= 0; s0= t0= 0.0f; se= te= 1.0f; }
  //void operator= (const ixSubTex &o) { dx= o.dx; dy= o.dy; s0= o.s0; se= o.se; t0= o.t0; te= o.te; }
};


/// usefull little struct, _notice the draw() func_
struct ixTexRect {
  float x0, y0, xe, ye;
  float s0, t0, se, te;

  ixTexRect() { delData(); }
  inline void delData() { x0= y0= xe= ye= 0.0f; s0= t0= 0.0f; se= te= 1.0f; }
  //void operator= (const ixTexRect &o) { x0= o.x0; xe= o.xe; y0= o.y0; ye= o.ye; s0= o.s0; se= o.se; t0= o.t0; te= o.te; }
};



class ixvkSampler: public VkoSampler {
  // using Vko::objects::addCustomSampler, so it's in the samplers list
public:
  uint32 nrTextures;    // number of textures that are using this sampler at this point in time
  bool privateSampler;

  ixvkSampler(vkObject *in_vko): VkoSampler(in_vko), nrTextures(0), privateSampler(false) {}
};





// ERROR numbers:
///==============
// resource.h numbers+
// 20 - data class is null
// 21 - build texture FAIL
// 22 - setting texture parameters FAIL
// 23 - texture download from gpu FAIL
// 24 - OpenGL error
// 25 - texture sizes are not power of two
// 26 - image format is not compatible
// 27 - image convert fail



