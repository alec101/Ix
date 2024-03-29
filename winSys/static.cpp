#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/* TODO / IDEEAS
================

- WHEN A CHAR IS DELETED, ALL IT'S DIECRITICALS MUST BE DELETED!!!

*/





ixStaticText::ixStaticText(): text(this), usage(this), ixBaseWindow(&is, &usage) {
  _type= ixeWinType::staticText;

  this->_createScrollbars();
  vscroll->is.visible= hscroll->is.visible= false;
  textX= textY= 0.0f;
  
  /// always a line in the editor
  //ixTxtData::Line *p= new ixTxtData::Line;
  //text.lines.add(p);
  //text._updateWrapList();

  //text.cur.pLine= p;
  //text.cur.pWline= (ixTxtData::Wline *)text._wrapLines.first;
}


ixStaticText::~ixStaticText() {
  delData();
  vscroll= hscroll= null;
}


void ixStaticText::delData() {
  text.delData();
  text.sel.delData();
  text.cur.pos= 0;
  text.cur.line= 0;
}


void ixStaticText::updateSizeFromText() {
  
  bool horiz= text.orientation& IX_TXT_HORIZONTAL;    /// shortcut, less comp
  float charDy= text.font.getCharDy();
  float dx= 0, dy= 0;
  
  // find out the text size, _not based on wlines_; text.textDx/Dy will not work
  for(ixTxtData::Line *p= (ixTxtData::Line *)text.lines.first; p; p= (ixTxtData::Line *)p->next) {
    float l= text.font.getTextLen32(p->text, 0, 0.0f, text.orientation)+ 1;
    if(horiz)
      dx= (dx< l? l: dx),
      dy+= charDy;
    else
      dx+= charDy,
      dy= (dy< l? l: dy);
  }

  // style->parent->title.texBRD[0].dy << POSIBILITY
  resize(dx+ (textX+ textX), dy+ (textY+ textY));
  //pos.resize(dx+ (textX+ textX), dy+ (textY+ textY));

  // textX= 3; ?
  // textY= 3; ?
}


// sets the window's text, accepts a str32
void ixStaticText::setText32(str32 *in_txt) {
  text.clearText();
  text.sel.delData();
  text.cur.pos= 0, text.cur.line= 0;
  text.sel.paste(in_txt);
}

void ixStaticText::setFont(const char *in_fname, const char *in_fnt, int in_size) {
  _fontFileName= in_fname;
  _fontName= in_fnt;
  _fontSize= in_size;

  Ix *ix= Ix::getMain();

  void *fnt= ix->pr.getFont(_fontName, _fontSize);
  if(fnt== null)
    fnt= ix->pr.loadFont(_fontFileName, _fontSize);
  if(fnt== null)
    error.window("ixStaticText::setFont(): CRITICAL - could not find requested font/fontfile/fontsize", true);

  //text.font.scale= _scale;
  text.setFont(fnt);
}

void ixStaticText::setFont(const void *in_fnt) {
  _ixFSize *fnt= (_ixFSize *)in_fnt;
  setFont(fnt->font->fileName, fnt->font->name, fnt->size);
}


float ixStaticText::getMinDx() {
  // these could include normal buttons to be able to be shown, the'x', minimize, restore, and maybe some window icon
  if(text.orientation& IX_TXT_HORIZONTAL) {
    return 50.0f;
  }else if(text.orientation& IX_TXT_VERTICAL) {
    return 15.0f;
  }
  return 0.0f;
}


float ixStaticText::getMinDy() {
  // these could include normal buttons to be able to be shown, the'x', minimize, restore, and maybe some window icon
  if(text.orientation& IX_TXT_HORIZONTAL) {
    return 15.0f;
  }else if(text.orientation& IX_TXT_VERTICAL) {
    return 50.0f;
  }
  return 0.0f;
}


void ixStaticText::_computeAll() {
  ixBaseWindow::_computeAll();
  text._computeWrapLen();
  text._updateWrapList();
}

void ixStaticText::_computeAllDelta(float x, float y) {
  ixBaseWindow::_computeAllDelta(x, y);
  text._computeWrapLen();
  text._updateWrapList();
}

/*
void ixStaticText::resize(float dx, float dy) {
  // identic func with ixEdit
  // any change here, must happen to the other one too

  ixBaseWindow::resize(dx, dy);
  text._computeWrapLen();
  text._updateWrapList();   // <<< _VIEW AND CUR WILL BE PLACED AT START. THAT IS NOT GOOD
  
}


void ixStaticText::resizeDelta(float dx, float dy) {
  // identic func with ixEdit
  // any change here, must happen to the other one too

  ixBaseWindow::resizeDelta(dx, dy);
  text._computeWrapLen();
  text._updateWrapList();
}


void ixStaticText::setPos(float x0, float y0, float dx, float dy) {
  // identic func with ixEdit
  // any change here, must happen to the other one too

  ixBaseWindow::setPos(x0, y0, dx, dy);
  text._computeWrapLen();
  text._updateWrapList();
}
*/





void ixStaticText::_computeChildArea() {
  // identic func with ixEdit
  // any change here, must happen to the other one too

  ixBaseWindow::_computeChildArea();

  if(_childArea.xe< text.textDx)
    _childArea.xe= text.textDx;
  if(_childArea.ye< text.textDy)
    _childArea.ye= text.textDy;
  _childArea.compDeltas();
}












bool ixStaticText::_checkLimits(char32 unicode) {
  if(usage.oneLine)       if(unicode== '\n') return false;

  return true;
}





// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW


#ifdef IX_USE_OPENGL
void ixStaticText::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme();
}
#endif /// IX_USE_OPENGL


#ifdef IX_USE_VULKAN
void ixStaticText::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  ixBaseWindow::_vkDraw(in_cmd, in_ix, in_style);
  if(!_clip.exists()) return;
  if(!is.visible) return;

  rectf r; _getVDviewArea(&r);


  //in_ix->vki.cmdScissor(in_cmd, &_clip);
  in_ix->vki.cmdScissorDefault(in_cmd);

  vec3 scr;
  scr.x= (hscroll? hscroll->position: 0.0f);
  scr.y= (vscroll? vscroll->position: 0.0f);
  scr.z= 0.0f;

  // text draw

  text.font.selFont= in_ix->pr.getFont(_fontName, _fontSize);
  text._vkDraw(in_cmd, in_ix, r, scr);
    
  /// scrollbars draw
  if(usage._scrollbars || usage._autoScrollbars) {
    if(hscroll) hscroll->_vkDraw(in_cmd, in_ix);
    if(vscroll) vscroll->_vkDraw(in_cmd, in_ix);
  }

  /// childrens draw
  for(ixBaseWindow *p= (ixBaseWindow *)childrens.last; p; p= (ixBaseWindow *)p->prev)
    if(p!= hscroll && p!= vscroll)
      p->_vkDraw(in_cmd, in_ix);
}
#endif /// IX_USE_VULKAN







//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixStaticText::_update(bool updateChildren) {
  if(!is.visible) return false;

  if(text._update())
    return true;

  return ixBaseWindow::_update(updateChildren);
}














