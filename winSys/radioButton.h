#pragma once



class ixWinRadioData: public chainData {
  str32 text;         // text shown on the button
  rectf pos;
  friend class ixRadioButton;
};


class ixRadioButton: public ixBaseWindow {
public:

  // ▄▄▄▄▄▄▄▄▄
  // █ usage █
  // ▀▀▀▀▀▀▀▀▀
  struct Usage: public ixBaseWindow::Usage {
  protected:
    unsigned radio: 1;          // if <true>, it's a classic radio button with a circle; if <false>, it's a button list without a circle; call setRadioCircle() to set
    int8 textHeading;           // use setTextHeading() - the text orientation (left to right or up to down or right to left)
    int8 listHeading;           // use setListHeading() - left to right  or  up to down
  public:

    // configuration functions

    void setTextHeading(ixOrientation);   // sets the text orientation on the buttons
    void setListHeading(ixOrientation);   // sets the list of buttons direction (horizontal or vertical)
    inline void setRadioCircle(bool in_circle) { radio= in_circle; ((ixRadioButton *)_win)->_computeRects(); }
    
    Usage(ixBaseWindow *in_p): ixBaseWindow::Usage(in_p) {}

  private:
    friend class ixRadioButton;
  } usage;

  Is is;

  chainList buttonList;     // list with all the radio buttons
  int32 selNr;              // selected button number
  ixWinRadioData *sel;      // selected button direct pointer
  float buttonDx, buttonDy; // button size    NYI >>> if not set, it will use text diameters, font size (excluding circle box) - origin.dx/dy will hold the original dx/dy
  //ixFontStyle font;         // font that will be used for the text

  // funcs

  void addRadioButton(cchar *name, bool implicitSelect= false);       // adds a button to the radio control
  void changeRadioButtoni(int buttonNr, cchar *newName);
  void changeRadioButton(cchar *oldName, cchar *newName);

  void delRadioButtonTxt(cchar *name);    // deletes button named <name> from the radio control
  void delRadioButtonn(int n);            // deletes button number <n> from the radio control

  void selectRadioButtonTxt(cchar *in_buttonText);    // sets the button with <in_buttonName> as activated
  void selectRadioButtonn(int in_number);             // sets the button number <in_number> as activated

  // constructor / destructor

  ixRadioButton();
  ~ixRadioButton();
  void delData();

protected:
  
  bool _update(bool updateChildren= true);
  #ifdef IX_USE_OPENGL
  void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  #ifdef IX_USE_VULKAN
  void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  virtual void _computeAll();
  virtual void _computeAllDelta(float x, float y);

  float _unit;
  void _computeInitialX0Y0(float *out_x0, float *out_y0);
  void _computeRects(float *in_initialX0= null, float *in_initialY0= null); //  computes <pos>, each button x0,y0,xe,ye; if left 0, it will compute them, but if the heading is changing, they must be computed beforehand;
};




