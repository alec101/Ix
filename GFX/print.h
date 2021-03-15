#pragma once

#ifndef IX_TXT_ORIENTATION_DEFINED
#define IX_TXT_ORIENTATION_DEFINED 1
#define IX_TXT_RIGHT      0x01
#define IX_TXT_LEFT       0x02
#define IX_TXT_DOWN       0x04
#define IX_TXT_UP         0x08
#define IX_TXT_HORIZONTAL 0x03      // IX_TXT_RIGHT and IX_TXT_LEFT bytes form IX_TXT_HORIZONTAL - used to know if text is horizontal in orientation
#define IX_TXT_VERTICAL   0x0C      // IX_TXT_DOWN  and IX_TXT_UP bytes   form IX_TXT_VERTICAL   - used to know if text is vertical in orientation
#endif



using namespace mlib;

class _ixFont;        /// [internal] font 'core'; this chainList is in Print class.
class _ixFSize;       /// [internal] all sizes in _ixFont
class _ixFPage;       /// [internal] all pages in _SizeList
class _ixFChar;       /// [internal] all chars in _Page
class ixPrint;
class ixShader;
class ixvkBuffer;

class _ixM3;
class _ixM5;
class _ixM5U;
int32 _getBytesMaxPixels(const void *txt, int utfType, int maxPixels, void *font, float spaceSize= 0.0f);
int32 _getGlyphsMaxPixels(bool unicodes, const void *txt, int utfType, int maxPixels, void *font, float spaceSize= 0.0f, int8 orientation= IX_TXT_RIGHT);
int32 _getTextLen(const void *txt, int utfType, bool unicodes, int32 nrGlyphs, void *font, float spaceSize= 0.0f, int8 orientation= IX_TXT_RIGHT);


class ixFontStyle {
public:
  void *selFont;        // printing font
  int8 dblPrecision;    // how many decimals after comma will be printed [0- 19 everything out of these bounds causes error]
  int8 orientation;     // [def:IX_TXT_RIGHT] text orientation, can be one of 4 cardinal orientations, check top of header for all defines

  vec4 color1;          // main text color, default white (1, 1, 1, 1)
  vec4 color2;          // shadow/outline color, default black (0, 0, 0, 1)

  int16 drawMode;       // [bit 0]= outline, [bit 1]= background shadow, [bit 2]= solid background
  vec2 shadowPos;       // shadow position when (drawmode& 0x0002)
  int outlineSize;      // [1-5] outline pixels, maximum 5 pixels when (drawmode& 0x0001)

  float spaceSize;      // [def:0.0f] when left default, it's the font's default space char size, else this value is used to delimit characters

  void operator=(ixFontStyle &s) { selFont= s.selFont, dblPrecision= s.dblPrecision, orientation= s.orientation, color1= s.color1, color2= s.color2, drawMode= s.drawMode, shadowPos= s.shadowPos, outlineSize= s.outlineSize, spaceSize= s.spaceSize; }

  ixFontStyle() { selFont= null; restoreDefaults(); }
  void restoreDefaults() { dblPrecision= 2; orientation= IX_TXT_RIGHT; color1.set(1.0f, 1.0f, 1.0f, 1.0f); drawMode= 0; color2.set(0.0f, 0.0f, 0.0f, 1.0f); shadowPos= vec2(2.0f, 2.0f); spaceSize= 0.0f; outlineSize= 0; }
protected:
  
};

extern ixFontStyle ixDefFontStyle;





class ixPrintShader: public ixShader {
public:

  #ifdef IX_USE_OPENGL
  int u_camera, u_color, u_viewportPos;
  int u_clip;                   // bool, enable/disable
  int u_clip0, u_clipE;         // clip rectangle 0= start, e= end - no need for start point to be lower then end point
  virtual void initUniforms();
  #endif

  virtual void printChar(uint32)= 0;
  virtual void txt(const void *, int, int, int, int, int)= 0;

  ixPrintShader(Ix *in_ix);

protected:
  ixPrint *_print;
  void *_prevTex;

  virtual void _prePrintInit()= 0;
  virtual void _afterPrint()= 0;
  virtual void _create(_ixFPage *)= 0;
  virtual void _delete(_ixFPage *)= 0;
  friend class ixPrint;
  friend class _ixFPage;
};






// PRINT class
///===================================================================

class ixPrint {
public:
  //void *selFont;        // printing font
  vec3 pos;             // cursor position
  //float spaceSize;      // [def:0.0f] when left default, it's the font's default space char size, else this value is used to delimit characters
  ixFontStyle *style;

  // font loading - check further down for a list of all pages that can be used

  void *loadFont(cchar *name, int size= 0, int16 page= 0);  /// returns an id for the loaded font, or null if failed. [size 0]: fnt- load the first size; ttf- size 12(?) !!! page 0 will be loaded even if a different page is specified
  void *loadFont(cchar *name, int size, cchar *pageDesc);   /// uses page descriptions instead to load a font (ex. "Latin Extended-A")
  void reloadAllInvalid();  // check validity of a texture and reloads it if neccesary

  // font unloading

  void delPage(void *font, int size, int16 page);   // use to delete a loaded font page
  void delSize(void *font, int size);               // use to delete a whole font size (all pages)
  void delFont(void *font);                         // use to delete a whole font

  // font selection

  void setFont(void *f) { style->selFont= f; }        // select a font to print with
  void *setFont(cchar *name, int size);               // select an already loaded font & size; returns font's id or NULL if failed
  void *getFont(cchar *name, int size);               // returns a font ID for the specified size & font (font must be already loaded)

  chainList fonts;       // chain list with all loaded fonts (_ixFont is data)

  // printing format funcs

  void setDblPrecision(int8 n) { style->dblPrecision= (n>= 0 && n<= 19? n: 2); }  // set the double and float number print precision (default 2)
  void setOrientation(int8 o)  { style->orientation= o; }   // [def:IX_TXT_RIGHT] text orientation, can be one of 4 cardinal orientations, check top of header for all defines
  int8 getDblPrecision() { return style->dblPrecision; }
  int8 getOrientation() { return style->orientation; }

  void saveStyle() { _saveStyle= *style; }          // saves current style (a fast push basically)
  void restoreStyle() { *style= _saveStyle; }       // restores current style (a fast pop basically)

  // cursor position funcs

  void setPos2(float x, float y)          { pos.x= x; pos.y= y; }           // cursor position
  void setPos3(float x, float y, float z) { pos.x= x; pos.y= y; pos.z= z; } // cursor position, z coordonate too

  // MAIN TEXT PRINTING FUNCTION - just pass a utf-8/16/32 string to print

  inline void txt(cchar *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)                             { _shader->txt(s, 8, start, end, startBytes, endBytes);  }        // MAIN PRINT FUNCTION set _txtFunc to point to a different printing method
  inline void txt2f(float x, float y, cchar *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)          { pos.x= x; pos.y= y; txt(s, start, end, startBytes, endBytes); }
  inline void txt2i(int32 x, int32 y, cchar *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)          { pos.x= (float)x; pos.y= (float)y; txt(s, start, end, startBytes, endBytes); }
  inline void txt3f(float x, float y, float z, cchar *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF) { pos.x= x; pos.y= y; pos.z= z; txt(s, start, end, startBytes, endBytes); }
  inline void txt3i(int32 x, int32 y, int32 z, cchar *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF) { pos.x= (float)x; pos.y= (float)y; pos.z= (float)z; txt(s, start, end, startBytes, endBytes); }

  inline void txt16(cchar16 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)                              { _shader->txt(s, 16, start, end, startBytes, endBytes); }   // MAIN PRINT FUNCTION set _txtFunc to point to a different printing method
  inline void txt16_2f(float x, float y, cchar16 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)          { pos.x= x; pos.y= y; txt16(s, start, end, startBytes, endBytes); }
  inline void txt16_2i(int32 x, int32 y, cchar16 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)          { pos.x= (float)x; pos.y= (float)y; txt16(s, start, end, startBytes, endBytes); }
  inline void txt16_3f(float x, float y, float z, cchar16 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF) { pos.x= x; pos.y= y; pos.z= z; txt16(s, start, end, startBytes, endBytes); }
  inline void txt16_3i(int32 x, int32 y, int32 z, cchar16 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF) { pos.x= (float)x; pos.y= (float)y; pos.z= (float)z; txt16(s, start, end, startBytes, endBytes); }

  inline void txt32(cchar32 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)                              { _shader->txt(s, 32, start, end, startBytes, endBytes); }  // MAIN PRINT FUNCTION set _txtFunc to point to a different printing method
  inline void txt32_2f(float x, float y, cchar32 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)          { pos.x= x; pos.y= y; txt32(s, start, end, startBytes, endBytes); }
  inline void txt32_2i(int32 x, int32 y, cchar32 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF)          { pos.x= (float)x; pos.y= (float)y; txt32(s, start, end, startBytes, endBytes); }
  inline void txt32_3f(float x, float y, float z, cchar32 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF) { pos.x= x; pos.y= y; pos.z= z; txt32(s, start, end, startBytes, endBytes); }
  inline void txt32_3i(int32 x, int32 y, int32 z, cchar32 *s, int start= 0, int end= 0xFFFFFFF, int startBytes= 0, int endBytes= 0xFFFFFFF) { pos.x= (float)x; pos.y= (float)y; pos.z= (float)z; txt32(s, start, end, startBytes, endBytes); }

  // print numbers, ! faster than using format funcs !

  void nint(int64);                     // INTEGER PRINT FUNCTION
  void nint2(float x, float y, int64 n)          { pos.x= x; pos.y= y; nint(n); }
  void nint3(float x, float y, float z, int64 n) { pos.x= x; pos.y= y; pos.z= z; nint(n); }
  void ndouble(double);                 // DOUBLE PRINT FUNCTION
  void ndouble2(float x, float y, double n)          { pos.x= x; pos.y= y; ndouble(n); }
  void ndouble3(float x, float y, float z, double n) { pos.x= x; pos.y= y; pos.z= z; ndouble(n); }

  // print text, using C format - SLOW FUNCTIONS
  
  void f(cchar *, ...);                 // FORMATED PRINT function
  void f2(float x, float y, cchar *s, ...);
  void f3(float x, float y, float z, cchar *s, ...);
  
  // utils

  static int32 getCharDx(uint32, void *f);       // returns the width(dx), in pixels, of a specific character
  static int32 getCharDy(void *);                // returns the size(dy), in pixels, of the specified font
  /// if <spaceSize> is 0.0f, font default is used for the size of the space character (used for wrapping)
  /// returns the size(dx), in pixels, of the specified utf-8 text,  using provided font. and maximum characters
  inline static int32 getTextLen  (cchar   *txt, int32 nrChars= 0, void *f= null, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getTextLen(txt,  8, false, nrChars, f, spaceSize, o); }
  inline static int32 getTextLen16(cchar16 *txt, int32 nrChars= 0, void *f= null, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getTextLen(txt, 16, false, nrChars, f, spaceSize, o); }
  inline static int32 getTextLen32(cchar32 *txt, int32 nrChars= 0, void *f= null, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getTextLen(txt, 32, false, nrChars, f, spaceSize, o); }
  /// returns the size(dx), in pixels, of the specified utf-8 text,  using provided font. and maximum unicodes
  inline static int32 getTextLenu  (cchar   *txt, int32 nrUnicodes= 0, void *f= null, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getTextLen(txt,  8, true, nrUnicodes, f, spaceSize, o); }
  inline static int32 getTextLenu16(cchar16 *txt, int32 nrUnicodes= 0, void *f= null, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getTextLen(txt, 16, true, nrUnicodes, f, spaceSize, o); }
  inline static int32 getTextLenu32(cchar32 *txt, int32 nrUnicodes= 0, void *f= null, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getTextLen(txt, 32, true, nrUnicodes, f, spaceSize, o); }
  /// returns maximum number of bytes in the txt that can be printed and fit into maxPixels (dx)
  inline static int32 getBytesMaxPixels  (cchar   *txt, int maxPixels, void *font, float spaceSize= 0.0f) { return _getBytesMaxPixels(txt,  8, maxPixels, font, spaceSize); }
  inline static int32 getBytesMaxPixels16(cchar16 *txt, int maxPixels, void *font, float spaceSize= 0.0f) { return _getBytesMaxPixels(txt, 16, maxPixels, font, spaceSize); }
  /// returns maximum number of chars in the txt that can be printed and fit into maxPixels (dx)
  inline static int32 getCharsMaxPixels  (cchar   *txt, int maxPixels, void *font, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getGlyphsMaxPixels(false, txt,  8, maxPixels, font, spaceSize, o); }
  inline static int32 getCharsMaxPixels16(cchar16 *txt, int maxPixels, void *font, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getGlyphsMaxPixels(false, txt, 16, maxPixels, font, spaceSize, o); }
  inline static int32 getCharsMaxPixels32(cchar32 *txt, int maxPixels, void *font, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getGlyphsMaxPixels(false, txt, 32, maxPixels, font, spaceSize, o); }
  /// returns maximum number of unicodes in the txt that can be printed and fit into maxPixels (dx)
  inline static int32 getUnicodesMaxPixels  (cchar   *txt, int maxPixels, void *font, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getGlyphsMaxPixels(true, txt,  8, maxPixels, font, spaceSize, o); }
  inline static int32 getUnicodesMaxPixels16(cchar16 *txt, int maxPixels, void *font, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getGlyphsMaxPixels(true, txt, 16, maxPixels, font, spaceSize, o); }
  inline static int32 getUnicodesMaxPixels32(cchar32 *txt, int maxPixels, void *font, float spaceSize= 0.0f, int8 o= IX_TXT_RIGHT) { return _getGlyphsMaxPixels(true, txt, 32, maxPixels, font, spaceSize, o); }



  // scissor funcs - print within a boundary, everything ouside is cut

  //inline void delScissor() { _shader->delScissor(); }
  //inline void setScissor(recti *in_size) { _shader->setScissor(in_size); }
  //inline void getScissor(recti *out_r) { _shader->getScissor(out_r); }

  // constructor / destructor

  ixPrint();
  ~ixPrint();
  void delData();              /// deallocs everything, unloads all fonts and textures, sets everything to default - can be useful
  void init();                  // called on ix::init() - loads shaders - performs any one-time init once the graphics is setup


  ixPrintShader *_shader;
  bool justDraw;

private:

  Ix *_ix;
  
  ixFontStyle _saveStyle;     // used for push/pop or save/restore of the font style



  // internal search funcs


  _ixFont *_getFont(cchar *name);
  //_ixFSize *_getSize(cchar *name, int size);
  _ixFSize *_getSizep(_ixFont *font, int size);
  //_ixFPage *_getPage(cchar *name, int size, int16 page);
  _ixFPage *_getPagep(_ixFSize *size, int16 page);  
  //_ixFPage *_getPagec(void *font, uint32 unicode);       /// returns what page a unicode character belongs to

  // internal loading funcs. use loadFont(), which in turn uses 1 of these
  
  void *_loadFNT(cchar *name, int size= 0, int16 page= 0);    /// creates font from a fnt file
  void *_loadTTF(cchar *name, int size= 0, int16 page= 0);    /// creates font from a ttf file

  friend class Ix;
  friend class _ixM3;
  friend class _ixM5;
  friend class _ixM5U;
  friend class _ixFPage;
};










// internal stuff from here on; 

class _ixFont: public chainData {

public:
  str8 name;
  int16 type;         /// 1= pre rendered font; 2= ttf/ otf, NEED RENDERER
  chainList sizes;    /// chainlist with all sizes (_Sizes is data)

};

/// sizes are in fonts
class _ixFSize: public chainData {
public:
  _ixFont *font;      /// points to it's parent font
  int16 size;         /// the actual size value
  str8 fileName;      /// file it was loaded from
  
  chainList pages;    /// chainList with all pages (_Pages is data)

  _ixFSize(): font(null), size(0) {}
};

/// chars are in pages
class _ixFChar {
public:
  float texX0, texY0; /// position on texture - start points (glTexcoord)
  float texXe, texYe; /// position on texture - end points (glTexcoord)

  int16 dx, dy;       /// character width / height in pixels (hopefully, dy never exceeds font size, or im screwed)

  int16 start;        /// point where to start drawing the glyph (in Windows structure ABC, this would be A)
  int16 end;          /// point where next character starts (in windows structure this would be A+B+C)
};

/// pages are in sizes
class _ixFPage: public chainData {
public:
  int16 id;             /// id of the page or 'character set' <<--- THIS MIGHT BE CHANGED TO A POINTER, or source might be a pointer
  _ixFChar *ch;         /// list with all chars (no need for a chainList)
  
  ixTexture *tex;       /// texture of the page
  uint16 texDx, texDy;  /// texture size

  _ixFSize *size;       /// parent size

  _ixFPage(ixPrint *parent); 
  ~_ixFPage() { delData(); }

  void delData(); // { id= 0; if(ch) delete[] ch; ch= null; if(osi.glr) { if(tex) glDeleteTextures(1, &tex); tex= 0; texDx= texDy= 0; } size= null; _ixM5::VBOdelete(this); _ixM5U::UBOdelete(this); _M5texPointer= 0; }

  ixPrint *_print;

  #ifdef IX_USE_OPENGL
  /// method5 printing data
  GLuint _M5UUBOid;
  GLuint _M5VBOid;
  uint64 _M5texPointer;
  #endif

  #ifdef IX_USE_VULKAN
  ixvkBuffer *data;
  VkoDynamicSet *set;
  #endif

};






// this structure holds all unicode pages that Print uses; check the list further down, when loading and creating a font
struct _ixPagesList {
  uint32 min, max;    /// unicode start char, unicode end char
  str8 name;          /// page description
  uint8 nrParts;      /// page can be on multiple page parts, that need to be all loaded
  static int16 getPage(cchar *name); /// searches pages for specified name and returns it's id; returns -1 if not found
};


/// this is a static const, not changed at all list, so thread safe when read by multiple threads
extern _ixPagesList pagesList[];
extern const str8 fontFileVer;




// Unicode pages list, used by Print class - http://www.alanwood.net/unicode/unicode_samples.html

/*

when LOADING A FONT, use either p[nnn] for a page number, or the page description (ex. "Latin Extended-A")

p[000] 0x0000-0x007F, "C0 Controls and Basic Latin"     // when loading a font, this page will be AUTO-LOADED if not loaded first
p[001] 0X0080-0x00FF, "C1 Controls and Latin-1 Supplement"
p[002] 0x0100-0x017F, "Latin Extended-A"
p[003] 0x0180-0x024F, "Latin Extended-B"
p[004] 0x0250-0x02AF, "IPA Extensions"
p[005] 0x02B0-0x02FF, "Spacing Modifier Letters"       // some arrows in here, some table building stuff too
p[006] 0x0300-0x036F, "Combining Diacritical Marks"
p[007] 0x0370-0x03FF, "Greek and Coptic"               // (Hellenic Republic - greece i guess)
p[008] 0x0400-0x04FF, "Cyrillic"
p[009] 0x0500-0x052F, "Cyrillic Supplement"
p[010] 0x0530-0x058F, "Armenian"                       // (Armenia)
p[011] 0x0590-0x05FF, "Hebrew"                         // (Israel)
p[012] 0x0600-0x06FF, "Arabic"
p[013] 0x0700-0x074F, "Syriac"
p[014] 0x0750-0x077F, "Arabic Supplement"
p[015] 0x0780-0x07BF, "Thaana"                         // (Maldives)
p[016] 0x07C0-0x07FF, "N’Ko"
p[017] 0x0800-0x083F, "Samaritan"
p[018] 0x0840-0x085F, "Mandaic"
p[019] 0x08A0-0x08FF, "Arabic Extended-A"
p[020] 0x0900-0x097F, "Devanagari (India)"
p[021] 0x0980-0x09FF, "Bengali"                        // (Bangladesh / India)
p[022] 0x0A00-0x0A7F, "Gurmukhi"                       // (India / Pakistan)
p[023] 0x0A80-0x0AFF, "Gujarati"                       // (India)
p[024] 0x0B00-0x0B7F, "Oriya"                          // (India)
p[025] 0x0B80-0x0BFF, "Tamil"                          // (India / Sri Lanka)
p[026] 0x0C00-0x0C7F, "Telugu"                         // (India)
p[027] 0x0C80-0x0CFF, "Kannada"                        // (India)
p[028] 0x0D00-0x0D7F, "Malayalam"                      // (India)
p[029] 0x0D80-0x0DFF, "Sinhala"                        // (Sri Lanka)
p[030] 0x0E00-0x0E7F, "Thai"                           // (Thailand)
p[031] 0x0E80-0x0EFF, "Lao (Lao people's republic)"
p[032] 0x0F00-0x0FFF, "Tibetan"
p[033] 0x1000-0x109F, "Myanmar"                        // (Myanmar)
p[034] 0x10A0-0x10FF, "Georgian"                       // (Georgia)
p[035] 0x1100-0x11FF, "Hangul Jamo"                    // (Korea)
p[036] 0x1200-0x137F, "Ethiopic"                       // (Ethiopia)
p[037] 0x1380-0x139F, "Ethiopic Supplement"            // (Ethiopia)
p[038] 0x13A0-0x13FF, "Cherokee"
p[039] 0x1400-0x167F, "Unified Canadian Aboriginal Syllabics"
p[040] 0x1680-0x169F, "Ogham"
p[041] 0x16A0-0x16FF, "Runic"
p[042] 0x1700-0x171F, "Tagalog"                        // (Philippines)
p[043] 0x1720-0x173F, "Hanunóo"                        // (Philippines)
p[044] 0x1740-0x175F, "Buhid"                          // (Philippines)
p[045] 0x1760-0x177F, "Tagbanwa"                       // (Philippines)
p[046] 0x1780-0x17FF, "Khmer"                          // (Cambodia)
p[047] 0x1800-0x18AF, "Mongolian"
p[048] 0x18B0-0x18FF, "Unified Canadian Aboriginal Syllabics Extended"
p[049] 0x1900-0x194F, "Limbu"                          // (India / Nepal)
p[050] 0x1950-0x197F, "Tai Le"
p[051] 0x1980-0x19DF, "New Tai Lue"
p[052] 0x19E0-0x19FF, "Khmer Symbols"                  // (Cambodia)
p[053] 0x1A00-0x1A1F, "Buginese"                       // (Indonesia)
p[054] 0x1A20-0x1AAF, "Tai Tham"
p[055] 0x1B00-0x1B7F, "Balinese"                       // (Indonesia)
p[056] 0x1B80-0x1BBF, "Sundanese"                      // (Indonesia)
p[057] 0x1BC0-0x1BFF, "Batak"                          // (Indonesia)
p[058] 0x1C00-0x1C4F, "Lepcha"                         // (India)
p[059] 0x1C50-0x1C7F, "Ol Chiki"
p[060] 0x1CC0-0x1CCF, "Sundanese Supplement"
p[061] 0x1CD0-0x1CFF, "Vedic Extensions"
p[062] 0x1D00-0x1D7F, "Phonetic Extensions"            // (some latins)
p[063] 0x1D80-0x1DBF, "Phonetic Extensions Supplement" // (some latins)
p[064] 0x1DC0-0x1DFF, "Combining Diacritical Marks Supplement"
p[065] 0x1E00-0x1EFF, "Latin Extended Additional"
p[066] 0x1F00-0x1FFF, "Greek Extended"                 // (Hellenic republic)
p[067] 0x2000-0x206F, "General Punctuation"
p[068] 0x2070-0x209F, "Superscripts and Subscripts"
p[069] 0x20A0-0x20CF, "Currency Symbols"               // (NOT ALL IN HERE, check further down)
p[070] 0x20D0-0x20FF, "Combining Diacritical Marks for Symbols"
p[071] 0x2100-0x214F, "Letterlike Symbols"
p[072] 0x2150-0x218F, "Number Forms"
p[073] 0x2190-0x21FF, "Arrows"                         // (lots!)
p[074] 0x2200-0x22FF, "Mathematical Operators"
p[075] 0x2300-0x23FF, "Miscellaneous Technical"
p[076] 0x2400-0x243F, "Control Pictures"
p[077] 0x2440-0x245F, "Optical Character Recognition"
p[078] 0x2460-0x24FF, "Enclosed Alphanumerics"
p[079] 0x2500-0x257F, "Box Drawing"                    // (!!!!! TABELS !!!!! what ascii extended was)
p[080] 0x2580-0x259F, "Block Elements"                 // ( !!!!! what ascii extended was)
p[081] 0x25A0-0x25FF, "Geometric Shapes"               // ( o.O )
p[082] 0x2600-0x26FF, "Miscellaneous Symbols"          // (stuff)
p[083] 0x2700-0x27BF, "Dingbats"                       // (stuff)
p[084] 0x27C0-0x27EF, "Miscellaneous Mathematical Symbols-A"
p[085] 0x27F0-0x27FF, "Supplemental Arrows-A"
p[086] 0x2800-0x28FF, "Braille Patterns"               // (blind ppl stuff)
p[087] 0x2900-0x297F, "Supplemental Arrows-B"
p[088] 0x2980-0x29FF, "Miscellaneous Mathematical Symbols-B"
p[089] 0x2A00-0x2AFF, "Supplemental Mathematical Operators"
p[090] 0x2B00-0x2BFF, "Miscellaneous Symbols and Arrows"
p[091] 0x2C00-0x2C5F, "Glagolitic"
p[092] 0x2C60-0x2C7F, "Latin Extended-C"
p[093] 0x2C80-0x2CFF, "Coptic"
p[094] 0x2D00-0x2D2F, "Georgian Supplement"            // (Georgia)
p[095] 0x2D30-0x2D7F, "Tifinagh"                       // (Morocco)
p[096] 0x2D80-0x2DDF, "Ethiopic Extended"              // (Ethiopia)
p[097] 0x2DE0-0x2DFF, "Cyrillic Extended-A"
p[098] 0x2E00-0x2E7F, "Supplemental Punctuation"
p[099] 0x2E80-0x2EFF, "CJK Radicals Supplement)"       // (CHINESE, JAPANESE, KOREAN
p[100] 0x2F00-0x2FDF, "KangXi Radicals"
p[101] 0x2FF0-0x2FFF, "Ideographic Description characters"
p[102] 0x3000-0x303F, "CJK Symbols and Punctuation"    // (CHINESE, JAPANESE, KOREAN)
p[103] 0x3040-0x309F, "Hiragana"                       // (Japan)
p[104] 0x30A0-0x30FF, "Katakana"                       // (Japan)
p[105] 0x3100-0x312F, "Bopomofo"
p[106] 0x3130-0x318F, "Hangul Compatibility Jamo"      // (Korea)
p[107] 0x3190-0x319F, "Kanbun"
p[108] 0x31A0-0x32BF, "Bopomofo Extended"
p[109] 0x31F0-0x31FF, "Katakana Phonetic Extensions"   // (Japan)
p[110] 0x3200-0x32FF, "Enclosed CJK Letters and Months"
p[111] 0x3300-0x33FF, "CJK Compatibility"
p[112] 0x3400-0x37FF, "CJK Unified Ideographs Extension A"
p[113] 0x4DC0-0x4DFF, "Yijing Hexagram Symbols"
p[114] 0x4E00-0x51FF, "CJK Unified Ideographs"
p[115] 0xA000-0xA3FF, "Yi Syllables"
p[116] 0xA490-0xA4CF, "Yi Radicals"
p[117] 0xA4D0-0xA4FF, "Lisu"
p[118] 0xA500-0xA63F, "Vai"                           // (Liberia)
p[119] 0xA640-0xA69F, "Cyrillic Extended-B"
p[120] 0xA6A0-0xA6FF, "Bamum"                         // (Cameroon)
p[121] 0xA700-0xA71F, "Modifier Tone Letters"
p[122] 0xA720-0xA7FF, "Latin Extended-D"
p[123] 0xA800-0xA82F, "Syloti Nagri"                  // (Bangladesh / India)
p[124] 0xA830-0xA83F, "Common Indic Number Forms"
p[125] 0xA840-0xA87F, "Phags-pa"
p[126] 0xA880-0xA8DF, "Saurashtra"                    // (India)
p[127] 0xA8E0-0xA8FF, "Devanagari Extended"
p[128] 0xA900-0xA92F, "Kayah Li"
p[129] 0xA930-0xA95F, "Rejang"                        // (Indonesia)
p[130] 0xA960-0xA97F, "Hangul Jamo Extended-A"        // (Korea)
p[131] 0xA980-0xA9DF, "Javanese"                      // (Indonesia)
p[132] 0xAA00-0xAA5F, "Cham"
p[133] 0xAA60-0xAA7F, "Myanmar Extended-A"            // (Myanmar)
p[134] 0xAA80-0xAADF, "Tai Viet"
p[135] 0xAB00-0xAB2F, "Ethiopic Extended-A"           // (Ethiopia)
p[136] 0xABC0-0xABFF, "Meetei Mayek"                  // (India)
p[137] 0xAC00-0xAFFF, "Hangul Syllables"              // (Korea)
p[138] 0xD7B0-0xD7FF, "Hangul Jamo Extended-B"        // (Korea)
p[139] 0xF900-0xFAFF, "CJK Compatibility Ideographs"
p[140] 0xFB00-0xFB4F, "Alphabetic Presentation Forms"
p[141] 0xFB50-0xFDFF, "Arabic Presentation Forms-A"
//  0xFE00, 0xFE0F, "These characters are not permitted in HTML Variation Selectors"},
p[142] 0xFE20-0xFE2F, "Combining Half Marks"
p[143] 0xFE30-0xFE4F, "CJK Compatibility Forms"
p[144] 0xFE50-0xFE6F, "Small Form Variants"
p[145] 0xFE70-0xFEFF, "Arabic Presentation Forms-B"
p[146] 0xFF00-0xFFEF, "Halfwidth and Fullwidth Forms"
p[147] 0xFFF0-0xFFFF, "Specials"                           // <<<<<<<<<<<<<<<<<<<<<
p[148] 0x10000-0x1007F, "Linear B Syllabary"
p[149] 0x10080-0x100FF, "Linear B Ideograms"
p[150] 0x10100-0x1013F, "Aegean Numbers"
p[151] 0x10140-0x1018F, "Ancient Greek Numbers"
p[152] 0x10190-0x101CF, "Ancient Symbols"
p[153] 0x101D0-0x101FF, "Phaistos Disc"
p[154] 0x10280-0x1029F, "Lycian"
p[155] 0x102A0-0x102DF, "Carian"
p[156] 0x10300-0x1032F, "Old Italic"
p[157] 0x10330-0x1034F, "Gothic"
p[158] 0x10380-0x1039F, "Ugaritic"
p[159] 0x10400-0x1044F, "Deseret"
p[160] 0x10450-0x1047F, "Shavian"
p[161] 0x10480-0x104AF, "Osmanya"                     // (Somalia)
p[162] 0x10800-0x1083F, "Cypriot Syllabary"
p[163] 0x10840-0x1085F, "Imperial Aramaic"
p[164] 0x10900-0x1091F, "Phoenician"
p[165] 0x10920-0x1093F, "Lydian"
p[166] 0x10A00-0x10A5F, "Kharoshthi"
p[167] 0x10A60-0x10A7F, "Old South Arabian"
p[168] 0x10B00-0x10B3F, "Avestan"
p[169] 0x10B40-0x10B5F, "Inscriptional Parthian"
p[170] 0x10B60-0x10B7F, "Inscriptional Pahlavi"
p[171] 0x10C00-0x10C4F, "Old Turkic"
p[172] 0x10E60-0x10E7F, "Rumi Numeral Symbols"
p[173] 0x11000-0x1107F, "Brahmi"                      // (India)
p[174] 0x11080-0x110CF, "Kaithi"                      // (India)
p[175] 0x110D0-0x110FF, "Sora Sompeng"                // (India)
p[176] 0x11100-0x1114F, "Chakma"                      // (Bangladesh / India)
p[177] 0x11180-0x111DF, "Sharada"                     // (Pakistan / India)
p[178] 0x11680-0x116CF, "Takri"                       // (India)
p[179] 0x12000-0x123FF, "Cuneiform"
p[180] 0x12400-0x1247F, "Cuneiform Numbers and Punctuation"
p[181] 0x13000-0x133FF, "Egyptian Hieroglyphs"
p[182] 0x16800-0x16A3F, "Bamum Supplement"            // (Cameroon)
p[183] 0x16F00-0x16F9F, "Miao"                        // (China)
p[184] 0x1B000-0x1B0FF, "Kana Supplement"
p[185] 0x1D000-0x1D0FF, "Byzantine Musical Symbols"
p[186] 0x1D100-0x1D1FF, "Musical Symbols"
p[187] 0x1D200-0x1D24F, "Ancient Greek Musical Notation"
p[188] 0x1D300-0x1D35F, "Tai Xuan Jing Symbols"
p[189] 0x1D360-0x1D37F, "Counting Rod Numerals"
p[190] 0x1D400-0x1D7FF, "Mathematical Alphanumeric Symbols"
p[191] 0x1EE00-0x1EEFF, "Arabic Mathematical Alphabetic Symbols"
p[192] 0x1F000-0x1F02F, "Mahjong Tiles"
p[193] 0x1F030-0x1F09F, "Domino Tiles"
p[194] 0x1F0A0-0x1F0FF, "Playing Cards"
p[195] 0x1F100-0x1F1FF, "Enclosed Alphanumeric Supplement"
p[196] 0x1F200-0x1F2FF, "Enclosed Ideographic Supplement"
p[197] 0x1F300-0x1F5FF, "Miscellaneous Symbols and Pictographs"
p[198] 0x1F600-0x1F64F, "Emoticons"
p[199] 0x1F680-0x1F6FF, "Transport and Map Symbols"
p[200] 0x1F700-0x1F77F, "Alchemical Symbols"
p[201] 0x20000-0x203FF, "CJK Unified Ideographs Extension B"
p[202] 0xA700-0xAAFF, "CJK Unified Ideographs Extension C"              // <<<<<<< NOT IN ORDER, BEWARE
p[203] 0x2B740-0x2B81F, "CJK Unified Ideographs Extension D"
p[204] 0x2F800-0x2FA1F, "CJK Compatibility Ideographs Supplement"

  { 0, 0, "", 0}                                                    // TERMINATOR ... au be bak












CURRENCY SYMBOLS NOT IN range u20a0 - u20cf

$	36	$	0024	 	DOLLAR SIGN   (present in WGL4, ANSI and MacRoman)
¢	162	¢	00A2	&cent;	CENT SIGN   (present in WGL4, ANSI and MacRoman)
£	163	£	00A3	&pound;	POUND SIGN   (present in WGL4, ANSI and MacRoman)   [lira sign is ₤ (&#8356;)]
¤	164	¤	00A4	&curren;	CURRENCY SIGN   (present in WGL4 and ANSI)
¥	165	¥	00A5	&yen;	YEN SIGN   (present in WGL4, ANSI and MacRoman)
ƒ	402	ƒ	0192	&fnof;	LATIN SMALL LETTER F WITH HOOK   (known as Florin or Guilder in Symbol font; present in WGL4, ANSI and MacRoman)
֏	1423	֏	058F	 	ARMENIAN DRAM SIGN
৲	2546	৲	09F2	 	BENGALI RUPEE MARK
৳	2547	৳	09F3	 	BENGALI RUPEE SIGN
૱	2801	૱	0AF1	 	GUJARATI RUPEE SIGN
௹	3065	௹	0BF9	 	TAMIL RUPEE SIGN
฿	3647	฿	0E3F	 	THAI CURRENCY SYMBOL BAHT
៛	6107	៛	17DB	 	KHMER CURRENCY SYMBOL RIEL
㍐	13136	㍐	3350	 	SQUARE YUAN
元	20803	元	5143	 	[Yuan, in China]
円	20870	円	5186	 	[Yen]
圆	22278	圆	5706	 	[Yen/Yuan variant]
圎	22286	圎	570E	 	[Yen/Yuan variant]
圓	22291	圓	5713	 	[Yuan, in Hong Kong and Taiwan ]
圜	22300	圜	571C	 	[Yen/Yuan variant]
꠸	43064	꠸	A838	 	NORTH INDIC RUPEE MARK
원	50896	원	C6D0	 	[Won]
﷼	65020	﷼	FDFC	 	RIAL SIGN
＄	65284	＄	FF04	 	FULLWIDTH DOLLAR SIGN
￠	65504	￠	FFE0	 	FULLWIDTH CENT SIGN
￡	65505	￡	FFE1	 	FULLWIDTH POUND SIGN
￥	65509	￥	FFE5	 	FULLWIDTH YEN SIGN
￦	65510	￦	FFE6	 	FULLWIDTH WON SIGN
💲	128178	💲	1F4B2	 	HEAVY DOLLAR SIGN

































































*/



