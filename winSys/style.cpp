#include "ix/ix.h"
#include "osi/include/util/fileOp.h"
#include "ix/winSys/_privateDef.h"


/* TODO:
 - FONTS. can't use the selected font, can you? it's kinda cheap. a style should have fonts, even more than one
   atm, it's working with currently selected font, but i don't think this is good
*/

/* NOTES:
 - half points wraps and such is not happening
 - texture must be tied to a corner; the corner must be up-left - opposite to the resizing corner (altho, resizing should be done on any border)
*/








/*
when creating more types of controls, check styleGPU::createVBO

*/




ixWSstyle::ixWSstyle() {
  window.parent=
    title.parent=
    button.parent=
    buttonPressed.parent=
    edit.parent=
    text.parent=
    scroll.parent= this;

  window._type=         _IX_SUBSTYLE_WINDOW;
  title._type=          _IX_SUBSTYLE_TITLE;
  button._type=         _IX_SUBSTYLE_BUTTON;
  buttonPressed._type=  _IX_SUBSTYLE_BUTTONPRESSED;
  edit._type=           _IX_SUBSTYLE_EDIT;
  text._type=           _IX_SUBSTYLE_TEXT;
  scroll._type=         _IX_SUBSTYLE_SCROLL;


  //tex= null;
  //_VBOid= 0;

  /// these are constants - set only here
  window.VBOindex= 0;
  title.VBOindex=  36;
  button.VBOindex= 72;
  buttonPressed.VBOindex= 108;
  edit.VBOindex=   144;
  text.VBOindex=   180;
  scroll.VBOindex= 216;
  
  delData();
}

ixWSstyle::~ixWSstyle() {
  delData();
}

void ixWSstyle::delData() {
  _texList.delData();
  //for(GPU *p= (GPU *)_gpu.first; p; p= (GPU *)p->next)
  //  p->delData();

  //_gpu.delData();
}



/*
// create the assets for the style, on every ix engine
void ixWSstyle::createAssets() {
  _gpu.delData();
  for(Ix *i= 
  for(_IxList *p= (_IxList *)Ix::_ixList().first; p; p= (_IxList *)p->next) {
    GPU *i= new GPU;
    _gpu.add(i);
    i->createAssets(p->ix, this);
  }
}
*/


// loads the texture on every ix engine
bool ixWSstyle::loadTexture() {
  #ifdef IX_USE_OPENGL
  Ix *saveGlr= *Ix::glActiveRen();   /// save current glr
  #endif

  bool ret= false;

  for(Ix *i= (Ix *)Ix::ixList().first; i; i= (Ix *)i->next) {
    Texture *t= new Texture(i);
    _texList.add(t);

    
    t->tex= i->res.tex.add.ixStaticTexture();
    t->tex->flags.setUp(0x02);
    if(!t->ix->res.tex.load(t->tex, _texName)) goto Exit;

    #ifdef IX_USE_OPENGL
    if(i->renOpenGL()) i->glMakeCurrent();
    #endif

    //if(!t->tex->upload()) goto Exit;
    _texDx= t->tex->data->dx;
    _texDy= t->tex->data->dy;
  }

  ret= true; // success

Exit:
  if(!ret) {
    _texList.delData();
  }

  #ifdef IX_USE_OPENGL
  if(saveGlr!= *Ix::glActiveRen())
    (*Ix::glActiveRen())->glMakeCurrent();   /// restore current glr
  #endif
  return ret;
}


ixTexture *ixWSstyle::getTexture(Ix *in_ix) {
  for(Texture *t= (Texture *)_texList.first; t; t= (Texture *)t->next)
    if(t->ix== in_ix)
      return t->tex;            // found

  return in_ix->vki.noTexture;  // not found
}


/*
// creates the VBO buffers on every ix engine
bool ixWSstyle::createBuffers() {
  bool b= true;
  for(GPU *p= (GPU *)_gpu.first; p; p= (GPU *)p->next)
    if(!p->createVBO()) b= false;
  return b;
}
*/
/*
ixWSstyle::GPU *ixWSstyle::getGPUassets(Ix *in_ix) {
  for(GPU *p= (GPU *)_gpu.first; p; p= (GPU *)p->next)
    if(p->ix== in_ix)
      return p;
  return null;
}
*/


/*
bool ixWSstyle::Texture::loadTex(cchar *fname) {
  osiRenderer *r= osi.glr;
  osiWindow *w= osi.glrWin;

  ix->glMakeCurrent();

  if(!tex)
    tex= ix->texSys.add.ixStaticTexture();
  if(!tex) goto Error;

  if(!tex->loadMem(fname)) goto Error;
  parent->_texDx= tex->data->dx;
  parent->_texDy= tex->data->dy;
  if(!tex->upload()) goto Error;

  osi.glMakeCurrent(r, w);
  return true;    // successs

Error:
  osi.glMakeCurrent(r, w);
  return false;   // failure
}
*/

/*
// styleGPU class //
///==============///

void ixWSstyle::GPU::delData() {
  if(!ix) return;
  osiRenderer *r= osi.glr;
  osiWindow *w= osi.glrWin;
  
  ix->glMakeCurrent();
  if(tex) { tex->unload(); delete tex; }
  if(VBOid) glDeleteBuffers(1, &VBOid);
  if(VAOid) glDeleteVertexArrays(1, &VAOid);
  VBOid= VAOid= 0;
  tex= null;

  osi.glMakeCurrent(r, w);
}



bool ixWSstyle::GPU::createAssets(Ix *in_ix, ixWSstyle *in_style) {
  osiRenderer *r= osi.glr;
  osiWindow *w= osi.glrWin;

  ix= in_ix;
  parent= in_style;

  //ix->makeCurrent();
  //if(!loadTex(parent->_texName)) goto Error;
  //createVBO();

  // success
  //osi.glMakeCurrent(r, w);
  return true;

//Error:
  // failure
  osi.glMakeCurrent(r, w);

  return false;
}


bool ixWSstyle::GPU::loadTex(cchar *fname) {
  osiRenderer *r= osi.glr;
  osiWindow *w= osi.glrWin;

  ix->glMakeCurrent();

  if(!tex)
    tex= ix->texSys.add.ixStaticTexture();
  if(!tex) goto Error;

  if(!tex->loadMem(fname)) goto Error;
  parent->_texDx= tex->data->dx;
  parent->_texDy= tex->data->dy;
  if(!tex->upload()) goto Error;

  osi.glMakeCurrent(r, w);
  return true;    // successs

Error:
  osi.glMakeCurrent(r, w);
  return false;   // failure
}
*/



/*
// func used in createVBO
float *_memfillSubTexVert(float *out_mem, ixSubTex *in_tex, int8 in_fill) {
  if(in_fill) {
// GL_QUADS
//    *out_mem++= 0;           *out_mem++= 0;           *out_mem++= 0;  // v1: x0= y0= z0= 0;
//    *out_mem++= 0;           *out_mem++= in_tex->dy;  *out_mem++= 0;  // v2: x0 ye z0
//    *out_mem++= in_tex->dx;  *out_mem++= in_tex->dy;  *out_mem++= 0;  // v3: xe ye z0
//    *out_mem++= in_tex->dx;  *out_mem++= 0;           *out_mem++= 0;  // v4: xe y0 z0

// GL_TRIANGLE_STRIP
    *out_mem++= 0;           *out_mem++= in_tex->dy;  *out_mem++= 0;  // v2: x0 ye z0
    *out_mem++= in_tex->dx;  *out_mem++= in_tex->dy;  *out_mem++= 0;  // v3: xe ye z0
    *out_mem++= 0;           *out_mem++= 0;           *out_mem++= 0;  // v1: x0= y0= z0= 0;
    *out_mem++= in_tex->dx;  *out_mem++= 0;           *out_mem++= 0;  // v4: xe y0 z0

  } else 
    out_mem+= 12;

  return out_mem;
}

// func used in createVBO
float *_memfillSubTexCoords(float *out_mem, ixSubTex *in_tex, int8 in_fill) {
  if(in_fill) {
// GL_QUADS
//    *out_mem++= in_tex->s0; *out_mem++= in_tex->t0;    // v1: x0 y0
//    *out_mem++= in_tex->s0; *out_mem++= in_tex->te;    // v2: x0 ye
//    *out_mem++= in_tex->se; *out_mem++= in_tex->te;    // v3: xe ye
//    *out_mem++= in_tex->se; *out_mem++= in_tex->t0;    // v4: xe y0
// GL_TRIANGLE_STRIP
    *out_mem++= in_tex->s0; *out_mem++= in_tex->te;    // v2: x0 ye
    *out_mem++= in_tex->se; *out_mem++= in_tex->te;    // v3: xe ye
    *out_mem++= in_tex->s0; *out_mem++= in_tex->t0;    // v1: x0 y0
    *out_mem++= in_tex->se; *out_mem++= in_tex->t0;    // v4: xe y0

  } else
    out_mem+= 8;
  
  return out_mem;
}
*/


/*
// creates a vbo for the style
bool ixWSstyle::GPU::createVBO() {
  if(!osi.glr) { error.detail("no renderer active", __FUNCTION__, __LINE__); return false; }

  error.glFlushErrors();
  /// 1 quad takes 80 bytes ([4vert[3float] 4tex[2float]]= 80bytes)
  /// bg  1x[4vert[3float] 4tex[2float]]            (48bytes vertPos + 32bytes texCoords = 80bytes)
  /// brd 8x[4vert[3float] 4tex[2float]]            8x 80bytes= 640bytes
  /// everything x6 [win edit title but butPrs text] 4320bytes
  /// scroll: arrows 4x[4vert[3float] 4tex[2float]]
  ///         scr    2x[4vert[3float] 4tex[2float]]
  ///         scrBck 2x[4vert[3float] 4tex[2float]] 640bytes
  //  total = 4960 bytes     6x subStyle[720]= 4320    scrollStyle[640] 
  //  vertex[2976]   texcoords[1984]

  #define _TOTAL 4960
  #define _TEXSTART 2976

  ixWSgenericStyle *s= null;
  float *f;
  uint8 *data= new uint8[_TOTAL];   // fixed size - if something is not used, it's 0
  
  if(!data) { error.detail("couldn't alloc mem", __FUNCTION__, __LINE__); return false; }
  
  Str::memclr(data, _TOTAL);
  f= (float*)data;     /// f will walk the data as a float
  


  // populate vertPos ===---
  for(int a= 0; a< 6; a++) {        // pass thru each substyle [6]
    /// set the substyle pointer
    if(a== 0)      s= &parent->window;
    else if(a== 1) s= &parent->title;
    else if(a== 2) s= &parent->button;
    else if(a== 3) s= &parent->buttonPressed;
    else if(a== 4) s= &parent->edit;
    else if(a== 5) s= &parent->text;

    /// background
    f= _memfillSubTexVert(f, &s->texBG, s->bTexBG);

    /// borders (8 max)
    for(int b= 0; b< 8; b++)
      f= _memfillSubTexVert(f, &s->texBRD[b], s->bTexBRD[b]);
  } /// pass thru each substyle [6]

  /// scroll bars
  for(int a= 0; a< 4; a++)
    f= _memfillSubTexVert(f, &parent->scroll.texArrows[a], parent->scroll.bTexArrows);

  for(int a= 0; a< 2; a++)
    f= _memfillSubTexVert(f, &parent->scroll.texDragbox[a], parent->scroll.bTexDragbox);

  for(int a= 0; a< 2; a++)
    f= _memfillSubTexVert(f, &parent->scroll.texScrollBack[a], parent->scroll.bTexScrollBack);


  // populate texPos ===---
  for(int a= 0; a< 6; a++) {        // pass thru each substyle [6]
    /// set the substyle pointer
    if(a== 0)      s= &parent->window;
    else if(a== 1) s= &parent->title;
    else if(a== 2) s= &parent->button;
    else if(a== 3) s= &parent->buttonPressed;
    else if(a== 4) s= &parent->edit;
    else if(a== 5) s= &parent->text;

    /// background
    f= _memfillSubTexCoords(f, &s->texBG, s->bTexBG);

    /// borders (8 max)
    for(int b= 0; b< 8; b++)
      f= _memfillSubTexCoords(f, &s->texBRD[b], s->bTexBRD[b]);
  } /// pass thru each substyle [6]

  /// scrollbar tex coords
  for(int a= 0; a< 4; a++)
    f= _memfillSubTexCoords(f, &parent->scroll.texArrows[a], parent->scroll.bTexArrows);

  for(int a= 0; a< 2; a++)
    f= _memfillSubTexCoords(f, &parent->scroll.texDragbox[a], parent->scroll.bTexDragbox);

  for(int a= 0; a< 2; a++)
    f= _memfillSubTexCoords(f, &parent->scroll.texScrollBack[a], parent->scroll.bTexScrollBack);
  
  // create the VBO buffer
  glGenBuffers(1, &VBOid);
  glBindBuffer(GL_ARRAY_BUFFER, VBOid);
  glBufferData(GL_ARRAY_BUFFER, _TOTAL, data, GL_STATIC_DRAW); /// size of the vertex data and tex coods

  error.glError("ixWSstyle::createVBO(): ");
  
  glGenVertexArrays(1, &VAOid);
  glBindVertexArray(VAOid);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)(_TEXSTART));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  
  //if(!_VAO.id)
  //  _VAO.genArray();
  //_VAO.bindAndVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  //_VAO.bindAndVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)(_TEXSTART));
  //_VAO.enableVertexAttribArray(0);
  //_VAO.enableVertexAttribArray(1);
  

  if(error.glError("ixWSstyle::createVBO(): ")) goto Error;

  #undef _TOTAL
  #undef _TEXSTART

  delete[] data;
  return true;

Error:

  delete[] data;
  return false;
}
*/


/*
// deletes the vbo tied to that style
void ixWSstyle::ixStyleGPU::delVBO() {
  if(!osi.glr) return;
  if(VBOid) glDeleteBuffers(1, &VBOid);
  if(VAOid) glDeleteVertexArrays(1, &VAOid);
  VBOid= VAOid= 0;
}
*/












// LOAD STYLE ===============--------------- //

int _getWrap(str8 *s) {
  s->lower();
  if     (*s== "fixed"          || *s== "fix"          || *s== "0") return 0;
  else if(*s== "stretch"        || *s== "stretched"    || *s== "1") return 1;
  else if(*s== "repeat"         || *s== "repeated"     || *s== "2") return 2;
  else if(*s== "mirroredrepeat" || *s== "mirrorrepeat" || *s== "3") return 3;
  else return 0;
}

int16 _getOrientation(str8 *s) {
  s->lower();
  if     (*s== "lefttoright" || *s== "right" || *s== "toright" || *s== "90")  return 90;
  else if(*s== "righttoleft" || *s== "left"  || *s== "toleft"  || *s== "270") return 270;
  else if(*s== "uptodown"    || *s== "down"  || *s== "todown"  || *s== "180") return 180;
  else if(*s== "downtoup"    || *s== "up"    || *s== "toup"    || *s== "0")   return 0;
  return 90;
}

int8 _getBorderPoint(str8 *s) {
  s->lower();
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
  if(!f) { Ix::console().print("ixWSstyle::loadStyle(): file not found"); return false; }

  bool useDeltas= true;
  str8 line, s, s1, s2, s3, s4;
  ixWSsubStyleBase *style= null;

  //createAssets();

  while(!feof(f)) {
    readLine8(f, &line);
    char *p= line.d;
    if(!p) break;
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
    p= readWordOrWordsInQuotes(p, &s1);   if(*p== ',') p= (char *)((int8 *)p+ 1);   //s1.lower();   WHAT IF IT'S A FILENAME? -.-
    p= readWordOrWordsInQuotes(p, &s2);   if(*p== ',') p= (char *)((int8 *)p+ 1);   //s2.lower();
    p= readWordOrWordsInQuotes(p, &s3);   if(*p== ',') p= (char *)((int8 *)p+ 1);   //s3.lower();
    p= readWordOrWordsInQuotes(p, &s4);   if(*p== ',') p= (char *)((int8 *)p+ 1);   //s4.lower();

    if(!s.d) continue;
    else if(s== "") continue;

    s.lower();
    if(s.len>= 2) {
      if(s.d[s.len- 2]== '=') s-= 1;
      else if(s.d[s.len- 2]== ':') s-= 1;
    }
    
    if(s== "texture" || s== "texturefile" || s== "texturefilename") {
      _texName= s1;
      loadTexture();
    } else if(s== "coords") {


    } else if(s== "window" || s== "win") {
      style= &window;

    } else if(s== "title" || s== "windowtitle" || s== "wintitle") {
      style= &title;

    } else if(s== "button" || s== "buttons") {
      style= &button;

    } else if(s== "buttonpressed" || s== "buttonspressed") {
      style= &buttonPressed;

    } else if(s== "edit" || s== "edittext") {
      style= &edit;

    } else if(s== "text" || s== "static" || s== "statictext" ) {
      style= &text;

    } else if(s== "scrollbar" || s== "scroll") {
      style= &scroll;



    } else if(s== "donotusedelta") {
      useDeltas= false;

    } else if(s== "usedelta" || s== "usedeltas") {
      if(s1== "false" || s1== "0") useDeltas= false;
      else useDeltas= true;

    } else
      if(style) style->_load_textLine(&s, &s1, &s2, &s3, &s4, useDeltas);

    

  }

  fclose(f);

  //if(!createBuffers()) return false;      /// buffers must be created after the texture and all style data is loaded

  return true;
}



// BASE VARS TO LOAD
bool ixWSsubStyleBase::_load_textLine(str8 *s, str8 *s1, str8 *s2, str8 *s3, str8 *s4, bool useDeltas, uint8 origin) {

  if(*s== "usetexture") {
    useTexture= _getBool(s1);
    return true;

  } else if(*s== "usebackcolor" || *s== "usebgcolor" || *s== "usebackgroundcolor") {
    useBackColor= _getBool(s1);
    return true;

  } else if(*s== "usecolorontexture") {
    useColorOnTexture= _getBool(s1);
    return true;

  } else if(*s== "color" || *s== "colorbg" || *s== "colorbackground" || *s== "bgcolor" || *s== "backgroundcolor") {
    color.set(s1->toFloat(), s2->toFloat(), s3->toFloat(), s4->toFloat());
    return true;

  } else if(*s== "colorfocus" || *s== "focuscolor") {
    colorFocus.set(s1->toFloat(), s2->toFloat(), s3->toFloat(), s4->toFloat());
    return true;

  } else if(*s== "colorhover" || *s== "hovercolor") {
    colorHover.set(s1->toFloat(), s2->toFloat(), s3->toFloat(), s4->toFloat());
    return true;
  }

  return false;
}


// GENERIC SPECIFIC VARS TO LOAD
bool ixWSgenericStyle::_load_textLine(str8 *s, str8 *s1, str8 *s2, str8 *s3, str8 *s4, bool useDeltas, uint8 origin) {
  if(ixWSsubStyleBase::_load_textLine(s, s1, s2, s3, s4, useDeltas))
    return true;

  int x0= (int)s1->toInt(), y0= (int)s2->toInt();
  int dx= (int)(useDeltas? s3->toInt(): (s3->toInt()- s1->toInt()));
  int dy= (int)(useDeltas? s4->toInt(): (s4->toInt()- s2->toInt()));
  
  if(*s== "colorbrd" || *s== "colorborder" || *s== "bordercolor" || *s== "brdcolor") {
    colorBRD.set(s1->toFloat(), s2->toFloat(), s3->toFloat(), s4->toFloat());
    return true;

  } else if(*s== "colorbrdfocus" || *s== "colorborderfocus" || *s== "borderfocuscolor" || *s== "brdfocuscolor") {
    colorBRDfocus.set(s1->toFloat(), s2->toFloat(), s3->toFloat(), s4->toFloat());
    return true;

  //  bTexBG and bTexBRD[8] are set to true if any tex coord happen

  } else if(*s== "background" || *s== "bg" || *s== "backgroundtexture" || *s== "bgtexture") {
    setBGcoords(x0, y0, dx, dy);
    return true;

  } else if(*s== "border0" || *s== "brd0" || *s== "bordertop" || *s== "brdtop") {
    setBRDcoords(0, x0, y0, dx, dy);
    return true;

  } else if(*s== "border1" || *s== "brd1" || *s== "borderright" || *s== "brdright") {
    setBRDcoords(1, x0, y0, dx, dy);
    return true;

  } else if(*s== "border2" || *s== "brd2" || *s== "borderbottom" || *s== "brdbottom") {
    setBRDcoords(2, x0, y0, dx, dy);
    return true;

  } else if(*s== "border3" || *s== "brd3" || *s== "borderleft" || *s== "brdleft") {
    setBRDcoords(3, x0, y0, dx, dy);
    return true;

  } else if(*s== "border4" || *s== "brd4" || *s== "bordertopleft" || *s== "brdtopleft") {
    setBRDcoords(4, x0, y0, dx, dy);
    return true;

  } else if(*s== "border5" || *s== "brd5" || *s== "bordertopright" || *s== "brdtopright") {
    setBRDcoords(5, x0, y0, dx, dy);
    return true;

  } else if(*s== "border6" || *s== "brd6" || *s== "borderbottomright" || *s== "brdbottomright") {
    setBRDcoords(6, x0, y0, dx, dy);
    return true;

  } else if(*s== "border7" || *s== "brd7" || *s== "borderbottomleft" || *s== "brdbottomleft") {
    setBRDcoords(7, x0, y0, dx, dy);
    return true;
    
  } else if(*s== "bgwrap" || *s== "backgroundwrap") {
    setBGwrap(_getWrap(s1));
    return true;

  } else if(*s== "brdwrap" || *s== "borderwrap") {
    setBRDallWrap(_getWrap(s1));
    return true;

  } else if(*s== "brd0wrap" || *s== "border0wrap" || *s== "brdtopwrap" || *s== "bordertopwrap") {
    setBRDwrap(0, _getWrap(s1));
    return true;

  } else if(*s== "brd1wrap" || *s== "border1wrap" || *s== "brdrightwrap" || *s== "borderrightwrap") {
    setBRDwrap(1, _getWrap(s1));
    return true;

  } else if(*s== "brd2wrap" || *s== "border2wrap" || *s== "brdbottomwrap" || *s== "borderbottomwrap") {
    setBRDwrap(2, _getWrap(s1));
    return true;

  } else if(*s== "brd3wrap" || *s== "border3wrap" || *s== "brdleftwrap" || *s== "borderleftwrap") {
    setBRDwrap(3, _getWrap(s1));
    return true;

  } else if(*s== "brddistance" || *s=="borderdistance") {
    setBRDallDist((int16)s1->toInt());
    return true;

  } else if(*s== "brd0distance" || *s== "border0distance" || *s== "brdtopdistance" || *s== "bordertopdistance") {
    setBRDdist(0, s1->toFloat());
    return true;

  } else if(*s== "brd1distance" || *s== "border1distance" || *s== "brdrightdistance" || *s== "borderrightdistance") {
    setBRDdist(1, s1->toFloat());
    return true;

  } else if(*s== "brd2distance" || *s== "border2distance" || *s== "brdbottomdistance" || *s== "borderbottomdistance") {
    setBRDdist(2, s1->toFloat());
    return true;

  } else if(*s== "brd3distance" || *s== "border3distance" || *s== "brdleftdistance" || *s== "borderleftdistance") {
    setBRDdist(3, s1->toFloat());
    return true;

  } else if(*s== "brd4distance" || *s== "border4distance" || *s== "brdtopleftdistance" || *s== "bordertopleftdistance") {
    setBRDdist(4, s1->toFloat());
    return true;

  } else if(*s== "brd5distance" || *s== "border5distance" || *s== "brdtoprightdistance" || *s== "bordertoprightdistance") {
    setBRDdist(5, s1->toFloat());
    return true;

  } else if(*s== "brd6distance" || *s== "border6distance" || *s== "brdbottomrightdistance" || *s== "borderbottomrightdistance") {
    setBRDdist(6, s1->toFloat());
    return true;

  } else if(*s== "brd7distance" || *s== "border7distance" || *s== "brdbottomleftdistance" || *s== "borderbottomleftdistance") {
    setBRDdist(7, s1->toFloat());
    return true;
  }

  return false;
}

// WINDOW SPECIFIC VARS TO LOAD
bool ixWSwindowStyle::_load_textLine(str8 *s, str8 *s1, str8 *s2, str8 *s3, str8 *s4, bool useDeltas, uint8 origin) {
  if(ixWSgenericStyle::_load_textLine(s, s1, s2, s3, s4, useDeltas))
    return true;

  if(*s== "usetitle" || *s== "usetitlebar") {
    useTitle= _getBool(s1);
    return true;

  } else if(*s== "titledist" || *s== "titledistance") {
    titleDist= (int16)s1->toInt();
    return true;

  } else if(*s== "titleinside") {
    titleInside= _getBool(s1);
    return true;

  } else if(*s== "titleorientation") {
    titleOrientation= _getOrientation(s1);
    return true;

  } else if(*s== "titlepointsnap") {
    titlePointSnap= _getBorderPoint(s1);
    return true;
  }

  return false;
}

// SCROLL SPECIFIC VARS TO LOAD
bool ixWSscrollStyle::_load_textLine(str8 *s, str8 *s1, str8 *s2, str8 *s3, str8 *s4, bool useDeltas, uint8 origin) {
  if(ixWSsubStyleBase::_load_textLine(s, s1, s2, s3, s4, useDeltas))
    return true;

  int x0= (int)s1->toInt(), y0= (int)s2->toInt();
  int dx= (int)(useDeltas? s3->toInt(): (s3->toInt()- s1->toInt()));
  int dy= (int)(useDeltas? s4->toInt(): (s4->toInt()- s2->toInt()));


  if(*s== "colorarrows" || *s== "arrowscolor") {
    colorArrows.set(s1->toFloat(), s2->toFloat(), s3->toFloat(), s4->toFloat());
    return true;

  } else if(*s== "colordragbox" || *s== "dragboxcolor") {
    colorDragbox.set(s1->toFloat(), s2->toFloat(), s3->toFloat(), s4->toFloat());
    return true;

  } else if(*s== "arrow0" || *s== "arrowtop" || *s== "arrowup") {
    setArrowcoords(0, x0, y0, dx, dy);
    return true;

  } else if(*s== "arrow1" || *s== "arrowright") {
    setArrowcoords(1, x0, y0, dx, dy);
    return true;

  } else if(*s== "arrow2" || *s== "arrowbottom" || *s== "arrowdown") {
    setArrowcoords(2, x0, y0, dx, dy);
    return true;

  } else if(*s== "arrow3" || *s== "arrowleft") {
    setArrowcoords(3, x0, y0, dx, dy);
    return true;

  } else if(*s== "horizontaldrag" || *s== "horizontaldragbox") {
    setDragboxCoords(0, x0, y0, dx, dy);
    return true;

  } else if(*s== "verticaldrag" || *s== "verticaldragbox") {
    setDragboxCoords(1, x0, y0, dx, dy);
    return true;

  } else if(*s== "horizontalbarback" || *s== "horizontalscrollbarback") {
    setScrollBackCoords(0, x0, y0, dx, dy);
    return true;

  } else if(*s== "verticalbarback" || *s== "verticalscrollbarback") {
    setScrollBackCoords(1, x0, y0, dx, dy);
    return true;



  } else if(*s== "horizontaldistance" || *s== "horizdist") {
    setScrollbarDist(0, s1->toFloat());
    return true;

  } else if(*s== "verticaldistance" || *s== "vertdist") {
    setScrollbarDist(1, s1->toFloat());
    return true;

  } else if(*s== "dragboxwrap" || *s== "dragwrap") {
    setDragboxWrap(_getWrap(s1));
    return true;

  } else if(*s== "barbackwrap" || *s== "scrollbarbackwrap") {
    setScrollBackWrap(_getWrap(s1));
    return true;
  }

  return false;
}














///===========================================///
// SUBSTYLE BASE class ==========------------- //
///===========================================///

ixWSsubStyleBase::ixWSsubStyleBase() {
  _type= _IX_SUBSTYLE_BASE;
  parent= null;
  VBOindex= 0;
  delData();
}


ixWSsubStyleBase::~ixWSsubStyleBase() {
}


void ixWSsubStyleBase::delData() {
  useTexture= false;
  useBackColor= true;
  useColorOnTexture= false;
  color.set(0.5f, 0.5f, 0.5f, 0.5f);
  colorFocus.set(0.7f, 0.5f, 0.7f, 0.55f);
  colorHover.set(0.6f, 0.5f, 0.6f, 0.525f);
}






///===================================================///
//  GENERIC STYLE class ===============--------------- //
///===================================================///

ixWSgenericStyle::ixWSgenericStyle(): ixWSsubStyleBase() {
}


ixWSgenericStyle::~ixWSgenericStyle() {
}


void ixWSgenericStyle::delData() {
  ixWSsubStyleBase::delData();

  colorBRD.set(1.0f, 1.0f, 1.0f, 1.0f);
  colorBRDfocus.set(0.7f, 0.5f, 0.7f, 0.55f);

  /// background vars
  bTexBG= 0;
  texBGwrap= 0;          /// background texture wrap: 0(fixed), 1(stretch), 2(repeat), 3(mirrored repeat)
  
  /// border vars
  for(short a= 0; a< 8; a++) {
    bTexBRD[a]= 0;
    texBRDdist[a]= 0.0f;
    texBRD[a].delData();
    if(a< 4)
      texBRDwrap[a]= 0;
  }
}

// funcs

/// sets the background texCoords (x0, y0, dx, dy)
void ixWSgenericStyle::setBGcoords(int _x0, int _y0, int _dx, int _dy) {
  texBG.dx= mlib::abs32(_dx);
  texBG.dy= mlib::abs32(_dy);
  // THIS NEEDS TESTING FOR NVIDIA/ATI COMPATIBILITY. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  texBG.s0= ((float)_x0)/ (float)parent->_texDx;
  texBG.t0= ((float)_y0)/ (float)parent->_texDy;
  texBG.se= texBG.s0+ (float)_dx/ (float)parent->_texDx;
  texBG.te= texBG.t0+ (float)_dy/ (float)parent->_texDy;
  bTexBG= 1;
}


/// sets the borders (0- 8) texCoords (x0, y0, dx, dy)
void ixWSgenericStyle::setBRDcoords(int16 nr, int _x0, int _y0, int _dx, int _dy) {
  texBRD[nr].dx= mlib::abs32(_dx);
  texBRD[nr].dy= mlib::abs32(_dy);
  // THIS NEEDS TESTING FOR NVIDIA/ATI COMPATIBILITY. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  texBRD[nr].s0= ((float)_x0)/ (float)parent->_texDx;
  texBRD[nr].t0= ((float)_y0)/ (float)parent->_texDy;
  texBRD[nr].se= texBRD[nr].s0+ (float)_dx/ (float)parent->_texDx;
  texBRD[nr].te= texBRD[nr].t0+ (float)_dy/ (float)parent->_texDy;
  bTexBRD[nr]= 1;
}



///===========================================================///
// WINDOW STYLE class ==================---------------------- //
///===========================================================///

ixWSwindowStyle::ixWSwindowStyle(): ixWSgenericStyle() {
}

ixWSwindowStyle::~ixWSwindowStyle() {
}

void ixWSwindowStyle::delData() {
  ixWSgenericStyle::delData();
  useTitle= true;
  titlePosition= 0;
  titlePointSnap= 0;
  titleOrientation= 90;
  titleDist= 0;
  titleInside= false;
}





///=======================================================///
//  SCROLL STYLE class ===============-------------------- //
///==================----=================================///


ixWSscrollStyle::ixWSscrollStyle(): ixWSsubStyleBase() {
}


ixWSscrollStyle::~ixWSscrollStyle() {
}


void ixWSscrollStyle::delData() {
  ixWSsubStyleBase::delData();

  // different colors
  colorArrows.set    (0.4f, 0.4f, 0.4f, 1.0f);
  colorDragbox.set   (0.4f, 0.4f, 0.4f, 1.0f);

  // texture cfg

  bTexArrows= false;
  bTexDragbox= false;
  bTexScrollBack= false;

  for(int a= 0; a< 4; a++)
    texArrows[a].delData();

  for(int a= 0; a< 2; a++)
    texDragbox[a].delData(),
    texScrollBack[a].delData();

  horizontalDist= 1.0f;
  verticalDist= 1.0f;
  texDragboxWrap= 1;
  texScrollBackWrap= 1;
}



void ixWSscrollStyle::setArrowcoords(int16 in_n, int in_x0, int in_y0, int in_dx, int in_dy) {
  texArrows[in_n].dx= mlib::abs32(in_dx);
  texArrows[in_n].dy= mlib::abs32(in_dy);
  // THIS NEEDS TESTING FOR NVIDIA/ATI COMPATIBILITY. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  texArrows[in_n].s0= ((float)in_x0)/ (float)parent->_texDx;
  texArrows[in_n].t0= ((float)in_y0)/ (float)parent->_texDy;
  texArrows[in_n].se= texArrows[in_n].s0+ ((float)in_dx/ (float)parent->_texDx);
  texArrows[in_n].te= texArrows[in_n].t0+ ((float)in_dy/ (float)parent->_texDy);
  bTexArrows= 1;
}

// sets the dragable button (0- 1) tex coords (x0, y0, dx, dy)
void ixWSscrollStyle::setDragboxCoords(int16 in_n, int in_x0, int in_y0, int in_dx, int in_dy) {
  texDragbox[in_n].dx= mlib::abs32(in_dx);
  texDragbox[in_n].dy= mlib::abs32(in_dy);
  // THIS NEEDS TESTING FOR NVIDIA/ATI COMPATIBILITY. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  texDragbox[in_n].s0= ((float)in_x0)/ (float)parent->_texDx;
  texDragbox[in_n].t0= ((float)in_y0)/ (float)parent->_texDy;
  texDragbox[in_n].se= texDragbox[in_n].s0+ (float)in_dx/ (float)parent->_texDx;
  texDragbox[in_n].te= texDragbox[in_n].t0+ (float)in_dy/ (float)parent->_texDy;
  bTexDragbox= 1;
}

// sets the back-line(0- 1) texCoords (x0, y0, dx, dy)
void ixWSscrollStyle::setScrollBackCoords(int16 in_n, int in_x0, int in_y0, int in_dx, int in_dy) {
  texScrollBack[in_n].dx= mlib::abs32(in_dx);
  texScrollBack[in_n].dy= mlib::abs32(in_dy);
  // THIS NEEDS TESTING FOR NVIDIA/ATI COMPATIBILITY. <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  texScrollBack[in_n].s0= ((float)in_x0)/ (float)parent->_texDx;
  texScrollBack[in_n].t0= ((float)in_y0)/ (float)parent->_texDy;
  texScrollBack[in_n].se= texScrollBack[in_n].s0+ (float)in_dx/ (float)parent->_texDx;
  texScrollBack[in_n].te= texScrollBack[in_n].t0+ (float)in_dy/ (float)parent->_texDy;
  bTexScrollBack= 1;
}










