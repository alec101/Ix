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

// if not WRAP/JUSTIFIED/WORDWRAP, there is a horizontal scrollbar, and the line extends until \n found
// WORDWRAP is DISABLED until i find a real reason for it's existance, cuz a hexeditor will not depend on it, i don't think
#ifndef IX_TXT_ALIGNMENT
#define IX_TXT_ALIGNMENT 1
#define IX_TXT_ALN_START      0x01
#define IX_TXT_ALN_END        0x02
#define IX_TXT_ALN_CENTER     0x04
#define IX_TXT_ALN_WRAP       0x08      // normal word wrap, normal spaceSize, no horizontal scroll bar
#define IX_TXT_ALN_JUSTIFIED  0x10      // word wrap, spaceSize extends so text ending is aligned too, no horiz scrollbar
#define IX_TXT_ALN_FIXEDWIDTH 0x18      // wrap/justified check, not defining anything new (check against both bits)
//#define IX_TXT_ALN_WORDWRAP   0x20      // wrap, it don't take into consideration where the window line ends, words can be cut to fit next line

#define IX_TXT_ALN_START_WRAP       0x09
#define IX_TXT_ALN_END_WRAP         0x0A
#define IX_TXT_ALN_CENTER_WRAP      0x0C
#define IX_TXT_ALN_START_JUSTIFIED  0x11
#define IX_TXT_ALN_END_JUSTIFIED    0x12
#define IX_TXT_ALN_CENTER_JUSTIFIED 0x14
//#define IX_TXT_ALN_START_WORDWRAP   0x21
//#define IX_TXT_ALN_END_WORDWRAP     0x22
//#define IX_TXT_ALN_CENTER_WORDWRAP  0x24
#endif


//struct ixTxtCursor;
class ixTxtSel;
class ixWSshader;


// structure with all the text database
class ixTxtData {
public:

  class Wline;

  // each text line data chainlist, used in TextData
  class Line: public chainData {
  public:
    str32 text;         // the text
    Wline *wline;       // points to the first winLine that wraps this text
    Line() { wline= null; }
  };


  chainList lines;      // will hold ixTxtLine

  float textDx, textDy; // total size of text, width and height, in pixels
  int32 nrUnicodes;     // total number of unicodes of all combined lines
  ixFontStyle font;     // font.selFont should be changed with ixTxtData::setFont func, so it will update the whole text, otherwise, the style is to be freely changed
  

  // >>> THERE CAN ALSO BE <end justified>... after a \n, the text aligns to the right <<<<<
  int32 alignment;      // 0= no alignment, 1= start alignment[default]; 2= end alignment; 3= centered; 4= justified; 5= word wrap
  int8 orientation;     // left to right, right to left, up to down, down to up
  
  // cursor class - blinking cursor in ixStatic and ixEdit
  struct Cursor {
    int32 pos;                        // unicode position, in Line
    int32 line;                       // line number (Line chainlist)
    int32 wline;                      // Wline number (_wrapLines chainlist)
    ixTxtData::Line *pLine;           // pointer to the Line it's on
    ixTxtData::Wline *pWline;         // pointer to the Wline it's on
    float x0, y0;                     // position in pixels (y0 when in horizontal, is the top of the first line, basically, not the bottom)

    float drawWidth;                  // [def:2* unitSize] in pixels
    int32 blinkRate;                  // [def:250ms] in milliseconds
    vec4 color;                       // [def:white] cursor color (rgba, 0.0f - 1.0f range)

    // funcs

    void makeSureInBounds();          // makes sure the cursor is in the text boundaries
    void makeSureVisible();           // makes sure the cursor is visible in the window

    ixTxtData::Wline *getWline(int32 *out_wline= null); // returns the current Wline the cursor is at; out_wline, is the line number
    inline void updateWline() { pWline= getWline(&wline); }
    void updateWlineAndCoords();

    // funcs for positioning

    void resetToStart();
    void resetToEnd();
    void up();        void down();    void left();      void right();
    void home();      void end();     void pgUp();      void pgDown();
    void ctrlHome();  void ctrlEnd(); void ctrlPgUp();  void ctrlPgDown();

    inline void decreaseLine(int32 in_nrLines= 1, bool in_computePosInPixels= true);
    inline void increaseLine(int32 in_nrLines= 1, bool in_computePosInPixels= true);
    void decreaseUnicode(int32 in_nrUnicodes= 1);
    void increaseUnicode(int32 in_nrUnicodes= 1);
    void advanceLineForPixels(float in_advanceInPixels);

    // ON WINDOW RESIZE, CURSOR POS MUST BE REMEMBERED, IN UNICODE COORDS
    // AND RE-POSITIONED ON WLINES, WITH THE UNICODE COORDS

    Cursor() { _parent= null; pos= line= 0; pLine= null; pWline= null; wline= 0; drawWidth= 2; blinkRate= 150; color.set(1.0f, 1.0f, 1.0f, 1.0f); _readTime= 0; _show= false; }
    void delData() { pos= line= 0; pLine= null; pWline= null; wline= 0; }

  private:
    #ifdef IX_USE_OPENGL
    void _glDraw(Ix *in_ix, const recti in_pos, const vec3i in_scroll);
    #endif
    #ifdef IX_USE_VULKAN
    void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, const rectf in_pos, const vec3 in_scroll);
    #endif

    float _getPosInPixels();    // returns either x0 or y0 depending on orientation of the cursor position
    float _getWlineInPixels();  // returns either x0 or y0 depending on text orientation
    inline void _updateX0Y0() { if(_parent->orientation& IX_TXT_HORIZONTAL ) x0= _getPosInPixels(), y0= _getWlineInPixels(); else x0= _getWlineInPixels(), y0= _getPosInPixels(); }

    void _setPosInPixels(float in_pixels);    // sets the position to the specified pixel position, or as close as possible to that
    void _setLineAndPosInPixels(float x, float y);  // sets cursor coords in the text, as close to the provided x and y coords in pixels
    ixTxtData *_parent;
    int64 _readTime;                // [internal] used for blinking
    bool _show;                     // [internal] used for blinking

    friend class ixEdit;
    friend class ixStaticText;
    friend class ixTxtData;
  };


  // selection class - select and copy a certain portion of the text
  // paste is within ixEdit, atm.
  class Sel {
  public:
    int32 start, end, startLine, endLine;
    vec4 color;
    ixTxtData::Line *pStartLine, *pEndLine;
    ixTxtData::Wline *pStartWline, *pEndWline;
    int32 startWline, endWline;
    float startX0, endX0;
    float startY0, endY0;

    Sel() { _parent= null; color.set(0.1f, 0.2f, 1.0f, 1.0f); delData(); }

    inline void delData() { start= end= startLine= endLine= 0; pStartLine= pEndLine= null; pStartWline= pEndWline= null; startWline= endWline= 0; startX0= endX0= startY0= endY0= 0; }
    inline int32 getStart()     { return (startLine== endLine? (MIN(start, end)): (startLine< endLine? start: end)); }
    inline int32 getEnd()       { return (startLine== endLine? (MAX(start, end)): (startLine< endLine? end: start)); }
    inline int32 getStartLine() { return MIN(startLine, endLine); }
    inline int32 getEndLine()   { return MAX(startLine, endLine); }
    inline int32 getStartWline() { return MIN(startWline, endWline); }
    inline int32 getEndWline()   { return MAX(startWline, endWline); }
    inline float getStartX0()   { return (startWline== endWline? MIN(startX0, endX0): (startWline< endWline? startX0: endX0)); }
    inline float getEndX0()     { return (startWline== endWline? MAX(startX0, endX0): (startWline< endWline? endX0: startX0)); }
    inline float getStartY0()   { return (startWline== endWline? MIN(startY0, endY0): (startWline< endWline? startY0: endY0)); }
    inline float getEndY0()     { return (startWline== endWline? MAX(startY0, endY0): (startWline< endWline? endY0: startY0)); }
    //inline int32 getPixelCoords(int32 *out_startX0, int32 *out_endX0, int32 *out_startY0, int32 *out_endY0) { }
    inline Line *getStartLinePointer() { return (startLine<= endLine? pStartLine: pEndLine); }
    inline Line *getEndLinePointer()   { return (startLine<= endLine? pEndLine: pStartLine); }
    inline Wline *getStartWlinePointer() { return (startWline<= endWline? pStartWline: pEndWline); }
    inline Wline *getEndWlinePointer()   { return (startWline<= endWline? pEndWline: pStartWline); }
    //inline ixTxtData::Wline *getStartWlinePointer() { return /*(startWline<= endWline? pStartWline: pEndWline);*/ pStartWline; }
    //inline ixTxtData::Wline *getEndWlinePointer() { return /*(startWline<= endWline? pEndWline: pStartWline);*/ pEndWline; }

    //inline void get(int32 *out_start, int32 *out_end, int32 *out_startLine, int32 *out_endLine) { if(out_start) *out_start= getStart(); if(out_end) *out_end= getEnd(); if(out_startLine) *out_startLine= getStartLine(); if(out_endLine) *out_endLine= getEndLine(); }
    inline bool operator!() { if(start|| end|| startLine|| endLine) return false; return true; }
    inline operator bool () { if(start|| end|| startLine|| endLine) return true; return false; }

    void addLeft();     void addRight();    void addUp();       void addDown();
    void addHome();     void addEnd();      void addPgUp();     void addPgDown();
    void addCtrlHome(); void addCtrlEnd();  void addCtrlPgUp(); void addCtrlPgDown();

    void checkValid() { if((startLine== endLine) && (start== end)) delData(); }

    void delSelection();
    void copy(str32 *out_str);
    void paste(str32 *in_str);
    

  private:

    void _glDraw(Ix *in_ix, const rectf in_pos, const vec3 in_scroll);
    void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, const rectf in_pos, const vec3 in_scroll);

    ixTxtData *_parent;
    void _startSelection();
    void _updateEndFromCursor();

    friend class ixEdit;
    friend class ixStaticText;
    friend class ixTxtData;
  };


  Cursor cur;      // text cursor
  Sel sel;         // text selection

  // funcs 

  /// sets a font and updates the whole text to fit with the new font
  /// the style of the font can be freely changed, with the ixTxtData::font struct
  void setFont(void *);

  void clearText();
  void findTextDx();              // finds and updates textDx
  void findTextDy();              // finds and updates textDy
  void findTextDxDy();            // finds and updates both textDx and textDy
  //void updateAllWlinesDxDy();     // updates every wline's dx & dy
  inline void computeAndSetChildArea() { findTextDxDy(); _parent->_childArea.setD(0, 0, textDx, textDy); }
  void findUnicodeLineForCoords(float in_x, float in_y, int32 *out_unicode, int32 *out_line);
  inline bool checkLimits(char32 unicode);
  inline void setDebug(bool b) { _debug= b; }



  ixTxtData(ixBaseWindow *);
  ~ixTxtData() { delData(); }
  void delData();

  // alignment / wrap funcs

  class Wline: public chainData {
  public:
    Line *line;         // target Line class
    //int32 lineNr;       // line number (Line class)
    int32 startUnicode; // starting unicode of the line
    int32 nrUnicodes;   // number of unicodes in the line
    float dx;           // text width in pixels
    float dy;           // text height in pixels
    float spaceSize;    // space size, default 0.0f, used for justified alignment, mainly
    int32 alignment;    // [def:IX_TXT_ALN_START] see IX_TXT_ALIGNMENT at header start
    Wline() { line= null; /*lineNr=*/ startUnicode= nrUnicodes= 0; dx= dy= 0; spaceSize= 0.0f; alignment= IX_TXT_ALN_START; }

    //inline void computeDx(int8 in_orientation, void *in_font);
    //inline void computeDy(int8 in_orientation, void *in_font);
  };

protected:

  #ifdef IX_USE_OPENGL
  void _glDraw(Ix *ix, const recti pos, const vec3i scroll);    // draws the text and selection and cursor, 
  #endif
  #ifdef IX_USE_VULKAN
  void _vkDraw(VkCommandBuffer in_cmd, Ix *ix, const rectf pos, const vec3 scroll);    // draws the text and selection and cursor, 
  #endif
  


  bool _update(); // handles keyboard input, text selection, depending if _parent has flags that allow any of such operations



  // first seen line - window position on the text, 


  // this should basically be the second cursor but without any position, only line and wline, everything about them
  //  based where one of the cursors goes, the other can follow
  struct ViewPos {
    int32 line;       
    int32 wline;      
    Wline *pWline;    
    Line *pLine;      
    float pos;        // x0 or y0, depending on orientation of the first line in view

    void resetToStart();
    void moveToCursor();
    void moveToScrollPosition();
    void moveRelativeToCursor(float in_pixels);     // in_pixels can be negative or positive to the cursor position in pixels
    void advanceLine();
    void decreaseLine();

    ViewPos(ixTxtData *in_parent): _parent(in_parent) { delData(); }

    inline void delData() { line= wline= 0; pos= 0.0f; pWline= null; pLine= null; }
  protected:
    ixTxtData *_parent;
  } _view;

  

  // wrap lines - actual lines in the window, not lines in the text

  chainList _wrapLines;     // will hold wlinData
  float _wrapLen;           // if any alignment happens, it must be within this length. text direction is a thing, and it is computed, therefore it is named only 'length'
  void _computeWrapLen();
  void _updateWrapList(Line *l= null, bool updateCur= true);
  void _delWrapForLine(Line *);   // this func CANNOT UPDATE THE cursor/_view, properly!!!
  inline float _getWlineX0orY0InPixels(Wline *);  // depending on alignment and orientation, the text start position. for horizontal left to right text, that is x0, and always 0
  // remaining private stuff

  char32 *_fixedBuffer;   // if using a fixed buffer, this will hold the array of it
  ixBaseWindow *_parent;
  bool _debug;

  friend class ixEdit;
  friend class ixStaticText;
  friend class ixTxtSel;
  friend struct ixTxtCursor;
  friend class ixTxtWinLines;
  friend struct _ixWlineData;
  friend class ixScroll;
  friend class ixBaseWindow;
};






