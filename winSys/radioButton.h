#pragma once



class ixWinRadioData: public chainData {
  str32 text;         // text shown on the button
  //bool selected;      // current button is the selected radio button
  recti pos;
  //int32 x0, y0, xe, ye;
  friend class ixRadioButton;
};


class ixRadioButton: public ixBaseWindow {
public:
  

  // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
  // THIS SHOULD BE THE DEFINING RULE OF ANY Usage CLASS OF ALL WINDOWS
  // ANY VARIABLE THAT REQUIRES MORE CHANGES INTERNALLY FOR THINGS TO CHANGE, MUST BE PRIVATE, AND FUNCTIONS TO CHANGE IT, MUST BE ADDED
  // ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
  // and maybe the usage funcs should be in usage class, too


  // ▄▄▄▄▄▄▄▄▄
  // █ usage █
  // ▀▀▀▀▀▀▀▀▀

  struct Usage: public ixBaseWindow::Usage {
    unsigned radio: 1;          // if <true>, it's a classic radio button with a circle; if <false>, it's a button list without a circle
  protected:
    int8 textHeading;           // use setTextHeading() - the text orientation (left to right or up to down or right to left)
    int8 listHeading;           // use setListHeading() - left to right  or  up to down
  public:
    //int32 circleDiameter;       // change to make the circle bigger or smaller of each buttons

    // configuration functions

    void setTextHeading(ixOrientation);   // sets the text orientation on the buttons
    void setListHeading(ixOrientation);   // sets the list of buttons direction (horizontal or vertical)

    // might just work:
    inline void setRadioCircle(bool in_circle) { radio= in_circle; }  // <<<<< ???? IS THIS SUFFICE?
    
  private:
    ixRadioButton *_parent;
    friend class ixRadioButton;
  } usage;

  chainList buttonList;     // list with all the radio buttons
  int32 selNr;              // selected button number
  ixWinRadioData *sel;      // selected button direct pointer
  int32 buttonDx, buttonDy; // button size    NYI >>> if not set, it will use text diameters, font size (excluding circle box)
  ixFontStyle font;         // font that will be used for the text

  // funcs

  //void draw(Ix *in_ix, ixWSsubStyleBase *in_style= null); // draws the window; can pass another style to use - ATM this is a simple but not elegant way draw the window in another state

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
  
  bool _update(bool mouseInside, bool updateChildren= true);
  #ifdef IX_USE_OPENGL
  void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif

  void _computeInitialX0Y0(int32 *out_x0, int32 *out_y0);
  void _computeRects(int32 in_initialX0= INT32_MIN, int32 in_initialY0= INT32_MIN); //  computes <pos>, each button x0,y0,xe,ye; if left 0, it will compute them, but if the heading is changing, they must be computed beforehand;
};















