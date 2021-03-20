#pragma once

using namespace mlib;

enum class ixFilter: uint32 {
  NEAREST= 0,
  LINEAR=  1,
  CUBIC=   1000015000
};

enum class ixMipmapFilter: uint32 {
  NEAREST= 0,
  LINEAR=  1
};

enum class ixBorderColor: uint32 {
  FLOAT_TRANSPARENT_BLACK= 0,
  INT_TRANSPARENT_BLACK=   1,
  FLOAT_OPAQUE_BLACK=      2,
  INT_OPAQUE_BLACK=        3,
  FLOAT_OPAQUE_WHITE=      4,
  INT_OPAQUE_WHITE=        5
};

class ixvkStageBuffer;
class ixvkResCluster;
class ixvkSampler;
class ixvkImage;
class ixvkDescPool;
class ixvkDescSet;

///=====================-----------------------///
// TEXTURE SYSTEM class ======================= //
///=====================-----------------------///

class ixTexSys {
  Ix *_ix;
public:
  chainList data;

  str8 fileTextureDB;     // [def: "/gfx/textures.tdb"]

  // settings of the texture system
  struct Settings {
    ixFilter magFilter;           // [def:LINEAR]
    ixFilter minFilter;           // [def:LINEAR]
    ixMipmapFilter mipmapFilter;  // [def:LINEAR]
    //ixMipmapFilter minMMfilter;   // [def:LINEAR]
    int8 anisotropy;              // [def:8] texture anisotropy 1-isotropic/disabled, 1> is minimum, about 16 is max
    uint8 compressRule;           // [def:0] 0= up to the texture; 1= decompress all; 2= compress all
  } cfg;


  #ifdef IX_USE_VULKAN
  class Vulkan {
  public:
    Ix *_ix;

    VkoDescriptorSetLayout *standard4mapsLayout;  // standard 4 slot material layout
    VkoDescriptorSetLayout *staticLayout;         // layout for the static texture pool sets
    ixvkDescPool           *staticPool;           // pool for static textures

    void assignSampler(ixTexture *out_tex);
    //void createImageView(ixTexture *out_tex, Tex *in_texData= null);  // leaving texData null, will just pass the current createInfoStruct

    // vvvv ??? either this, or when a texture is destroyed, a check thru samplers happen
    void cleanSamplers();   // destroys all samplers that have no texture assigned                MAKEME+add me strategically <<<<<<<<<<<<<<<<

    void init();                    // initialize stuff, called by ix::init()


    bool upload(ixTexture *out_tex);      // [STATIC ONLY] static textures can upload data on will; doesn't work for streaming textures
    bool download(ixTexture *out_tex);    // [STATIC ONLY] data must exist - Vulkan doesn't have any sizes and such for the texture. only data->bitmap is downloaded
    bool unload(ixTexture *out_tex);      // [STATIC ONLY]
  } vkd;
  #endif // IX_USE_VULKAN

  #ifdef IX_USE_OPENGL
  struct OpenGL {
    ixTexSys *parent;
    bool upload(ixTexture *out_tex);
    bool download(ixTexture *out_tex);
    void unload(ixTexture *out_tex);

    // anisotropy funcs

    static int getMaxAnisotropy();     // bind texture first
    static int getAnisotropy(int in_target);
    static void setAnisotropy(int in_target, int in_level);
  } glData;
  #endif // IX_USE_OPENGL



  class Stream: public ixClass {
    Ix *_ix;
  public:
    // void upload() would for sure have the lowest possible detail assigned
    // would this be ok? you need to request high/medium
    // also mark that high/medium are not required anymore, up for grabs
    // void loadMedRes();
    // void loadHighRes();
    // void unloadHighRes();

    //this upload/download/unload system works for static textures
    //this system will not work for streaming system
    //the streaming sys must be told what you need, and it will load for you. low or high or watever res, depending on other rendering conditions

    // configuration / creation
    ixvkResCluster *cluster;      // what cluster will be used
    uint32 dx, dy, dz;            // texture size
    ImgFormat format;
    VkComponentMapping swizzle;
    uint32 levels;                // [def:0] mipmap number of levels; if left 0, it will be auto-computed for the maximum number of mipmaps
    uint16 segmentLayers;         // how many array layers will have 1 segment
    uint16 maxSegmentLayers;      // maximum segments- another one can be alocated from the cluster if required; 
    //uint16 setPoolSegmentSize;    // [def:10] number of sets that are allocated at once per pool dynamic segment <<< HANDLED BY MATERIAL

    //VkoDynamicSetPool *setPool;               // [def:null] if left null, a def one will be made; <<< HANDLED BY MATERIAL
    //VkoDescriptorSetLayout *setLayout;        // [def:null] if left null, a def one will be made; <<< STANDARD4MAPS, BUT HANDLED BY MATERIAL
    ixvkSampler *sampler;                     // [def:null] if left null, a def one will be made; samplerInfo will hold all the cfg for the default one;
    VkSamplerCreateInfo samplerInfo;          // all sampler options can be adjusted here

    ixFlags8 _customObjects;              // 01[customPool] 02[customSampler] custom objects will not be destroyed on delData()

    class Segment: public chainData {
    public:
      //uint32 layersInUse;
      ixvkImage *image;
      //VkImageView view;
      //VkoDynamicSet *set;         // a default set for this image array; more can be created if needed <<< HANDLED BY MATERIAL

      /// free space handling
      void **freeSpace;           // array list with all free spaces of image array layers in segment
      uint32 freeSpacePeak;       // peak of freeSpace. Acts like some kind of stack: u pick a free space from the top/ u put the space taken back

      /// each layer (parent::segmentLayers) data
      class Layer: public chainData {
      public:
        uint32 index;             // image array layer actual number
        ixTexture *tex;           // texture that uses this

        //uint64 timeBind;        // last time this was binded
      };

      Layer *layersMem;
      chainList layers;           // [data:Layer], NO delete AUTOMATION FOR THIS LIST; memory will be handled by the segment, all list allocated in one go

      bool arangeListOnBind;    // WIP on texture bind, the node will be moved to front, so the list will hold at last, the least used texture

      Segment();
      ~Segment();
    };

    chainList segments;

    // a class can happen here, that will hold all textures that need to switch to another res (lower/higher) 
    // a segList in that class with all textures that would require such change would be needed


    // configuration + build

    void cfgSize(uint32 dx, uint32 dy, uint32 dz);
    void cfgSegment(uint32 segmentLayers, uint32 maxSegments);
    void cfgFormat(ImgFormat format);
    void cfgSwizzle(VkComponentSwizzle in_r, VkComponentSwizzle in_g, VkComponentSwizzle in_b, VkComponentSwizzle in_a);
    void cfgLevels(uint32 in_n);                                // mipmap levels; put 0 to auto-compute maximum number of mip's
    //void cfgSetPoolSegmentSize(uint16 in_n);                    // [def:10] the set pool number of sets allocated at once
    //void cfgSetLayout(VkoDescriptorSetLayout *in_l);            // [def:null] if left null, standard 4 channel is used
    //void cfgSetPool(VkoDynamicSetPool *in_p);
    void cfgSampler(ixvkSampler *in_p);

    bool build();

    // funcs

    bool assignSpot(ixTexture *out_tex);
    void releaseSpot(ixTexture *out_tex);

    void _addSegment();
    void _delSegment(Segment *out_s);
    void _linkTexture(ixTexture *out_t, Stream *in_p1, Stream::Segment *in_p2, Stream::Segment::Layer *in_p3);
    void _unlinkTexture(ixTexture *out_t);

    //void _createView();

    // constructor / destructor

    Stream(Ix *in_ix);
    ~Stream();
    void delData();
  };

  chainList streams;            // [data: Stream] list with all created streams

  
  // create / delete texture

  struct Add {
    ixTexture *unconfiguredTexture();

    ixTexture *staticTexture();
    ixTexture *ixStaticTexture(); // same as static, but using ix's resCluster (ix resource)

    ixTexture *streamTexture(ixTexSys::Stream *in_stream);
    
  private:
    friend class ixTexSys;
    inline Add(ixTexSys *in_parent): _ix(in_parent->_ix) {}
    Ix *_ix;
  } add;

  ixTexture *search(cchar *in_name, uint32 in_id); // return texture that has ID or filename, one of the two

  void delTexture(ixTexture *t);

  // load from disk

  bool load(ixTexture *out_t, cchar *fileName);       // for 3D texture, filename should be the base name without 0000
  bool loadMem(ixTexture *out_t, cchar *fileName);    // loads from file, but does not upload to GPU
  bool reload(ixTexture *out_t);                      // reloads from file the texture, and uploads to GPU

  bool upload(ixTexture *out_t);          // uploads     from out_t->data  to GPU
  bool download(ixTexture *out_t);        // downloades  from GPU          to out_t->data
  bool unload(ixTexture *out_t);          // destroys the texture on the GPU side

  bool checkAllTextures();    // checks validity of all textures; if 1 is not valid, returns false
  bool reloadAllInvalid();    // reloads all textures that are not valid; if 1 failed to reload (if not valid), returns false



  bool assignStream(ixTexture *out_t, uint32 in_dx= 0, uint32 in_dy= 0, uint32 in_dz= 0);
  bool assignSpecificStream(ixTexture *out_t, Stream *in_stream);
  
  // standard constructor/destructor

  ixTexSys(Ix *in_ix);
  void delData();
  ~ixTexSys() { delData(); }

protected:

  friend class ixTexture;
  friend class Ix;

};




#include "texture.h"


// ERROR numbers:
// resource.h numbers+
// 20 - data class is null
// 21 - build texture FAIL
// 22 - setting texture parameters FAIL
// 23 - texture download from gpu FAIL
// 24 - OpenGL error
// 25 - texture sizes are not power of two
// 26 - image format is not compatible
// 27 - image convert fail



