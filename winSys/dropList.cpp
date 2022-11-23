#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/* TODO
  - maybe a style set for it, check draw() to update?
  - draw has a unit compute, must compute only when needed
*/


///========================///
// constructor / destructor //
///========================///

ixDropList::ixDropList(): usage(this), ixBaseWindow(&is, &usage) {
  _type= ixeWinType::dropList;

  buttonDx= buttonDy= 0.0f;
  selNr= -1;
  sel= null;


  _scr= new ixScroll;
  childrens.add(_scr);

  _scr->parent= this;
  _scr->style= &Ix::wsys().selStyle->scroll;
  _scr->target= null;
  _scr->orientation= 1;
  _scr->setVisible(false);
  _scr->hook.set(this, ixEBorder::topLeft, ixEBorder::topLeft);
  _scr->_applyColorsFromStyle();
  _scr->setDragboxFixed(false);

  //_scr->_computePos();
  //_scr->_computeButtons();
  _dropped= false;

}


ixDropList::~ixDropList() {
  delData();
}


void ixDropList::delData() {
  ixBaseWindow::delData();
  optionList.delData();
  sel= null;
  selNr= -1;
}



///===========///
// USAGE funcs //
///===========///


void ixDropList::Usage::setListMaxLength(float in_len) {
  if(in_len< 0.0f) in_len= 0.0f;
  _maxLength= in_len;
  if(in_len== 0.0f)
    ((ixDropList *)_win)->_scr->setVisible(false);
}





///================///
// ixDropList FUNCS //
///================///



void ixDropList::setButtonDxDy(float dx, float dy) {
  if(dx< 0.0f) dx= 0.0f;
  if(dy< 0.0f) dy= 0.0f;

  buttonDx= dx, buttonDy= dy;
  _computeRects();
}


void ixDropList::addOption(cchar *in_text) {
  ixDropListData *dat= new ixDropListData;
  dat->text= in_text;

  if(!sel)
    sel= dat, selNr= 0;

  optionList.add(dat);

  _computeRects();
}


void ixDropList::addOptionAfter(cchar *in_text, ixDropListData *in_after) {
  // this func has user interaction so it has to be kinda safe and generate errors / warnings
  if(in_after== null) { addOption(in_text); return; }

  /// searching thru the list to actually find the in_after node. if not found, func does nothing
  for(ixDropListData *p= (ixDropListData *)optionList.first; p; p= (ixDropListData *)p->next) {
    // node exists in list
    if(in_after == p) {
      ixDropListData *dat= new ixDropListData;
      dat->text= in_text;

      optionList.addAfter(dat, in_after);
      _computeRects();
      return;
    }
  }

  error.console("ixDropList::addOptionAfter(): in_after node not found");
}


void ixDropList::addOptionAftern(cchar *in_text, int32 in_after) {
  // <in_after> out of bounds handling
  bool oob= false;
  if(in_after< 0)
    oob= true, in_after= 0;
  if((optionList.nrNodes> 0) && (in_after> (int32)optionList.nrNodes- 1))
    oob= true, in_after= optionList.nrNodes- 1;
  if((optionList.nrNodes== 0) && (in_after != 0))
    oob= true, in_after= 0;
  if(oob) error.console("ixDropList::addOptionAftern() <in_after> out of bounds");

  // create & add
  ixDropListData *dat= new ixDropListData;
  dat->text= in_text;

  optionList.addi(dat, in_after);
  _computeRects();
}


void ixDropList::delAllOptions() {
  optionList.delData();
  sel= null;
  selNr= 0;

  _computeRects();
}







void ixDropList::_computeRects() {
  float y0list= pos.y0+ buttonDy;
  float dyList= 0;
  ixDropListData *p= (ixDropListData *)optionList.first;

  for(uint a= 0; a< optionList.nrNodes; a++, p= (ixDropListData *)p->next) {
    p->pos.setD(pos.x0, y0list+ (buttonDy* (float)a), pos.dx, buttonDy);     /// rects could differ in height, ATM buttonDy is fine
    if(is.expanded)
      dyList+= p->pos.dy;
  }

  if(usage._maxLength)
    if(dyList> usage._maxLength)
      dyList= usage._maxLength;
  
  dyList+= buttonDy;        // the selected always visible on top

  pos.setD(pos.x0, pos.y0, pos.dx, dyList);

  _computeAll();
  _computeScr();
}


void ixDropList::_computeScr() {
  float x, y, dx, dy, minDy, lastYe;
  bool isNeeded= false;      // if the scroll is needed
  
  /// if the list is expanded & there is at least one option, the scroll might be needed
  if(is.expanded && optionList.last) {
    lastYe= ((ixDropListData *)optionList.last)->pos.ye;
    // if last option y0 is further down than pos.ye, then scroll is needed
    if(lastYe> pos.ye)
      isNeeded= true;
  }

  // scroll is needed
  if(isNeeded) {
    dx= _scr->getMinDx();
    minDy= _scr->getMinDy();
    dy= (MAX(pos.dy- buttonDy, minDy));
    x= pos.dx- dx;
    y= buttonDy;

    
    _scr->steps= int(lastYe- pos.ye);
    _scr->setPos(x, y, dx, dy);
    _scr->_computeButtons();
    _scr->_asurePosInBounds();
    _scr->setVisible(true);

  // scroll is NOT needed
  } else {
    _scr->setVisible(false);
    _scr->steps= 0;
    _scr->setPosition(0);
  }
}


void ixDropList::select(int32 in_n) {
  /// out of bounds check
  if((in_n< 0) || (in_n>= (int32)optionList.nrNodes)) return;

  sel= (ixDropListData *)optionList.get(in_n);
  selNr= in_n;
}










//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixDropList::_update(bool updateChildren) {
  if(!is.visible) return false;

  rectf r;
  bool insideThis;
  float mx, my;

  // update it's children first
  if(updateChildren)
    if(_updateChildren())
      return true;

  mx= _scaleDiv(in.m.x), my= _scaleDiv(in.m.y);
  getPosVD(&r);
  insideThis= r.inside(mx, my);

  // FUNCTION CORE

    // an operation is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win != this) return false;

    // button activate / toggle
    if(Ix::wsys()._op.mLclick) {

      // the left mouse button is depressed -> action is needed
      if((!in.m.but[0].down) && insideThis) {
        rectf b(r.x0, r.y0, pos.dx, buttonDy);

        // top button click - expand/retract the list
        if(b.inside(mx, my)) {
          is.expanded= !is.expanded;
          _computeRects();
          Ix::wsys()._op.delData();
          Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
          return true;

        // click on one of the options in the list
        } else if(is.expanded) {
          int a= 0;
          ixDropListData *p= (ixDropListData *)optionList.first;
          if(p) {
            b= p->pos;
            b.moveD(hook.pos.x, hook.pos.y- _scr->position);
          }

          for(; p; p= (ixDropListData *)p->next, a++) {
            if(b.inside(mx, my)) {
              sel= p;
              selNr= a;
              is.expanded= 0;
              _computeRects();
              Ix::wsys()._op.delData();
              Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
              return true;
            }
            b.moveD(0.0f, buttonDy);

          } /// for each option
        } /// depress on top or on list
      } /// left click depressed, inside this
    } /// left click operation

  // no operation is in progress
  } else if(insideThis) {
    if(in.m.but[0].down) {
      Ix::wsys()._op.mLclick= true;
      Ix::wsys()._op.win= this;
      Ix::wsys().bringToFront(this);
      Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
      return true;
    } /// left mouse button is being pressed
  } /// operation in progress / no operation in progress


  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(false);
}












// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW



#ifdef IX_USE_VULKAN
void ixDropList::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  if(!_clip.exists()) return;
  if(!is.visible) return;
  

  in_ix->pr.style= &font;
  float charDy= font.getCharDy();
  float _x, _y; getPosVD(&_x, &_y);
  
  //ixWSgenericStyle *s= (ixWSgenericStyle *)(in_style? in_style: style); // <<< NO STYLE SET FOR DROPLIST

  ixDropListData *p= (ixDropListData *)optionList.first;
  
  in_ix->vki.cmdScissor(in_cmd, &_clip);

  

  // background
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
  in_ix->vki.draw.quad.cmdTexture(in_cmd, null);
  in_ix->vki.draw.quad.flagTexture(false);

  in_ix->vki.draw.quad.push.color= color;
  in_ix->vki.draw.quad.push.hollow= -2.0f;
  in_ix->vki.draw.quad.setPosD(_x, _y, 0.0f, pos.dx, buttonDy);

  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  // a rectangle around the border
  in_ix->vki.draw.quad.push.color= colorBRD;
  in_ix->vki.draw.quad.push.hollow= 2.0f;
  
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  /// non-textured down arrow
  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.triangle.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.triangle.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
  in_ix->vki.draw.triangle.cmdTexture(in_cmd, null);

  in_ix->vki.draw.triangle.push.color= font.color1;
  in_ix->vki.draw.triangle.flagTexture(false);
  in_ix->vki.draw.triangle.flagDisabled(is.disabled);
  
  float unit= (float)_getUnit()* 4;
  in_ix->vki.draw.triangle.setPos(0, _x+ buttonDx- unit,               _y+ unit);
  in_ix->vki.draw.triangle.setPos(1, _x+ (buttonDx- buttonDy)+ unit,   _y+ unit);
  in_ix->vki.draw.triangle.setPos(2, _x+ (buttonDx- (buttonDy/ 2.0f)), _y+ buttonDy- unit);


  in_ix->vki.draw.triangle.cmdPushAll(in_cmd);
  in_ix->vki.draw.triangle.cmdDraw(in_cmd);
  in_ix->vki.draw.quad.push.hollow= -1;

  // selected option draw
  if(sel) {
    in_ix->pr.txt32_2f(_x, _y+ ((buttonDy- charDy)/ 2.0f), sel->text);
  }

  // expanded list draw
  if(is.expanded && p) {
    rectf clp;
    clp.setD(_x, _y+ buttonDy, pos.dx, pos.dy- buttonDy);
    clp.intersectRect(_clip);
    if(!clp.exists()) return;

    in_ix->vki.cmdScissor(in_cmd, &clp);
    /// background
    in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
    in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
    in_ix->vki.draw.quad.cmdTexture(in_cmd, null);

    in_ix->vki.draw.quad.push.color= color;
    in_ix->vki.draw.quad.setPosD(_x, _y+ buttonDy, 0.0f, pos.dx, pos.dy- buttonDy);
    in_ix->vki.draw.quad.push.hollow= -2;
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
    /// border
    in_ix->vki.draw.quad.push.color= colorBRD;
    in_ix->vki.draw.quad.push.hollow= 2;
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);

    in_ix->vki.draw.quad.push.hollow= -1;

    
    float prx= hook.pos.x+ p->pos.x0;
    float pry= hook.pos.y+ p->pos.y0+ ((buttonDy- charDy)/ 2.0f)- _scr->position;

    for(; p; p= (ixDropListData *)p->next, pry+= buttonDy)
      in_ix->pr.txt32_2f(prx, pry, p->text);
   
    _scr->_vkDraw(in_cmd, in_ix, in_style);  
  }
}
#endif /// IX_USE_VULKAN



#ifdef IX_USE_OPENGL
void ixDropList::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme();
}
#endif /// IX_USE_OPENGL
















