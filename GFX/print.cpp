#include "ix/ix.h"
#include <stdarg.h>







// maybe just altering the glyph abit, the outline could happen way easier...
// https://blog.mapbox.com/drawing-text-with-signed-distance-fields-in-mapbox-gl-b0933af6f817
// for certain tex offset might not be any use (the func in the shader), you can just use texture func, i think, will see... it could be faster



/*

if a request for a specific unichar that is not loaded, loads that charset auto?

TODO:
 - a mutex would solve the thread safety

 - there's text orientation, but a simple rotation would be needed too (NEEDED FOR WINDOWS); this would have 0- 359 degrees, any degree

 - compressed texture or not? im tending towards not...

 - color for each vertex, for blending of colors, kewl effect!

 - check the list for missing characters (not 100% shure of that website that were copied from)
 - a loadFontForUnicodeChar(font name, size, unicode character) ?
*/


using namespace mlib;

ixFontStyle ixDefFontStyle;

//_PagesList pagesList[];                 // holds all unicode pages that Print supports; check at the back of print.h for a list
const str8 fontFileVer("Font File v1.1"); // <<<<<<<<<< change this if a new version is added (don't forget to create the previous ver load func)







namespace _ixpr {
  #ifdef IX_USE_OPENGL
  extern void glInit();
  #endif

  #ifdef IX_USE_VULKAN
  extern void vkInit(Ix *in_ix, ixPrintShader **out_s);
  #endif
};




ixPrint::ixPrint() {
  _shader= null;

  delData();
}


ixPrint::~ixPrint() {
  delData();
}


/// called by destructor; can be called to clear and set everything to default; unloads all fonts and textures too
void ixPrint::delData() {
  pos= 0.0f;
  style= &ixDefFontStyle;
  style->restoreDefaults();

  justDraw= false;    /// internal, used when printing

  /// delete all fonts
  while(fonts.first)
    fonts.del(fonts.first);
}



void ixPrint::init() {
  #ifdef IX_USE_OPENGL
  _ixpr::glInit(... pass things here, needs heavy work IF it will ever be done);
  #endif // IX_USE_OPENGL

  #ifdef IX_USE_VULKAN
  _ixpr::vkInit(_ix, &_shader);
  #endif
}




ixPrintShader::ixPrintShader(Ix *in_ix): ixShader(in_ix), _print(&in_ix->pr) {
  _prevTex= null;
}





















// INTEGER printing function
void ixPrint::nint(int64 n) {
  uint8 buf[21], *p= buf+ 21; /// int64 can be represented in 20 chars (with - sign), plus terminator= 21

  // buf will be filled backwards by p
  *--p= 0;                    /// string terminator;

  if(n== 0)
    *--p= '0';                /// number is 0 - special case
  
  else if(n< 0) {             /// negative number
    while(n)
      *--p= '0'- n% 10, n/= 10;

    *--p= '-';
    
  } else                      /// positive number
    while(n)
      *--p= '0'+ n% 10, n/= 10;
  
  txt((cchar *)p);
}




// DOUBLE printing function; a precision can be set with dblPrecision or Print::setDblPrecision(n)
void ixPrint::ndouble(double n) {
  /// NaN check
  if(n!= n) {
    txt("NaN");
    return;
  }

  /// INFINITY check
  if((n- n) != 0.0) {
    if(n > 0.0) txt("+INF");
    else        txt("-INF");
    return;
  }

  uint8 buf[256], *p= buf+ 256;
  bool sign= false;
  int64 n1= (int64)n;           /// n1 can be printed as is
  int64 n2= (int64)((n- (int64)n)* pow10i[20+ style->dblPrecision]);

  // buf will be filled backwards by p
  *--p= 0;                    /// string terminator

  if(n< 0) {
    sign= true;
    //n= -n;                   // NEEDED?
    n1= -n1;
    n2= -n2;
  }

  /// print fractionary part only if dblPrecision > 0
  if(n2) {
    for(int a= 0; a< style->dblPrecision; a++) {
      *--p= '0'+ n2% 10;
      n2/= 10;
    }

    *--p= '.';
  }

  /// integer part
  if(n1== 0) *--p= '0';         /// n1 is 0 - special case
  else 
    while(n1)                   /// n1 to text
      *--p= '0'+ n1% 10, n1/= 10;

  if(sign)
    *--p= '-';

  txt((cchar *)p);
}


// formatted print text (printf format)  TEXT MAX LENGTH IS 2048 bytes long
void ixPrint::f(cchar *s, ...) {
  /// bounds check
  int32 len= Str::strlen8(s)- 1;
  if(len>= 2048) return;

  va_list args;
  char b[2048];                 /// buffer that will be used for format 
  va_start(args, s);
  vsprintf(b, s, args);         /// vsprintf does the fomatting
  va_end(args);

  txt(b);                       /// actual printing of the result
}


void ixPrint::f2(float x, float y, cchar *s, ...) {
  pos.x= x; pos.y= y;

  /// bounds check
  int32 len= Str::strlen8(s)- 1;
  if(len>= 2048) return;

  va_list args;
  char b[2048];                 /// buffer that will be used for format 
  va_start(args, s);
  vsprintf(b, s, args);         /// vsprintf does the fomatting
  va_end(args);

  txt(b);                       /// actual printing of the result
}

void ixPrint::f3(float x, float y, float z, cchar *s, ...) {
  pos.x= x; pos.y= y; pos.z= z;

  /// bounds check
  int32 len= Str::strlen8(s)- 1;
  if(len>= 2048) return;

  va_list args;
  char b[2048];                 /// buffer that will be used for format 
  va_start(args, s);
  vsprintf(b, s, args);         /// vsprintf does the fomatting
  va_end(args);

  txt(b);                       /// actual printing of the result
}















///=============///
// UTILITY funcs //
///=============///


// returns the width(dx), in pixels, of a specific character
float ixFontStyle::getCharDx(uint32 c) {
  /// this func returns the "box" width of the character, not the actual width of the image of the character
  /// the box is used for computations not the image width
  /// so the characters could start 1-2 pixels sooner and end 1-2 pixels later

  _ixFSize *fs= (_ixFSize *)selFont;
  if(!fs) return 0;

  _ixFPage *fp;
  /// search for the page the unicode char belongs to
  for(fp= (_ixFPage *)fs->pages.first; fp; fp= (_ixFPage *)fp->next)
    if(pagesList[fp->id].min<= c && c<= pagesList[fp->id].max)
      break;

  /// if no fp is found, there is no page that has this character
  if(!fp) return 0;

  //return fp->ch[c- pagesList[fp->id].min].dx;  << this would be the exact width of the bitmap
  return (float)(fp->ch[c- pagesList[fp->id].min].end)* scale;
}


float ixFontStyle::getCharDy() {
  if(!selFont) return 0;
  return (float)(((_ixFSize *)selFont)->size)* scale;
}


// returns text size in pixels
// <in_txt>: string, utf8/16/32
// <in_utfType>: utf type, one of 8/16/32
// <in_unicodes>: if <in_nrGlyphs>, it processes either unicodes or characters
// <in_nrGlyphs>: [optional][def:0] maximum number of unicodes or chars (depending on <in_unicodes>) to process in <in_txt>
// <in_font>: text font
// <in_spaceSize>: [optional][default: 0.0f] - if left default, it's the space size of the font - text wrap uses this
// <in_orientation>: [optional][default: IX_TXT_RIGHT] - text orientation, one of the 4 cardinal directions
float _getTextLen(const void *in_txt, int in_utfType, bool in_unicodes, int32 in_nrGlyphs, void *in_font, float in_spaceSize, int8 in_o) {
  /// the "box" of the text starts where the cursor is
  /// the starting and ending char could have bits and parts outside of this "box"
  /// char->start is where the glyph starts
  /// char->end is where the box end / next box start
  /// this func computes the box of the text; i don't think at this moment that the true pixels of the text are important
  /// in the future maybe a func can be done, easily, to see the exact lenght of the text, in pixels, or where the text start, in pixels, where the text end in pixels
  /// for the first char, ch->start, for the last char, ch->start+ ch->dx
  if(!in_font)
    in_font= (Ix::getMain()? Ix::getMain()->pr.style->selFont: null);
  if(!in_font) return 0.0f;
  if(!in_txt) return 0.0f;

  _ixFSize *fs= (_ixFSize *)in_font;
  _ixFPage *fp= null;
  const uint8 *p= (const uint8*)in_txt;
  uint32 c= 0;
  int32 len= 0;                               /// len will be the text length - the return value
  int32 charDy= fs->size;
  int32 nrSpaces= 0;

  if(in_nrGlyphs<= 0) in_nrGlyphs= INT32_MAX;

  if(in_o& IX_TXT_HORIZONTAL) {
    while(*p && in_nrGlyphs) {                    // for each character
      /// utf-8 string
      if(in_utfType== 8)        p= Str::utf8to32fast(p, &c);
      /// utf-16 string
      else if(in_utfType== 16)  p= (uint8 *)Str::utf16to32fast((const uint16 *)p, &c);
      /// utf-32 string
      else if(in_utfType== 32)  c= *(const uint32 *)p, p+= 4;
      else return 0;                             // in_utfType must be set or this func fails

      /// process diacriticals
      if(Str::isComb(c)) {
        if(in_unicodes)                         /// using a maximum number of unicodes
          in_nrGlyphs--;
        continue;
      }

      in_nrGlyphs--;

      /// search for the page the unicode char belongs to
      for(fp= (_ixFPage *)fs->pages.first; fp; fp= (_ixFPage *)fp->next)
        if(pagesList[fp->id].min<= c && c<= pagesList[fp->id].max)
          break;

      /// if no fp is found, there is no page that has this character
      if(!fp)
        c= '?', fp= (_ixFPage *)fs->pages.first;    /// this should be a character that is on all fonts for sure

      if(c== ' ') 
        nrSpaces++;
      else
        len+= fp->ch[c- pagesList[fp->id].min].end; /// adjust string length
    }	/// for each character

  } else {
    while(*p && in_nrGlyphs) {                    // for each character
      /// utf-8 string
      if(in_utfType== 8)        p= Str::utf8to32fast(p, &c);
      /// utf-16 string
      else if(in_utfType== 16)  p= (uint8 *)Str::utf16to32fast((const uint16 *)p, &c);
      /// utf-32 string
      else if(in_utfType== 32)  c= *(const uint32 *)p, p+= 4;
      else return 0;                             // in_utfType must be set or this func fails

      /// process diacriticals
      if(Str::isComb(c)) {
        if(in_unicodes)                         /// using a maximum number of unicodes
          in_nrGlyphs--;
        continue;
      }

      in_nrGlyphs--;

      if(c== ' ') nrSpaces++;
      else        len+= charDy;
    }	/// for each character
  } /// text orientation


  if(in_spaceSize== 0.0f)
    return (float)(len+ ((int32)(((_ixFPage *)fs->pages.first)->ch[' '].end)* nrSpaces));
  else
    return (float)len+ ((float)nrSpaces* in_spaceSize);
    

  /* TO DEL
  /// the "box" of the text starts where the cursor is
  /// the starting and ending char could have bits and parts outside of this "box"
  /// char->start is where the glyph starts
  /// char->end is where the box end / next box start
  /// this func computes the box of the text; i don't think at this moment that the true pixels of the text are important
  /// in the future maybe a func can be done, easily, to see the exact lenght of the text, in pixels, or where the text start, in pixels, where the text end in pixels
  /// for the first char, ch->start, for the last char, ch->start+ ch->dx
  if(!in_font)
    in_font= (Ix::getMain()? Ix::getMain()->pr.style->selFont: null);
  if(!in_font) return 0.0f;

  const uint8 *p= (const uchar*)in_txt;
  uint32 c= 0;
  if(in_nrGlyphs<= 0) in_nrGlyphs= INT32_MAX;
  _ixFSize *fs= (_ixFSize *)in_font;
  _ixFPage *fp= null;
  //_ixFChar *ch= null;
  int32 len= 0;                               /// len will be the text length - the return value
  int32 charDy= ((_ixFSize *)in_font)->size;

  bool defSpace= (in_spaceSize== 0.0f? true: false);  /// using default space size or not, used for faster check, than a float == 0.0f
  float lenFloat= 0.0f;                       /// lenFloat will be the text length - the return value, if the space size is not the font default

  if(!fs) return 0;                           /// if no font is selected, just return
  if(!in_txt) return 0;


  if(in_o& IX_TXT_HORIZONTAL) {
    while(*p && in_nrGlyphs) {                    // for each character

      /// utf-8 string
      if(in_utfType== 8)
        p= Str::utf8to32fast(p, &c);
      /// utf-16 string
      else if(in_utfType== 16)
        p= (uint8 *)Str::utf16to32fast((const uint16 *)p, &c);
      /// utf-32 string
      else if(in_utfType== 32)
        c= *(const uint32 *)p, p+= 4;

      else return 0;                             // in_utfType must be set or this func fails

      /// process diacriticals
      if(Str::isComb(c)) {
        if(in_unicodes)                         /// using a maximum number of unicodes
          in_nrGlyphs--;
        continue;
      }

      in_nrGlyphs--;

      /// search for the page the unicode char belongs to
      for(fp= (_ixFPage *)fs->pages.first; fp; fp= (_ixFPage *)fp->next)
        if(pagesList[fp->id].min<= c && c<= pagesList[fp->id].max)
          break;

      /// if no fp is found, there is no page that has this character
      if(!fp) {
        fp= (_ixFPage *)fs->pages.first;
        c= '?';                                 /// this should be a character that is on all fonts for shure
      }

      /// space size is not default font size - used mainly for text wrap
      if(!defSpace)
        lenFloat+= (c== ' '? in_spaceSize: (float)fp->ch[c- pagesList[fp->id].min].end);

      /// default space size
      else
        len+= fp->ch[c- pagesList[fp->id].min].end; /// adjust string length
    
    }	/// for each character

  } else {
    while(*p && in_nrGlyphs) {                    // for each character

      /// utf-8 string
      if(in_utfType== 8)
        p= Str::utf8to32fast(p, &c);
      /// utf-16 string
      else if(in_utfType== 16)
        p= (uint8 *)Str::utf16to32fast((const uint16 *)p, &c);
      /// utf-32 string
      else if(in_utfType== 32)
        c= *(const uint32 *)p, p+= 4;

      else return 0;                             // in_utfType must be set or this func fails

      /// process diacriticals
      if(Str::isComb(c)) {
        if(in_unicodes)                         /// using a maximum number of unicodes
          in_nrGlyphs--;
        continue;
      }

      in_nrGlyphs--;

      /// space size is not default font size - used mainly for text wrap
      if(!defSpace)
        lenFloat+= (c== ' '? in_spaceSize: (float)charDy);

      /// default space size
      else
        len+= charDy; /// adjust string length
    
    }	/// for each character
  } /// text orientation


  // return value
  if(defSpace) return len;                    /// default space size
  else         return mlib::roundf(lenFloat); /// other space size - used maily for text wrap
  */
}



// returns the maximum number of bytes of that text that can fit into the maximum number of pixels (in_maxPixels)
// <in_txt>: input text
// <in_utfType>: utf type - can be 8 or 16, coresponding to utf-8 and utf-16
// <in_maxPixels>: the maximum number of pixels the text can fit in
// <in_fnt>: font of the text
// <in_spaceSize>: [optional][def= 0.0f] - space size , used for text wrap, mainly. if left 0.0f, the font default space char size is taken
int32 _getBytesMaxPixels(const void *in_txt, int in_utfType, float in_maxPixels, void *in_fnt, float in_spaceSize) {
  const uint8 *p= (const uint8*)in_txt;
  uint32 c= 0;
  _ixFSize *fs= (_ixFSize *)in_fnt;
  _ixFPage *fp= null;
  int32 len= 0;                       /// len will be the text length in bytes - the return value
  //int32 lenInPixels= 0;               /// tmp - length of current text in pixels - it must always be less than maxPixels
  float lenInPixels= 0;               /// tmp - length of current text in pixels - it must always be less than maxPixels
  int32 charLen;                      /// current char length in bytes
  int32 diacriticalBytes= 0;          /// the number of bytes, summed, of the diacriticals of the current char


  /// variable space size tmp vars - using floats for computations
  bool defSpace= (in_spaceSize== 0.0f? true: false);
  //float lenInPixelsFloat= 0.0f;

  /// safety checks
  if(!fs) return 0;
  if(!in_txt) return 0;

  while(*p) {                          // for each character

    /// UTF-8 text
    if(in_utfType== 8) {
      if(*p < 128) {                    /// character uses 1 byte (ascii 0-127)
        c= *p++;
        charLen= 1;
      } else {
        if((*p& 0xe0) == 0xc0)          /// character uses 2 bytes
          c= (*p++)& 0x1f, charLen= 2;
        else if((*p& 0xf0) == 0xe0)     /// character uses 3 bytes
          c= (*p++)& 0x0f, charLen= 3;
        else if((*p& 0xf8) == 0xf0)     /// character uses 4 bytes
          c= (*p++)& 0x07, charLen= 4;
        else if((*p& 0xfc) == 0xf8)     /// character uses 5 bytes
          c= (*p++)& 0x03, charLen= 5;
        else if((*p& 0xfe) == 0xfc)     /// character uses 6 bytes
          c= (*p++)& 0x01, charLen= 6;

        for(int a= 1; a< charLen; a++)
          c<<= 6, c+= (*p++)& 0x3f;
      }

    /// UTF-16 text
    } else if(in_utfType== 16) {
      if(Str::isHighSurrogate(*p))
        c= (*p<< 10)+ *(p+ 1)+ Str::UTF16_SURROGATE_OFFSET, p+= 2, charLen= 4;
      else
        c= *p, p++, charLen= 2;

    } else return 0;

    /// combining diacritical - this unicode is part of the advancing char
    if(Str::isComb(c)) {
      diacriticalBytes+= charLen;
      continue;
    }

    /// search for the page the unicode char belongs to
    for(fp= (_ixFPage *)fs->pages.first; fp; fp= (_ixFPage *)fp->next)
      if(pagesList[fp->id].min<= c && c<= pagesList[fp->id].max)
        break;

    /// if no fp is found, there is no page that has this character
    if(!fp) {
      fp= (_ixFPage *)fs->pages.first;
      c= '?';                               /// this should be a character that is on all fonts for shure
      charLen= 1;
    }

    // using different space size - mainly for text wrap
    if(!defSpace) {
      if(c== ' ')
        lenInPixels+= in_spaceSize;
      else
        lenInPixels+= (float)fp->ch[c- pagesList[fp->id].min].end;

      if(lenInPixels> in_maxPixels)
        return len;

    // default font space size
    } else {
      lenInPixels+= (float)fp->ch[c- pagesList[fp->id].min].end;

      /// if this char would exceed the maximum pixels allowed on dx, return current text length in bytes
      if(lenInPixels> in_maxPixels)
        return len;
    }

    len+= charLen+ diacriticalBytes;
    diacriticalBytes= 0;            /// reset diacriticalBytes

  }	/// for each character

  return len+ diacriticalBytes;     /// reached this point, the whole text fits in maxPixels
}




// returns maximum number of chars in the txt that can be printed and fit into maxPixels (dx)
// <in_unicodes>:  what to return: unicodes(true) or chars(false) - chars can have multiple diacriticals (made from multiple unicodes)
// <in_txt>:       pointer to the string
// <in_utfType>:   string type, must be 8/16/32, coresponding to utf-8 / utf-16 / utf-32
// <in_maxPixels>: maximum number of pixels that the string must fit in
// <in_fnt>:       font
// <in_spaceSize?: [optional][def=0.0f] the space size of the text, if it is not the font default
int32 _getGlyphsMaxPixels(bool in_unicodes, const void *in_txt, int in_utfType, float in_maxPixels, void *in_fnt, float in_spaceSize, int8 in_o) {
  const uint8 *p= (const uint8*)in_txt;
  uint32 c;
  _ixFSize *fs= (_ixFSize *)in_fnt;
  _ixFPage *fp= null;
  int32 len= 0;                         /// len will be the text length in chars - the return value
  //int32 lenInPixels= 0;                 /// tmp - length of current text in pixels - it must always be less than maxPixels
  float lenInPixels= 0.0f;                 /// tmp - length of current text in pixels - it must always be less than maxPixels
  float charLenInPixels;
  int32 diacriticals= 0;                /// how many diacriticals current proccesed char has
  bool defSpace= (in_spaceSize== 0.0f? true: false);
  //float lenInPixelsFloat= 0.0f;
  //float charLenInPixelsFloat;
  bool horiz= in_o& IX_TXT_HORIZONTAL;
  int32 charDy= ((_ixFSize *)in_fnt)->size;

  if(!fs) return 0;                     /// if no font is selected, just return
  if(!in_txt) return 0;

  while(*p) {                            // for each character
    charLenInPixels= 0;

    /// utf-8 string
    if(in_utfType== 8) {
      p= Str::utf8to32fast(p, &c);

    /// utf-16 string
    } else if(in_utfType== 16) {
      p= (uint8 *)Str::utf16to32fast((const uint16 *)p, &c);

    /// utf-32 string
    } else if(in_utfType== 32)
      c= *(const uint32 *)p, p+= 4;

    else return 0;  // in_utfType must be set or this func fails

    /// search for the page the unicode char belongs to
    for(fp= (_ixFPage *)fs->pages.first; fp; fp= (_ixFPage *)fp->next)
      if(pagesList[fp->id].min<= c && c<= pagesList[fp->id].max)
        break;

    /// if no fp is found, there is no page that has this character
    if(!fp)
      fp= (_ixFPage *)fs->pages.first,
      c= '?';                               /// this should be a character that is on all fonts for shure

    /// combining diacritical - this unicode is part of the advancing char
    if(Str::isComb(c)) {
      diacriticals++;
      continue;
    }

    // spacesize is not default font size - mostly used for wrapping text
    /// using floats for this part
    if(!defSpace) {
      if(c== ' ') charLenInPixels= in_spaceSize;
      else        charLenInPixels= (float)(horiz? (fp->ch[c- pagesList[fp->id].min].end): charDy);

      /// if this char would exceed the maximum pixels allowed on dx, return current text length in bytes
      if(lenInPixels+ charLenInPixels> in_maxPixels)
        return len;

      lenInPixels+= charLenInPixels;

    // spacesize is default
    } else {
      charLenInPixels= (float)(horiz? fp->ch[c- pagesList[fp->id].min].end: charDy);

      /// if this char would exceed the maximum pixels allowed on dx, return current text length in bytes
      if(lenInPixels+ charLenInPixels> in_maxPixels)
        return len;

      lenInPixels+= charLenInPixels;
    }
    len++;
    if(in_unicodes)
      len+= diacriticals, diacriticals= 0;
      
  }	/// for each character

  return (in_unicodes? len+ diacriticals: len);     /// reached this point, the whole text fits in maxPixels
}



/*
// clipping funcs

void ixPrint::setScissor(int32 in_x0, int32 in_y0, int32 in_xe, int32 in_ye) {
  /// fragment shader works with viewport coords
  in_x0-= _ix->win->x0;
  in_y0-= _ix->win->y0;
  
  glUseProgram(_shader->gl->id);
  glUniform1i(_shader->u_clip, 1);
  glUniform2f(_shader->u_clip0, (float)in_x0, (float)in_y0);
  glUniform2f(_shader->u_clipE, (float)in_xe, (float)in_ye);
}


void ixPrint::setClipPlaneD(int32 in_x0,int32 in_y0,int32 in_dx, int32 in_dy) {
  /// fragment shader works with viewport coords
  in_x0-= _ix->win->x0;
  in_y0-= _ix->win->y0;
  
  glUseProgram(_shader->gl->id);
  glUniform1i(_shader->u_clip, 1);
  glUniform2f(_shader->u_clip0, (float)in_x0,          (float)in_y0);
  glUniform2f(_shader->u_clipE, (float)(in_x0+ in_dx), (float)(in_y0+ in_dy));
}


void ixPrint::setClipPlaneR(const recti *in_r) {
  /// fragment shader works with viewport coords
  int x0= in_r->x0- _ix->win->x0;
  int y0= in_r->y0- _ix->win->y0;
  
  glUseProgram(_shader->gl->id);
  glUniform1i(_shader->u_clip, 1);
  glUniform2f(_shader->u_clip0, (float)x0,             (float)y0);
  glUniform2f(_shader->u_clipE, (float)(x0+ in_r->dx), (float)(y0+ in_r->dy)); /// fragment shader works with viewport coords
}
*/



///====================///
// Font selection funcs //
///====================///

/// select an already loaded font & size
void *ixPrint::setFont(cchar *name, int size) {
  return (style->selFont= getFont(name, size));
}


/// returns a font ID for the specified size & font (font must be already loaded)
void *ixPrint::getFont(cchar *name, int size) {
  _ixFont *f;
  _ixFSize *fs;

  /// loop thru all fonts
  for(f= (_ixFont *)fonts.first; f; f= (_ixFont *)f->next)
    if(f->name== name)
      break;

  if(!f) return null;             /// fail search

  /// loop thru all sizes
  for(fs= (_ixFSize *)f->sizes.first; fs; fs= (_ixFSize *)fs->next)
    if(fs->size== size)
      return fs;                   // return found font&size ID

  return null;                    /// fail search
}







///=========================================///
// ------======= FONT LOADING =======------- //
///=========================================///


/*
cchar *name - font file name. Can be a TTF file or FNT file (custom uber file)
int size -    character height in pixels
int16 page -  unicode page to load

unicode page 0 is auto-loaded if it was not loaded first
*/
void *ixPrint::loadFont(cchar *name, int size, int16 page) {
  // init print method buffers/etc should be done on a font load, so there is a renderer active
  if(!_shader) init();

  str8 s("   ");
  int32 len= Str::strlen8(name)- 1;
  
  for(int a= 0; a< 3; a++)
    s.d[a]= name[len- 3+ a];
  
  s.lower();

  if(s== "ttf") return _loadTTF(name, size, page);
  else if(s== "fnt") return _loadFNT(name, size, page);
  else return null;
}

/// this version uses page description
void *ixPrint::loadFont(cchar *name, int size, cchar *pageDesc) {
  int16 n= _ixPagesList::getPage(pageDesc);
  if(n== -1) return null;
  return loadFont(name, size, n);
}



/*
  font type 1= pre rendered, FNT file is for this type

  FNT FILE STRUCTURE (v1.1):
    [nnn]               font file id string - must match fontFileVer, will try to load older files automatically
    [2B]                nameSize  (nem string size)
    [1B* nameSize]      name      (string)
    [2B]                nrSizes   (number of sizes)
    [2B* nrSizes]       size[]    (each size actual size... :)
    [4B* nrSizes]       sizeLoc[] (each size location in file)

    ---=== for each size ===---
    [2B]                nrPages   (number of pages this size has)
    [2B* nrPages]       page[]    (each page ID in pagesList[] struct)
    [4B* nrPages]       pageLoc[] (each page location in file)

    ---=== for each page ===---
    [2B]                texDx     (texture delta-x or width)
    [2B]                texDy     (texture delta-y or height)
    [1B* texDx* texDx]  tex       (texture data)

    ---=== for each unicode character in the page ===---
    [2B]                x0        (char position in texture x0)
    [2B]                y0        (char position in texture y0)
    [2B]                dx        (char width)
    [2B]                dy        (char height)
    [2B]                start     (previous char end position or start of this char)
    [2B]                end       (next char start position or end of this char)
*/





void *ixPrint::_loadFNT(cchar *name, int size, int16 page) {
  
  struct char10 {
    float texX0, texY0; /// position on texture - start points (glTexcoord)
    float texXe, texYe; /// position on texture - end points (glTexcoord)
    int16 dx, dy;       /// character width / height in pixels (hopefully, dy never exceeds font size, or im screwed)
  };

  if(!_shader) return null;
  
  bool ret= false;        /// return success or failure

  str8 s;                 /// tmp
  int16 ns;               /// tmp
  bool found;             /// tmp var used in different searches
  int16 a;

  int16 *sizes= null;     /// holds all font sizes the font file has
  int32 *sizesLoc= null;  /// holds all font sizes locations in file
  int16 sizeID= -1;       /// holds the requested size id (or index) in sizes[] & sizesLoc[]

  int16 *pages= null;     /// array with all pages IDs that this font size has
  int32 *pagesLoc= null;  /// array with each page location in file
  int16 pageID= -1;       /// will hold the requested page id or index in pages[] & pagesLoc[]
  //uchar *ttex= null;      /// will hold the raw texture

  _ixFont *pf= null;
  _ixFSize *ps= null;
  _ixFPage *pp= null;

  /// s will first be used to test for font file version
  s.d= new char[fontFileVer.len];
  s.d[fontFileVer.len- 1]= 0;

  /// read open & start reading from font file
  FILE *f= fopen(name, "rb");
  if(!f) return null;

  fread(s.d, fontFileVer.len- 1, 1, f);
  s.updateLen();

  if(s!= fontFileVer) {
    /// previous versions go here <<<
    /// the first version will just close and return false;
    fclose(f);
    return _loadFNT10(name, size, page);
  }

  /// font name read
  fread(&ns, 2, 1, f);      /// font name length
  s.delData();
  s.d= new char[ns+ 1];
  s.d[ns]= 0;               /// string terminator (au be back)
  fread(s.d, ns, 1, f);     /// actual font name read
  s.updateLen();            /// string does not auto update it's length

  /// read all sizes this font has (+ where they are in the file)
  fread(&ns, 2, 1, f);      /// nr sizes in the font
  sizes= new int16[ns];
  sizesLoc= new int32[ns];
  fread(sizes, ns, 2, f);
  fread(sizesLoc, ns, 4, f);

  // current 'ns' is still NEEDED for next few lines

  /// requested size can be 0, meaning to load the first size in the file:
  if(!size) {
    size= sizes[0];
    sizeID= 0;

  /// if a specific size is requested, search for it in the file
  } else {
    found= false;
    for(a= 0; a< ns; a++) {
      if(sizes[a]== size) {
        found= true;        /// found it
        sizeID= a;
        break;
      }
    }

    if(!found) {            /// if requested size is not in the file, close everything and return false
      goto Return;
    } /// if size !found in file
  } /// if !size else
    
  /// read all unicode pages headers in this font size (+where they are in the file)
  fseek(f, sizesLoc[sizeID], SEEK_SET);   /// move to the requested size location
  fread(&ns, 2, 1, f);          /// nr of pages in this size
  pages= new int16[ns];
  pagesLoc= new int32[ns];
  fread(pages, ns, 2, f);
  fread(pagesLoc, ns, 4, f);
  
  // ns holds number of pages in current size; used in next search

  /// search for requested page in the file
  found= false;
  for(a= 0; a< ns; a++)
    if(pages[a]== page) {
      found= true;
      pageID= a;
      break;
    }
  
  if(!found) goto Return;     /// requested page is not in the file

  // at this point, it is known what font size & unicode page is requested

  /// try to search for already loaded font, font size and page
  pf= _getFont(s);
  if(pf) ps= _getSizep(pf, size);
  if(ps) pp= _getPagep(ps, page);

  // at this point, it is known if font, size and page are already created (if any pf, ps, pp are not NULL)
     
  if(pp) { ret= true; goto Return; } /// page is already loaded

  /// if a page is requested (not page0->ascii) and requested size is not created, load page0 first
  if(page && !ps) {
    fclose(f);                        /// close file
    if(!_loadFNT(name, size, 0))      /// load ascii page first (page 0)
      goto Return;
    f= fopen(name, "rb");             /// reopen the file

    pf= _getFont(s);                   /// find pf (it is surely loaded now)
    ps= _getSizep(pf, size);           /// find ps (it is surely loaded now)
  }
    
  if(!pf) {               /// create a new font (in case it's not already created)
    pf= new _ixFont;
    fonts.add(pf);
    pf->type= 1;          /// pre rendered font
    pf->name= s;
    pf->fileName= name;
  }

  if(!ps) {               /// create a new font size (in case it's not already created)
    ps= new _ixFSize;
    pf->sizes.add(ps);
    ps->size= size;
    //ps->fileName= name; moved to font
    ps->font= pf;
  }

  /// create a new page
  pp= new _ixFPage(this);
  ps->pages.add(pp);
  pp->id= page;
  pp->size= ps;
  pp->ch= new _ixFChar[pagesList[page].max- pagesList[page].min+ 1];

  fseek(f, pagesLoc[pageID], SEEK_SET);

  /// texture read from file
  fread(&pp->texDx, 2, 1, f);         /// texture dx
  fread(&pp->texDy, 2, 1, f);         /// texture dy
  
  pp->tex= _ix->res.tex.add.ixStaticTexture();
  

  //if(pp->tex->data) delete pp->tex->data;
  pp->tex->data= new Tex;
  pp->tex->flags.setDown(0x02);   // delete *data
  pp->tex->flags.setDown(0x04);   // delete *data::*bitmap

  pp->tex->data->dx= pp->texDx;
  pp->tex->data->dy= pp->texDy;
  pp->tex->data->depth= 1;
  pp->tex->data->format= ImgFormat::R8_UNORM;
  pp->tex->data->computePixelInfo();

  pp->tex->data->bitmap= new uint8[pp->tex->data->dx* pp->tex->data->dy];

  pp->tex->data->nrLevels= 1;
  pp->tex->data->size= (uint32)(pp->texDx)* (uint32)(pp->texDy);
  pp->tex->data->levFrom[0]= 0;
  pp->tex->data->levSize[0]= pp->tex->data->size;
  
  fread(pp->tex->data->bitmap, pp->tex->data->size, 1, f);      /// texture data read (8bit greyscale)

  /// each character position & size read
  for(uint32 c= 0; c< pagesList[page].max- pagesList[page].min+ 1; c++) {
    fread(&pp->ch[c].x0,    sizeof(int16), 1, f);
    fread(&pp->ch[c].y0,    sizeof(int16), 1, f);
    fread(&pp->ch[c].dx,    sizeof(int16), 1, f);
    fread(&pp->ch[c].dy,    sizeof(int16), 1, f);
    fread(&pp->ch[c].start, sizeof(int16), 1, f);
    fread(&pp->ch[c].end,   sizeof(int16), 1, f);
  }

  #ifdef IX_USE_VULKAN
  pp->tex->vkd.flags.setDown(0x02);     // set is handled by print class
  #endif

  _ix->res.tex.upload(pp->tex);      // texture upload to GPU


  #ifdef IX_USE_OPENGL
  if((_ixpr::glM== 5) || (_ixpr::glM== 55)) _shader->_create(pp);   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< METHOD 5 & 5U PRINTING INIT <<<<<<<<<<<<<<<<<<<<<<<<<<<
  #endif

  #ifdef IX_USE_VULKAN
  _shader->_create(pp);
  #endif



  /// there are some pages that span more than 1024 characters. these are split in multiple pages of 1024 characters max
  /// all pages will be loaded
  for(a= 1; a< pagesList[pageID].nrParts; a++)
    if(!_loadFNT(name, size, page+ a)) 
      goto Return;

  

  ret= true;                           // return successful only if reached this point

  
  
Return:
  fclose(f);
  if(!ret) {
    delPage(ps, size, page);
    if(pp)
      if(pp->tex)
        if(_ix)
          _ix->res.tex.delTexture(pp->tex);
  }
  if(sizes)    delete[] sizes;
  if(sizesLoc) delete[] sizesLoc;
  if(pages)    delete[] pages;
  if(pagesLoc) delete[] pagesLoc;
  if(ret) return ps; else return null;
}






/*
  OLD version 1.0 load =================--------------------
  font type 1= pre rendered, FNT file is for this type

  FNT FILE STRUCTURE (v1.0):
    [nnn]               font file id string - must match fontFileVer, will try to load older files automatically
    [2B]                nameSize  (nem string size)
    [1B* nameSize]      name      (string)
    [2B]                nrSizes   (number of sizes)
    [2B* nrSizes]       size[]    (each size actual size... :)
    [4B* nrSizes]       sizeLoc[] (each size location in file)

    ---=== for each size ===---
    [2B]                nrPages   (number of pages this size has)
    [2B* nrPages]       page[]    (each page ID in pagesList[] struct)
    [4B* nrPages]       pageLoc[] (each page location in file)

    ---=== for each page ===---
    [2B]                texDx     (texture delta-x or width)
    [2B]                texDy     (texture delta-y or height)
    [1B* texDx* texDx]  tex       (texture data)

    ---=== for each unicode character in the page ===---
    [4B]                texX0     (char in texture x0) - float!!!
    [4B]                texY0     (char in texture y0) - float!!!
    [4B]                texDx     (char in texture dx) - float!!!
    [4B]                texDy     (char in texture dy) - float!!!
    [2B]                dx        (character width)
    [2B]                dy        (character height)
    [2B]                start     (previous char end position or start of this char)
    [2B]                end       (next char start position or end of this char)
*/
void *ixPrint::_loadFNT10(cchar *name, int size, int16 page) {
  const str8 fontFileVer10("Font File v1.0");

  if(!_shader) return null;
  
  bool ret= false;        /// return success or failure

  str8 s;                 /// tmp
  int16 ns;               /// tmp
  bool found;             /// tmp var used in different searches
  int16 a;
  float flt;

  int16 *sizes= null;     /// holds all font sizes the font file has
  int32 *sizesLoc= null;  /// holds all font sizes locations in file
  int16 sizeID= -1;       /// holds the requested size id (or index) in sizes[] & sizesLoc[]

  int16 *pages= null;     /// array with all pages IDs that this font size has
  int32 *pagesLoc= null;  /// array with each page location in file
  int16 pageID= -1;       /// will hold the requested page id or index in pages[] & pagesLoc[]
  //uchar *ttex= null;      /// will hold the raw texture

  _ixFont *pf= null;
  _ixFSize *ps= null;
  _ixFPage *pp= null;

  /// s will first be used to test for font file version
  s.d= new char[fontFileVer10.len];
  s.d[fontFileVer10.len- 1]= 0;

  /// read open & start reading from font file
  FILE *f= fopen(name, "rb");
  if(!f) return null;

  fread(s.d, fontFileVer10.len- 1, 1, f);
  s.updateLen();

  if(s!= fontFileVer10) {
    // first version, so there are no other
    goto Return;
  }

  /// font name read
  fread(&ns, 2, 1, f);      /// font name length
  s.delData();
  s.d= new char[ns+ 1];
  s.d[ns]= 0;               /// string terminator (au be back)
  fread(s.d, ns, 1, f);     /// actual font name read
  s.updateLen();            /// string does not auto update it's length

  /// read all sizes this font has (+ where they are in the file)
  fread(&ns, 2, 1, f);      /// nr sizes in the font
  sizes= new int16[ns];
  sizesLoc= new int32[ns];
  fread(sizes, ns, 2, f);
  fread(sizesLoc, ns, 4, f);

  // current 'ns' is still NEEDED for next few lines

  /// requested size can be 0, meaning to load the first size in the file:
  if(!size) {
    size= sizes[0];
    sizeID= 0;

  /// if a specific size is requested, search for it in the file
  } else {
    found= false;
    for(a= 0; a< ns; a++) {
      if(sizes[a]== size) {
        found= true;        /// found it
        sizeID= a;
        break;
      }
    }

    if(!found) {            /// if requested size is not in the file, close everything and return false
      goto Return;
    } /// if size !found in file
  } /// if !size else
    
  /// read all unicode pages headers in this font size (+where they are in the file)
  fseek(f, sizesLoc[sizeID], SEEK_SET);   /// move to the requested size location
  fread(&ns, 2, 1, f);          /// nr of pages in this size
  pages= new int16[ns];
  pagesLoc= new int32[ns];
  fread(pages, ns, 2, f);
  fread(pagesLoc, ns, 4, f);
  
  // ns holds number of pages in current size; used in next search

  /// search for requested page in the file
  found= false;
  for(a= 0; a< ns; a++)
    if(pages[a]== page) {
      found= true;
      pageID= a;
      break;
    }
  
  if(!found) goto Return;     /// requested page is not in the file

  // at this point, it is known what font size & unicode page is requested

  /// try to search for already loaded font, font size and page
  pf= _getFont(s);
  if(pf) ps= _getSizep(pf, size);
  if(ps) pp= _getPagep(ps, page);

  // at this point, it is known if font, size and page are already created (if any pf, ps, pp are not NULL)
     
  if(pp) { ret= true; goto Return; } /// page is already loaded

  /// if a page is requested (not page0->ascii) and requested size is not created, load page0 first
  if(page && !ps) {
    fclose(f);                        /// close file
    if(!_loadFNT(name, size, 0))      /// load ascii page first (page 0)
      goto Return;
    f= fopen(name, "rb");             /// reopen the file

    pf= _getFont(s);                   /// find pf (it is surely loaded now)
    ps= _getSizep(pf, size);           /// find ps (it is surely loaded now)
  }
    
  if(!pf) {               /// create a new font (in case it's not already created)
    pf= new _ixFont;
    fonts.add(pf);
    pf->type= 1;          /// pre rendered font
    pf->name= s;
    pf->fileName= name;
  }

  if(!ps) {               /// create a new font size (in case it's not already created)
    ps= new _ixFSize;
    pf->sizes.add(ps);
    ps->size= size;
    //ps->fileName= name; moved to font <<<<<<<<<
    ps->font= pf;
  }

  /// create a new page
  pp= new _ixFPage(this);
  ps->pages.add(pp);
  pp->id= page;
  pp->size= ps;
  pp->ch= new _ixFChar[pagesList[page].max- pagesList[page].min+ 1];

  fseek(f, pagesLoc[pageID], SEEK_SET);

  /// texture read from file
  fread(&pp->texDx, 2, 1, f);         /// texture dx
  fread(&pp->texDy, 2, 1, f);         /// texture dy
  
  pp->tex= _ix->res.tex.add.ixStaticTexture();
  

  //if(pp->tex->data) delete pp->tex->data;
  pp->tex->data= new Tex;
  pp->tex->flags.setDown(0x02);   // delete *data
  pp->tex->flags.setDown(0x04);   // delete *data::*bitmap

  pp->tex->data->dx= pp->texDx;
  pp->tex->data->dy= pp->texDy;
  pp->tex->data->depth= 1;
  pp->tex->data->format= ImgFormat::R8_UNORM;
  pp->tex->data->computePixelInfo();

  pp->tex->data->bitmap= new uint8[pp->tex->data->dx* pp->tex->data->dy];

  pp->tex->data->nrLevels= 1;
  pp->tex->data->size= (uint32)(pp->texDx)* (uint32)(pp->texDy);
  pp->tex->data->levFrom[0]= 0;
  pp->tex->data->levSize[0]= pp->tex->data->size;
  

  fread(pp->tex->data->bitmap, pp->tex->data->size, 1, f);      /// texture data read (8bit greyscale)
  //ttex= new uchar[pp->texDx* pp->texDy];
  //fread(ttex, pp->texDx* pp->texDy, 1, f);      /// texture data read (8bit greyscale)

  /*
  /// create OpenGL texture
  if(glGetError()) for(int a= 0; a< 20; a++) if(!glGetError()) break;   /// flush prev OpenGL errors 
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &pp->tex->glData.id);
  glBindTexture(GL_TEXTURE_2D, pp->tex->glData.id);
  pp->tex->upload();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pp->texDx, pp->texDy, 0, GL_RED, GL_UNSIGNED_BYTE, ttex); // no mipmaps <<<
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_INTENSITY, pp->texDx, pp->texDy, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ttex); // DEBUG BLACK BACKGROUND no mipmaps <<<
  //glGenerateMipmap(dst->target);  // << mipmaps?

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,	GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,	GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,   GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,   GL_CLAMP_TO_EDGE);
  glFinish();

  if(glGetError() != GL_NO_ERROR)     /// failed to create gl texture
    goto Return;
  */

  /// each character position & size read
  for(uint32 c= 0; c< pagesList[page].max- pagesList[page].min+ 1; c++) {
    // original:
    //fread(&pp->ch[c].texX0, sizeof(pp->ch[c].texX0), 1, f);
    //fread(&pp->ch[c].texY0, sizeof(pp->ch[c].texY0), 1, f);
    //fread(&pp->ch[c].texXe, sizeof(pp->ch[c].texXe), 1, f);
    //fread(&pp->ch[c].texYe, sizeof(pp->ch[c].texYe), 1, f);
    fread(&flt, sizeof(float), 1, f); // texX0
    pp->ch[c].x0= (int16)(flt* (float)pp->texDx);
    fread(&flt, sizeof(float), 1, f); // texY0
    pp->ch[c].y0= (int16)(flt* (float)pp->texDy);
    fread(&flt, sizeof(float), 1, f); // texXe - ignore
    fread(&flt, sizeof(float), 1, f); // texYe - ignore

    fread(&pp->ch[c].dx,    sizeof(pp->ch[c].dx),    1, f);
    fread(&pp->ch[c].dy,    sizeof(pp->ch[c].dy),    1, f);
    fread(&pp->ch[c].start, sizeof(pp->ch[c].start), 1, f);
    fread(&pp->ch[c].end,   sizeof(pp->ch[c].end),   1, f);
  }

  #ifdef IX_USE_VULKAN
  pp->tex->vkd.flags.setDown(0x02);     // set is handled by print class
  #endif

  _ix->res.tex.upload(pp->tex);      // texture upload to GPU


  #ifdef IX_USE_OPENGL
  if((_ixpr::glM== 5) || (_ixpr::glM== 55)) _shader->_create(pp);   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< METHOD 5 & 5U PRINTING INIT <<<<<<<<<<<<<<<<<<<<<<<<<<<
  #endif

  #ifdef IX_USE_VULKAN
  _shader->_create(pp);
  #endif



  /// there are some pages that span more than 1024 characters. these are split in multiple pages of 1024 characters max
  /// all pages will be loaded
  for(a= 1; a< pagesList[pageID].nrParts; a++)
    if(!_loadFNT(name, size, page+ a)) 
      goto Return;

  

  ret= true;                           // return successful only if reached this point

  
  
Return:
  fclose(f);
  if(!ret) {
    delPage(ps, size, page);
    if(pp)
      if(pp->tex)
        if(_ix)
          _ix->res.tex.delTexture(pp->tex);
  }
  if(sizes)    delete[] sizes;
  if(sizesLoc) delete[] sizesLoc;
  if(pages)    delete[] pages;
  if(pagesLoc) delete[] pagesLoc;
  //if(ttex) delete[] ttex;
  if(ret) return ps; else return null;
}




// W I P
void *ixPrint::_loadTTF(cchar *name, int size, int16 page) {
  return null;
}




///===========================================///
// ------======= FONT UNLOADING =======------- //
///===========================================///

/// deletes a page; font and size must be specified
void ixPrint::delPage(void *font, int size, int16 page) {
  if(!font) return;
  _ixFSize *fs= _getSizep(((_ixFSize *)font)->font, size);
  if(!fs) return;

  /// page(s) deletion; there are some pages that are very big, so they get split into multiple parts (CJK, 42k characters, for example)
  /// these pages are treated as 1, and are all deleted & all loaded
  _ixFPage *pp= null;
  for(int16 a= 0; a< pagesList[page].nrParts; a++) {
    pp= _getPagep(fs, page+ a);
    if(pp) fs->pages.del(pp);
  }

  if(!fs->pages.nrNodes) {     /// size is empty, del it
    if(fs->font->sizes.nrNodes== 1)
      fonts.del(fs->font);     /// font has no sizes left, del the whole font
    else
      fs->font->sizes.del(fs); /// just del the size
  }
}

/// deletes a whole font size, deletes font too if no more sizes are left inside
void ixPrint::delSize(void *font, int size) {
  if(!font) return;
  _ixFSize *fs= _getSizep(((_ixFSize *)font)->font, size);
  
  if(fs) {
    _ixFont *f= fs->font;
    f->sizes.del(fs);                // actual delete of font size
    if(!f->sizes.nrNodes) fonts.del(f);    /// if no sizes left in font, delete it
  }
}

/// deletes whole font
void ixPrint::delFont(void *font) {
  if(!font) return;
  fonts.del(((_ixFSize *)font)->font);
}












///=========================================================///
// -------======= INTERNAL STUFF from here on =======------- //
///=========================================================///


/// internal - searches for a font, using specified name
_ixFont *ixPrint::_getFont(cchar *name) {
  _ixFont *p= (_ixFont *)fonts.first;
  while(p) {
    if(p->name== name)
      break;
    p= (_ixFont *)p->next;
  }

  return p;
}

_ixFSize *ixPrint::_getSizep(_ixFont *font, int size) {
  if(!font) return null;

  for(_ixFSize *p= (_ixFSize *)font->sizes.first; p; p= (_ixFSize *)p->next)
    if(p->size== size)    // found
      return p;

  return null;            // not found
}

_ixFPage *ixPrint::_getPagep(_ixFSize *size, int16 page) {
  if(!size) return null;

  for(_ixFPage *p= (_ixFPage *)size->pages.first; p; p= (_ixFPage *)p->next)
    if(p->id== page)
      return p;

  return null;
}


_ixFPage::_ixFPage(ixPrint *in_parent): id(0), ch(null), tex(null),  size(null), _print(in_parent) { 
  _texData= null;
  #ifdef IX_USE_OPENGL
  _M5UUBOid= 0;
  _M5VBOid= 0;
  _M5texPointer= 0;
  #endif

  #ifdef IX_USE_VULKAN
  vkData= null;
  vkSet= null;
  #endif

  delData();
}


void _ixFPage::delData() {
  id= 0;

  if(ch) delete[] ch; ch= null;
  
  if(tex) {
    _print->_ix->res.tex.delTexture(tex);
    tex= null;
  }

  size= null;
  texDx= texDy= 0;
  if(_texData) { delete[] _texData; _texData= null; }

  if(_print) if(_print->_shader) _print->_shader->_delete(this);

  #ifdef IX_USE_OPENGL
  //if(_ixpr::glM== 5) if(_print) if(_print->_shader) _print->_shader->_delete(this);
  //if(_ixpr::glM== 55) if(_print) if(_print->_shader) _print->_shader->_delete(this);
  _M5texPointer= 0;
  #endif

  #ifdef IX_USE_VULKAN
  #endif
}






/* NOT USED !!!
_ixFSize *Print::_getSize(cchar *name, int size) {
  _ixFont *pf= _getFont(name);
  if(!pf) return null;

  _ixFSize *ps= (_ixFSize *)pf->sizes.first;
  while(ps)
    if(ps->size== size)
      break;

  return ps;
}
*/

/* NOT USED !!!
_ixFPage *Print::_getPage(cchar *name, int size, int16 page) {
  _ixFont *pf= _getFont(name);
  _ixFSize *ps= _getSize(name, size);
  if(!pf || !ps) return null;

  _ixFPage *pp= (_ixFPage *)ps->pages.first;
  while(pp)
    if(pp->id== page)
      break;

  return pp;
}
*/

/* NOT USED !!!
/// returns what page a unicode char belongs to, or null if no loaded page has the requested character
_ixFPage *Print::_getPagec(void *font, uint32 unicode) {
  _ixFPage *p= (_ixFPage *)((_ixFSize *)font)->pages.first;
  for(; p; p= (_ixFPage *)p->next) {
    if(pagesList[p->id].min>= unicode && pagesList[p->id].max<= unicode)
      return p;
  }
  return p;
}
*/































































































///=================================================///
// -------======= PAGESLIST structure =======------- //
///=================================================///


// POSSIBLE RACE CONDITION HERE -> NOT THREAD SAFE? <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// returns page number that has the specified name
int16 _ixPagesList::getPage(cchar *name) {
  int16 a= 0;
  /// loops thru all pages; do not remove 'terminator' that has 0 as nrParts...
  while(pagesList[a].nrParts) {
    if(pagesList[a].name== name)
      return a;
    a++;
  }
  return -1;
}


int16 _ixPagesList::computeNrPages() {
  for(int16 ret= 0; ret< INT16_MAX; ret++)
    if(pagesList[ret].nrParts== 0)
      return ret;
  return 0;
}

/// all possible unicode pages list
_ixPagesList pagesList[]= {
  { 0x0000, 0x007F, "C0 Controls and Basic Latin", 1 },
  { 0X0080, 0x00FF, "C1 Controls and Latin-1 Supplement", 1 },
  { 0x0100, 0x017F, "Latin Extended-A", 1 }, 
  { 0x0180, 0x024F, "Latin Extended-B", 1 },
  { 0x0250, 0x02AF, "IPA Extensions", 1 },
  { 0x02B0, 0x02FF, "Spacing Modifier Letters", 1 },       // some arrows in here, some table building stuff too
  { 0x0300, 0x036F, "Combining Diacritical Marks", 1 },
  { 0x0370, 0x03FF, "Greek and Coptic", 1 },               // (Hellenic Republic - greece i guess)
  { 0x0400, 0x04FF, "Cyrillic", 1 },
  { 0x0500, 0x052F, "Cyrillic Supplement", 1 },
  { 0x0530, 0x058F, "Armenian", 1 },                       // (Armenia)
  { 0x0590, 0x05FF, "Hebrew", 1 },                         // (Israel)
  { 0x0600, 0x06FF, "Arabic", 1 },
  { 0x0700, 0x074F, "Syriac", 1 },
  { 0x0750, 0x077F, "Arabic Supplement", 1 },
  { 0x0780, 0x07BF, "Thaana", 1 },                         // (Maldives)
  { 0x07C0, 0x07FF, "N’Ko", 1 },
  { 0x0800, 0x083F, "Samaritan", 1 },
  { 0x0840, 0x085F, "Mandaic", 1 },
  { 0x08A0, 0x08FF, "Arabic Extended-A", 1 },
  { 0x0900, 0x097F, "Devanagari (India)", 1 },
  { 0x0980, 0x09FF, "Bengali", 1 },                        // (Bangladesh / India)
  { 0x0A00, 0x0A7F, "Gurmukhi", 1 },                       // (India / Pakistan)
  { 0x0A80, 0x0AFF, "Gujarati", 1 },                       // (India)
  { 0x0B00, 0x0B7F, "Oriya", 1 },                          // (India)
  { 0x0B80, 0x0BFF, "Tamil", 1 },                          // (India / Sri Lanka)
  { 0x0C00, 0x0C7F, "Telugu", 1 },                         // (India)
  { 0x0C80, 0x0CFF, "Kannada", 1 },                        // (India)
  { 0x0D00, 0x0D7F, "Malayalam", 1 },                      // (India)
  { 0x0D80, 0x0DFF, "Sinhala", 1 },                        // (Sri Lanka)
  { 0x0E00, 0x0E7F, "Thai", 1 },                           // (Thailand)
  { 0x0E80, 0x0EFF, "Lao (Lao people's republic)", 1 },
  { 0x0F00, 0x0FFF, "Tibetan", 1 },
  { 0x1000, 0x109F, "Myanmar", 1 },                        // (Myanmar)
  { 0x10A0, 0x10FF, "Georgian", 1 },                       // (Georgia)
  { 0x1100, 0x11FF, "Hangul Jamo", 1 },                    // (Korea)
  { 0x1200, 0x137F, "Ethiopic", 1 },                       // (Ethiopia)
  { 0x1380, 0x139F, "Ethiopic Supplement", 1 },            // (Ethiopia)
  { 0x13A0, 0x13FF, "Cherokee", 1 },
  { 0x1400, 0x167F, "Unified Canadian Aboriginal Syllabics", 1 },
  { 0x1680, 0x169F, "Ogham", 1 },
  { 0x16A0, 0x16FF, "Runic", 1 },
  { 0x1700, 0x171F, "Tagalog", 1 },                        // (Philippines)
  { 0x1720, 0x173F, "Hanunóo", 1 },                        // (Philippines)
  { 0x1740, 0x175F, "Buhid", 1 },                          // (Philippines)
  { 0x1760, 0x177F, "Tagbanwa", 1 },                       // (Philippines)
  { 0x1780, 0x17FF, "Khmer", 1 },                          // (Cambodia)
  { 0x1800, 0x18AF, "Mongolian", 1 },
  { 0x18B0, 0x18FF, "Unified Canadian Aboriginal Syllabics Extended", 1 },
  { 0x1900, 0x194F, "Limbu", 1 },                          // (India / Nepal)
  { 0x1950, 0x197F, "Tai Le", 1 },
  { 0x1980, 0x19DF, "New Tai Lue", 1 },
  { 0x19E0, 0x19FF, "Khmer Symbols", 1 },                  // (Cambodia)
  { 0x1A00, 0x1A1F, "Buginese", 1 },                       // (Indonesia)
  { 0x1A20, 0x1AAF, "Tai Tham", 1 },
  { 0x1B00, 0x1B7F, "Balinese", 1 },                       // (Indonesia)
  { 0x1B80, 0x1BBF, "Sundanese", 1 },                      // (Indonesia)
  { 0x1BC0, 0x1BFF, "Batak", 1 },                          // (Indonesia)
  { 0x1C00, 0x1C4F, "Lepcha", 1 },                         // (India)
  { 0x1C50, 0x1C7F, "Ol Chiki", 1 },
  { 0x1CC0, 0x1CCF, "Sundanese Supplement", 1 },
  { 0x1CD0, 0x1CFF, "Vedic Extensions", 1 },
  { 0x1D00, 0x1D7F, "Phonetic Extensions", 1 },            // (some latins)
  { 0x1D80, 0x1DBF, "Phonetic Extensions Supplement", 1 }, // (some latins)
  { 0x1DC0, 0x1DFF, "Combining Diacritical Marks Supplement", 1 },
  { 0x1E00, 0x1EFF, "Latin Extended Additional", 1 },
  { 0x1F00, 0x1FFF, "Greek Extended", 1 },                 // (Hellenic republic)
  { 0x2000, 0x206F, "General Punctuation", 1 },
  { 0x2070, 0x209F, "Superscripts and Subscripts", 1 },
  { 0x20A0, 0x20CF, "Currency Symbols", 1 },               // (NOT ALL IN HERE, check further down)
  { 0x20D0, 0x20FF, "Combining Diacritical Marks for Symbols", 1 },
  { 0x2100, 0x214F, "Letterlike Symbols", 1 },
  { 0x2150, 0x218F, "Number Forms", 1 },
  { 0x2190, 0x21FF, "Arrows", 1 },                         // (lots!)
  { 0x2200, 0x22FF, "Mathematical Operators", 1 },
  { 0x2300, 0x23FF, "Miscellaneous Technical", 1 },
  { 0x2400, 0x243F, "Control Pictures", 1 },
  { 0x2440, 0x245F, "Optical Character Recognition", 1 },
  { 0x2460, 0x24FF, "Enclosed Alphanumerics", 1 },
  { 0x2500, 0x257F, "Box Drawing", 1 },                    // (!!!!! TABELS !!!!! what ascii extended was)
  { 0x2580, 0x259F, "Block Elements", 1 },                 // ( !!!!! what ascii extended was)
  { 0x25A0, 0x25FF, "Geometric Shapes", 1 },               // ( o.O )
  { 0x2600, 0x26FF, "Miscellaneous Symbols", 1 },          // (stuff)
  { 0x2700, 0x27BF, "Dingbats", 1 },                       // (stuff)
  { 0x27C0, 0x27EF, "Miscellaneous Mathematical Symbols-A", 1 },
  { 0x27F0, 0x27FF, "Supplemental Arrows-A", 1 },
  { 0x2800, 0x28FF, "Braille Patterns", 1 },               // (blind ppl stuff)
  { 0x2900, 0x297F, "Supplemental Arrows-B", 1 },
  { 0x2980, 0x29FF, "Miscellaneous Mathematical Symbols-B", 1 },
  { 0x2A00, 0x2AFF, "Supplemental Mathematical Operators", 1 },
  { 0x2B00, 0x2BFF, "Miscellaneous Symbols and Arrows", 1 },
  { 0x2C00, 0x2C5F, "Glagolitic", 1 },
  { 0x2C60, 0x2C7F, "Latin Extended-C", 1 },
  { 0x2C80, 0x2CFF, "Coptic", 1 },
  { 0x2D00, 0x2D2F, "Georgian Supplement", 1 },            // (Georgia)
  { 0x2D30, 0x2D7F, "Tifinagh", 1 },                       // (Morocco)
  { 0x2D80, 0x2DDF, "Ethiopic Extended", 1 },              // (Ethiopia)
  { 0x2DE0, 0x2DFF, "Cyrillic Extended-A", 1 },
  { 0x2E00, 0x2E7F, "Supplemental Punctuation", 1 },
  { 0x2E80, 0x2EFF, "CJK Radicals Supplement)", 1 },       // (CHINESE, JAPANESE, KOREAN
  { 0x2F00, 0x2FDF, "KangXi Radicals", 1 },
  { 0x2FF0, 0x2FFF, "Ideographic Description characters", 1 },
  { 0x3000, 0x303F, "CJK Symbols and Punctuation", 1 },    // (CHINESE, JAPANESE, KOREAN)
  { 0x3040, 0x309F, "Hiragana", 1 },                       // (Japan)
  { 0x30A0, 0x30FF, "Katakana", 1 },                       // (Japan)
  { 0x3100, 0x312F, "Bopomofo", 1 },
  { 0x3130, 0x318F, "Hangul Compatibility Jamo", 1 },      // (Korea)
  { 0x3190, 0x319F, "Kanbun", 1 },
  { 0x31A0, 0x32BF, "Bopomofo Extended", 1 },
  { 0x31F0, 0x31FF, "Katakana Phonetic Extensions", 1 },   // (Japan)
  { 0x3200, 0x32FF, "Enclosed CJK Letters and Months", 1 },
  { 0x3300, 0x33FF, "CJK Compatibility", 1 },
//{ 0x3400, 0x4DB5   (13312–19893)[!]㐅	㒅	㝬	㿜 CJK Unified Ideographs Extension A   
  { 0x3400, 0x37FF, "CJK Unified Ideographs Extension A", 7 },    // MULTIPLE PAGES, load all vvv
  { 0x3800, 0x3BFF, "CJK Unified Ideographs Extension A p2", 1 },
  { 0x3C00, 0x3FFF, "CJK Unified Ideographs Extension A p3", 1 },
  { 0x4000, 0x43FF, "CJK Unified Ideographs Extension A p4", 1 },    // CJK 
  { 0x4400, 0x47FF, "CJK Unified Ideographs Extension A p5", 1 },
  { 0x4800, 0x4BFF, "CJK Unified Ideographs Extension A p6", 1 },
  { 0x4C00, 0x4DB5, "CJK Unified Ideographs Extension A p7", 1 },    // MULTIPLE PAGES, load all ^^^

  { 0x4DC0, 0x4DFF, "Yijing Hexagram Symbols", 1 },
//{ 0x4E00, 0x9FFF   (19968–40959)[!]一	憨	田	龥 CJK Unified Ideographs   
  { 0x4E00, 0x51FF, "CJK Unified Ideographs", 21 },    // MULTIPLE PAGES, load all vvv
  { 0x5200, 0x55FF, "CJK Unified Ideographs p2", 1 },
  { 0x5600, 0x59FF, "CJK Unified Ideographs p3", 1 },
  { 0x5A00, 0x5DFF, "CJK Unified Ideographs p4", 1 },
  { 0x5E00, 0x61FF, "CJK Unified Ideographs p5", 1 },
  { 0x6200, 0x65FF, "CJK Unified Ideographs p6", 1 },
  { 0x6600, 0x69FF, "CJK Unified Ideographs p7", 1 },
  { 0x6A00, 0x6DFF, "CJK Unified Ideographs p8", 1 },
  { 0x6E00, 0x71FF, "CJK Unified Ideographs p9", 1 },
  { 0x7200, 0x75FF, "CJK Unified Ideographs p10", 1 },
  { 0x7600, 0x79FF, "CJK Unified Ideographs p11", 1 },
  { 0x7A00, 0x7DFF, "CJK Unified Ideographs p12", 1 },   // CJK 
  { 0x7E00, 0x81FF, "CJK Unified Ideographs p13", 1 },
  { 0x8200, 0x85FF, "CJK Unified Ideographs p14", 1 },
  { 0x8600, 0x89FF, "CJK Unified Ideographs p15", 1 },
  { 0x8A00, 0x8DFF, "CJK Unified Ideographs p16", 1 },
  { 0x8E00, 0x91FF, "CJK Unified Ideographs p17", 1 },
  { 0x9200, 0x95FF, "CJK Unified Ideographs p18", 1 },
  { 0x9600, 0x99FF, "CJK Unified Ideographs p19", 1 },
  { 0x9A00, 0x9DFF, "CJK Unified Ideographs p20", 1 },
  { 0x9E00, 0x9FFF, "CJK Unified Ideographs p21", 1 },   // MULTIPLE PAGES, load all ^^^

//{ 0xA000, 0xA48F   (40960–42127)[!]ꀀ	ꅴ	ꊩ	ꒌ Yi Syllables   
  { 0xA000, 0xA3FF, "Yi Syllables", 2 },              // MULTIPLE PAGES load all vvv
  { 0xA400, 0xA48F, "Yi Syllables p2", 1 },              //                         ^^^

  { 0xA490, 0xA4CF, "Yi Radicals", 1},
  { 0xA4D0, 0xA4FF, "Lisu", 1},
  { 0xA500, 0xA63F, "Vai", 1},                           // (Liberia)
  { 0xA640, 0xA69F, "Cyrillic Extended-B", 1},
  { 0xA6A0, 0xA6FF, "Bamum", 1},                         // (Cameroon)
  { 0xA700, 0xA71F, "Modifier Tone Letters", 1},
  { 0xA720, 0xA7FF, "Latin Extended-D", 1},
  { 0xA800, 0xA82F, "Syloti Nagri", 1},                  // (Bangladesh / India)
  { 0xA830, 0xA83F, "Common Indic Number Forms", 1},
  { 0xA840, 0xA87F, "Phags-pa", 1},
  { 0xA880, 0xA8DF, "Saurashtra", 1},                    // (India)
  { 0xA8E0, 0xA8FF, "Devanagari Extended", 1},
  { 0xA900, 0xA92F, "Kayah Li", 1},
  { 0xA930, 0xA95F, "Rejang", 1},                        // (Indonesia)
  { 0xA960, 0xA97F, "Hangul Jamo Extended-A", 1},        // (Korea)
  { 0xA980, 0xA9DF, "Javanese", 1},                      // (Indonesia)
  { 0xAA00, 0xAA5F, "Cham", 1},
  { 0xAA60, 0xAA7F, "Myanmar Extended-A", 1},            // (Myanmar)
  { 0xAA80, 0xAADF, "Tai Viet", 1},
  { 0xAB00, 0xAB2F, "Ethiopic Extended-A", 1},           // (Ethiopia)
  { 0xABC0, 0xABFF, "Meetei Mayek", 1}, // (India)
//{ 0xAC00, 0xD7A3   (44032–55203)[!]가	뮀	윸	힣 Hangul Syllables (Korea)
  { 0xAC00, 0xAFFF, "Hangul Syllables", 11}, // (Korea)      MULTIPLE PAGES load all vvv
  { 0xB000, 0xB3FF, "Hangul Syllables p2", 1},
  { 0xB400, 0xB7FF, "Hangul Syllables p3", 1},
  { 0xB800, 0xBBFF, "Hangul Syllables p4", 1},
  { 0xBC00, 0xBFFF, "Hangul Syllables p5", 1},
  { 0xC000, 0xC3FF, "Hangul Syllables p6", 1},
  { 0xC400, 0xC7FF, "Hangul Syllables p7", 1},
  { 0xC800, 0xCBFF, "Hangul Syllables p8", 1},
  { 0xCC00, 0xCFFF, "Hangul Syllables p9", 1},
  { 0xD000, 0xD3FF, "Hangul Syllables p10", 1},
  { 0xD400, 0xD7A3, "Hangul Syllables p11", 1}, //             MULTIPLE PAGES load all ^^^

  { 0xD7B0, 0xD7FF, "Hangul Jamo Extended-B", 1},          // (Korea)
  { 0xF900, 0xFAFF, "CJK Compatibility Ideographs", 1},
  { 0xFB00, 0xFB4F, "Alphabetic Presentation Forms", 1},
  { 0xFB50, 0xFDFF, "Arabic Presentation Forms-A", 1},
//{ 0xFE00, 0xFE0F, "These characters are not permitted in HTML Variation Selectors"},
  { 0xFE20, 0xFE2F, "Combining Half Marks", 1},
  { 0xFE30, 0xFE4F, "CJK Compatibility Forms", 1},
  { 0xFE50, 0xFE6F, "Small Form Variants", 1},
  { 0xFE70, 0xFEFF, "Arabic Presentation Forms-B", 1},
  { 0xFF00, 0xFFEF, "Halfwidth and Fullwidth Forms", 1},
  { 0xFFF0, 0xFFFF, "Specials", 1},                           // <<<<<<<<<<<<<<<<<<<<<
  { 0x10000, 0x1007F, "Linear B Syllabary", 1},
  { 0x10080, 0x100FF, "Linear B Ideograms", 1},
  { 0x10100, 0x1013F, "Aegean Numbers", 1},
  { 0x10140, 0x1018F, "Ancient Greek Numbers", 1},
  { 0x10190, 0x101CF, "Ancient Symbols", 1},
  { 0x101D0, 0x101FF, "Phaistos Disc", 1},
  { 0x10280, 0x1029F, "Lycian", 1},
  { 0x102A0, 0x102DF, "Carian", 1},
  { 0x10300, 0x1032F, "Old Italic", 1},
  { 0x10330, 0x1034F, "Gothic", 1},
  { 0x10380, 0x1039F, "Ugaritic", 1},
  { 0x10400, 0x1044F, "Deseret", 1},
  { 0x10450, 0x1047F, "Shavian", 1},
  { 0x10480, 0x104AF, "Osmanya", 1},                     // (Somalia)
  { 0x10800, 0x1083F, "Cypriot Syllabary", 1},
  { 0x10840, 0x1085F, "Imperial Aramaic", 1},
  { 0x10900, 0x1091F, "Phoenician", 1},
  { 0x10920, 0x1093F, "Lydian", 1},
  { 0x10A00, 0x10A5F, "Kharoshthi", 1},
  { 0x10A60, 0x10A7F, "Old South Arabian", 1},
  { 0x10B00, 0x10B3F, "Avestan", 1},
  { 0x10B40, 0x10B5F, "Inscriptional Parthian", 1},
  { 0x10B60, 0x10B7F, "Inscriptional Pahlavi", 1},
  { 0x10C00, 0x10C4F, "Old Turkic", 1},
  { 0x10E60, 0x10E7F, "Rumi Numeral Symbols", 1},
  { 0x11000, 0x1107F, "Brahmi", 1},                      // (India)
  { 0x11080, 0x110CF, "Kaithi", 1},                      // (India)
  { 0x110D0, 0x110FF, "Sora Sompeng", 1},                // (India)
  { 0x11100, 0x1114F, "Chakma", 1},                      // (Bangladesh / India)
  { 0x11180, 0x111DF, "Sharada", 1},                     // (Pakistan / India)
  { 0x11680, 0x116CF, "Takri", 1},                       // (India)
  { 0x12000, 0x123FF, "Cuneiform", 1},
  { 0x12400, 0x1247F, "Cuneiform Numbers and Punctuation", 1},
//{ 0x13000, 0x1342F (77824–78895)[!]Egyptian Hieroglyphs   
  { 0x13000, 0x133FF, "Egyptian Hieroglyphs", 2},     // MULTIPLE PAGES   vvv
  { 0x13400, 0x1342F, "Egyptian Hieroglyphs p2", 1},     //                  ^^^

  { 0x16800, 0x16A3F, "Bamum Supplement", 1},            // (Cameroon)
  { 0x16F00, 0x16F9F, "Miao", 1},                        // (China)
  { 0x1B000, 0x1B0FF, "Kana Supplement", 1},
  { 0x1D000, 0x1D0FF, "Byzantine Musical Symbols", 1},
  { 0x1D100, 0x1D1FF, "Musical Symbols", 1},
  { 0x1D200, 0x1D24F, "Ancient Greek Musical Notation", 1},
  { 0x1D300, 0x1D35F, "Tai Xuan Jing Symbols", 1},
  { 0x1D360, 0x1D37F, "Counting Rod Numerals", 1},
  { 0x1D400, 0x1D7FF, "Mathematical Alphanumeric Symbols", 1},
  { 0x1EE00, 0x1EEFF, "Arabic Mathematical Alphabetic Symbols", 1},
  { 0x1F000, 0x1F02F, "Mahjong Tiles", 1},
  { 0x1F030, 0x1F09F, "Domino Tiles", 1},
  { 0x1F0A0, 0x1F0FF, "Playing Cards", 1},
  { 0x1F100, 0x1F1FF, "Enclosed Alphanumeric Supplement", 1},
  { 0x1F200, 0x1F2FF, "Enclosed Ideographic Supplement", 1},
  { 0x1F300, 0x1F5FF, "Miscellaneous Symbols and Pictographs", 1},
  { 0x1F600, 0x1F64F, "Emoticons", 1},
  { 0x1F680, 0x1F6FF, "Transport and Map Symbols", 1},
  { 0x1F700, 0x1F77F, "Alchemical Symbols", 1},
//{ 0x20000, 0x2A6D6 (131072–173782)[!] CJK Unified Ideographs Extension B (CHINESE, JAPANESE, KOREAN, 46k !!!!)
  { 0x20000, 0x203FF, "CJK Unified Ideographs Extension B", 42}, // MULTIPLE PAGES, load all vvv
  { 0x20400, 0x207FF, "CJK Unified Ideographs Extension B p2", 1},
  { 0x20800, 0x20BFF, "CJK Unified Ideographs Extension B p3", 1},
  { 0x20C00, 0x20FFF, "CJK Unified Ideographs Extension B p4", 1},
  { 0x21000, 0x213FF, "CJK Unified Ideographs Extension B p5", 1},
  { 0x21400, 0x217FF, "CJK Unified Ideographs Extension B p6", 1},
  { 0x21800, 0x21BFF, "CJK Unified Ideographs Extension B p7", 1},
  { 0x21C00, 0x21FFF, "CJK Unified Ideographs Extension B p8", 1},
  { 0x22000, 0x223FF, "CJK Unified Ideographs Extension B p9", 1},
  { 0x22400, 0x227FF, "CJK Unified Ideographs Extension B p10", 1},
  { 0x22800, 0x22BFF, "CJK Unified Ideographs Extension B p11", 1},
  { 0x22C00, 0x22FFF, "CJK Unified Ideographs Extension B p12", 1},
  { 0x23000, 0x233FF, "CJK Unified Ideographs Extension B p13", 1},
  { 0x23400, 0x237FF, "CJK Unified Ideographs Extension B p14", 1},
  { 0x23800, 0x23BFF, "CJK Unified Ideographs Extension B p15", 1},
  { 0x23C00, 0x23FFF, "CJK Unified Ideographs Extension B p16", 1},
  { 0x24000, 0x243FF, "CJK Unified Ideographs Extension B p17", 1},
  { 0x24400, 0x247FF, "CJK Unified Ideographs Extension B p18", 1},
  { 0x24800, 0x24BFF, "CJK Unified Ideographs Extension B p19", 1},
  { 0x24C00, 0x24FFF, "CJK Unified Ideographs Extension B p20", 1},
  { 0x25000, 0x253FF, "CJK Unified Ideographs Extension B p21", 1},
  { 0x25400, 0x257FF, "CJK Unified Ideographs Extension B p22", 1},
  { 0x25800, 0x25BFF, "CJK Unified Ideographs Extension B p23", 1},
  { 0x25C00, 0x25FFF, "CJK Unified Ideographs Extension B p24", 1},
  { 0x26000, 0x263FF, "CJK Unified Ideographs Extension B p25", 1},
  { 0x26400, 0x267FF, "CJK Unified Ideographs Extension B p26", 1},
  { 0x26800, 0x26BFF, "CJK Unified Ideographs Extension B p27", 1},
  { 0x26C00, 0x26FFF, "CJK Unified Ideographs Extension B p28", 1},
  { 0x27000, 0x273FF, "CJK Unified Ideographs Extension B p29", 1},
  { 0x27400, 0x277FF, "CJK Unified Ideographs Extension B p30", 1},
  { 0x27800, 0x27BFF, "CJK Unified Ideographs Extension B p31", 1},
  { 0x27C00, 0x27FFF, "CJK Unified Ideographs Extension B p32", 1},
  { 0x28000, 0x283FF, "CJK Unified Ideographs Extension B p33", 1},
  { 0x28400, 0x287FF, "CJK Unified Ideographs Extension B p34", 1},
  { 0x28800, 0x28BFF, "CJK Unified Ideographs Extension B p35", 1},
  { 0x28C00, 0x28FFF, "CJK Unified Ideographs Extension B p36", 1},
  { 0x29000, 0x293FF, "CJK Unified Ideographs Extension B p37", 1},
  { 0x29400, 0x297FF, "CJK Unified Ideographs Extension B p38", 1},
  { 0x29800, 0x29BFF, "CJK Unified Ideographs Extension B p39", 1},
  { 0x29C00, 0x29FFF, "CJK Unified Ideographs Extension B p40", 1},
  { 0x2A000, 0x2A3FF, "CJK Unified Ideographs Extension B p41", 1},
  { 0x2A400, 0x2A6D6, "CJK Unified Ideographs Extension B p42", 1}, // MULTIPLE PAGES, load all ^^^

//{ 0x2A700, 0x2B73F (173824–177983)[!] CJK Unified Ideographs Extension C (CHINESE, JAPANESE, KOREAN, 4k !!!!)
  { 0xA700, 0xAAFF, "CJK Unified Ideographs Extension C", 5},    // MULTIPLE PAGES, load all vvv
  { 0xAB00, 0xAEFF, "CJK Unified Ideographs Extension C p2", 1},
  { 0xAF00, 0xB2FF, "CJK Unified Ideographs Extension C p3", 1},
  { 0xB300, 0xB6FF, "CJK Unified Ideographs Extension C p4", 1},
  { 0xB700, 0xB73F, "CJK Unified Ideographs Extension C p5", 1},    // MULTIPLE PAGES, load all ^^^

  { 0x2B740, 0x2B81F, "CJK Unified Ideographs Extension D", 1},
  { 0x2F800, 0x2FA1F, "CJK Compatibility Ideographs Supplement", 1},
  { 0, 0, "", 0}                                                    // TERMINATOR ... au be bak
};


int16 _ixPagesList::nrPages= _ixPagesList::computeNrPages();


