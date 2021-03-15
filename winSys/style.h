#pragma once

class ixWSstyle;


class ixWSsubStyleBase {
public:
  ixWSstyle *parent;

  bool useTexture;        // is using textures                                     <<< bTexBG? ?????? which one to use? USE BOTH ! ... ?
  bool useBackColor;      // draw a rectangle using current color as a background
  bool useColorOnTexture; // is using color attribute for the texture
  
  // texture
  
  uint VBOindex;          // CONSTANT: 9 ids for each basicWindow: window[0]  winTitle[9] button[18] edit[27] staticText[36] 

  // common colors thru all substyle types

  vec4 color;             // background/scroll bar color of the window 
  vec4 colorFocus;        // color if it has focus (currently selected/focused window)
  vec4 colorHover;        // color of background (in general) or specific part of the window when the cursor/pointer is over it

  // constructor / destructor

  ixWSsubStyleBase();
  ~ixWSsubStyleBase();
  virtual void delData();

protected:
  int8 _type;

  virtual bool _load_textLine(str8 *cmd, str8 *attr1, str8 *attr2, str8 *attr3, str8 *attr4, bool useDeltas, uint8 origin= 4);

  friend class ixWSstyle;
};




class ixWSgenericStyle: public ixWSsubStyleBase {
public:
  vec4 colorBRD;          // border color of the window
  vec4 colorBRDfocus;     // border color of the window when it has focus (currently selected / focused window)

  // texture cfg

  // 4 0 5
  // 3   1   <- 8 borders, this is the order
  // 7 2 6

  int8 bTexBG;            // has a background texture
  int8 bTexBRD[8];        // each corner / side border if it is textured or not

  ixSubTex texBG;         // background texture coordinates
  ixSubTex texBRD[8];     // texture BORDERS, 8 in total

  int16 texBGwrap;        // background texture wrap: 0(fixed), 1(stretch), 2(repeat), 3(mirrored repeat)
  int16 texBRDwrap[4];    // WRAP: only non-corners: 0(fixed), 1(stretch), 2(repeat), 3(mirrored repeat)
  float texBRDdist[8];    // texture DISTance: negative - inside the window, positive - outside

  // texture functions

  void setBGcoords(int x0, int y0, int dx, int dy);   // sets the background texCoords (x0, y0, dx, dy)
  void setBRDcoords(int16 nr, int, int, int, int);    // sets the borders (0- 8) texCoords (x0, y0, dx, dy)
  //void setBRDcoordsFromFirst(short nr);               // rotates border 0 or 4 and creates border[nr] from it

  // inline helper funcs - can do without them...

  void setBGwrap(int16 w)            { texBGwrap= w; }
  void setBRDwrap(int16 nr, short w) { if(nr!= -1) texBRDwrap[nr]= w; }
  void setBRDallWrap(int16 w)        { texBRDwrap[0]= texBRDwrap[1]= texBRDwrap[2]= texBRDwrap[3]= w; }
  void setBRDdist(int16 nr, float d) { if(nr!= -1) texBRDdist[nr]= d; }
  void setBRDallDist(int16 d)        { for(short a= 0; a< 8; a++) texBRDdist[a]= d; }

  // constructor/ destructor/ operators

  ixWSgenericStyle();
  ~ixWSgenericStyle();
  virtual void delData();
protected:
  virtual bool _load_textLine(str8 *cmd, str8 *attr1, str8 *attr2, str8 *attr3, str8 *attr4, bool useDeltas, uint8 origin= 4);
};



class ixWSwindowStyle: public ixWSgenericStyle {
public:
  // title cfg

  bool useTitle;          // create a title bar or not
  int8 titlePosition;     // <<< CONFLICT WITH POINTSNAP  >>> 0, 1, 2, 3 - up, down, left, right - DEFAULT 0(up)
  int8 titlePointSnap;    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 0-3: middle points, same as titlePosition; 4-7: snap to a corner (check texBrd[8] figure) - DEFAULT 0(up-middle)
  int16 titleOrientation; // text orientation, in degrees - DEFAULT 90(left to right)
  int16 titleDist;        // distance from border; default: fits entirely inside the window
  bool titleInside;       // if the title is inside or outside the object - DEFAULT false(outside)

  ixWSwindowStyle();
  void delData();
  ~ixWSwindowStyle();

protected:
  virtual bool _load_textLine(str8 *cmd, str8 *attr1, str8 *attr2, str8 *attr3, str8 *attr4, bool useDeltas, uint8 origin= 4);
};





///============///
// scroll style //
///============///

class ixWSscrollStyle: public ixWSsubStyleBase {
public:

  // different colors

  vec4 colorArrows;           // color of the arrows
  vec4 colorDragbox;          // color of the movable button that you can drag for scrolling

  // textures cfg

  int8 bTexArrows;            // arrows are textured
  int8 bTexDragbox;           // scrollbar movable/draggable (dragbox) button is textured
  int8 bTexScrollBack;        // scrollbar background line is textured

  ixSubTex texArrows[4];      // arrows tex coords [0-up | 1-right | 2-down | 3-left]
  ixSubTex texDragbox[2];     // scroll dragable button tex coords [0- horizontal | 1- vertical]
  ixSubTex texScrollBack[2];  // scrollbar back 'line' tex coords [0- horizontal | 1- vertical]

  float horizontalDist;       // if the scrollbar is on the edge of a window, this value can slightly change it's position to be more 'inside' or more 'outside'
  float verticalDist;         // if the scrollbar is on the edge of a window, this value can slightly change it's position to be more 'inside' or more 'outside'
  int16 texDragboxWrap;       // scrollbar dragable button can be bigger/smaller so there is a need for a wrapping mode
  int16 texScrollBackWrap;    // scrollbar 'back-line' wrapping

  // texture functions

  void setArrowcoords(int16 nr, int x0, int y0, int dx, int dy);      // sets the arrows (0- 3) texCoords (x0, y0, dx, dy)
  void setDragboxCoords(int16 nr, int x0, int y0, int dx, int dy);    // sets the dragable button (0- 1) tex coords (x0, y0, dx, dy)
  void setScrollBackCoords(int16 nr, int x0, int y0, int dx, int dy); // sets the back-line(0- 1) texCoords (x0, y0, dx, dy)

  // inline helper funcs - can do without them...

  void setDragboxWrap(int16 w)     { texDragboxWrap= w; }
  void setScrollBackWrap(int16 w) { texScrollBackWrap= w; }
  void setScrollbarDist(int16 nr, float w)  { if(nr==0) horizontalDist= w; else verticalDist= w; }

  // constructor/ destructor/ operators

  ixWSscrollStyle();
  //ixWSscrollStyle(const ixWSscrollStyle *o);
  //void operator= (const ixWSscrollStyle &o);

  void delData();
  ~ixWSscrollStyle();

protected:
  virtual bool _load_textLine(str8 *cmd, str8 *attr1, str8 *attr2, str8 *attr3, str8 *attr4, bool useDeltas, uint8 origin= 4);
};







class ixWSstyle {
public:

  // substyles classes

  ixWSwindowStyle window, edit, text;
  ixWSgenericStyle title, button, buttonPressed;
  ixWSscrollStyle scroll;

  bool loadStyle(cchar *);  // reads from file everything needed for a ixWSstyle
  //void createAssets();      // created at start when a style is loaded
  //void delAssets() { _gpu.delData(); }

  bool loadTexture();       // texture must be loaded right away - texture dimensions are needed
  void delTexture() { _texList.delData(); }
  //bool createBuffers();     // buffers must be created after every data about the style is loaded
  ixTexture *getTexture(Ix *in_ix);

  // behaivor flags

  /// *** wind blowing in the desert ***

  ixWSstyle();
  ~ixWSstyle();
  void delData();


private:

  class Texture: public chainData {
  public:

    Ix *ix;             // ix engine this texture belong to
    ixTexture *tex;     // texture of the style
    
    //bool loadTex(cchar *);    // loads a texture from file

    Texture(Ix *in_ix): ix(in_ix), tex(null) {}
    //~Texture() {}; // { tex->delData(); }
  };

  
  chainList _texList;         // each ix engine will load the texture, therefore a list is needed

  str8 _texName;              // texture filename
  int32 _texDx, _texDy;       // texture size in pixels
  friend class ixWSsubStyleBase;
  friend class ixWSscrollStyle;
  friend class ixWSgenericStyle;
  friend class ixWSwindowStyle;
  friend class ixBaseWindow;
  friend class ixScroll;
  friend class ixWinSys;
};



// WIP
class ixStyleSys {
public:

  chainList styles, customStyles, customSubStyles;


  ixStyleSys();
  ~ixStyleSys();
  void delData();

private:



};















