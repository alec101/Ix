#include "ix/ix.h"

#include "osi/include/util/mzPacker.h"
#include "osi/include/util/fileOp.h"


/* TODO:
- format i think should be the vulkan format.... convert function will not have much issues,
  it just needs a new func to know old format and new format, fromt hose funcs
  so you'd know if it is compressed, what compression, etc

- DDS file format handling, maybe
- let's face it, this handles the basic texture, but there are other textures with floating point values for example
  so in order for tex to be complete, it should handle everything ogl can handle...
  on second thout, the strange textures... could just have the Tex class params wrong, but the user should know what to do with them...
- load specific level
- texture arrays
*/

/*
  OPENGL has to stay and will stay, i can compress and decompress textures with it, and only with it
  i ain't gonna manually compress textures, looking thru all formats and algorithms, that's stoopid
*/


/* VULKAN FORMAT SUFFIXES
UNORM     means that the values are unsigned integers that are converted into floating points.
          The maximum possible representable value becomes 1.0, and the minimum representable value becomes 0.0.
          For example the value 255 in a R8Unorm will be interpreted as 1.0.
SNORM     is the same as Unorm, but the integers are signed and the range is from -1.0 to 1.0 instead.
USCALED   means that the values are unsigned integers that are converted into floating points.
          No change in the value is done. For example the value 255 in a R8Uscaled will be interpreted as 255.0.
SSCALED   is the same as Uscaled expect that the integers are signed.
UINT      means that the values are unsigned integers. No conversion is performed.
SINT      means that the values are signed integers. No conversion is performed.
UFLOAT    means that the values are unsigned floating points. No conversion is performed. This format is very unUsual.
SFLOAT    means that the values are regular floating points. No conversion is performed.
SRGB      is the same as Unorm, except that the value is interpreted as being in the sRGB color space.
          This means that its value will be converted to fit in the RGB color space when it is read.
          The fourth channel (usually used for alpha), if present, is not concerned by the conversion.
*/



#ifndef max
#define max(a, b) ((a)> (b)? (a): (b))
#endif

#define SFREAD(a, b, c, d) if(fread(a, b, c, d)!= (c)) { t->err= 9; goto LoadFail; }
#define SFWRITE(a, b, c, d) if(fwrite(a, b, c, d)!= (c)) { t->err= 10; goto SaveFail; }

/*
namespace _Tex {
//extern bool _loadTGA(cchar *, Img *);
//extern bool _saveTGA(cchar *, Img *);
//extern bool _loadPNG(cchar *, Img *);
//extern bool _savePNG(cchar *, Img *);
bool _loadTEX(cchar *, Tex *);
bool _saveTEX(cchar *, Tex *, int8);
bool _loadV13(cchar *, Tex *);
bool _loadV12(cchar *, Tex *);
extern bool _buildTexture(Img *, ixTexture *, bool, bool);
extern void _populateTexture(Tex *in_src, ixTexture *out_dst, int magFilter, int minFilter, int8 anisotropy);
};
*/

class _Tex {
  static const char *FILE_VER; //= "TEX file v1.4";
  friend class Tex;
  static bool loadTEX(cchar *, Tex *);
  static bool saveTEX(cchar *, Tex *, int8);
  static bool loadV13(cchar *, Tex *);
  static bool loadV12(cchar *, Tex *);
  static Img::Type    getFromGlType(int);
  static ImgFormat    getFromGlFormat(int);
  static Img::Wrap    getFromGlWrap(int);
  static Img::Swizzle getFromGlSwizzle(int, Img::Swizzle);

  static inline uint32 r2101010(uint32 c) { return (c>> 20)& 0x3FF; }
  static inline uint32 g2101010(uint32 c) { return (c>> 10)& 0x3FF; }
  static inline uint32 b2101010(uint32 c) { return c& 0x3FF; }
  static inline uint32 a2101010(uint32 c) { return (c>> 30)& 0x3; }

  static inline uint8 r44(uint8 c) { return c>> 4; }
  static inline uint8 g44(uint8 c) { return c& 0xF; }

  static inline uint16 r4444(uint16 c) { return c>> 12; }
  static inline uint16 g4444(uint16 c) { return (c>> 8)& 0xF; }
  static inline uint16 b4444(uint16 c) { return (c>> 4)& 0xF; }
  static inline uint16 a4444(uint16 c) { return c& 0xF; }

//  extern bool _buildTexture(Img *, ixTexture *, bool, bool);
//  extern void _populateTexture(Tex *in_src, ixTexture *out_dst, int magFilter, int minFilter, int8 anisotropy);
};
const char *_Tex::FILE_VER= "TEX1.4";

Tex::Tex() {
  bitmap= null;
  //levSize= null;
  //levFrom= null;
  errFileName= "no file";

  Tex::delData();
}

// dealloc / clear everything, standard func
void Tex::delData() {
  nrLevels= 0;
  type= Type::T_2D;
  dx= dy= size= 0;
  depth= 0;

  border= ixBorderColor::FLOAT_TRANSPARENT_BLACK;
  swizzR= swizzG= swizzB= swizzA= Swizzle::IDENTITY;
  wrapU= wrapV= wrapW= Img::Wrap::REPEAT;

  Img::delData();
}

// error status return (as a string)
cchar *Tex::getError() {
  if(err== 23) return "TEX: Texture size/depth not power of two";
  if(err== 24) return "TEX: Unsupported resample";
  if(err== 25) return "TEX: cannot convert image";
  if(err== 26) return "TEX: oGL error (must check there)";
  if(err== 27) return "TEX: Vulkan error (must check there)";
  return Img::getError();
}



///=================///
// LOAD / SAVE funcs //
///=================///

// load a TEX file
bool Tex::load(cchar *fname) {
  if(!fname) { error.simple(str8(__FUNCTION__)+ " File name is NULL"); return false; }
  delData();
  err= 0;

  
  str8 s= pointFileExt(fname);
  s.lower();

  // TEX file load
  if(s== "tex") return _Tex::loadTEX(fname, this); // tex loading ends here

  // PNG/TGA/ rest of image types
  if(!Img::load(fname)) goto Error;

  if(dx== 0) { err= 2; goto Error; }
  if(!areSizesPowerOfTwo()) { err= 23; goto Error; }

  if(!depth) depth= 1;
  nrLevels= 1;                  /// no mipmap is generated from PNG/TGA by default - must be created later
  
  /// texture type
  if((dx== 1) || (dy== 1))  type= Type::T_1D;
  else if(depth== 1)        type= Type::T_2D;
  else if(depth> 1)         type= Type::T_3D;

  #ifdef IMG_CLASS_USE_OPENGL
  //glInternalFormat= glGetGlFormat(format);
  #endif

  size= dx* dy* (bpp/ 8)* depth;
  if(size== 0) { err= 2; goto Error; }

  levSize[0]= size; /// dx* dy* (bpp/ 8)* depth - no mipmaps tho
  levFrom[0]= 0;    /// levFrom[prev]+ levSize[prev]

  return true; // success

Error:
  errFileName= fname;
  error.simple(str8(__FUNCTION__)+ " ["+ fname+ "] "+ getError());
  delData();
  return false;
}



bool Tex::save(cchar *fname, int compressionType) {
  str8 s= pointFileExt(fname);
  s.lower();

  if(s== "tex")
    return _Tex::saveTEX(fname, this, compressionType);
  else
    return Img::save(fname);
}


bool _Tex::loadTEX(cchar *fname, Tex *t) {
  if(!t) return false;
  t->delData();
  t->err= 0;
  t->fileName= fname;
  t->fileType= 0;

  int32 tmp= Str::strlen8(FILE_VER);
  char buf[32];

  FILE *f= fopen(fname, "rb");
  if(!f) { t->err= 8; goto Error;}

  fread(buf, tmp, 1, f);
  buf[tmp- 1]= 0;                           // force terminator (v important!!! this was missing)

  if(Str::strcmp8(FILE_VER, buf)) {
    fclose(f);
    return _Tex::loadV13(fname, t);         // try load v1.3
  }

  int8 dataCompression;
  int32 dataSize;

  SFREAD(&t->type,           1, 1, f)
  SFREAD(&t->dx,             4, 1, f)
  SFREAD(&t->dy,             4, 1, f)
  SFREAD(&t->depth,          4, 1, f)
  SFREAD(&t->size,           4, 1, f)
  SFREAD(&t->format,         4, 1, f) /// 1.4
  SFREAD(&t->compressed,     1, 1, f) /// 1.4
  SFREAD(&t->nrLevels,       2, 1, f)
  SFREAD(&t->wrapU,          1, 1, f)
  SFREAD(&t->wrapV,          1, 1, f)
  SFREAD(&t->wrapW,          1, 1, f)
  SFREAD(&t->border,         4, 4, f) /// 1.3 - texture border
  SFREAD(&t->swizzR,         1, 1, f) /// 1.3 - texture swizzle
  SFREAD(&t->swizzG,         1, 1, f) /// 1.3 - texture swizzle
  SFREAD(&t->swizzB,         1, 1, f) /// 1.3 - texture swizzle
  SFREAD(&t->swizzA,         1, 1, f) /// 1.3 - texture swizzle

  t->computePixelInfo();

  SFREAD(t->levSize, 4, t->nrLevels, f)
  SFREAD(t->levFrom, 4, t->nrLevels, f)

  if(t->_wrap) {
    *t->wrapBitmap= new uint8[t->size];
    t->bitmap= *t->wrapBitmap;
  } else
    t->bitmap= new uint8[t->size];

  if(!t->bitmap) { t->err= 12; goto LoadFail; }

  SFREAD(&dataCompression, 1, 1, f)
  SFREAD(&dataSize,        4, 1, f)

  if(dataCompression== 0) {
    SFREAD(t->bitmap, t->size, 1, f);

  } else if(dataCompression== 1) { // RLE
    uint8 pix[32];
    uint8 pixSize= ((t->bpp< 8 || t->compressed)? 1: t->bpp/ 8);
    if(pixSize> 32) pixSize= 32;                    // maximum 32 bytes chunks, per pixel if possible
    uint8 *p= (uint8 *)t->bitmap;
    uint8 cheader;
    uint8 n;

    for(uint32 a= dataSize; a> 0; a--) {
      SFREAD(&cheader, 1, 1, f);
      n= (cheader& 0x7F)+ 1;                       /// chunk value

      if(cheader& 0x80) {
        SFREAD(&pix, pixSize, 1, f);
        
        for(uint32 b= n; b> 0; b--, p+= pixSize)
          for(uint8 c= 0; c< pixSize; c++)
            p[c]= pix[c];

      } else { 
        SFREAD(p, n* pixSize, 1, f);
        p+= n* pixSize;
      }
    }

  } else if(dataCompression== 2) { // mzPacker
    mzPacker pack;
    pack.startAdvDecomp(dataSize, STDIO_FILE, f, 0, USR_BUFFER, t->bitmap, t->size);
    while(pack.doAdvDecomp())
      if(pack.err) { t->err= 16; goto LoadFail; }
  }


  fclose(f);
  return true; // success

LoadFail:
  t->delData();
  fclose(f);
Error:
  t->errFileName= fname;
  error.simple(str8(__FUNCTION__)+ "() ["+ fname+ "] "+ t->getError());
  return false;
}



// save current Tex object as a TEX file
bool _Tex::saveTEX(cchar *fileName, Tex *t, int8 compressionType) {
  /// simple checks
  if(!t->dx) return false;
  if(t->_wrap) { if(!t->wrapBitmap) return false; if(!*t->wrapBitmap) return false; }
  else if(!t->bitmap) return false;
  if(t->compressed) compressionType= 0;   // do not compress the file if the texture is compressed

  t->err= 0;

  int32 bufLen= Str::strlen8(FILE_VER);
  int32 dataSize;

  FILE *f= fopen(fileName, "wb");
  if(!f) { t->err= 8; goto Error; }

  SFWRITE(FILE_VER, 1, bufLen, f)
  SFWRITE(&t->type,       1, 1, f)
  SFWRITE(&t->dx,         4, 1, f)
  SFWRITE(&t->dy,         4, 1, f)
  SFWRITE(&t->depth,      4, 1, f)
  SFWRITE(&t->size,       4, 1, f)
  SFWRITE(&t->format,     4, 1, f) /// 1.4
  SFWRITE(&t->compressed, 1, 1, f) /// 1.4
  SFWRITE(&t->nrLevels,   2, 1, f)
  SFWRITE(&t->wrapU,      1, 1, f)
  SFWRITE(&t->wrapV,      1, 1, f)
  SFWRITE(&t->wrapW,      1, 1, f)
  SFWRITE(&t->border,     4, 1, f) /// 1.4 - texture border color, modified for vulkan compat
  SFWRITE(&t->swizzR,     1, 1, f) /// 1.3 - texture swizzle
  SFWRITE(&t->swizzG,     1, 1, f) /// 1.3 - texture swizzle
  SFWRITE(&t->swizzB,     1, 1, f) /// 1.3 - texture swizzle
  SFWRITE(&t->swizzA,     1, 1, f) /// 1.3 - texture swizzle

  SFWRITE(t->levSize, 4, t->nrLevels, f)
  SFWRITE(t->levFrom, 4, t->nrLevels, f)

  SFWRITE(&compressionType, 1, 1, f)
  
  if(compressionType== 0) {         // no file data compression
    SFWRITE(&t->size,  4, 1,       f)
    SFWRITE(t->bitmap, 1, t->size, f)

  } else if(compressionType== 1) {  // RLE
    /// save data size position in file (atm data size is not known)
    long dataSizePos= ftell(f);
    uint8 pixel[32];               /// max R64 G64 B64 A64
    uint8 pixSize= (uint8)(t->bpp< 8? 1: t->bpp/ 8);
    if(pixSize> 32) pixSize= 32;

    uint8 chunkSize;
    uint8 *p= (uint8 *)t->bitmap; /// p will walk bitmap

    /// write 4 zero bytes, to make space for dataSize
    dataSize= 0;
    SFWRITE(&dataSize, 4, 1, f)

    for(uint32 a= 0, n= t->size/ pixSize; a< n;) {  /// pass thru all the bitmap data

      /// pixel will hold current pixel (or pack of pixels if bpp< 8)
      for(uint8 c= 0; c< pixSize; c++)
        pixel[c]= p[c];

      if((a+ 1) < n) {
        /// check if the next pixel == current pixel (equal will hold this bool)
        bool equal= true;
        for(uint8 c= 0; c< pixSize; c++)
          if(pixel[c]!= p[pixSize+ c])
            equal= false;

        /// RLE chunk - next pixel == current pixel
        if(equal) {
          chunkSize= 0;
          for(; a+ 1< n; a++, chunkSize++) {

            equal= true;
            int32 n= chunkSize* pixSize;
            for(uint8 c= 0; c< pixSize; c++)
              if(pixel[c]!= p[n+ c])
                equal= false;

            if(!equal)          break;
            if(chunkSize== 127) break;
          }

          /// create RLE chunk - got all the data necessary
          uint8 b= 128+ chunkSize;       /// RLE chunk header first bit is type, next 7 bits are the chunk size (how many times the next pixel repeats)
          SFWRITE(&b,    1,       1, f);
          SFWRITE(pixel, pixSize, 1, f);
          
          p+= (chunkSize+ 1)* pixSize;   /// advance p (now because chunkSize is getting messed with)
          dataSize= 1+ pixSize;          /// chunk header (1) + pixel size (packSize)
        }

      /// uncompressed chunk - next pixel != current pixel
      } else {
        chunkSize= 0;

        for(; a+ 1< n; a++, chunkSize++) {
          bool equal= true;

          uint32 n= chunkSize* pixSize;
          for(uint8 c= 0; c< pixSize; c++)
            if(pixel[c]!= p[n+ c])
              equal= false;

          if(equal)           break;
          if(chunkSize== 127) break;
        }

        /// create uncompressed chunk - got all the data neccesary
        SFWRITE(&chunkSize, 1,                       1, f);
        SFWRITE(p,          (chunkSize+ 1)* pixSize, 1, f);
        p+= (chunkSize+ 1)* pixSize;
        
        dataSize+= n+ 1;
      } /// packed RLE chunk / unpacked chunk
    } /// pass thru all the bitmap data
        
    /// write data size
    fseek(f, dataSizePos, SEEK_SET);
    SFWRITE(&dataSize, 4, 1, f);

  } else if(compressionType== 2) {  // mzPacker
    /// save dataSize position (atm is unknown)
    long dataSizePos= ftell(f);
    dataSize= 0;
    SFWRITE(&dataSize, 4, 1, f)

    /// pack the data
    mzPacker pack;
    pack.startAdvComp(t->size, USR_BUFFER, t->bitmap, t->size, STDIO_FILE, f, 0);
    while(pack.doAdvComp())
      if(pack.err) { t->err= 15; goto SaveFail; }
    
    /// write data size
    dataSize= (int32)pack.results.outTotalFilled;
    fseek(f, dataSizePos, SEEK_SET);
    SFWRITE(&dataSize, 4, 1, f);
  }

  fclose(f);

  t->fileName= fileName;
  t->fileType= 0;

  return true; // success

SaveFail:
  fclose(f);
Error:
  t->errFileName= fileName;
  error.simple(str8(__FUNCTION__)+ "() ["+ fileName+ "] "+ t->getError());
  return false;
}

//} // namespace _Tex






bool Tex::loadOnlyLevel(cchar *fileName) {
  
  /*
  this is clearly needed, somehow

    and maybe even loading from PNG's and other images
    
    maybe it can downgrade the texture

    i think there should be a specific func for it


    clearly a specific level load, or a specific level range
    or create mipmaps only from specific level, and not top level
    but all this, after vulkan i think
    enough with the ogl, need vulkan stuff

;
*/
  error.makeme(__FUNCTION__);
  return false;
}




bool Tex::loadFromLevel(cchar *fileName) {
  error.makeme(__FUNCTION__);
  return false;
}




bool Tex::mipmapGenerate() {
  uint32 tdx, tdy, tdz;
  uint8 *newBitmap;

  /// simple checks
  if(nrLevels> 1)            return true;
  if(type== Img::Type::T_1D) return true;

  if(!areSizesPowerOfTwo())    { err= 2;  goto Exit; } // power of two sizes only
  if(_wrap) { if(!wrapBitmap)  { err= 1;  goto Exit; }
              if(!*wrapBitmap) { err= 1;  goto Exit; } }
  else if(!bitmap)             { err= 1;  goto Exit; }
  if(!dx)                      { err= 2;  goto Exit; }
  if(compressed)               { err= 14; goto Exit; } // atm i don't see a way to process compressed images... they could be processed by vulkan/ogl only

  err= 0;
  
  // populate levels sizes
  size= 0;
  nrLevels= mipmapGetMaxLevels(this);

  if(nrLevels== 1) return true;

  levFrom[0]= 0, levSize[0]= 0;
  
  tdx= dy, tdy= dy, tdz= depth;
  for(uint a= 0; a< nrLevels; a++) {
    levSize[a]= tdx* tdy* tdz* (bpp/ 8);
    if(a) levFrom[a]= levFrom[a- 1]+ levSize[a- 1];
    size+= levSize[a];

    tdx/= (tdx> 1? 2: 1), tdy/= (tdy> 1? 2: 1), tdz/= (tdz> 1? 2: 1);
  }
  
  // new bitmap alloc and old deleted
  newBitmap= new uint8[size];
  Str::memcpy(newBitmap, bitmap, levSize[0]);

  if(_wrap) {
    delete[] *wrapBitmap;
    *wrapBitmap= newBitmap;
  } else
    delete[] bitmap;
  bitmap= newBitmap;

  tdx= dy, tdy= dy, tdz= depth;

  if(format== ImgFormat::R4G4_UNORM_PACK8) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint8 *p= bitmap+ levFrom[a], *src= bitmap+ levFrom[a- 1];
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdx)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdx)
      for(uint32 x= max(1, tdx/ 2); x; --x, src++, p++) {
        uint32 t= _Tex::r44(*src)+ _Tex::r44(*(src+ 1))+ _Tex::r44(*(src+ tdx))+ _Tex::r44(*(src+ tdx+ 1));
        *p=  (uint8)((t/ 4)+ (t% 4> 2? 1: 0))<< 4;   // t% 4 can return 1, 2 or 3. 3 will be tied to the ceil of the division
        t= _Tex::g44(*src)+ _Tex::g44(*(src+ 1))+ _Tex::g44(*(src+ tdx))+ _Tex::g44(*(src+ tdx+ 1));
        *p|= (uint8)((t/ 4)+ (t% 4> 2? 1: 0));
      } /// for each texel on the x
    } /// for each level- 1
    
  } else if(format== ImgFormat::R4G4B4A4_UNORM_PACK16 || format== ImgFormat::B4G4R4A4_UNORM_PACK16) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint16 *p= (uint16 *)bitmap+ levFrom[a], *src= (uint16 *)bitmap+ levFrom[a- 1];
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdx)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdx)
      for(uint32 x= max(1, tdx/ 2); x; --x, src++, p++) {
        uint32 t= _Tex::r4444(*src)+ _Tex::r4444(*(src+ 1))+ _Tex::r4444(*(src+ tdx))+ _Tex::r4444(*(src+ tdx+ 1));
        *p=  (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 12;

        t= _Tex::g4444(*src)+ _Tex::g4444(*(src+ 1))+ _Tex::g4444(*(src+ tdx))+ _Tex::g4444(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 8;

        t= _Tex::b4444(*src)+ _Tex::b4444(*(src+ 1))+ _Tex::b4444(*(src+ tdx))+ _Tex::b4444(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 4;

        t= _Tex::a4444(*src)+ _Tex::a4444(*(src+ 1))+ _Tex::a4444(*(src+ tdx))+ _Tex::a4444(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0));
      } /// for each texel on the x
    } /// for each level- 1

  } else if(format== ImgFormat::R5G6B5_UNORM_PACK16 || format== ImgFormat::B5G6R5_UNORM_PACK16) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint16 *p= (uint16 *)bitmap+ levFrom[a], *src= (uint16 *)bitmap+ levFrom[a- 1];
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdx)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdx)
      for(uint32 x= max(1, tdx/ 2); x; --x, src++, p++) {
        uint32 t= getR565(*src)+ getR565(*(src+ 1))+ getR565(*(src+ tdx))+ getR565(*(src+ tdx+ 1));
        *p=  (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 11;

        t= getG565(*src)+ getG565(*(src+ 1))+ getG565(*(src+ tdx))+ getG565(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 5;

        t= getB565(*src)+ getB565(*(src+ 1))+ getB565(*(src+ tdx))+ getB565(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0));
      } /// for each texel on the x
    } /// for each level- 1

  } else if(format== ImgFormat::R5G5B5A1_UNORM_PACK16 || format== ImgFormat::B5G5R5A1_UNORM_PACK16) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint16 *p= (uint16 *)bitmap+ levFrom[a], *src= (uint16 *)bitmap+ levFrom[a- 1];
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdx)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdx)
      for(uint32 x= max(1, tdx/ 2); x; --x, src++, p++) {
        uint32 t= getR5551(*src)+ getR5551(*(src+ 1))+ getR5551(*(src+ tdx))+ getR5551(*(src+ tdx+ 1));
        *p= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 11;

        t= getG5551(*src)+ getG5551(*(src+ 1))+ getG5551(*(src+ tdx))+ getG5551(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 6;

        t= getB5551(*src)+ getB5551(*(src+ 1))+ getB5551(*(src+ tdx))+ getB5551(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 1;

        t= getA5551(*src)+ getA5551(*(src+ 1))+ getA5551(*(src+ tdx))+ getA5551(*(src+ tdx+ 1));
        *p|= (uint16)(t/ 4);
      } /// for each texel on the x
    } /// for each level- 1

  } else if(format== ImgFormat::A1R5G5B5_UNORM_PACK16) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint16 *p= (uint16 *)bitmap+ levFrom[a], *src= (uint16 *)bitmap+ levFrom[a- 1];
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdx)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdx)
      for(uint32 x= max(1, tdx/ 2); x; --x, src++, p++) {
        uint32 t= getA1555(*src)+ getA1555(*(src+ 1))+ getA1555(*(src+ tdx))+ getA1555(*(src+ tdx+ 1));
        *p=  (uint16)((t/ 4)<< 15);

        t= getR1555(*src)+ getR1555(*(src+ 1))+ getR1555(*(src+ tdx))+ getR1555(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 10;

        t= getG1555(*src)+ getG1555(*(src+ 1))+ getG1555(*(src+ tdx))+ getG1555(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0))<< 5;

        t= getB1555(*src)+ getB1555(*(src+ 1))+ getB1555(*(src+ tdx))+ getB1555(*(src+ tdx+ 1));
        *p|= (uint16)((t/ 4)+ (t% 4> 2? 1: 0));
      } /// for each texel on the x
    } /// for each level- 1

  } else if(bpc[0]== 2) { // only A2R10G10B10_UNORM_PACK32 family will start with 2 bits for first channel
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint32 *p32= (uint32 *)bitmap+ levFrom[a], *src32= (uint32 *)bitmap+ levFrom[a- 1];
      for(uint32 z= max(1, tdz/ 2); z; --z, src32+= tdy* tdx)
      for(uint32 y= max(1, tdy/ 2); y; --y, src32+= tdx)
      for(uint32 x= max(1, tdx/ 2); x; --x, src32++, p32++) {
        uint64 t= _Tex::a2101010(*src32)+ _Tex::a2101010(*(src32+ 1))+ _Tex::a2101010(*(src32+ tdx))+ _Tex::a2101010(*(src32+ tdx+ 1));
        *p32=  (uint32)((t/ 4)+ (t% 4> 2? 1: 0))<< 30;   // t% 4 can return 1, 2 or 3. 3 will be tied to the ceil of the division
          
        t= _Tex::r2101010(*src32)+ _Tex::r2101010(*(src32+ 1))+ _Tex::r2101010(*(src32+ tdx))+ _Tex::r2101010(*(src32+ tdx+ 1));
        *p32|= (uint32)((t/ 4)+ (t% 4> 2? 1: 0))<< 20;

        t= _Tex::g2101010(*src32)+ _Tex::g2101010(*(src32+ 1))+ _Tex::g2101010(*(src32+ tdx))+ _Tex::g2101010(*(src32+ tdx+ 1));
        *p32|= (uint32)((t/ 4)+ (t% 4> 2? 1: 0))<< 10;

        t= _Tex::b2101010(*src32)+ _Tex::b2101010(*(src32+ 1))+ _Tex::b2101010(*(src32+ tdx))+ _Tex::b2101010(*(src32+ tdx+ 1));
        *p32|= (uint32)((t/ 4)+ (t% 4> 2? 1: 0));
      } /// for each texel on the x
    } /// for each level- 1

  // vvvvvvvvvv the most used vvvvvvvvvv
  } else if(bpc[0]== 8) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint8 *p= bitmap+ levFrom[a], *src= bitmap+ levFrom[a- 1];
      uint32 tdxNchan= tdx* nchannels;
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdxNchan)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdxNchan)
      for(uint32 x= max(1, tdx/ 2); x; --x, src+= nchannels)
        for(uint c= nchannels; c> 0; c--, src++, p++) {
          uint32 t= (*src)+ (*(src+ nchannels))+ (*(src+ tdxNchan))+ (*(src+ tdxNchan+ nchannels));
          *p= (uint8)(t/ 4)+ (t% 4> 2? 1: 0);
        } /// for each channel
    } /// for each level- 1
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  } else if(bpc[0]== 16) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint16 *p= (uint16 *)bitmap+ levFrom[a], *src= (uint16 *)bitmap+ levFrom[a- 1];
      uint32 tdxNchan= tdx* nchannels;
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdxNchan)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdxNchan)
      for(uint32 x= max(1, tdx/ 2); x; --x, src+= nchannels)
        for(uint c= nchannels; c> 0; c--, src++, p++) {
          uint32 t= (*src)+ (*(src+ nchannels))+ (*(src+ tdxNchan))+ (*(src+ tdxNchan+ nchannels));
          *p= (uint16)(t/ 4)+ (t% 4> 2? 1: 0);
        } /// for each channel
    } /// for each level- 1

  } else if(bpc[0]== 32) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint32 *p= (uint32 *)bitmap+ levFrom[a], *src= (uint32 *)bitmap+ levFrom[a- 1];
      uint32 tdxNchan= tdx* nchannels;
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdxNchan)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdxNchan)
      for(uint32 x= max(1, tdx/ 2); x; --x, src+= nchannels)
        for(uint c= nchannels; c> 0; c--, src++, p++) {
          uint32 t= (*src)+ (*(src+ nchannels))+ (*(src+ tdxNchan))+ (*(src+ tdxNchan+ nchannels));
          *p= (uint32)(t/ 4)+ (t% 4> 2? 1: 0);
        } /// for each channel
    } /// for each level- 1

  } else if(bpc[0]== 64) {
    for(uint a= 1; a< nrLevels; a++, tdx= max(1, tdx/ 2), tdy= max(1, tdy/ 2), tdz= max(1, tdz/ 2)) {
      uint64 *p= (uint64 *)bitmap+ levFrom[a], *src= (uint64 *)bitmap+ levFrom[a- 1];
      uint32 tdxNchan= tdx* nchannels;
      for(uint32 z= max(1, tdz/ 2); z; --z, src+= tdy* tdxNchan)
      for(uint32 y= max(1, tdy/ 2); y; --y, src+= tdxNchan)
      for(uint32 x= max(1, tdx/ 2); x; --x, src+= nchannels)
        for(uint c= nchannels; c> 0; c--, src++, p++) {
          uint64 avg1, avg2, swp;
          uint64 v1= *src;
          uint64 v2= *(src+ nchannels);
          if(v1> v2) swp= v1, v1= v2, v2= swp;              // making sure v2 > v1
          uint64 v3= *(src+ tdxNchan);
          uint64 v4= *(src+ tdxNchan+ nchannels);
          if(v3> v4) swp= v3, v3= v4, v4= swp;              // making sure v4 > v3
        
          avg1= ((v2- v1)/ 2)+ v1;                          // won't exceed i64
          avg2= ((v4- v3)/ 2)+ v3;                          // won't exceed i64
          if(avg1> avg2) swp= avg1, avg1= avg2, avg2= swp;  // making sure avg2 > avg1
          *p= ((avg2- avg1)/ 2)+ avg1;                      // won't exceed i64
        } /// for each channel
    } /// for each level- 1

  } else {
    err= 14;
    goto Exit;
  }
   
Exit:
  if(err) {
    error.simple(str8(__FUNCTION__)+ "() ["+ fileName+ "] "+ getError());
    return false;
  }
  return true;
}


bool Tex::mipmapDelete() {
  /// simple checks
  if(nrLevels<= 1) return true;
  if(_wrap) { if(!wrapBitmap) return false; if(!*wrapBitmap) return false; }
  else if(!bitmap) return false;
  if(!dx) return false;

  /// err handling
  err= 0;

  size= levSize[0];
  nrLevels= 1;

  levFrom[0]= 0;
  levSize[0]= size;

  uint8 *bm= new uint8[size];
  Str::memcpy(bm, bitmap, size);

  if(_wrap) {
    delete[] *wrapBitmap;
    *wrapBitmap= bm;
  } else
    delete[] bitmap;

  bitmap= bm;
  return true; // success
}





























#ifdef IMG_CLASS_USE_OPENGL
bool Tex::glGetID(int32 in_id, int32 in_type) {
  //cchar *func= "Tex::getID()";
  delData();
  err= 0;
  error.glFlushErrors();

  GLint g;                /// will hold temporary GL vars
  int32 savePackAlgn;
  int32 test, sx, sy;
  uint8 *bm= null;
  float brd[4];
  int dataFormat;
  int dataType;

  glGetIntegerv(GL_PACK_ALIGNMENT, &savePackAlgn);

  glBindTexture(in_type, in_id);
  if(error.glError()) return false;
  type= glGetType(in_type);
  
  //glEnable(type);
  
  glGetTexLevelParameteriv(in_type, 0, GL_TEXTURE_WIDTH,           (GLint *)&dx);
  glGetTexLevelParameteriv(in_type, 0, GL_TEXTURE_HEIGHT,          (GLint *)&dy);
  glGetTexLevelParameteriv(in_type, 0, GL_TEXTURE_DEPTH,           (GLint *)&depth);
  glGetTexLevelParameteriv(in_type, 0, GL_TEXTURE_INTERNAL_FORMAT, &g); format= glGetFormat(g);
  this->computePixelInfo();
  glGetTexLevelParameteriv(in_type, 0, GL_TEXTURE_COMPRESSED,      (GLint *)&compressed);

  glGetTexParameteriv(in_type, GL_TEXTURE_WRAP_S,       &g); wrapU= glGetWrap(g);
  glGetTexParameteriv(in_type, GL_TEXTURE_WRAP_T,       &g); wrapV= glGetWrap(g);
  glGetTexParameteriv(in_type, GL_TEXTURE_WRAP_R,       &g); wrapW= glGetWrap(g);
  
  glGetTexParameterfv(in_type, GL_TEXTURE_BORDER_COLOR, brd);     /// v1.3
  if(brd[3]<= 0.01f)                        border= ixBorderColor::FLOAT_TRANSPARENT_BLACK;
  else if((brd[0]<= 0.01f) && (brd[1]<= 0.01f) && (brd[2]<= 0.01f)) border= ixBorderColor::FLOAT_OPAQUE_BLACK;
  else                                      border= ixBorderColor::FLOAT_OPAQUE_WHITE;

  glGetTexParameteriv(in_type, GL_TEXTURE_SWIZZLE_R,    &g); swizzR= glGetSwizzle(g, GL_RED);   /// v1.3
  glGetTexParameteriv(in_type, GL_TEXTURE_SWIZZLE_G,    &g); swizzG= glGetSwizzle(g, GL_GREEN); /// v1.3
  glGetTexParameteriv(in_type, GL_TEXTURE_SWIZZLE_B,    &g); swizzB= glGetSwizzle(g, GL_BLUE);  /// v1.3
  glGetTexParameteriv(in_type, GL_TEXTURE_SWIZZLE_A,    &g); swizzA= glGetSwizzle(g, GL_ALPHA); /// v1.3
  //glGetTexParameteriv(in_type, GL_TEXTURE_COMPARE_MODE, &compareMode); /// v1.3  v1.4 DISABLED
  //glGetTexParameteriv(in_type, GL_TEXTURE_COMPARE_FUNC, &compareFunc); /// v1.3  v1.4 DISABLED
  if(error.glError()) { err= 26; goto Error; }

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  
  if(glHasMipmap(in_type))
    nrLevels= mipmapGetMaxLevels(this);
  else
    nrLevels= 1;

  levFrom[0]= 0;

  for(int a= 0; a< nrLevels; a++) {
    if(compressed)
      glGetTexLevelParameteriv(in_type, a, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (GLint *)&levSize[a]);
    else {
      glGetTexLevelParameteriv(in_type, a, GL_TEXTURE_WIDTH, &sx);
      glGetTexLevelParameteriv(in_type, a, GL_TEXTURE_HEIGHT, &sy);

      levSize[a]= sx* sy* nchannels;
    }

    if(a) levFrom[a]= levFrom[a- 1]- levSize[a- 1];
    size+= levSize[a];
  }
  if(error.glError()) { err= 26; goto Error; }

  
  if(_wrap) {
    *wrapBitmap= new uint8[size];
    bitmap= *wrapBitmap;
  } else
    bitmap= new uint8[size];
  bm= (uint8 *)bitmap;
  if(!bm) { err= 12; goto Error; }

  dataFormat= this->glGetGlFormat(format);
  dataType= this->glGetDataType(format);

  for(int a= 0; a< nrLevels; a++) {
    if(compressed) {
      glGetTexLevelParameteriv(in_type, a, GL_TEXTURE_COMPRESSED, &test);
      if(!test) { err= 15; return false; }
      glGetCompressedTexImage(in_type, a, bm+ levFrom[a]);
    } else {
      glGetTexImage(in_type, a, dataFormat,  dataType, bm+ levFrom[a]);
    }
  }
  
  
  if(error.glError()) { err= 26; goto Error; }

  glPixelStorei(GL_PACK_ALIGNMENT, savePackAlgn);
  return true; // success

Error:
  delData();
  
  error.simple(str8(__FUNCTION__)+ "() "+ getError());
  error.glError();
  glPixelStorei(GL_PACK_ALIGNMENT, savePackAlgn);
  return false;
}

// save a texture that is already created/stored in grcard
bool Tex::glSaveID(cchar *in_fname, int32 in_id, int32 in_type, int8 in_fCompression) {
  if(!glGetID(in_id, in_type)) return false;
  return save(in_fname, in_fCompression);
}


bool Tex::glGetTexture(ixTexture *t) {
  if(!t) return false;
  return glGetID(t->glData.id, Img::glGetGlType(t->type));
}


bool Tex::glSaveTexture(cchar *fname, ixTexture *t, int8 fileCompression) {
  if(!t) return false;
  if(!glGetID(t->glData.id, Img::glGetGlType(t->type))) return false;
  return save(fname, fileCompression);
}




bool Tex::glCompressImage(int32 in_format) {
  if(!dx) return false;
  if(_wrap) { if(!wrapBitmap) return false; if(!*wrapBitmap) return false; }
  else if(!bitmap) return false;
  if(compressed) return false;

  err= 0;
  error.glFlushErrors();

  GLint lpack, lunpack;
  uint8 *p= (uint8 *)bitmap;
  //uint8 *newBitmap= null;
  int32 target= glGetGlType(type);
  int32 internalFormat= glGetGlFormat(format);
  int32 dataType= glGetDataType(format);
  
  GLuint tid= 0;
  GLint newFormat;

  /// the new internal format of the texture
  if(in_format)
    newFormat= in_format;
  else {
    if(nchannels== 1) newFormat= GL_COMPRESSED_RED;
    else if(nchannels== 2) newFormat= GL_COMPRESSED_RG;
    else if(nchannels== 3) newFormat= GL_COMPRESSED_RGB;
    else if(nchannels== 4) newFormat= GL_COMPRESSED_RGBA;
    else { err= 14; goto Exit; }
  }

  /// pixel pack / unpack alignment (save current + new type)
  glGetIntegerv(GL_PACK_ALIGNMENT, &lpack);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &lunpack);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /// create & compress texture using oGL
  glGenTextures(1, &tid);
  glBindTexture(target, tid);
  if(error.glError(__FUNCTION__)) { err= 26; goto Exit; }

  for(int a= 0; a< nrLevels; a++)
         if(target== GL_TEXTURE_1D) glTexImage1D(target, 0, newFormat, dx, 0,            internalFormat, dataType, p);
    else if(target== GL_TEXTURE_2D) glTexImage2D(target, a, newFormat, dx, dy, 0,        internalFormat, dataType, p+ levFrom[a]);
    else if(target== GL_TEXTURE_3D) glTexImage3D(target, a, newFormat, dx, dy, depth, 0, internalFormat, dataType, p+ levFrom[a]);
  
  if(error.glError(__FUNCTION__)) { err= 26; goto Exit; }

  /// test to see if the texture was successfuly compressed
  glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED, (GLint *)&compressed);
  if(!compressed) { err= 26; goto Exit; }
  glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
  if(error.glError(__FUNCTION__)) { err= 26; goto Exit; }

  /// populate levSize & levFrom
  levFrom[0]= 0;
  size= 0;
  for(int a= 0; a< nrLevels; a++) {
    glGetTexLevelParameteriv(target, a, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (GLint *)&levSize[a]);

    if(a) 
      levFrom[a]= levFrom[a- 1]+ levSize[a- 1];

    size+= levSize[a];
  }

  /// get texture bitmap from OpenGL
  if(_wrap) {
    delete[] *wrapBitmap;
    *wrapBitmap= new uint8[size];
    bitmap= *wrapBitmap;
  } else {
    delete[] bitmap;
    bitmap= new uint8[size];
  }
  p= (uint8 *)bitmap;

  for(int a= 0; a< nrLevels; a++, p+= levSize[a])
    glGetCompressedTexImage(target, a, p);

  /// compute the rest of pixel data
  computePixelInfo(glGetFormat(internalFormat));

Exit:
  if(tid) glDeleteTextures(1, &tid);
  glPixelStorei(GL_PACK_ALIGNMENT, lpack);
  glPixelStorei(GL_UNPACK_ALIGNMENT, lunpack);

  if(err) {
    //delData();  // IN THEORY THE TEX SHOULD STILL BE VIABLE
    error.glError(__FUNCTION__);
    error.simple(str8(__FUNCTION__)+ "() ["+ fileName+ "] "+ getError());

    return false;
  } else
    return true;
}



//  DECOMPRESS image

bool Tex::glDecompressImage(ImgFormat in_f) {
  /// simple checks
  if(!dx) return false;
  if(_wrap) { if(!wrapBitmap) return false; if(!*wrapBitmap) return false; }
  else if(!bitmap) return false;
  if(!compressed) return false;

  /// error handling
  err= 0;
  error.glFlushErrors();
  
  /// tmp vars
  //uint8 *p;                           /// p will 'walk' the bitmap
  GLuint tid= 0;
  GLenum target= glGetGlType(type);
  int32 iformat, lpack, lunpack;
  int32 dataType= 0;

  /// save current pack/unpack alignments
  glGetIntegerv(GL_PACK_ALIGNMENT, &lpack);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &lunpack);

  iformat= glGetGlFormat(format);
  if(iformat== 0) { err= 14; goto Exit; }
  
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /// create & upload the compressed texture data using oGL
  glGenTextures(1, &tid);
  glBindTexture(target, tid);
  if(error.glError(__FUNCTION__)) { err= 26; goto Exit; }
  
  for(int a= 0; a< (int)nrLevels; a++)
         if(target== GL_TEXTURE_1D) glCompressedTexImage1D(target, a, iformat, (int32)dx,                          0, levSize[a], bitmap);
    else if(target== GL_TEXTURE_2D) glCompressedTexImage2D(target, a, iformat, (int32)dx, (int32)dy,               0, levSize[a], bitmap+ levFrom[a]);
    else if(target== GL_TEXTURE_3D) glCompressedTexImage3D(target, a, iformat, (int32)dx, (int32)dy, (int32)depth, 0, levSize[a], bitmap+ levFrom[a]);
  
  if(error.glError(__FUNCTION__)) { err= 26; goto Exit; }

  /// populate Tex internal vars
  format= (in_f== ImgFormat::UNDEFINED? compressedToUncompressed(format): in_f);
  if(format== ImgFormat::UNDEFINED) { err= 14; goto Exit; }
  computePixelInfo();
  compressed= false;

  iformat= glGetGlFormat(format);
  dataType= glGetDataType(format);

  /// populate levSize & levFrom
  levFrom[0]= 0;
  size= 0;
  for(int a= 0; a< nrLevels; a++) {
    int tx= 0, ty= 0;
    glGetTexLevelParameteriv(target, a, GL_TEXTURE_WIDTH, &tx);
    glGetTexLevelParameteriv(target, a, GL_TEXTURE_HEIGHT, &ty);
    levSize[a]= tx* ty* nchannels;

    if(a) 
      levFrom[a]= levFrom[a- 1]+ levSize[a- 1];

    size+= levSize[a];
  }

  /// aloc memory for new image
  if(_wrap) {
    delete[] *wrapBitmap;
    *wrapBitmap= new uint8[size];
    bitmap= *wrapBitmap;
  } else {
    delete[] bitmap;
    bitmap= new uint8[size];
  }
  if(!bitmap) { delData(); err= 12;  goto Exit; }

  // opengl will auto-decompress and store each level in bm
  for(int a= 0; a< nrLevels; a++)
    glGetTexImage(target, a, iformat, dataType, bitmap+ levFrom[a]);


Exit:
  if(tid) glDeleteTextures(1, &tid);
  glPixelStorei(GL_PACK_ALIGNMENT, lpack);
  glPixelStorei(GL_UNPACK_ALIGNMENT, lunpack);

  if(err) {
    error.glError(__FUNCTION__);
    error.simple(str8(__FUNCTION__)+ "() ["+ fileName+ "] "+ getError());

    return false; // failed
  } else
    return true;  // success
}


bool Tex::glMipmapGenerate() {
  /// simple checks
  if(nrLevels> 1) return true;
  if(type== Img::Type::T_1D) return true;
  if(_wrap) { if(!wrapBitmap) return false; if(!*wrapBitmap) return false; }
  else if(!bitmap) return false;
  if(!dx) return false;

  /// err handling
  err= 0;
  error.glFlushErrors();

  GLuint tid= 0;
  GLenum tgt= (depth> 1? GL_TEXTURE_3D: GL_TEXTURE_2D); // CUBE MAP CHANGE HERE<<<
  GLint lpack, lunpack;
  GLint internalFormat= Img::glGetGlFormat(format);
  GLint dataFormat= Img::glGetDataType(format);

  uint8 *bm= (uint8 *)bitmap;
  
  /// pixel pack / unpack alignment (save current + new type)
  glGetIntegerv(GL_PACK_ALIGNMENT, &lpack);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &lunpack);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glGenTextures(1, &tid);
  glBindTexture(tgt, tid);
  if(error.glError(__FUNCTION__)) { err= 26; goto Exit; }
  
  if(compressed) {
    if(tgt== GL_TEXTURE_2D) glCompressedTexImage2D(tgt, 0, internalFormat, dx, dy,        0, levSize[0], bm);
    if(tgt== GL_TEXTURE_3D) glCompressedTexImage3D(tgt, 0, internalFormat, dx, dy, depth, 0, levSize[0], bm);
  } else {
    if(tgt== GL_TEXTURE_2D) glTexImage2D(tgt, 0, internalFormat, dx, dy,        0, internalFormat, dataFormat, bm);
    if(tgt== GL_TEXTURE_3D) glTexImage3D(tgt, 0, internalFormat, dx, dy, depth, 0, internalFormat, dataFormat, bm);
  }
  glGenerateMipmap(tgt);

  nrLevels= mipmapGetMaxLevels(this);

  /// populate levSize / size
  levSize[0]= 0;
  levFrom[0]= 0;
  size= 0;

  for(uint a= 0; a< nrLevels; a++) {
    uint tx= 0, ty= 0, td= 0;
    if(compressed) {
      glGetTexLevelParameteriv(tgt, (GLint)a, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, (GLint *)&levSize[a]);
    } else {
      glGetTexLevelParameteriv(tgt, (GLint)a, GL_TEXTURE_WIDTH,  (GLint *)&tx);
      glGetTexLevelParameteriv(tgt, (GLint)a, GL_TEXTURE_HEIGHT, (GLint *)&ty);
      glGetTexLevelParameteriv(tgt, (GLint)a, GL_TEXTURE_DEPTH,  (GLint *)&td);
      
      levSize[a]= tx* ty* nchannels* td;
    }

    if(a) 
      levFrom[a]= levFrom[a- 1]+ levSize[a- 1];

    size+= levSize[a];
  }

  /// get texture and mipmap levels from OpenGL
  if(_wrap) {
    delete[] *wrapBitmap;
    *wrapBitmap= new uint8[size];
    bitmap= *wrapBitmap;
  } else {
    delete[] bitmap;
    bitmap= new uint8[size];
  }
  bm= (uint8 *)bitmap;

  for(uint a= 0; a< nrLevels; a++, bm+= levSize[a])
    if(compressed)
      glGetCompressedTexImage(tgt, (GLint)a, bm);
    else
      glGetTexImage(tgt, (GLint)a, internalFormat, dataFormat, bm);

Exit:
  glPixelStorei(GL_PACK_ALIGNMENT, lpack);
  glPixelStorei(GL_UNPACK_ALIGNMENT, lunpack);
  if(tid) glDeleteTextures(1, &tid);

  if(err) {
    error.glError(__FUNCTION__);
    error.simple(str8(__FUNCTION__)+ "() ["+ fileName+ "] "+ getError());
    errFileName= this->fileName;
    return false;
  }

  return true;
}


bool Tex::glHasMipmap(int in_tgt) {
  int a= 0;
  error.glFlushErrors();
  glGetTexLevelParameteriv(in_tgt, 1, GL_TEXTURE_WIDTH, &a);
  if((a== 0) || glGetError())
    return false;
  return true;
}
#endif // IMG_CLASS_USE_OPENGL













/*
// creates the texture in grCard then populates out_t - THERE CAN'T BE JUST ONE BUILD TEX (in texData)
bool Tex::glBuildTexture(ixTexture *out_t, bool in_mipmap, bool in_compressed, int in_magFilter, int in_minFilter, int8 in_anisotropy) {
  if(!bitmap) return false;
  if(!out_t) return false;
  
  build and populate must be done in vulkan i guess, to see the differences, and see the best way to do stuff
    opengl's only downfall is the single thread i guess, cuz else i'd still use it
    i like it better, i don't see any advantage of the vulkan


  _Tex::populateTexture(this, out_t, in_magFilter, in_minFilter, in_anisotropy);
  if(!_Tex::buildTexture(this, out_t, in_mipmap, in_compressed)) { out_t->delData(); return false; }
  
  out_t->setParams();
  return true;
}
*/






  

Img::Type _Tex::getFromGlType(int in_gl) {
  if(in_gl== 0x0DE0) return Img::Type::T_1D;
  if(in_gl== 0x0DE1) return Img::Type::T_2D;
  if(in_gl== 0x806F) return Img::Type::T_3D;
  return (Img::Type)~0u;
}

ImgFormat _Tex::getFromGlFormat(int in_gl) {
  
  if(in_gl== 0x1903) return ImgFormat::R8_UNORM;       // GL_RED
  if(in_gl== 0x8227) return ImgFormat::R8G8_UNORM;     // GL_RG
  if(in_gl== 0x1907) return ImgFormat::R8G8B8_UNORM;   // GL_RGB
  if(in_gl== 0x1908) return ImgFormat::R8G8B8A8_UNORM; // GL_RGBA
       
  if(in_gl== 0x8D62) return ImgFormat::R5G6B5_UNORM_PACK16; // GL_RGB565
  if(in_gl== 0x8057) return ImgFormat::R5G5B5A1_UNORM_PACK16; // GL_RGB5_A1
  if(in_gl== 0x8229) return ImgFormat::R8_UNORM;           // GL_R8
  if(in_gl== 0x822B) return ImgFormat::R8G8_UNORM;         // GL_RG8
  if(in_gl== 0x8051) return ImgFormat::R8G8B8_UNORM;       // GL_RGB8
  if(in_gl== 0x8058) return ImgFormat::R8G8B8A8_UNORM;     // GL_RGBA8
  // i doubt any other format type was made with old versions, so this should cover everything
  return (ImgFormat)~0u;
}

Img::Wrap _Tex::getFromGlWrap(int in_gl) {
  if(in_gl== 0x2901) return Img::Wrap::REPEAT;
  if(in_gl== 0x8370) return Img::Wrap::MIRRORED_REPEAT;
  if(in_gl== 0x8743) return Img::Wrap::MIRROR_CLAMP_TO_EDGE;
  if(in_gl== 0x812F) return Img::Wrap::CLAMP_TO_EDGE;
  if(in_gl== 0x812D) return Img::Wrap::CLAMP_TO_BORDER;
  return (Img::Wrap)~0u;
}

Img::Swizzle _Tex::getFromGlSwizzle(int in_gl, Img::Swizzle in_s) {
  Img::Swizzle r;
  if     (in_gl== 0x1903) r= Img::Swizzle::R;
  else if(in_gl== 0x1904) r= Img::Swizzle::G;
  else if(in_gl== 0x1905) r= Img::Swizzle::B;
  else if(in_gl== 0x1906) r= Img::Swizzle::A;
  else if(in_gl== 1)      r= Img::Swizzle::ONE;
  else if(in_gl== 0)      r= Img::Swizzle::ZERO;
  else                    r= (Img::Swizzle)~0u;
  if(r== in_s) r= Img::Swizzle::IDENTITY;
  return r;
}









///======================///
// Old file versions load //
///======================///


// old ver 1.3 load
bool _Tex::loadV13(cchar *fname, Tex *t) {
  t->delData();
  t->err= 0;
  t->fileName= fname;
  t->fileType= 0;

  cchar *v13= "TEX file V1.3";
  char buf[32];
  int32 tmp;
  float brd[4];

  FILE *f= fopen(fname, "rb");
  if(!f) { t->err= 8; goto Error;}

  tmp= (int32)Str::strlen8(v13);
  SFREAD(buf, tmp, 1, f)
  buf[tmp- 1]= 0;

  if(Str::strcmp8(v13, buf)) {
    fclose(f);
    return _Tex::loadV12(fname, t);        // try load v1.2
  }

  //int32 dummy;
  SFREAD(&tmp,         4, 1, f); t->type= getFromGlType(tmp); if(t->type== (Img::Type)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&t->dx,       4, 1, f);
  SFREAD(&t->dy,       4, 1, f);
  SFREAD(&t->depth,    4, 1, f);
  SFREAD(&t->size,     4, 1, f);
  SFREAD(&t->compressed, 4, 1, f);
  SFREAD(&tmp,         4, 1, f); t->format= getFromGlFormat(tmp); if(t->format== (ImgFormat)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&t->nrLevels, 2, 1, f);
  SFREAD(&tmp,         4, 1, f); /// magFilter 1.4 disabled
  SFREAD(&tmp,         4, 1, f); /// minFilter 1.4 disabled
  SFREAD(&tmp,         4, 1, f); t->wrapU= getFromGlWrap(tmp); if(t->wrapU== (Img::Wrap)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&tmp,         4, 1, f); t->wrapV= getFromGlWrap(tmp); if(t->wrapV== (Img::Wrap)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&tmp,         4, 1, f); t->wrapW= getFromGlWrap(tmp); if(t->wrapW== (Img::Wrap)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(brd,          4, 4, f); /// 1.3 - texture border RGBAf
  if(brd[3]== 0.0f)
    t->border= ixBorderColor::FLOAT_TRANSPARENT_BLACK;
  else if((brd[0]<= 0.01f) && (brd[1]<= 0.01f) && (brd[2]<= 0.01f))
    t->border= ixBorderColor::FLOAT_OPAQUE_BLACK;
  else
    t->border= ixBorderColor::FLOAT_OPAQUE_WHITE;
  SFREAD(&tmp,         4, 1, f); t->swizzR= getFromGlSwizzle(tmp, Img::Swizzle::R); /// 1.3 - texture swizzle
  SFREAD(&tmp,         4, 1, f); t->swizzG= getFromGlSwizzle(tmp, Img::Swizzle::G); /// 1.3 - texture swizzle
  SFREAD(&tmp,         4, 1, f); t->swizzB= getFromGlSwizzle(tmp, Img::Swizzle::B); /// 1.3 - texture swizzle
  SFREAD(&tmp,         4, 1, f); t->swizzA= getFromGlSwizzle(tmp, Img::Swizzle::A); /// 1.3 - texture swizzle
  if((t->swizzR== (Img::Swizzle)~0u) || (t->swizzG== (Img::Swizzle)~0u) ||
     (t->swizzB== (Img::Swizzle)~0u) || (t->swizzA== (Img::Swizzle)~0u))
  { t->err= 14; goto LoadFail; }
  SFREAD(&tmp, 4, 1, f); /// 1.3 - compare mode     DISABLED 1.4
  SFREAD(&tmp, 4, 1, f); /// 1.3 - compare function DISABLED 1.4
  SFREAD(t->levSize, 4, t->nrLevels, f);
  SFREAD(t->levFrom, 4, t->nrLevels, f);

  /* 1.4 disabled
  if(nrLevels>1) mipmapFLAG= true;
  else		       mipmapFLAG= false;
  */

  if(t->_wrap) {
    *t->wrapBitmap= new uint8[t->size];
    t->bitmap= *t->wrapBitmap;
  } else
    t->bitmap= new uint8[t->size];
  if(!t->bitmap) { t->err= 12; goto LoadFail; }

  SFREAD(t->bitmap, 1, t->size, f);
  fclose(f);

  t->computePixelInfo();

  return true; // success

LoadFail:
  fclose(f);
  t->delData();
Error:
  t->errFileName= fname;
  error.simple(str8(__FUNCTION__)+ "() ["+ fname+ "] "+ t->getError());
  return false;
}

// VERSION 1.2 load
bool _Tex::loadV12(cchar *fname, Tex *t) {
  if(!t) return false;
  t->delData();
  t->err= 0;
  t->fileName= fname;
  t->fileType= 0;

  cchar *v12= "TEX file V1.2";
  char buf[32];
  int32 tmp;

  FILE *f= fopen(fname, "rb");
  if(!f) { t->err= 8; goto Error; }

  tmp= (int)Str::strlen8(v12);
  SFREAD(buf, tmp, 1, f)
  buf[tmp- 1]= 0;

  if(Str::strcmp8(v12, buf)) { t->err= 14; goto LoadFail; } // unknown file if reached this point

  SFREAD(&t->type, 4, 1, f); t->type= getFromGlType(tmp); if(t->type== (Img::Type)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&t->dx,         4, 1, f);
  SFREAD(&t->dy,         4, 1, f);
  SFREAD(&t->depth,      4, 1, f);
  SFREAD(&t->size,       4, 1, f);
  SFREAD(&t->compressed, 4, 1, f);
  SFREAD(&tmp,           4, 1, f); t->format= getFromGlFormat(tmp); if(t->format== (ImgFormat)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&t->nrLevels,   2, 1, f);
  SFREAD(&tmp,           4, 1, f); /// magFilter 1.4 disabled
  SFREAD(&tmp,           4, 1, f); /// minFilter 1.4 disabled
  SFREAD(&tmp,           4, 1, f); t->wrapU= getFromGlWrap(tmp); if(t->wrapU== (Img::Wrap)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&tmp,           4, 1, f); t->wrapV= getFromGlWrap(tmp); if(t->wrapV== (Img::Wrap)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(&tmp,           4, 1, f); t->wrapW= getFromGlWrap(tmp); if(t->wrapW== (Img::Wrap)~0u) { t->err= 14; goto LoadFail; }
  SFREAD(t->levSize, 4, t->nrLevels, f);
  SFREAD(t->levFrom, 4, t->nrLevels, f);

  /* 1.4 disabled
  if(nrLevels> 1) mipmapFLAG= true;
  else            mipmapFLAG= false;
  */

  if(t->_wrap) {
    *t->wrapBitmap= new uint8[t->size];
    t->bitmap= *t->wrapBitmap;
  } else
    t->bitmap= new uint8[t->size];

  if(!t->bitmap) { t->err= 12; goto LoadFail; }
  SFREAD(t->bitmap, 1, t->size, f);
  fclose(f);

  t->computePixelInfo();

  return true;

LoadFail:
  fclose(f);
  t->delData();
Error:
  t->errFileName= fname;
  error.simple(str8(__FUNCTION__)+ "() ["+ fname+ "] "+ t->getError());
  return false;
}









