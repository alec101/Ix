#pragma once

enum class ixEBorder: uint8 {
  top= 0,
  right= 1,
  bottom= 2,
  left= 3,
  topLeft= 4,
  topRight= 5,
  bottomRight= 6,
  bottomLeft= 7
};

// window types <_type> private var
enum class ixeWinType: uint16 {
  baseWindow= 0,  // ixBaseWindow
  window,         // ixWindow
  button,         // ixButton
  staticText,     // ixStatic
  edit,           // ixEdit
  title,          // ixTitle
  scrollBar,      // ixScroll
  menu,           // ixMenu         "menu.h"
  menuBar,        // ixMenuBar      "menu.h"
  radioButton,    // ixRadioButton  
  dropList,       // ixDropList     
  progressBar,    // ixProgressBar  

  endOfList
};

#include "ix/winSys/style.h"
#include "ix/winSys/winBase.h"
#include "ix/winSys/txtShared.h"
#include "ix/winSys/scroll.h"
#include "ix/winSys/button.h"
#include "ix/winSys/edit.h"
#include "ix/winSys/static.h"
#include "ix/winSys/window.h"
#include "ix/winSys/radioButton.h"
#include "ix/winSys/dropList.h"
#include "ix/winSys/menu.h"
#include "ix/winSys/progressBar.h"

#include "ix/winSys/eventSys.h"




enum class ixeWSflags: uint32 {
  none=         0x0000,

  mouseUsed=    0x0001,     // any mouse activity used in any of the windows
  keyboardUsed= 0x0002,     // any keyboard activity used in any of the windows
  joyUsed=      0x0004,     // any joystick activity used in any of the windows
  gpUsed=       0x0008,     // any gamepad activity used in any of the windows
  gwUsed=       0x0010,     // any gamewheel activity used in any of the windows

  endOfList
};





//class ixWSshader;

// core of the whole window system

class ixWinSys {
public:

  ixWSstyle *selStyle;      // current 'selected' style that will be used when creating a new object

  ixBaseWindow *focus;      // WIP - window that has focus - USING IT FOR IXMENUS AND THE SIMPLICITY IS MAKING IT WORK GREAT
  ixBaseWindow *hover;      // window that is hovered
  ixWinEventSys eventSys;   // event system


  ixFlags32 flags;          // [ixeWSflags: flag enum] 

  // CAN YOU REALLY HAVE 2 FOCUSES? ... AS A HUMAN YOU CAN'T SO WHY SHOULD THERE BE MORE THAN ONE VARIABLE WITH THE FOCUS?

  //ixBaseWindow *kbFocus;    // WIP - window that has keyboard focus       SHOULD THIS EXIST?
  //ixBaseWindow *joyFocus;   // WIP - window that has joystick/gpad focus  SHOUDL THIS EXIST?
  //joyHover
  //keyHover
  //mouseHover




  //this chainList must be more than a list.
  //  the drawing is done from last to first
  //  the updating is done from first to last
  //  when focus on a control is gained, that window moves to first
  //  and this should be the simplest way to do the ordering of windows
    
  chainList topObjects;          // all windows are put here

  ixWindow *createWindow(cchar *name, ixBaseWindow *parent, int32 x, int32 y, int32 dx, int32 dy);
  ixButton *createButton(cchar *text, ixBaseWindow *parent, int32 x, int32 y, int32 dx, int32 dy);
  ixEdit *createEdit(ixBaseWindow *, int32 x0, int32 y0, int32 dx, int32 dy);
  ixRadioButton *createRadioButton(ixBaseWindow *parent, int32 x, int32 y, int32 dx, int32 dy);
  ixDropList *createDropList(ixBaseWindow *parent, int32 x, int32 y, int32 buttonDx, int32 buttonDy);
  ixProgressBar *createProgressBar(ixBaseWindow *parent, int32 x, int32 y, int32 dx, int32 dy);
  ixMenu *createMenu(ixBaseWindow *parent);
  ixMenuBar *createMenuBar(ixBaseWindow *parent);

  ixStaticText *createStaticText(ixBaseWindow *parent, int32 in_x0, int32 in_y0, int32 in_dx, int32 in_dy);

  void bringToFront(ixBaseWindow *);                // brings window to the top in the chainlist it's from (the parent chainlist); it will be drawn last, updated first
  void bringToBack(ixBaseWindow *);                 // brings window to bottom in the chainlist it's from (the parent chainlist); it will be drawn first, updated last
  void bringAfter(ixBaseWindow *win, ixBaseWindow *after);    // aranges window to be after the selected window
  void bringBefore(ixBaseWindow *win, ixBaseWindow *before);  // arranges window to be before the selected window

  void switchParent(ixBaseWindow *win, ixBaseWindow *parent); // parent can be null, so it will be a top window

  void delWindow(ixBaseWindow *);   // this func will delete safely any kind of window, and will handle every child/parent link
  void loadDef1Style();         // loads def1Style style - textured everything (defWinTex.tga)

  void blablaWindow();

  void draw();                  // draws all the top objects on all Ix engines
  void drawSpecific(Ix *in_ix); // draws all the top objects, on the specified engine only
  bool update();                // sets [flags] down, returns true if any HID activity happened with any of the windows, and flags what HID was used in [flags]
  void updateHooks();           // updates all top objects and their children hooks (in case hooked target moved)




  // constructor / destructor

  ixWinSys();
  ~ixWinSys();
  void delData();



protected:
  
  // there can be only one operation performed to only one window
  
  struct _Op {
    ixBaseWindow *win;     // window that has an action being performed to. THIS IS NULL IF NO OP IS BEING MADE
    uint64 time;            // time started, mostly, but it's up for every message to handle the time as it sees fit

    unsigned resizeLeft:1, resizeRight:1, resizeBottom:1;
    unsigned moving:1;
    unsigned mLclick:1, mRclick: 1, mMclick:1, m4click:1, m5click:1;

    unsigned scrArr1:1;    // up / right arrow press, depending on orientation
    unsigned scrArr2:1;    // down / left arrow press, depending on orientation
    unsigned scrBar:1;     // the bar is being pressed, pageup/down until the dragbox middle is on the cursor
    unsigned scrDragbox:1; // the dragbox is being dragged
    void *p;               // a pointer that can point to anything

    _Op() { delData(); }
    void delData() { resizeLeft= resizeRight= resizeBottom= moving= mLclick= mRclick= mMclick= m4click= m5click=
                     scrArr1= scrArr2= scrBar= scrDragbox= 0;
                     time= 0;
                     win= null, p= null; }
  } _op;  // operation in progress

  friend class ixBaseWindow;
  friend class ixWindow;
  friend class ixButton;
  friend class ixEdit;
  friend class ixStaticText;
  friend class ixScroll;
  friend class ixTxtData;
  friend class ixRadioButton;
  friend class ixDropList;
  friend class ixMenu;
  friend class ixMenuBar;
};







