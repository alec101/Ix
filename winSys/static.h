#pragma once


class ixStaticText: public ixBaseWindow {
public:

  ixTxtData text;
  int32 textX, textY;             // NYI - text position inside the window

  struct Usage: public ixBaseWindow::Usage {
    // the text is readonly, of course
    unsigned oneLine: 1;        // [def: 0] limit the window to print only one line
    unsigned hasCursor: 1;      // [def: 1] text has a cursor that is movable
    unsigned selection: 1;      // [def: 1] text can be selected - all selection functions avaible
    

    Usage(ixBaseWindow *in_p): ixBaseWindow::Usage(in_p) { delData(); }
    void delData() { oneLine= 0; hasCursor= selection= 1; }

  protected:
    //ixStaticText *_parent;
    friend class ixStaticText;
  } usage;

  Is is;

  // funcs

  //void draw(Ix *in_ix, ixWSsubStyleBase *in_style= null);

  void updateSizeFromText();      /// measures the text size and shrinks or expands the size of the object
  void setText32(str32 *in_txt);   // sets the window's text, accepts a str32

  void setFont(const char *filename, const char *fnt, int size);
  void setFont(const void *);

  int32 getMinDx();
  int32 getMinDy();

  // virtual funcs that must be changed

  void resize(int32 dx, int32 dy);          // resizes window, this will move all children hooked on the right and bottom side
  void resizeDelta(int32 dx, int32 dy);     // resizes window (enlarges, shrinkens by specified amount), this will move all children hooked on the right and bottom side
  void setPos(int32 x0, int32 y0, int32 dx, int32 dy); // sets position and size of the window
  void _computeChildArea();     // computes the total child area - call after a child moves/resizes - updated with textDx/Dy

  // constructor / destructor

  ixStaticText();
  ~ixStaticText();
  void delData();

private:
  bool _update(bool mouseInside, bool updateChildren= true);
  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  str8 _fontFileName;
  str8 _fontName;
  int32 _fontSize;

  bool _checkLimits(char32 unicode);
  void _applyColorsFromStyle() { ixBaseWindow::_applyColorsFromStyle(); colorBRD= ((ixWSgenericStyle *)style)->colorBRD, colorBRDfocus= ((ixWSgenericStyle *)style)->colorBRDfocus; }
  friend class ixWindow;
  friend class ixWinSys;
  friend class ixTxtData;
};













