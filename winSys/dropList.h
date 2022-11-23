#pragma once


class ixDropListData: public chainData {
  str32 text;               // option text
  rectf pos;                // option exact coordinates in the dropList, from parentX0/Y0 (basically an offset, not exact VDcoords)
  friend class ixDropList;
};



class ixDropList: public ixBaseWindow {
public:

  // ▄▄▄▄▄▄▄▄▄
  // █ usage █
  // ▀▀▀▀▀▀▀▀▀
  struct Usage: public ixBaseWindow::Usage {
    void setListMaxLength(float in_len);  // sets the list maximum drop down length. if options don't fit, a scroll appears
    
  private:
    Usage(ixBaseWindow *in_p): ixBaseWindow::Usage(in_p), _maxLength(0) {}
    
    float _maxLength;       // list maximum length. if the options don't fit, the scroll shows up. 
    friend class ixDropList;
  } usage;

  // ▄▄▄▄▄▄
  // █ is █
  // ▀▀▀▀▀▀
  struct Is: public ixBaseWindow::Is {
    unsigned expanded: 1;   // window is droped down / expanded down / unfolded
    Is(): ixBaseWindow::Is() { expanded= 0; }
  } is;
  

  chainList optionList;     // list with all the list options
  int32 selNr;              // selected button number
  ixDropListData *sel;      // selected button direct pointer
  float buttonDx, buttonDy; // button size - if not set, it will use text diameters, font size (excluding circle box)
  float listLen;            // list drop down length (minus the current option)
  //ixFontStyle font;         // font that will be used for the text

  // funcs

  void setButtonDxDy(float dx, float dy); // sets each option dimensions
  void addOption(cchar *in_text);
  void addOptionAfter(cchar *in_text, ixDropListData *in_after);
  void addOptionAftern(cchar *in_text, int32 in_after);
  void delAllOptions();
  

  void select(int32);   // select an option from the list, directly

  // constructor / destructor

  ixDropList();
  ~ixDropList();
  void delData();

protected:
  bool _dropped;
  ixScroll *_scr;           // list's scrollbar, it can be hidden if not needed

  void _computeRects();     // list is only horizontal, therefors _computeinitialx0y0 not required
  void _computeScr();       // computes everything needed for the _scr;

  bool _update(bool updateChildren= true);
  #ifdef IX_USE_OPENGL
  virtual void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  virtual void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  friend class ixWinSys;
};










