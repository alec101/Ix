#pragma once

class ixProgressBar: public ixBaseWindow {
public:

  // ▄▄▄▄▄▄▄▄▄
  // █ usage █
  // ▀▀▀▀▀▀▀▀▀
  struct Usage:public ixBaseWindow::Usage {

    // barType:
    // 0: percentage bar (0%-100%)
    // 1: custom amount (0 to n)
    // 2: time bar (n seconds)
    // 3: -n to +n, with 0 in the middle (or close to), the bar would go left from center for negative, right from center for positive
    // NR3 SHOULD BE DONE LATER, IT'S NOT GONNA BE A COMMON USED BAR, IF AT ALL
    int32 barType;
    
    int32 value1;               // [def:0]   the [left / down] value of the bar
    int32 value2;               // [def:100] the [right / up] value of the bar
    
    bool txtShowValue;          // [def:true] enable/disable text of current position
    uint32 txtPosition;         // [def:center] text on bar position 0= center, 1= left/ down, 2= right
    // NOT ATM but in future: uint32 txtDecimals;         // [def:0]      text number of decimals (nnnn.dd= 2, nnnn= 0)
    ixOrientation orientation;  // [def:RIGHT] bar orientation / the growth direction of the bar. 


    void setPercentageBar(int32 firstPercentage, int32 secondPercentage, ixOrientation in_o= RIGHT);
    void setCustomAmountBar(int32 firstAmount, int32 secondAmount, ixOrientation in_o= RIGHT);
    void setTimeBar(int32 startingSecond, int32 endingSecond, ixOrientation in_o= RIGHT);
    
    void setOrientation(ixOrientation in_o) { orientation= in_o; ((ixProgressBar *)_win)->resetPosition(); }
    


    Usage(ixBaseWindow *in_p): ixBaseWindow::Usage(in_p) { barType= value1= value2= -1; txtShowValue= true, txtPosition= 0; }

  private:
    friend class ixProgressBar;
  } usage;

  Is is;

  int32 position;             // [def:0]   current bar position / current percentage
  ixFontStyle font;           // font that will be used for the text on the bar

  

  // funcs

  //void draw(Ix *in_ix, ixWSsubStyleBase *in_style= null); // draws the window; can pass another style to use - ATM this is a simple but not elegant way draw the window in another state

  void setPosition(int32 in_pos);
  void resetPosition();   // sets position on value 1 or 2, depending on the orientation

  
  void move(int32 x0, int32 y0);            // moves window and all children to specified coords
  void moveDelta(int32 dx, int32 dy);       // moves window and all children a delta distance (deltax, deltay)
  void resize(int32 dx, int32 dy);          // resizes window, this will move all children hooked on the right and bottom side
  void resizeDelta(int32 dx, int32 dy);     // resizes window (enlarges, shrinkens by specified amount), this will move all children hooked on the right and bottom side
  void setPos(int32 x0, int32 y0, int32 dx, int32 dy); // sets position and size of the window


  // constructor / destructor

  ixProgressBar();
  ~ixProgressBar();
  void delData();

protected:

  bool _update(bool mouseInside, bool updateChildren= true);
  #ifdef IX_USE_OPENGL
  void _glDraw(Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  #ifdef IX_USE_VULKAN
  void _vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style= null);  // the drue draw func, init stuff, pass it to this
  #endif
  recti _filledRect;

  void _asurePositionInBounds(); // makes sure position is in bounds of value1 and value2
  void _computeFillingRect();     // computes the bar filled rect, based on position
};










