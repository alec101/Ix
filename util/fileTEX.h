#pragma once

// keep an eye on glTextureView - http://www.opengl.org/sdk/docs/man/docbook4/xhtml/glTextureView.xml


//img png tga tex errors are all over the place - need a sort
//too many defines in tex, imho... fuk em
//error checks for tex, return true/false on funcs
//getStatus for Img
//these must go 'over' the Img errors

// >> HILY POSSIBLE ADDING: <<<
// a new type of host data, the *bitmap could point to one of 2 massive stage buffers
//   loading would be streamed this way, no allocs
//  -tbh, lefSize+levFrom can be an array, cuz they can only be 12 in size for a 2048 size texture
//   so a 13 in size, would fit a 8096 texture... and that would have... 263mb lol


using namespace mlib;


class ixTexture;
enum class ixBorderColor: uint32;

class Tex: public Img {
public:

  // all texture data / parameters
  Type type;          // one of 1D, 2D, 3D
  uint32 size;        // size of *bitmap (includes all levels)

  uint16 nrLevels;    // how many levels of mipmapping in the texture. if only 1, no mipmaps
  uint32 levSize[13]; // each level size [13 enough for 8096 size texture]
  uint32 levFrom[13]; // each level surface start [13 enough for 8096 size texture]

  ixBorderColor border; // border color [def:FLOAT_TRANSPARENT_BLACK]
  Wrap wrapU;         // U wrapping; [def:REPEAT]
  Wrap wrapV;         // V wrapping; [def:REPEAT]
  Wrap wrapW;         // W wrapping; [def:REPEAT]

  Swizzle swizzR;     // [def:identity] red   component swizzle
  Swizzle swizzG;     // [def:identity] green component swizzle
  Swizzle swizzB;     // [def:identity] blue  component swizzle
  Swizzle swizzA;     // [def:identity] alpha component swizzle

  //DISABLED ATM int32 compareMode;    // def: GL_NONE; http://www.opengl.org/sdk/docs/man/docbook4/xhtml/glTexParameter.xml
  //DISABLED ATM int32 compareFunc;    // def: GL_LEQUAL; http://www.opengl.org/sdk/docs/man/docbook4/xhtml/glTexParameter.xml

  // load / save

  bool load(cchar *fileName);   // load a TEX file
  bool save(cchar *fileName, int compressionType= 1);   // save current texture as a TEX file(or other type); compression: [0 none][1 RLE][2 ZIP]
  //bool load3D(cchar *fileBase); // (base img formats, not tex) overide the Img:load3D to fill in tex specific vars

  bool loadOnlyLevel(cchar *fileName);
  bool loadFromLevel(cchar *fileName);

  #ifdef IMG_CLASS_USE_VULKAN
  // THIS MUST BE DONE. like the GL one. As many funcs as there should be needed
  #endif // IMG_CLASS_USE_VULKAN


  #ifdef IMG_CLASS_USE_OPENGL
  bool glSaveID(cchar *in_file, int32 in_id, int32 in_target= GL_TEXTURE_2D, int8 in_fCompression= 1); /// getID(id) + save(fname)
  bool glGetID(int32 in_id, int32 in_type= GL_TEXTURE_2D);

  bool glGetTexture(ixTexture *t);
  bool glSaveTexture(cchar *fname, ixTexture *t, int8 fileCompression= 1);

  //bool glBuildTexture(ixTexture *out_t, bool mipmap, bool compress, int magFilter, int minFilter, int8 anisotropy= 0);    // creates the texture in grCard then populates out_t - 
  
  bool glCompressImage(int32 in_format= 0); // if left 0, compression algorithm is left to oGL
  bool glDecompressImage(ImgFormat in_outFormat= ImgFormat::UNDEFINED); // if left undefined, the new image format will be auto-picked to fit

  bool glHasMipmap(int target); // returns true/false if texture has mipmaps built in gpu; BIND TEXTURE FIRST
  bool glMipmapGenerate();      // creates the mipmap levels (if already mipmaps present/ GL_TEXTURE_1D returns true)
  #endif // IMG_CLASS_USE_OPENGL

  
  bool mipmapGenerate();      // creates the mipmaps
  bool mipmapDelete();        // deletes all the mipmap levels (if no mipmaps returns true)
  inline bool mipmapExist() { return (nrLevels> 1); }

  // error handling

  str8 errFileName;
  cchar *getError();     /// returns error status description, as a string

  // constructor / destructor

  void delData();
  Tex();
  ~Tex() { delData(); }
  
private:
  //friend class Texture;
  friend class _Tex;
};



// Error codes:

// 0 -  OK - No errors
// 1 -  [bitmap]/[wrapBitmap] error
// 2 -  Image size error
// 3 -  Invalid BPP / BPC
// 4 -  Image pixels already packed / unpacked
// 5 -  Image type does not support pixel pack/unpack operations (only [rgb16]/[grey1/2/4]/[cmap1/2/4])
// 6 -  Image type does not support channel swapping (only 1 channel?)
// 7 -  Channel number error (asking for channel 3 of a 2 channel image?)
// 8 -  Cannot open file
// 9 -  File read error
// 10 - File write error
// 11 - CMAP is empty / CMAP expected - not found
// 12 - Memory allocation failed
// 13 - CMAP palette not suported
// 14 - Unknown / unsupported image type
// 15 - Compression error
// 16 - Decompression error
// 17 - PNG header not read first
// 18 - PNG IHDR chunk error
// 19 - PNG PLTE chunk error
// 20 - PNG IDAT chunk error
// 21 - PNG raw data not filled
// 22 - PNG does not support this image format
// 23 - TEX: Texture size/depth not power of two
// 24 - TEX: Unsupported resample
// 25 - TEX: cannot convert image
// 26 - TEX: oGL error (must check there)
// 27 - TEX: Vulkan error (must check there)













