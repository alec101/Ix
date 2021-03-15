//#define OSI_USE_OPENGL_LEGACY 1
#include "osinteraction.h"
#include "ixGfx/RGB.h"
#include "ixGfx/texture.h"
#include "ixGfx/print.h"
#include "ixGfx/winSystem.h"
#include "ix.h"
#include "ixConsole.h"
#include "shaderSys.h"

#define _VBOID_BG 0
#define _VBOID_BRD_TOP 4
#define _VBOID_BRD_RIGHT 8
#define _VBOID_BRD_BOTTOM 12
#define _VBOID_BRD_LEFT 16
#define _VBOID_BRD_TOPLEFT 20
#define _VBOID_BRD_TOPRIGHT 24
#define _VBOID_BRD_BOTTOMRIGHT 28
#define _VBOID_BRD_BOTTOMLEFT 32

#define _BRD_TOP 0
#define _BRD_RIGHT 1
#define _BRD_BOTTOM 2
#define _BRD_LEFT 3
#define _BRD_TOPLEFT 4
#define _BRD_TOPRIGHT 5
#define _BRD_BOTTOMRIGHT 6
#define _BRD_BOTTOMLEFT 7

#define _IX_BASE_WINDOW 0
#define _IX_WINDOW 1
#define _IX_BUTTON 2
#define _IX_STATIC_TEXT 3
#define _IX_EDIT 4
#define _IX_TITLE 5

// MUST TEST if wrap is on full texture, or on parameters (i really doubt it's on params)
// not on params= must implement wrap with multiple rectangles

// special clamping must be done when zooming, unfortunately, as the clamping is done manually

// IDEEAS:
/*
 - effect: when selecting a new window, blacken all unselected windows for half a second (a gradual return to normal colors over half a second)
          this might work best when using a kb/stick/pad/wheel, only. WHEN USING THE MOUSE THIS WOULD BE MORE OF AN ANNOYANCE
 - the baseWindow style could not be a pointer, so when creating a window, it's style will not change at all,
         this could be a minor bonus, but wasted memory
*/

// TODO:
/*
 - window coordonates, as all orthographic drawing should have viewport coordonates, like osi has,
     so if a window spans 2 screens or even 2 contexts, it can easily be drawn

 - buttons
 - edit controls
 - big edit controls
 - think of a new type of object
 
      there seems to be no problems. one texture for one style. i think it's fine like this
      any baseWSobject can have any style, so no problems.
      save/load a style in bynary too, eventually... why not, right? the binary one, can have a unique signature,
      and the loadStyle will know to load the binary loadFunc

      different texture for selection/ inactive? or simply, a different style... hmm...
      effects -> over time shades / glows / bigger / smaller
      custom draw - bCustomDraw, maybe + func that has to be called for custom draw
*/



// opengl notes:
/*


  - when stretching something, a .5 to both ends must be added/substracted, or tex coords from adjancent points are used...
    *TESTED, not working* a solution could be to add/substract .49 in every draw... see what happens
    only when stretching... the rest is perfect pixel per pixel... this implies that EXTRA CARE MUST BE TAKEN WHEN STRETCHING
    it's possible that there is a difference when minifying and magnifying when stretching, too

  - lines: are drawn using the 'diamond rule', the last 'diamond' is not drawn
  - polygons: must fit in a mathematical boundary, to be drawn,
  - this .375 is still not certain, must test on ati too, else just use some .5 on tex coords
  - i've read that translating to .5x .5y for ortho, is good for lines, BUT still, 0, 0 is better for polygons,
    because they are in that 'mathematical boundary' - just add 1 at the end for a perfect pixel to polygon draw (will be a -0.5 +0.5 mathematical boundry)
*/



ixWinSys wsys;
ixWSstyle def1Style, def2Style;



///======================-----------------///
// WINDOW SYS STYLE class ================ //
///======================-----------------///

ixWSstyle::ixWSstyle() {
  window.parent= this;
  title.parent= this;
  button.parent= this;
  edit.parent= this;
  text.parent= this;
  tex= null;
  _VBOid= 0;

  window.VBOindex= 0;
  title.VBOindex=  36;
  button.VBOindex= 72;
  edit.VBOindex=   108;
  text.VBOindex=   144;
  delData();
}

ixWSstyle::~ixWSstyle() {
  delData();
}

void ixWSstyle::delData() {
  if(tex)
    texSys.data.del(tex);
  if(_VBOid)
    delVBO();
}


bool ixWSstyle::loadTex(cchar *fname) {
  /// if it already handles a texture, it has to be deleted first
  if(tex)
    texSys.data.del(tex);

  if((tex= texSys.loadTexture(fname))== null)
    return false;

  /// set the background as the full extent of the texture (DEFAULT)
  window.useTexture= true;
  window.bTexBG= true;
  window.texBG.dx= tex->dx;
  window.texBG.dy= tex->dy;

  /// usage flags change
  window.useBackColor= false;

  return true;
}



// creates a vbo for the style
void ixWSstyle::createVBO() {
  if(!osi.glr) { error.console("ixWSstyle::createVBO(): no renderer active"); return; }
  error.glFlushErrors();
  /// bg  1x[4vert[3float] 4tex[2float]]            (48bytes vertPos + 32bytes texCoords = 80bytes)
  /// brd 8x[4vert[3float] 4tex[2float]]            8x 80bytes= 640bytes
  /// everything x5 [win edit title button text]    
  //  total = 3600 bytes - 2160 bytes verPos + 1440 bytes texCoords
  uint8 *data= new uint8[3600];   // fixed size - if something is not used, it's 0
  if(!data) { error.console("ixWSstyle::createVBO(): couldn't alloc mem"); return; }
  Str::memclr(data, 3600);
  float *f= (float*)data;     /// f will walk the data as a float
  ixWSsubStyle *s= null;


  // populate vertPos
  for(int a= 0; a< 5; a++) {        // pass thru each substyle [5]
    /// set the substyle pointer
    if(a== 0)      s= &window;
    else if(a== 1) s= &title;
    else if(a== 2) s= &button;
    else if(a== 3) s= &edit;
    else if(a== 4) s= &text;

    /// background
    if(s->bTexBG) {
      f+= 3;                                        // v1: x0= y0= z0= 0;
      f++;                *f++= s->texBG.dy;  f++;  // v2: x0 ye z0
      *f++= s->texBG.dx;  *f++= s->texBG.dy;  f++;  // v3: xe ye z0
      *f++= s->texBG.dx;  f++;                f++;  // v4: xe y0 z0
    } else f+= 12;

    /// borders (8 max)
    for(int b= 0; b< 8; b++) {
      if(s->bTexBrd[b]) {
        f+= 3;                                                // v1: x0= y0= z0= 0;
        f++;                    *f++= s->texBrd[b].dy;  f++;  // v2: x0 ye z0
        *f++= s->texBrd[b].dx;  *f++= s->texBrd[b].dy;  f++;  // v3: xe ye z0
        *f++= s->texBrd[b].dx;  f++;                    f++;  // v4: xe y0 z0
      } else
        f+= 12;
    }
  } /// pass thru each substyle [5]

  // populate texPos
  for(int a= 0; a< 5; a++) {        // pass thru each substyle [5]
    /// set the substyle pointer
    if(a== 0)      s= &window;
    else if(a== 1) s= &title;
    else if(a== 2) s= &button;
    else if(a== 3) s= &edit;
    else if(a== 4) s= &text;

    /// background
    if(s->bTexBG) {
      *f++= s->texBG.s0; *f++= s->texBG.t0;    // v1: x0 y0
      *f++= s->texBG.s0; *f++= s->texBG.te;    // v2: x0 ye
      *f++= s->texBG.se; *f++= s->texBG.te;    // v3: xe ye
      *f++= s->texBG.se; *f++= s->texBG.t0;    // v4: xe y0
    } else f+= 8;

    /// borders (8 max)
    for(int b= 0; b< 8; b++) {
      if(s->bTexBrd[b]) {
        *f++= s->texBrd[b].s0; *f++= s->texBrd[b].t0;    // v1: x0 y0
        *f++= s->texBrd[b].s0; *f++= s->texBrd[b].te;    // v2: x0 ye
        *f++= s->texBrd[b].se; *f++= s->texBrd[b].te;    // v3: xe ye
        *f++= s->texBrd[b].se; *f++= s->texBrd[b].t0;    // v4: xe y0
      } else
        f+= 8;
    }
  } /// pass thru each substyle [5]

  
  // create the VBO buffer
  glGenBuffers(1, &_VBOid);
  glBindBuffer(GL_ARRAY_BUFFER, _VBOid);
  glBufferData(GL_ARRAY_BUFFER, 3600, data, GL_STATIC_DRAW); /// size of the vertex data and tex coods

  error.glError("ixWSstyle::createVBO(): ");
  
  if(!_VAO.id)
    _VAO.genArray();
  _VAO.bindAndVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  _VAO.bindAndVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)(2160));
  _VAO.enableVertexAttribArray(0);
  _VAO.enableVertexAttribArray(1);
  error.glError("ixWSstyle::createVBO(): ");

  delete[] data;
}

// deletes the vbo tied to that style
void ixWSstyle::delVBO() {
  if(osi.glr)
    glDeleteBuffers(1, &_VBOid);
  _VBOid= 0;
  if(_VAO.id) _VAO.delArray();
}


int _getWrap(str8 *s) {
  if     (*s== "fixed"          || *s== "fix"          || *s== "0") return 0;
  else if(*s== "stretch"        || *s== "stretched"    || *s== "1") return 1;
  else if(*s== "repeat"         || *s== "repeated"     || *s== "2") return 2;
  else if(*s== "mirroredrepeat" || *s== "mirrorrepeat" || *s== "3") return 3;
  else return 0;
}

bool _getBool(str8 *s) {
  if(*s== "true"  || *s== "1") return true;
  if(*s== "false" || *s== "0") return false;
  return false;
}

int16 _getOrientation(str8 *s) {
  if     (*s== "lefttoright" || *s== "right" || *s== "toright" || *s== "90")  return 90;
  else if(*s== "righttoleft" || *s== "left"  || *s== "toleft"  || *s== "270") return 270;
  else if(*s== "uptodown"    || *s== "down"  || *s== "todown"  || *s== "180") return 180;
  else if(*s== "downtoup"    || *s== "up"    || *s== "toup"    || *s== "0")   return 0;
  return 90;
}

int8 _getBorderPoint(str8 *s) {
  if     (*s== "up"        || *s== "0" || *s== "top")         return 0;
  else if(*s== "right"     || *s== "1")                       return 1;
  else if(*s== "down"      || *s== "2" || *s== "bottom")      return 2;
  else if(*s== "left"      || *s== "3")                       return 3;
  else if(*s== "upleft"    || *s== "4" || *s== "topleft")     return 4;
  else if(*s== "upright"   || *s== "5" || *s== "topright")    return 5;
  else if(*s== "downright" || *s== "6" || *s== "bottomright") return 6;
  else if(*s== "downleft"  || *s== "7" || *s== "bottomleft")  return 7;
  else return 0;
}


bool ixWSstyle::loadStyle(cchar *fname) {
  delData();
  FILE *f= fopen(fname, "rb");
  if(!f) { console.print("ixWSstyle::loadStyle(): file not found"); return false; }

  bool useDeltas= true;
  str8 line, s, s1, s2, s3, s4;
  ixWSsubStyle *style= null;

  while(!feof(f)) {
    line.readLineUTF8(f);
    uint8 *p= line.d;
    p= skipWhitespace(p);
    if(*p== '[') {
      p= readWordsInBrackets(p, &s);
      
    } else {
      p= readWordOrWordsInQuotes(p, &s);
      p= skipWhitespace(p);
      if(*p== '=' || *p== ':' || *p== '\'' || *p== '\"' || *p== ']' || *p== ',') p++;
      p= skipWhitespace(p);
    }

    // 4 possible assignments
    p= readWordOrWordsInQuotes(p, &s1);   if(*p== ',') p++;   s1.lower();
    p= readWordOrWordsInQuotes(p, &s2);   if(*p== ',') p++;   s2.lower();
    p= readWordOrWordsInQuotes(p, &s3);   if(*p== ',') p++;   s3.lower();
    p= readWordOrWordsInQuotes(p, &s4);   if(*p== ',') p++;   s4.lower();

    if(!s.d) continue;
    
    s.lower();
    if(s.len>= 2) {
      if(s.d[s.len- 2]== '=') s-= 1;
      else if(s.d[s.len- 2]== ':') s-= 1;
    }
    
    if(s== "texture" || s== "texturefile" || s== "texturefilename") {
      this->loadTex(s1);

    } else if(s== "window" || s== "win") {
      style= &window;

    } else if(s== "title" || s== "windowtitle" || s== "wintitle") {
      style= &title;

    } else if(s== "button" || s== "buttons") {
      style= &button;

    } else if(s== "edit" || s== "edittext") {
      style= &edit;

    } else if(s== "text" || s== "static" || s== "statictext" ) {
      style= &text;

    } else if(s== "donotusedelta") {
      useDeltas= false;

    } else if(s== "usedelta" || s== "usedeltas") {
      if(s1== "false" || s1== "0") useDeltas= false;
      else useDeltas= true;

    } else if(s== "background" || s== "bg") {
      if(useDeltas)   style->setBGcoords((int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBGcoords((int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));
    
    } else if(s== "border0" || s== "brd0" || s== "bordertop" || s== "brdtop") {
      if(useDeltas)   style->setBRDcoords(0, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(0, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "border1" || s== "brd1" || s== "borderright" || s== "brdright") {
      if(useDeltas)   style->setBRDcoords(1, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(1, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "border2" || s== "brd2" || s== "borderbottom" || s== "brdbottom") {
      if(useDeltas)   style->setBRDcoords(2, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(2, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "border3" || s== "brd3" || s== "borderleft" || s== "brdleft") {
      if(useDeltas)   style->setBRDcoords(3, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(3, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "border4" || s== "brd4" || s== "bordertopleft" || s== "brdtopleft") {
      if(useDeltas)   style->setBRDcoords(4, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(4, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "border5" || s== "brd5" || s== "bordertopright" || s== "brdtopright") {
      if(useDeltas)   style->setBRDcoords(5, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(5, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "border6" || s== "brd6" || s== "borderbottomright" || s== "brdbottomright") {
      if(useDeltas)   style->setBRDcoords(6, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(6, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "border7" || s== "brd7" || s== "borderbottomleft" || s== "brdbottomleft") {
      if(useDeltas)   style->setBRDcoords(7, (int)s1.toInt(), (int)s2.toInt(), (int)s3.toInt(), (int)s4.toInt());
      else            style->setBRDcoords(7, (int)s1.toInt(), (int)s2.toInt(), (int)(s3.toInt()- s1.toInt()), (int)(s4.toInt()- s2.toInt()));

    } else if(s== "bgwrap" || s== "backgroundwrap") {
      style->setBGwrap(_getWrap(&s1));

    } else if(s== "brdwrap" || s== "borderwrap") {
      style->setBRDallWrap(_getWrap(&s1));

    } else if(s== "brd0wrap" || s== "border0wrap" || s== "brdtopwrap" || s== "bordertopwrap") {
      style->setBRDwrap(0, _getWrap(&s1));

    } else if(s== "brd1wrap" || s== "border1wrap" || s== "brdrightwrap" || s== "borderrightwrap") {
      style->setBRDwrap(1, _getWrap(&s1));

    } else if(s== "brd2wrap" || s== "border2wrap" || s== "brdbottomwrap" || s== "borderbottomwrap") {
      style->setBRDwrap(2, _getWrap(&s1));

    } else if(s== "brd3wrap" || s== "border3wrap" || s== "brdleftwrap" || s== "borderleftwrap") {
      style->setBRDwrap(3, _getWrap(&s1));

    } else if(s== "brddistance" || s=="borderdistance") {
      style->setBRDallDist((int16)s1.toInt());

    } else if(s== "brd0distance" || s== "border0distance" || s== "brdtopdistance" || s== "bordertopdistance") {
      style->setBRDdist(0, s1.toFloat());

    } else if(s== "brd1distance" || s== "border1distance" || s== "brdrightdistance" || s== "borderrightdistance") {
      style->setBRDdist(1, s1.toFloat());

    } else if(s== "brd2distance" || s== "border2distance" || s== "brdbottomdistance" || s== "borderbottomdistance") {
      style->setBRDdist(2, s1.toFloat());

    } else if(s== "brd3distance" || s== "border3distance" || s== "brdleftdistance" || s== "borderleftdistance") {
      style->setBRDdist(3, s1.toFloat());

    } else if(s== "brd4distance" || s== "border4distance" || s== "brdtopleftdistance" || s== "bordertopleftdistance") {
      style->setBRDdist(4, s1.toFloat());

    } else if(s== "brd5distance" || s== "border5distance" || s== "brdtoprightdistance" || s== "bordertoprightdistance") {
      style->setBRDdist(5, s1.toFloat());

    } else if(s== "brd6distance" || s== "border6distance" || s== "brdbottomrightdistance" || s== "borderbottomrightdistance") {
      style->setBRDdist(6, s1.toFloat());

    } else if(s== "brd7distance" || s== "border7distance" || s== "brdbottomleftdistance" || s== "borderbottomleftdistance") {
      style->setBRDdist(7, s1.toFloat());

    } else if(s== "colorbg" || s== "colorbackground" || s== "bgcolor" || s== "backgroundcolor") {
      style->colorBG.set(s1.toFloat(), s2.toFloat(), s3.toFloat(), s4.toFloat());

    } else if(s== "colorbrd" || s== "colorborder" || s== "brdcolor" || s== "bordercolor") {
      style->colorBRD.set(s1.toFloat(), s2.toFloat(), s3.toFloat(), s4.toFloat());
    } else if(s== "usebacktexture" || s== "usebgtexture" || s== "usebackgroundtexture") {
      style->bTexBG= _getBool(&s1);

    } else if(s== "usebackcolor" || s== "usebgcolor" || s== "usebackgroundcolor") {
      style->useBackColor= _getBool(&s1);

    } else if(s== "usetexture") {
      style->useTexture= _getBool(&s1);

    } else if(s== "usecolorontexture") {
      style->useColorOnTexture= _getBool(&s1);

    } else if(s== "selcolor" || s== "selectedcolor") {
      style->selColor.set(s1.toFloat(), s2.toFloat(), s3.toFloat(), s4.toFloat());

    } else if(s== "usetitle" || s== "usetitlebar") {
      style->useTitle= _getBool(&s1);

    } else if(s== "titledist" || s== "titledistance") {
      style->titleDist= s1.toFloat();

    } else if(s== "titleinside") {
      style->titleInside= _getBool(&s1);

    } else if(s== "titleorientation") {
      style->titleOrientation= _getOrientation(&s1);

    } else if(s== "titlepointsnap") {
      style->titlePointSnap= _getBorderPoint(&s1);

    }
  }


  fclose(f);
  return true;
}

///===================================================///
//  SUBSTYLE class ===============-------------------- //
///===================================================///

ixWSsubStyle::ixWSsubStyle() {
  parent= null;
  delData();
}

ixWSsubStyle::~ixWSsubStyle() {
  delData();
}

void ixWSsubStyle::delData() {
  // default flags
  useTexture= false;
  useBackColor= true;
  useColorOnTexture= false;

  colorBG.set(0.5f, 0.5f, 0.5f, 0.5f);
  colorBRD.set(1.0f, 1.0f, 1.0f, 1.0f);
  selColor.set(0.7f, 0.5f, 0.7f, 0.55f);

  /// background vars
  bTexBG= 0;
  texBGwrap= 0;          /// background texture wrap: 0(fixed), 1(stretch), 2(repeat), 3(mirrored repeat)
  
  /// border vars
  for(short a= 0; a< 8; a++) {
    bTexBrd[a]= 0;
    texBrdDist[a]= 0.0f;
    texBrd[a].delData();
    if(a< 4)
      texBrdWrap[a]= 0;
  }

  /// title vars
  useTitle= true;
  titlePosition= 0;
  titlePointSnap= 0;
  titleOrientation= 90;
  titleDist= 0;
  titleInside= false;
}






ixWSsubStyle::ixWSsubStyle(const ixWSsubStyle *o) {
  *this= *o;
}


void ixWSsubStyle::operator= (const ixWSsubStyle &o) {
  useTexture= o.useTexture;
  useBackColor= o.useBackColor;
  useColorOnTexture= o.useColorOnTexture;
  VBOindex= o.VBOindex;

  colorBG= o.colorBG;
  colorBRD= o.colorBRD;
  selColor= o.selColor;
  parent= o.parent;
  /// background
  bTexBG= o.bTexBG;
  texBG= o.texBG;
  texBGwrap= o.texBGwrap;

  /// borders
  for(short a= 0; a< 8; a++) {
    bTexBrd[a]= o.bTexBrd[a];
    texBrdDist[a]= o.texBrdDist[a];
    texBrd[a]= o.texBrd[a];
    if(a< 4)
      texBrdWrap[a]= o.texBrdWrap[a];
  }

  /// title
  useTitle= o.useTitle;
  titlePosition= o.titlePosition;
  titlePointSnap= o.titlePointSnap;
  titleOrientation= o.titleOrientation;
  titleDist= o.titleDist;
  titleInside= o.titleInside;
}




// funcs from here

/// sets the background texCoords (x0, y0, dx, dy)
void ixWSsubStyle::setBGcoords(int _x0, int _y0, int _dx, int _dy) {
  texBG.dx= _dx;
  texBG.dy= _dy;
  // THIS NEEDS TESTING FOR NVIDIA/ATI COMPATIBILITY. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  texBG.s0= ((float)_x0)/ (float)parent->tex->dx;
  texBG.t0= ((float)_y0)/ (float)parent->tex->dy;
  texBG.se= texBG.s0+ (float)_dx/ (float)parent->tex->dx;
  texBG.te= texBG.t0+ (float)_dy/ (float)parent->tex->dy;
  bTexBG= 1;
}


/// sets the borders (0- 8) texCoords (x0, y0, dx, dy)
void ixWSsubStyle::setBRDcoords(short nr, int _x0, int _y0, int _dx, int _dy) {
  texBrd[nr].dx= _dx;
  texBrd[nr].dy= _dy;
  // THIS NEEDS TESTING FOR NVIDIA/ATI COMPATIBILITY. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  texBrd[nr].s0= ((float)_x0)/ (float)parent->tex->dx;
  texBrd[nr].t0= ((float)_y0)/ (float)parent->tex->dy;
  texBrd[nr].se= texBrd[nr].s0+ (float)_dx/ (float)parent->tex->dx;
  texBrd[nr].te= texBrd[nr].t0+ (float)_dy/ (float)parent->tex->dy;
  bTexBrd[nr]= 1;
}









///==========================================================-------------------------------------///
// WinSys class - 'top' handler of all windows / etc controls ==================================== //
///==========================================================-------------------------------------///

ixWinSys::ixWinSys() {
  selStyle= null;
  _sl= null;
}


ixWinSys::~ixWinSys() {
  delData();
  delProgram();
}


void ixWinSys::delData() {
  /// delete all constructed objects
  while(topObjects.first)
    topObjects.del(topObjects.first);  
}


// funcs

/// draws all the top objects <<< TO BE OR NOT TO BE
void ixWinSys::draw() {
  for(ixBaseWindow *p= (ixBaseWindow *)topObjects.first; p; p= (ixBaseWindow *)p->next)
    p->draw();
}

/// updates all top objects and their children hooks (in case hooked target moved)
void ixWinSys::updateHooks() {
  for(ixBaseWindow *p= (ixBaseWindow *)topObjects.first; p; p= (ixBaseWindow *)p->next)
    p->updateHooks();
}


ixBaseWindow *ixWinSys::createWindow(cchar *in_title, float in_x, float in_y, float in_dx, float in_dy) {
  ixWindow *w= new ixWindow;
  if(!_sl) loadProgram();

  /// object style(s)
  w->style= selStyle->window;
  
  // settings go here
  w->setHookAnchor();   /// windows are hooked to the virtual desktop by default
  w->x= in_x;
  w->y= in_y;
  w->dx= in_dx;
  w->dy= in_dy;
  w->is.visible= true;
  //w->origin= vec3(0.0f, 0.0f, 0.0f);

  /*
  problem might be if the title is outside the window, scrolling problems, clipping problems, etc.
  there can be a check when computing the scrolling sysrem, if(title== checked children), skip it
  there can also be a check when computing the clip when drawing, if(title== checked children), skip clipping
  */

  /// window title
  if(w->style.useTitle)
    w->setTitle(in_title, &selStyle->title);

  /// final stuff
  topObjects.add(w);
  return w;
}


ixButton *ixWinSys::createButton(cchar *s, ixBaseWindow *parent, float _x, float _y, float _dx, float _dy) {
  if(!_sl) loadProgram();
  ixButton *b= new ixButton;
  b->x= _x;
  b->y= _y;
  b->dx= _dx;
  b->dy= _dy;
  b->text= s;
  b->font= pr.selFont;
  b->is.visible= true;
  b->style= selStyle->button;
  b->parent= parent;

  // THIS NEEDS MORE ATTENTION
  /// add the object to the parent or the 'top' window
  if(parent) {
    //b->origin= vec3(parent->x, parent->y, parent->z);
    parent->childrens.add(b);

    b->setHookAnchor(parent);
  } else {
    //b->origin= vec3(0.0f, 0.0f, 0.0f);
    topObjects.add(b);
    b->setHookAnchor();
  }

  return b;
}


void ixWinSys::delWindow(ixBaseWindow *w) {
  topObjects.del(w);
}


void ixWinSys::loadDef1Style() {
  def1Style.loadStyle("defWinTex.txt");
  /*
  def1Style.loadTex("defWinTex.tga");

  // WINDOW STYLE =============---------------
  def1Style.window.useBackColor= true;
  def1Style.window.bTexBG= false;
    
  /// borders
  def1Style.window.setBRDcoords(0, 2,  15, 10, 9);
  def1Style.window.setBRDcoords(1, 14, 14, 9,  10);
  def1Style.window.setBRDcoords(2, 25, 15, 10, 9);
  def1Style.window.setBRDcoords(3, 37, 14, 9,  10);

  /// corners
  def1Style.window.setBRDcoords(4, 2,  77, 49, 49);
  def1Style.window.setBRDcoords(5, 53, 77, 49, 49);
  def1Style.window.setBRDcoords(6, 2,  26, 49, 49);
  def1Style.window.setBRDcoords(7, 53, 26, 49, 49);
  def1Style.window.setBRDallWrap(2);                 /// repeat wrap

  /// borders have a 1 pixel dist value
  def1Style.window.setBRDdist(0, 1.0f);
  def1Style.window.setBRDdist(1, 1.0f);
  def1Style.window.setBRDdist(2, 1.0f);
  def1Style.window.setBRDdist(3, 1.0f);

  /// corners have a 4 pixel dist value
  def1Style.window.setBRDdist(4, 5.0f);
  def1Style.window.setBRDdist(5, 5.0f);
  def1Style.window.setBRDdist(6, 5.0f);
  def1Style.window.setBRDdist(7, 5.0f);


  // TITLE STYLE =============---------------
  def1Style.title.useTexture= true;
  def1Style.title.useBackColor= true;
  def1Style.title.bTexBG= false;

  def1Style.title.setBRDcoords(0, 106, 57, 4, 5);
  def1Style.title.setBRDcoords(1, 106, 64, 20, 30);
  def1Style.title.setBRDcoords(2, 112, 57, 4, 5);
  def1Style.title.setBRDcoords(3, 106, 96, 20, 30);

  def1Style.title.setBRDallWrap(1);
  def1Style.title.setBRDdist(1, 17);
  def1Style.title.setBRDdist(3, 17);

  // BUTTON STYLE ============----------------
  def1Style.button.useTexture= true;
  def1Style.button.useBackColor= true;
  def1Style.button.bTexBG= false;
  def1Style.button.colorBG.set(.3f, .3f, .3f, 1); /// opaque

  def1Style.button.setBRDcoords(0, 49, 20, 2, 4);
  def1Style.button.setBRDcoords(1, 54, 21, 4, 2);
  def1Style.button.setBRDcoords(2, 55, 20, 2, 4);
  def1Style.button.setBRDcoords(3, 48, 21, 4, 2);
  def1Style.button.setBRDcoords(4, 60, 20, 4, 4);
  def1Style.button.setBRDcoords(5, 66, 20, 4, 4);
  def1Style.button.setBRDcoords(6, 72, 20, 4, 4);
  def1Style.button.setBRDcoords(7, 78, 20, 4, 4);
  def1Style.button.setBRDallDist(4);    // borders are on the outside... there should be an inside variant, but there has to be a wrap BETWEEN corners options on the baseObject
  def1Style.button.setBRDallWrap(1);   /// stretch

  // EDIT STYLE ==============----------------
  def1Style.edit.useTexture= true;
  def1Style.edit.useBackColor= true;
  def1Style.edit.bTexBG= false;

  def1Style.edit.setBRDcoords(0, 49, 15, 1, 3);
  def1Style.edit.setBRDcoords(1, 53, 16, 3, 1);
  def1Style.edit.setBRDcoords(2, 59, 15, 1, 3);
  def1Style.edit.setBRDcoords(3, 63, 16, 3, 1);
  def1Style.edit.setBRDcoords(4, 68, 15, 3, 3);
  def1Style.edit.setBRDcoords(5, 73, 15, 3, 3);
  def1Style.edit.setBRDcoords(6, 78, 15, 3, 3);
  def1Style.edit.setBRDcoords(7, 83, 15, 3, 3);

  // WRAP BETWEEN CORNERS NEEDED HERE TOO
  def1Style.edit.setBRDallWrap(1);   /// stretch

  // STATIC STYLE ===========-----------------
  */

  def1Style.createVBO();

  // set to def1Style as the active style
  selStyle= &def1Style;
}






void ixWinSys::ixWSshader::initUniforms() {
  ixShader::initUniforms();
  u_origin=       glGetUniformLocation(id, "u_origin");
  u_useTexture=   glGetUniformLocation(id, "u_useTexture");
  u_useClipping=  glGetUniformLocation(id, "u_useClipping");
  u_clip0=        glGetUniformLocation(id, "u_clip0");
  u_clipE=        glGetUniformLocation(id, "u_clipE");
  u_customPos=    glGetUniformLocation(id, "u_customPos");
  u_quadPos0=     glGetUniformLocation(id, "u_quadPos0");
  u_quadPosE=     glGetUniformLocation(id, "u_quadPosE");
  u_customTex=    glGetUniformLocation(id, "u_customTex");
  u_quadTex0=     glGetUniformLocation(id, "u_quadTex0");
  u_quadTexE=     glGetUniformLocation(id, "u_quadTexE");
  
}


void ixWinSys::loadProgram() {
  ixShaders.loadShader(&_sl, "../source/shaders/windowsV.glsl", "../source/shaders/windowsF.glsl");
  if(_sl) _sl->initUniforms();
}


void ixWinSys::delProgram() {
  if(_sl)
    ixShaders.delShader(_sl);
  _sl= null;
}





///=================------------------///
// BASEWINDOW class ------------------ //
///=================------------------///

ixBaseWindow::ixBaseWindow() {
  _type= _IX_BASE_WINDOW;
  parent= null;
  
  //_handlesTex= false;
  delData();
}


ixBaseWindow::~ixBaseWindow() {
  delData();
}


void ixBaseWindow::delData() {

  /// default usage flags
  usage.movable= false;
  usage.resizeable= false;
  usage.minimizable= false;

  //useMovable= false;
  //useResizeable= false;
  //useMinimizable= false;
  
  /// current state flags
  is.visible= true;
  is.minimized= false;

  is.opening= false;
  is.closing= false;
  
  is.MOUSEfocus= false;
  is.KBfocus= false;
  is.GPfocus= false;
  is.JOYfocus= false;
  
  //setHookAnchor();    // defaults to border 7 - virtual desktop (bottom left) - CAUSES CRASH ON DESTRUCTOR

  /// base parameters
  x= y= z= 0;
  dx= dy= 0;
  style.delData();

  // ALL CHILDRENS WILL BE DELETED
  while(childrens.first)
    childrens.del(childrens.first);
}





// Base drawing function, usually called by derived object first
void ixBaseWindow::draw() {
  // visibility is not that easy, check for a minimized button first...

  if(!wsys._sl) return;

  // this should be placed in the calling program, tho...
  // placing multiple disables & enables is not good i think
  // enable - draw scene / disable - draw menus
  style.parent->_VAO.bind();
  glUseProgram(wsys._sl->id);
  glBindBuffer(GL_ARRAY_BUFFER, style.parent->_VBOid);
  glUniformMatrix4fv(wsys._sl->u_camera, 1, GL_FALSE, camera->cameraMat);
  glUniform1ui(wsys._sl->u_useTexture, 1);
  glUniform3f(wsys._sl->u_origin, 0.0f, 0.0f, 0.0f);  // TO BE OR NOT TO BE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  glDisable(GL_DEPTH_TEST);   /// no depth testing - just draw over what is behind

  /// tmp vars
  int nrS, nrT;           /// these will hold the number of times the texture will repeat on S and T axis
  float _x, _y;
  getVDcoords3f(&_x, &_y, null);
  

  // WINDOW BACKGROUND =========-----------

  /// useBackColor usage flag - draw a rectangle using current color
  if(style.useBackColor) {
    glUniform1ui(wsys._sl->u_useClipping, 0);
    glUniform4fv(wsys._sl->u_color, 1, style.colorBG.v);

    //must be able to draw this simple box... atm nothing is drawn for some reason... check if i made the change in the camera or not, too
    glUniform1ui(wsys._sl->u_useTexture, 0);
    glUniform1ui(wsys._sl->u_customPos, 1);            // enable shader custom quad draw (vert positions)
    glUniform1ui(wsys._sl->u_customTex, 1);            // enable shader custom quad draw (tex coords)
    glUniform2f(wsys._sl->u_quadPos0, _x, _y);
    glUniform2f(wsys._sl->u_quadPosE, _x+ dx, _y+ dy);
    glUniform2f(wsys._sl->u_quadTex0, 0.0f, 0.0f);
    glUniform2f(wsys._sl->u_quadTexE, 1.0f, 1.0f);
    glDrawArrays(GL_QUADS, 0, 4);
    glUniform1ui(wsys._sl->u_customPos, 0);            // disable shader custom quad draw
    glUniform1ui(wsys._sl->u_customTex, 0);            // disable shader custom quad draw
    glUniform1ui(wsys._sl->u_useTexture, 1);
  }

  /// useColorOnTexture usage flag - use color for current texture
  if(style.useColorOnTexture)
    glUniform4fv(wsys._sl->u_color, 1, style.colorBG.v);
  else
    glUniform4f(wsys._sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);
  
  if(style.useTexture) 
    glBindTexture(style.parent->tex->target, style.parent->tex->id);


  /// BACKGROUND texturing
  if(style.useTexture && style.bTexBG) {
 
    /// nr of times the texture repeats
    nrS= (int)dx/ style.texBG.dx+ (((int)dx% style.texBG.dx)? 1: 0);
    nrT= (int)dy/ style.texBG.dy+ (((int)dy% style.texBG.dy)? 1: 0);
    
    // FIXED background
    if(style.texBGwrap== 0) {

      glUniform1ui(wsys._sl->u_useClipping, 1); /// enable  clipping
      glUniform2f(wsys._sl->u_clip0, _x- osi.glrWin->x0,     _y- osi.glrWin->y0);     /// fragment shader works with viewport coords
      glUniform2f(wsys._sl->u_clipE, _x+ dx- osi.glrWin->x0, _y+ dy- osi.glrWin->y0); /// fragment shader works with viewport coords
      glUniform3f(wsys._sl->u_origin, _x, _y, 0.0f);

      glDrawArrays(GL_QUADS, style.VBOindex, 4);

      glUniform1ui(wsys._sl->u_useClipping, 0); /// disable clipping

    // STRETCHED background
    } else if(style.texBGwrap== 1) {

      glUniform1ui(wsys._sl->u_customPos, 1); /// enable custom quad position
      glUniform2f(wsys._sl->u_quadPos0, _x, _y);
      glUniform2f(wsys._sl->u_quadPosE, _x+ dx, _y+ dy);
      glUniform3f(wsys._sl->u_origin, 0.0f, 0.0f, 0.0f);

      glDrawArrays(GL_QUADS, style.VBOindex, 4);

      glUniform1ui(wsys._sl->u_customPos, 1); /// disable custom quad position

    
    } else if(style.texBGwrap== 2) {
    // REPEATED / MIRRORED REPEATED background
      // must be tied to a point, when stretching. if not, the background will move and that CANNOT happen
      //
      // X-+-++
      // | | ||
      // +-+-++
      // | | ||
      // +-+-++
      // +-+-++


      // ++-+-+-++
      // ++-+-+-++
      // || | | || <<< WRONG IDEEA. background will move, this won't work. MUST BE TIED TO up-left point (or a different one)
      // ++-+-+-++
      // || |X| ||
      // ++-+-+-++
      // || | | ||
      // ++-+-+-++
      // ++-+-+-++

      glUniform1ui(wsys._sl->u_useClipping, 1);       /// enable clipping
      glUniform2f(wsys._sl->u_clip0, _x- osi.glrWin->x0,     _y- osi.glrWin->y0);     /// fragment shader works with viewport coords
      glUniform2f(wsys._sl->u_clipE, _x+ dx- osi.glrWin->x0, _y+ dy- osi.glrWin->y0); /// fragment shader works with viewport coords
      
      for(short a= 0; a< nrT; a++) 
        for(short b= 0; b< nrS; b++) {
          glUniform3f(wsys._sl->u_origin, _x+ b* style.texBG.dx, _y+ a* style.texBG.dy, 0.0f);
          glDrawArrays(GL_QUADS, style.VBOindex, 4);
        }

      glUniform1ui(wsys._sl->u_useClipping, 0);       /// disable clipping

    } else if(style.texBGwrap== 3) {
      glUniform1ui(wsys._sl->u_useClipping, 1);       /// enable clipping
      glUniform2f(wsys._sl->u_clip0, _x- osi.glrWin->x0,     _y- osi.glrWin->y0);     /// fragment shader works with viewport coords
      glUniform2f(wsys._sl->u_clipE, _x+ dx- osi.glrWin->x0, _y+ dy- osi.glrWin->y0); /// fragment shader works with viewport coords
      glUniform1ui(wsys._sl->u_customTex, 1);         /// enable custom texcoords

      ixSubTex *p= &style.texBG;

      for(short a= 0; a< nrT; a++) {
        for(short b= 0; b< nrS; b++) {
          glUniform3f(wsys._sl->u_origin, _x+ b* style.texBG.dx, _y+ a* style.texBG.dy, 0.0f);

          /// inverse tex coords not even
          glUniform2f(wsys._sl->u_quadTex0, (b% 2? p->se: p->s0), (a% 2? p->te: p->t0));
          glUniform2f(wsys._sl->u_quadTexE, (b% 2? p->s0: p->se), (a% 2? p->t0: p->te));

          glDrawArrays(GL_QUADS, style.VBOindex, 4);
        }
      }

      glUniform1ui(wsys._sl->u_customTex, 0);         /// disable custom texcoords
      glUniform1ui(wsys._sl->u_useClipping, 0);       /// disable clipping

    } /// pass thru all possible background texturing types
  } /// if using a texture
  



  /// useColorOnTexture usage flag - use color for current texture
  if(style.useColorOnTexture)
    glUniform4fv(wsys._sl->u_color, 1, style.colorBRD.v);
  else
    glUniform4f(wsys._sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);

  ///---------------------------------------------------------------
  // WINDOW BORDER ========================-------------------------
  ///---------------------------------------------------------------

  // TOP border ===============----------------------------------
  if(style.useTexture && style.bTexBrd[_BRD_TOP]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= (int)dx/ style.texBrd[_BRD_TOP].dx+ (((int)dx% style.texBrd[_BRD_TOP].dx)? 1: 0);
    nrT= (int)dy/ style.texBrd[_BRD_TOP].dy+ (((int)dy% style.texBrd[_BRD_TOP].dy)? 1: 0);

    float yorg= _y+ dy+ style.texBrdDist[_BRD_TOP]- style.texBrd[_BRD_TOP].dy;

    glUniform1ui(wsys._sl->u_useClipping, 1); /// enable clipping
    glUniform2f(wsys._sl->u_clip0, _x- osi.glrWin->x0,     yorg- osi.glrWin->y0);     /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted
    glUniform2f(wsys._sl->u_clipE, _x+ dx- osi.glrWin->x0, yorg+ dy- osi.glrWin->y0); /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted

    /// fixed border
    if(style.texBrdWrap[_BRD_TOP]== 0) {
      glUniform3f(wsys._sl->u_origin, _x, yorg, 0.0f);
      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_TOP, 4);

    /// stretched border
    } else if(style.texBrdWrap[_BRD_TOP]== 1) {
      glUniform3f(wsys._sl->u_origin, _x, yorg, 0.0f);
      glUniform1ui(wsys._sl->u_customPos, 1); /// enable custom vertex position
      glUniform2f(wsys._sl->u_quadPos0, 0.0f, 0.0f);
      glUniform2f(wsys._sl->u_quadPosE, dx, style.texBrd[_BRD_TOP].dy);

      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_TOP, 4);

      glUniform1ui(wsys._sl->u_customPos, 0); /// disable custom vertex position

    /// repeat border
    } else if(style.texBrdWrap[_BRD_TOP]== 2) {
      for(int a= 0; a< nrS; a++) {
        glUniform3f(wsys._sl->u_origin, _x+ a* style.texBrd[_BRD_TOP].dx, yorg, 0.0f);
        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_TOP, 4);
      }

    /// mirrored repeat border
    } else if(style.texBrdWrap[_BRD_TOP]== 3) {
      glUniform1ui(wsys._sl->u_customTex, 1);   /// enable custom tex coords
      
      for(int a= 0; a< nrS; a++) {
        glUniform3f(wsys._sl->u_origin, _x+ a* style.texBrd[_BRD_TOP].dx, yorg, 0.0f);
        glUniform2f(wsys._sl->u_quadTex0, (a% 2? style.texBrd[_BRD_TOP].se: style.texBrd[_BRD_TOP].s0), style.texBrd[_BRD_TOP].t0);
        glUniform2f(wsys._sl->u_quadTexE, (a% 2? style.texBrd[_BRD_TOP].s0: style.texBrd[_BRD_TOP].se), style.texBrd[_BRD_TOP].te);

        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_TOP, 4);
      }

      glUniform1ui(wsys._sl->u_customTex, 0);   /// disable custom tex coords
    }
    glUniform1ui(wsys._sl->u_useClipping, 0); /// disable clipping
  } /// if there's a TOP texture


  // BOTTOM border ===============----------------------------------
  if(style.useTexture && style.bTexBrd[_BRD_BOTTOM]) {   /// there's texture for it
    /// nr of times the texture repeats
    nrS= (int)dx/ style.texBrd[_BRD_BOTTOM].dx+ (((int)dx% style.texBrd[_BRD_BOTTOM].dx)? 1: 0);
    nrT= (int)dy/ style.texBrd[_BRD_BOTTOM].dy+ (((int)dy% style.texBrd[_BRD_BOTTOM].dy)? 1: 0);
    float yorg= _y- style.texBrdDist[_BRD_BOTTOM];
    

    glUniform1ui(wsys._sl->u_useClipping, 1); /// enable clipping
    glUniform2f(wsys._sl->u_clip0, _x- osi.glrWin->x0,     yorg- osi.glrWin->y0);      /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted
    glUniform2f(wsys._sl->u_clipE, _x+ dx- osi.glrWin->x0, yorg+ dy- osi.glrWin->y0);  /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted

    /// fixed border
    if(style.texBrdWrap[_BRD_BOTTOM]== 0) {
      glUniform3f(wsys._sl->u_origin, _x, yorg, 0.0f);
      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_BOTTOM, 4);

    /// stretched border
    } else if(style.texBrdWrap[_BRD_BOTTOM]== 1) {
      glUniform3f(wsys._sl->u_origin, _x, yorg, 0.0f);
      glUniform1ui(wsys._sl->u_customPos, 1); /// enable custom vertex position
      glUniform2f(wsys._sl->u_quadPos0, 0, 0);
      glUniform2f(wsys._sl->u_quadPosE, dx, style.texBrd[_BRD_BOTTOM].dy);

      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_BOTTOM, 4);

      glUniform1ui(wsys._sl->u_customPos, 0); /// disable custom vertex position

    /// repeat border
    } else if(style.texBrdWrap[_BRD_BOTTOM]== 2) {
      for(int a= 0; a< nrS; a++) {
        glUniform3f(wsys._sl->u_origin, _x+ a* style.texBrd[_BRD_BOTTOM].dx, yorg, 0.0f);
        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_BOTTOM, 4);
      }

    /// mirrored repeat border
    } else if(style.texBrdWrap[_BRD_BOTTOM]== 3) {
      glUniform1ui(wsys._sl->u_customTex, 1);   /// enable custom tex coords
      
      for(int a= 0; a< nrS; a++) {
        glUniform3f(wsys._sl->u_origin, _x+ a* style.texBrd[_BRD_BOTTOM].dx, yorg, 0.0f);
        glUniform2f(wsys._sl->u_quadTex0, (a% 2? style.texBrd[_BRD_BOTTOM].se: style.texBrd[_BRD_BOTTOM].s0), style.texBrd[_BRD_BOTTOM].t0);
        glUniform2f(wsys._sl->u_quadTexE, (a% 2? style.texBrd[_BRD_BOTTOM].s0: style.texBrd[_BRD_BOTTOM].se), style.texBrd[_BRD_BOTTOM].te);

        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_BOTTOM, 4);
      }

      glUniform1ui(wsys._sl->u_customTex, 0);   /// disable custom tex coords
    }
    glUniform1ui(wsys._sl->u_useClipping, 0);   /// disable clipping
  } /// if there's a BOTTOM texture
  

  // RIGHT border ===============----------------------------------
  if(style.useTexture && style.bTexBrd[_BRD_RIGHT]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= (int)dx/ style.texBrd[_BRD_RIGHT].dx+ (((int)dx% style.texBrd[_BRD_RIGHT].dx)? 1: 0);
    nrT= (int)dy/ style.texBrd[_BRD_RIGHT].dy+ (((int)dy% style.texBrd[_BRD_RIGHT].dy)? 1: 0);
    float xorg= _x+ dx+ style.texBrdDist[_BRD_RIGHT]- style.texBrd[_BRD_RIGHT].dx;
    float yorg= _y+ dy- style.texBrd[_BRD_RIGHT].dy;

    glUniform1ui(wsys._sl->u_useClipping, 1); /// enable clipping
    glUniform2f(wsys._sl->u_clip0, xorg- osi.glrWin->x0,     _y- osi.glrWin->y0);     /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted
    glUniform2f(wsys._sl->u_clipE, xorg+ dx- osi.glrWin->x0, _y+ dy- osi.glrWin->y0); /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted

    /// fixed border
    if(style.texBrdWrap[_BRD_RIGHT]== 0) {
      glUniform3f(wsys._sl->u_origin, xorg, yorg, 0.0f);
      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_RIGHT, 4);

    /// stretched border
    } else if(style.texBrdWrap[_BRD_RIGHT]== 1) {
      glUniform3f(wsys._sl->u_origin, xorg, _y, 0.0f);
      glUniform1ui(wsys._sl->u_customPos, 1); /// enable custom vertex position
      glUniform2f(wsys._sl->u_quadPos0, 0.0f, 0.0f);
      glUniform2f(wsys._sl->u_quadPosE, style.texBrd[_BRD_RIGHT].dx, dy);

      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_RIGHT, 4);

      glUniform1ui(wsys._sl->u_customPos, 0); /// disable custom vertex position

    /// repeat border
    } else if(style.texBrdWrap[_BRD_RIGHT]== 2) {
      for(int a= 0; a< nrT; a++) {
        glUniform3f(wsys._sl->u_origin, xorg, yorg- a* style.texBrd[_BRD_RIGHT].dy, 0.0f);
        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_RIGHT, 4);
      }

    /// mirrored repeat border
    } else if(style.texBrdWrap[_BRD_RIGHT]== 3) {
      glUniform1ui(wsys._sl->u_customTex, 1);   /// enable custom tex coords
      
      for(int a= 0; a< nrT; a++) {
        glUniform3f(wsys._sl->u_origin, xorg, yorg- a* style.texBrd[_BRD_RIGHT].dy, 0.0f);
        glUniform2f(wsys._sl->u_quadTex0, style.texBrd[_BRD_RIGHT].s0, (a% 2? style.texBrd[_BRD_RIGHT].te: style.texBrd[_BRD_RIGHT].t0));
        glUniform2f(wsys._sl->u_quadTexE, style.texBrd[_BRD_RIGHT].se, (a% 2? style.texBrd[_BRD_RIGHT].t0: style.texBrd[_BRD_RIGHT].te));

        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_RIGHT, 4);
      }

      glUniform1ui(wsys._sl->u_customTex, 0);   /// disable custom tex coords
    }
    glUniform1ui(wsys._sl->u_useClipping, 0);   /// disable clipping
  } /// if there's a RIGHT texture


  // LEFT border ===============----------------------------------
  if(style.useTexture && style.bTexBrd[3]) {     /// there's texture for it
    /// nr of times the texture repeats
    nrS= (int)dx/ style.texBrd[_BRD_LEFT].dx+ (((int)dx% style.texBrd[_BRD_LEFT].dx)? 1: 0);
    nrT= (int)dy/ style.texBrd[_BRD_LEFT].dy+ (((int)dy% style.texBrd[_BRD_LEFT].dy)? 1: 0);
    float xorg= _x- style.texBrdDist[_BRD_LEFT];
    float yorg= _y+ dy- style.texBrd[_BRD_LEFT].dy;

    glUniform1ui(wsys._sl->u_useClipping, 1); /// enable clipping
    glUniform2f(wsys._sl->u_clip0, xorg- osi.glrWin->x0,     _y- osi.glrWin->y0);     /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted
    glUniform2f(wsys._sl->u_clipE, xorg+ dx- osi.glrWin->x0, _y+ dy- osi.glrWin->y0); /// fragment shader coords are in actual viewport coords, so osiWindow must be substracted

    /// fixed border
    if(style.texBrdWrap[_BRD_LEFT]== 0) {
      glUniform3f(wsys._sl->u_origin, xorg, yorg, 0.0f);
      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_LEFT, 4);

    /// stretched border
    } else if(style.texBrdWrap[_BRD_LEFT]== 1) {
      glUniform3f(wsys._sl->u_origin, xorg, _y, 0.0f);
      glUniform1ui(wsys._sl->u_customPos, 1); /// enable custom vertex position
      glUniform2f(wsys._sl->u_quadPos0, 0.0f, 0.0f);
      glUniform2f(wsys._sl->u_quadPosE, style.texBrd[_BRD_LEFT].dx, dy);

      glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_LEFT, 4);

      glUniform1ui(wsys._sl->u_customPos, 0); /// disable custom vertex position

    /// repeat border
    } else if(style.texBrdWrap[_BRD_LEFT]== 2) {
      for(int a= 0; a< nrT; a++) {
        glUniform3f(wsys._sl->u_origin, xorg, yorg- a* style.texBrd[_BRD_LEFT].dy, 0.0f);
        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_LEFT, 4);
      }

    /// mirrored repeat border
    } else if(style.texBrdWrap[_BRD_LEFT]== 3) {
      glUniform1ui(wsys._sl->u_customTex, 1);   /// enable custom tex coords
      
      for(int a= 0; a< nrT; a++) {
        glUniform3f(wsys._sl->u_origin, xorg, yorg- a* style.texBrd[_BRD_LEFT].dy, 0.0f);
        glUniform2f(wsys._sl->u_quadTex0, style.texBrd[_BRD_LEFT].s0, (a% 2? style.texBrd[_BRD_LEFT].te: style.texBrd[_BRD_LEFT].t0));
        glUniform2f(wsys._sl->u_quadTexE, style.texBrd[_BRD_LEFT].se, (a% 2? style.texBrd[_BRD_LEFT].t0: style.texBrd[_BRD_LEFT].te));

        glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_LEFT, 4);
      }

      glUniform1ui(wsys._sl->u_customTex, 0);   /// disable custom tex coords
    }
    glUniform1ui(wsys._sl->u_useClipping, 0);   /// disable clipping
  } /// if there's a LEFT texture


  // CORNERS ==============-----------------------

  // TOP- LEFT corner
  if(style.useTexture && style.bTexBrd[_BRD_TOPLEFT]) {
    glUniform3f(wsys._sl->u_origin, _x- style.texBrdDist[_BRD_TOPLEFT],
                                    _y+ dy- style.texBrd[_BRD_TOPLEFT].dy+ style.texBrdDist[_BRD_TOPLEFT],
                                    0.0f);
    glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_TOPLEFT, 4);
  }

  // TOP- RIGHT corner
  if(style.useTexture && style.bTexBrd[_BRD_TOPRIGHT]) {
    glUniform3f(wsys._sl->u_origin, _x+ dx- style.texBrd[_BRD_TOPRIGHT].dx+ style.texBrdDist[_BRD_TOPRIGHT],
                                    _y+ dy- style.texBrd[_BRD_TOPRIGHT].dy+ style.texBrdDist[_BRD_TOPRIGHT],
                                    0.0f);
    glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_TOPRIGHT, 4);

  }

  // BOTTOM- RIGHT corner
  if(style.useTexture && style.bTexBrd[_BRD_BOTTOMRIGHT]) {
    glUniform3f(wsys._sl->u_origin, _x+ dx- style.texBrd[_BRD_BOTTOMRIGHT].dx+ style.texBrdDist[_BRD_BOTTOMRIGHT],
                                    _y- style.texBrdDist[_BRD_BOTTOMRIGHT],
                                    0.0f);
    glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_BOTTOMRIGHT, 4);
  }

  // BOTTOM- LEFT corner
  if(style.useTexture && style.bTexBrd[_BRD_BOTTOMLEFT]) {
    glUniform3f(wsys._sl->u_origin, _x- style.texBrdDist[_BRD_BOTTOMLEFT],
                                    _y- style.texBrdDist[_BRD_BOTTOMLEFT],
                                    0.0f);
    glDrawArrays(GL_QUADS, style.VBOindex+ _VBOID_BRD_BOTTOMLEFT, 4);
  }

  glEnable(GL_DEPTH_TEST);
}





void ixBaseWindow::update() {
}


void ixBaseWindow::move(float in_x, float in_y) {
  float deltax= in_x- x;
  float deltay= in_y- y;
  x= in_x, y= in_y;
  
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
    p->updateHooksDelta(deltax, deltay, 0.0f);
}


/*
bool BaseWindow::loadTex(cchar *file) {
  /// if it already handles a texture, it has to be deleted first
  if(style._handlesTex)
    texData.data.del(style.tex);

  style.tex= texData.createTexture(file);

  if(!style.tex) {
    style._handlesTex= false;
    delData();
    return false;
  }
  

  /// set the background as the full extent of the texture (DEFAULT)
  style.useTexture= true;
  style.bTexBG= true;
  style.texBG.dx= style.tex->dx;
  style.texBG.dy= style.tex->dy;

  /// usage flags change
    isVisible= true;      // set the flag to show the window
  // useColorOnTexture= false;
  style.useBackColor= false;

  return style._handlesTex= true;
}
*/




inline void _setHookPosFromBorder(ixBaseWindow *out_window, int8 in_border, int32 in_dx, int32 in_dy) {
  if(in_border== IX_BORDER_BOTTOMLEFT) return;
  if(in_border== IX_BORDER_TOP)
    out_window->hookPos.x+= in_dx/ 2,
    out_window->hookPos.y+= in_dy;
  else if(in_border== IX_BORDER_RIGHT)
    out_window->hookPos.x+= in_dx,
    out_window->hookPos.y+= in_dy/ 2;
  else if(in_border== IX_BORDER_BOTTOM)
    out_window->hookPos.x+= in_dx/ 2;
  else if(in_border== IX_BORDER_LEFT)
    out_window->hookPos.y+= in_dy/ 2;
  else if(in_border== IX_BORDER_TOPLEFT)
    out_window->hookPos.y+= in_dy;
  else if(in_border== IX_BORDER_TOPRIGHT)
    out_window->hookPos.x+= in_dx,
    out_window->hookPos.y+= in_dy;
  else if(in_border== IX_BORDER_BOTTOMRIGHT)
    out_window->hookPos.x+= in_dx;
  
}

void _setWinPosBorderTo0(ixBaseWindow *out_window, int8 in_border) {
  out_window->x= out_window->y= out_window->z= 0;
  
  if(in_border== IX_BORDER_TOP)
    out_window->x-= out_window->dx/ 2,
    out_window->y-= out_window->dy;

  else if(in_border== IX_BORDER_RIGHT)
    out_window->x-= out_window->dx,
    out_window->y-= out_window->dy/ 2;

  else if(in_border== IX_BORDER_BOTTOM)
    out_window->x-= out_window->dx/ 2;

  else if(in_border== IX_BORDER_LEFT)
    out_window->y-= out_window->dy/ 2;

  else if(in_border== IX_BORDER_TOPLEFT)
    out_window->y-= out_window->dy;

  else if(in_border== IX_BORDER_TOPRIGHT)
    out_window->x-= out_window->dx,
    out_window->y-= out_window->dy;

  else if(in_border== IX_BORDER_BOTTOMRIGHT)
    out_window->x-= out_window->dx;

  ///else if(in_border== IX_BORDER_BOTTOMLEFT)
    /// do notin
  
}

inline void _setHookPosFromBorderIxWin(ixBaseWindow *out_window, ixBaseWindow *in_window, int8 in_border) {
  out_window->hookPos= in_window->hookPos;
  out_window->hookPos.x+= in_window->x;
  out_window->hookPos.y+= in_window->y;

  _setHookPosFromBorder(out_window, in_border, in_window->dx, in_window->dy);
}

inline void _setHookPosFromBorderOsiWin(ixBaseWindow *out_window, osiWindow *in_window, int8 in_border) {
  out_window->hookPos= vec3(in_window->x0, in_window->y0, 0.0f);
  _setHookPosFromBorder(out_window, in_border, in_window->dx, in_window->dy);
}

inline void _setHookPosFromBorderOsiMon(ixBaseWindow *out_window, osiMonitor *in_window, int8 in_border) {
  out_window->hookPos= vec3(in_window->x0, in_window->y0, 0.0f);
  _setHookPosFromBorder(out_window, in_border, in_window->dx, in_window->dy);
}


// window hooking functions =============-----------------------

void ixBaseWindow::setHookAnchor(ixBaseWindow *in_window, int8 in_border) {
  hookBorder= in_border;
  hookIxWin= in_window;
  hookOsiWin= null;
  hookOsiMon= null;

  if(parent) parent->childrens.release(this);
  else       wsys.topObjects.release(this);

  parent= in_window;

  if(parent) in_window->childrens.add(this);
  else       wsys.topObjects.add(this);

  _setHookPosFromBorderIxWin(this, in_window, in_border);
  updateHooks(false);
}

void ixBaseWindow::setHookAnchor(osiWindow *in_window, int8 in_border) {
  hookBorder= in_border;
  hookIxWin= null;
  hookOsiWin= in_window;
  hookOsiMon= null;
  if(parent) {
    parent->childrens.release(this);
    parent= null;
    wsys.topObjects.add(this);
  }

  _setHookPosFromBorderOsiWin(this, in_window, in_border);
  updateHooks(false);
}

void ixBaseWindow::setHookAnchor(osiMonitor *in_window, int8 in_border) {
  hookBorder= in_border;
  hookIxWin= null;
  hookOsiWin= null;
  hookOsiMon= in_window;
  if(parent) {
    parent->childrens.release(this);
    parent= null;
    wsys.topObjects.add(this);
  }

  _setHookPosFromBorderOsiMon(this, in_window, in_border);
  updateHooks(false);
}

/// sets the hook to the virtual desktop
void ixBaseWindow::setHookAnchor(int8 in_border) {
  hookBorder= in_border;
  hookIxWin= null;
  hookOsiWin= null;
  hookOsiMon= null;

  if(parent) {
    parent->childrens.release(this);
    parent= null;
    wsys.topObjects.add(this);
  }

  _setHookPosFromBorder(this, in_border, osi.display.vdx, osi.display.vdy);
  updateHooks(false);
}

/// hooks to ixBaseWindow (in point border1) and moves current window (point border2) to touch it
void ixBaseWindow::setHook(ixBaseWindow *in_window, int8 in_border1, int8 in_border2) {
  hookBorder= in_border1;
  hookIxWin= in_window;
  hookOsiWin= null;
  hookOsiMon= null;

  if(parent) parent->childrens.release(this);
  else       wsys.topObjects.release(this);

  parent= in_window;

  if(parent) in_window->childrens.add(this);
  else       wsys.topObjects.add(this);

  _setHookPosFromBorderIxWin(this, in_window, in_border1);    /// set hook anchor
  _setWinPosBorderTo0(this, in_border2);                      /// set current window position to touch anchor
  updateHooks(false);
}

/// hooks to osiWindow (in point border1) and moves current window (point border2) to touch it
void ixBaseWindow::setHook(osiWindow *in_window, int8 in_border1, int8 in_border2) {
  hookBorder= in_border1;
  hookIxWin= null;
  hookOsiWin= in_window;
  hookOsiMon= null;

  if(parent) {
    parent->childrens.release(this);
    parent= null;
    wsys.topObjects.add(this);
  }

  _setHookPosFromBorderOsiWin(this, in_window, in_border1);
  _setWinPosBorderTo0(this, in_border2);
  updateHooks(false);
}

/// hooks to osiMonitor (in point border1) and moves current window (point border2) to touch it
void ixBaseWindow::setHook(osiMonitor *in_window, int8 in_border1, int8 in_border2) {
  hookBorder= in_border1;
  hookIxWin= null;
  hookOsiWin= null;
  hookOsiMon= in_window;

  if(parent) {
    parent->childrens.release(this);
    parent= null;
    wsys.topObjects.add(this);
  }

  _setHookPosFromBorderOsiMon(this, in_window, in_border1);
  _setWinPosBorderTo0(this, in_border2);
  updateHooks(false);
}
 
/// hooks to virtual desktop (in point border1) and moves current window (point border2) to touch it
void ixBaseWindow::setHook(int8 in_border1, int8 in_border2) {
  hookBorder= in_border1;
  hookIxWin= null;
  hookOsiWin= null;
  hookOsiMon= null;

  if(parent) {
    parent->childrens.release(this);
    parent= null;
    wsys.topObjects.add(this);
  }

  _setHookPosFromBorder(this, in_border1, osi.display.vdx, osi.display.vdy);
  _setWinPosBorderTo0(this, in_border2);
  updateHooks(false);
}


/*  SCRAPED
void ixBaseWindow::setHookDefault() {
  hookBorder= 7;                      // bottom left (0, 0 default)
  hookIxWin= null;
  hookOsiWin= null;
  hookOsiMon= null;
  hookPos= vec3(0.0f, 0.0f, 0.0f);
  
}
*/

// !!! after a window is moved, the hook and all the children's hooks _must_ be updated !!!
void ixBaseWindow::updateHooks(bool in_updateThis) {
  if(in_updateThis) {
    /// update own vars based on parent change
    if(hookIxWin)           /// hooked to an ix window
      _setHookPosFromBorderIxWin(this, hookIxWin, hookBorder);
    else if(hookOsiWin)     /// hooked to an osi window
      _setHookPosFromBorderOsiWin(this, hookOsiWin, hookBorder);
    else if(hookOsiMon)     /// hooked to an osi monitor
      _setHookPosFromBorderOsiMon(this, hookOsiMon, hookBorder);
    else                    /// hooked to the virtual desktop
      _setHookPosFromBorder(this, hookBorder, osi.display.vdx, osi.display.vdy);
  }

  /// update childrens
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
    p->updateHooks();
}

void ixBaseWindow::updateHooksDelta(float in_dx, float in_dy, float in_dz, bool in_updateThis) {
  if(in_updateThis) {
    hookPos.x+= in_dx;
    hookPos.y+= in_dy;
    hookPos.z+= in_dz;
  }

  /// update childrens
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.first; p; p= (ixBaseWindow *)p->next)
    p->updateHooksDelta(in_dx, in_dy, in_dz);
}


// returns window coordinates based on the hook, in the virtual dektop
void ixBaseWindow::getVDcoords3f(float *out_x, float *out_y, float *out_z) {
  if(out_x) *out_x= hookPos.x+ x;
  if(out_y) *out_y= hookPos.y+ y;
  if(out_z) *out_z= hookPos.z+ z;
}

void ixBaseWindow::getVDcoords3i(int32 *out_x, int32 *out_y, int32 *out_z) {
  if(out_x) *out_x= (int32)(hookPos.x+ x);
  if(out_y) *out_y= (int32)(hookPos.y+ y);
  if(out_z) *out_z= (int32)(hookPos.z+ z);
}

void ixBaseWindow::getVDcoordsv3(vec3 *out) {
  if(out) *out= vec3(hookPos.x+ x, hookPos.y+ y, hookPos.z+ z);
}





/// rotates corner 4 or border 0, depeanding on var <nr>, and creates border <nr>
/// this avoids further unnecesary work in photoshop and texture will hold only the first corner/border,
///  if the borders are the same, but need rotating

/* SEEMS IT CAN'T BE DONE <<<

void BaseWindow::setBRDcoordsFromFirst(short nr) {

  /// right border is requested
  if(nr== 1) {
    texBrd[nr].dx= texBrd[0].dy;
    texBrd[nr].dy= texBrd[0].dx;
    texBrd[nr].
    texBrd[nr].s0= texBrd[0].t0;
    texBrd[nr].se= texBrd[0].t0;
    texBrd[nr].t0= 
  /// bottom border is requested
  } else if(nr== 2) {

  /// left border is requested
  } else if(nr== 3) {

  /// top - right corner requested
  } else if(nr== 5) {

  }

}
*/
















///=============-----------------------------
// WINDOW class =============================
///=============-----------------------------


ixWindow::ixWindow() {
  ixBaseWindow();
  title= null;
  _type= _IX_WINDOW;
  _op.moving= _op.resizeBottom= _op.resizeLeft= _op.resizeRight= false;
}


ixWindow::~ixWindow() {
  delData();
}


void ixWindow::delData() {
  _op.moving= _op.resizeBottom= _op.resizeLeft= _op.resizeRight= false;
  ixBaseWindow::delData();
  title= null;
}




// funcs

void ixWindow::draw() {
  ixBaseWindow::draw();       /// start by drawing the base
  if(useTitle && title)
    title->draw();            /// draw the window title

  ixBaseWindow *p= (ixBaseWindow *)childrens.first;

  while(p) {
    p->draw();
    p= (ixBaseWindow *)p->next;
  }
}


void ixWindow::update() {
  // these can be in one variable... if(currentAction== _RESIZE_BOTTOM) ... currentAction can be 0, meaning no active stuff is happening.
  // for many windows, this must be changed i think. problems can happen, must further test with many buttons/windows

  if(_op.resizeBottom || _op.resizeLeft || _op.resizeRight || _op.moving) {

    if(!in.m.but[0].down) {
      _op.resizeBottom= _op.resizeLeft= _op.resizeRight= _op.moving= false;
      return;
    }

    if(_op.moving) {
      move(x+ in.m.dx, y+ in.m.dy);
      //if(title) title->updateHooksDelta((float)in.m.dx, (float)in.m.dy, 0.0f);
      return;
    }
    if(_op.resizeBottom) {
      if(dy- in.m.dy>= 15) {      /// minimum 15 pixels <<<<<<<<<<<<
        move(x, y+ in.m.dy);
        dy-= in.m.dy;
        updateHooks(false);
        if(title) title->updateHooks();
      }
      return;
    }
    if(_op.resizeLeft) {
      if(dx- in.m.dx>= 15) {      /// minimum 15 pixels <<<<<<<<<<<<
        move(x+ in.m.dx, y);
        dx-= in.m.dx;
        updateHooks(false);
        if(title) title->updateHooks();
      }
      return;
    }
    if(_op.resizeRight) {
      if(dx+ in.m.dx>= 15) {     /// minimum 15 pixels <<<<<<<<<<<<
        dx+= in.m.dx;
        updateHooks(false);
        if(title) title->updateHooks();
      }
      return;
    }
  } else {

    

    if(in.m.but[0].down) {

      
      /// check if user wants to drag the window
      if(usage.movable) {
        if(useTitle && title) {         /// if using a window title, moving is done by dragging the title
          if(mPos(title->hookPos.x+ title->x, title->hookPos.y+ title->y, title->dx, title->dy)) {
            _op.moving= true;
            return;
          }
        } /// useTitle drag

        /// if window has no title, moving is done by dragging anywhere inside the window
        if(mPos(hookPos.x+ x+ 5, hookPos.y+ y+ 5, dx- 10, dy- 5)) {      /// 5 pixels ok?
          _op.moving= true;
          return;
        }

      } /// useMovable

      /// check if user wants to resize the window
      if(usage.resizeable) {
        /// left resize
        if(mPos(hookPos.x+ x, hookPos.y+ y, 4, dy)) {
          _op.resizeLeft= true;
        }
        if(mPos(hookPos.x+ x+ dx- 5, hookPos.y+ y, 4, dy- 1)) {
          _op.resizeRight= true;
        }
        if(mPos(hookPos.x+ x, hookPos.y+ y, dx- 1, 4)) {
          _op.resizeBottom= true;
        }
      } /// useResizeable
    } /// if left mouse button is down
  } /// check if starting of a drag or resize

}


void ixWindow::setTitle(cchar *in_text, ixWSsubStyle *in_style) {
  useTitle= true;
  
  /// if there's no title static text created, this window will create it and set it as child
  if(!title) {
    title= new ixStaticText;
    childrens.add(title);
    title->parent= this;

    if(in_style)
      title->style= in_style;
    else
      title->style= style;

    title->font= pr.selFont;        /// using currently selected font
    title->text= in_text;
    title->updateSizeFromText();
    title->is.visible= true;

    setTitlePosition(title->style.titlePosition, title->style.titleOrientation, title->style.titleDist, title->style.titleInside);
  } /// window has a title

    
  else {
    title->text= in_text;
    title->updateSizeFromText();
  }
}

// set window title positions/characteristics: hookBorder- border that it hooks to the main window; orientation 90/270 - horizontal 0/180 vertical; distance- distance from the border, in pixels; inside- the title is inside the window
void ixWindow::setTitlePosition(int8 in_hookBorder, int16 in_orientation, float in_distance, bool in_inside) {
  if(title) {
    /// title border hook (snap) based on orientation
    int8 b= in_hookBorder;

    if(!in_inside) {
      if(b== IX_BORDER_TOP)               b= IX_BORDER_BOTTOM;
      else if(b== IX_BORDER_RIGHT)        b= IX_BORDER_LEFT;
      else if(b== IX_BORDER_BOTTOM)       b= IX_BORDER_TOP;
      else if(b== IX_BORDER_LEFT)         b= IX_BORDER_RIGHT;

      if(in_orientation== 90|| in_orientation== 270) {
        if(b== IX_BORDER_TOPLEFT)           b= IX_BORDER_BOTTOMLEFT;
        else if(b== IX_BORDER_TOPRIGHT)     b= IX_BORDER_BOTTOMRIGHT;
        else if(b== IX_BORDER_BOTTOMRIGHT)  b= IX_BORDER_TOPRIGHT;
        else if(b== IX_BORDER_BOTTOMLEFT)   b= IX_BORDER_TOPLEFT;
      } else {
        if(b== IX_BORDER_TOPLEFT)           b= IX_BORDER_TOPRIGHT;
        else if(b== IX_BORDER_TOPRIGHT)     b= IX_BORDER_TOPLEFT;
        else if(b== IX_BORDER_BOTTOMRIGHT)  b= IX_BORDER_BOTTOMLEFT;
        else if(b== IX_BORDER_BOTTOMLEFT)   b= IX_BORDER_BOTTOMRIGHT;
      }
    }

    title->setHook(this, in_hookBorder, b);

    /// distance from the border
    if(in_orientation== 90 || in_orientation== 270) {
      if(in_hookBorder== IX_BORDER_TOP || in_hookBorder== IX_BORDER_TOPLEFT|| in_hookBorder== IX_BORDER_TOPRIGHT)
        y+= in_distance;
      else if(in_hookBorder== IX_BORDER_BOTTOM || in_hookBorder== IX_BORDER_BOTTOMLEFT || in_hookBorder== IX_BORDER_BOTTOMRIGHT)
        y-= in_distance;
      else if(in_hookBorder== IX_BORDER_LEFT)
        x-= in_distance;
      else if(in_hookBorder== IX_BORDER_RIGHT)
        x+= in_distance;
    } else {
      if(in_hookBorder== IX_BORDER_LEFT || in_hookBorder== IX_BORDER_TOPLEFT || in_hookBorder== IX_BORDER_BOTTOMLEFT)
        x-= in_distance;
      else if(in_hookBorder== IX_BORDER_RIGHT || in_hookBorder== IX_BORDER_TOPRIGHT || in_hookBorder== IX_BORDER_BOTTOMRIGHT)
        x+= in_distance;
      else if(in_hookBorder== IX_BORDER_TOP)
        y+= in_distance;
      else if(in_hookBorder== IX_BORDER_BOTTOM)
        y-= in_distance;
    } /// orientation

  } else
    error.console("ixWindow::setTitlePosition(): changes to window title were requested, but title is not created");
}








///==============----------------------///
// BUTTON object ====================== //
///==============----------------------///

ixButton::ixButton() {
  ixBaseWindow::ixBaseWindow();
  _type= _IX_BUTTON;
}

ixButton::~ixButton() {
  delData();
}

void ixButton::delData() {
  ixBaseWindow::delData();
  text.delData(); 
}


/*
void ixButton::setText(cchar *s) {
  text= s;
}
*/


void ixButton::draw() {
  ixBaseWindow::draw();
  pr.selFont= font;
  pr.txt2(hookPos.x+ x+ textX, hookPos.y+ y+ textY, text);

}


void ixButton::update() {

}


///===================----------------------///
// STATIC TEXT object ====================== //
///===================----------------------///


ixStaticText::ixStaticText() {
  ixBaseWindow::ixBaseWindow();
  _type= _IX_STATIC_TEXT;
  font= null;
}

ixStaticText::~ixStaticText() {
  delData();
}

void ixStaticText::delData() {
  text.delData();
  font= null;
}


void ixStaticText::updateSizeFromText() {
  pr.selFont= font;
  dx= (float)pr.getTextDx(text)+ 6;
  dy= (float)pr.getCharDy()+ 6;
  textX= 3;
  textY= 3;
}

void ixStaticText::draw() {
  ixBaseWindow::draw();
  pr.selFont= font;
  pr.txt2(hookPos.x+ x+ textX, hookPos.y+ y+ textY, text);
  
}



  ///==========----------------------///
// EDIT object ====================== //
///============----------------------///


ixEdit::ixEdit() {
  ixBaseWindow::ixBaseWindow();
  _type= _IX_EDIT;
}


ixEdit::~ixEdit() {
  delData();
}


void ixEdit::delData() {

}


void ixEdit::draw() {
  ixBaseWindow::draw();
}

void ixEdit::update() {
  ixBaseWindow::update();
}









