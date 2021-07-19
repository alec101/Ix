#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

///========================///
// constructor / destructor //
///========================///

ixDropList::ixDropList(): ixBaseWindow() {
  _type= ixeWinType::dropList;

  buttonDx= buttonDy= 0;
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
  usage._parent= this;
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


void ixDropList::Usage::setListMaxLength(int32 in_len) {
  if(in_len< 0) in_len= 0;
  _maxLength= in_len;
  if(in_len== 0)
    _parent->_scr->setVisible(false);
}





///================///
// ixDropList FUNCS //
///================///



void ixDropList::setButtonDxDy(int32 dx, int32 dy) {
  if(dx< 0) dx= 0;
  if(dy< 0) dy= 0;

  buttonDx= dx, buttonDy= dy;
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




void ixDropList::_computeRects() {
  pos.setD(pos.x0, pos.y0, buttonDx, buttonDy);

  // for each option in list
  int32 y= pos.ye;
  for(ixDropListData *p= (ixDropListData *)optionList.first; p; p= (ixDropListData *)p->next) {
    /// pos update
    if(is.expanded) {
      pos.ye+= buttonDy;
      if(usage._maxLength && ((pos.ye- pos.y0- buttonDy)> usage._maxLength))
        pos.ye= pos.y0+ buttonDy+ usage._maxLength;
    }

    p->pos.set(pos.x0, y, pos.xe, y+ buttonDy);

    y+= buttonDy;
  } /// for each option in list

  pos.compDeltas();
  
  _computeAll();
  _computeScr();
}


void ixDropList::_computeScr() {
  int32 x, y;
  int32 dx, dy, minDy;
  int32 lastYe;
  bool isNeeded= false;      // if the scroll is needed
  
  /// if the list is expanded & there is at least one option, the scroll might be needed
  if(is.expanded && optionList.last) {
    lastYe= ((ixDropListData *)optionList.last)->pos.ye;
    // if last option y0 is further down than pos.y0, then scroll is needed
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

    
    _scr->steps= lastYe- pos.ye;
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













//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixDropList::_update(bool in_mIn, bool updateChildren) {
  recti r; getVDcoordsRecti(&r);
  bool insideThis= r.inside(in.m.x, in.m.y);

  // update it's children first
  if(updateChildren)
    if(_updateChildren((in_mIn? insideThis: false)))
      return true;




  // FUNCTION CORE

    // an operation is in progress
  if(Ix::wsys()._op.win) {
    if(Ix::wsys()._op.win != this) return false;

    // button activate / toggle
    if(Ix::wsys()._op.mLclick) {

      // the left mouse button is depressed -> action is needed
      if((!in.m.but[0].down) && insideThis) {
        recti b(r.x0, r.y0, r.dx, buttonDy);

        // top button click - expand/retract the list
        if(b.inside(in.m.x, in.m.y)) {
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
            if(b.inside(in.m.x, in.m.y)) {
              sel= p;
              selNr= a;
              is.expanded= 0;
              _computeRects();
              Ix::wsys()._op.delData();
              Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
              return true;
            }
            b.moveD(0, buttonDy);

          } /// for each option
        } /// depress on top or on list
      } /// left click depressed, inside this
    } /// left click operation

  // no operation is in progress
  } else if(in_mIn) {
    if(in.m.but[0].down) {
      if(r.inside(in.m.x, in.m.y)) {
        //is.pressed= !is.activated; //(is.activated? false: true);

        Ix::wsys()._op.mLclick= true;
        Ix::wsys()._op.win= this;
        Ix::wsys().bringToFront(this);
        Ix::wsys().flags.setUp((uint32)ixeWSflags::mouseUsed);
        return true;
      }
    } /// left mouse button is being pressed
  } /// operation in progress / no operation in progress




  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(in_mIn, false);
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
  int32 charDy= ixPrint::getCharDy(font.selFont);
  int32 _x, _y;
  getVDcoords2i(&_x, &_y);

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
  in_ix->vki.draw.quad.setPosD((float)_x, (float)_y, 0.0f, (float)pos.dx, (float)buttonDy);

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

  
  in_ix->vki.draw.triangle.setPos(0, (float)(_x+ buttonDx- 4),               (float)(_y+ 4));
  in_ix->vki.draw.triangle.setPos(1, (float)(_x+ (buttonDx- buttonDy)+ 4),   (float)(_y+ 4));
  in_ix->vki.draw.triangle.setPos(2, (float)(_x+ (buttonDx- (buttonDy/ 2))), (float)(_y+ buttonDy- 4));

  in_ix->vki.draw.triangle.cmdPushAll(in_cmd);
  in_ix->vki.draw.triangle.cmdDraw(in_cmd);
  in_ix->vki.draw.quad.push.hollow= -1;

  // selected option draw
  if(sel) {
    in_ix->pr.txt32_2i(_x, _y+ ((buttonDy- charDy)/ 2), sel->text);
  }

  // expanded list draw
  if(is.expanded && p) {
    recti clp;
    clp.setD(_x, _y+ buttonDy, pos.dx, pos.dy- buttonDy);
    clp.intersectRect(_clip);
    if(!clp.exists()) return;

    in_ix->vki.cmdScissor(in_cmd, &clp);
    /// background
    in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
    in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
    in_ix->vki.draw.quad.cmdTexture(in_cmd, null);

    in_ix->vki.draw.quad.push.color= color;
    in_ix->vki.draw.quad.setPosD((float)_x, (float)_y+ buttonDy, 0.0f, (float)pos.dx, (float)pos.dy- buttonDy);
    in_ix->vki.draw.quad.push.hollow= -2;
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);
    /// border
    in_ix->vki.draw.quad.push.color= colorBRD;
    in_ix->vki.draw.quad.push.hollow= 2;
    in_ix->vki.draw.quad.cmdPushAll(in_cmd);
    in_ix->vki.draw.quad.cmdDraw(in_cmd);

    in_ix->vki.draw.quad.push.hollow= -1;


    int32 prx= hook.pos.x+ p->pos.x0;
    int32 pry= hook.pos.y+ p->pos.y0+ ((buttonDy- charDy)/ 2)- _scr->position;

    for(; p; p= (ixDropListData *)p->next, pry+= buttonDy)
      in_ix->pr.txt32_2i(prx, pry, p->text);
   
    _scr->_vkDraw(in_cmd, in_ix, in_style);  
  }
}
#endif /// IX_USE_VULKAN



#ifdef IX_USE_OPENGL
void ixDropList::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  /*
  int32 charDy= ixPrint::getCharDy(font.selFont);
  int32 prx, pry;
  ixDropListData *p= (ixDropListData *)optionList.first;


  //ixWSgenericStyle *s= (ixWSgenericStyle *)in_style;
  //if(!s)
  //  s= (ixWSgenericStyle *)style;
  //ixWSstyle::GPU *sGPU= s->parent->getGPUassets(in_ix);
  //if(!sGPU) return;


  in_ix->pr.style= &font;
  in_ix->pr.setScissor(&_clip);

  
  glUseProgram(in_sl->gl->id);
  in_sl->setClipPlaneR(_clip);
  //glUniform4f(sl->u_color, 1.0f, 1.0f, 1.0f, 1.0f);

  // selected option draw
  if(sel) {
    prx= pos.x0;
    pry= (buttonDy- charDy)/ 2;
    if(pry< 0) pry= 0;
    pry+= pos.ye- buttonDy;
    prx+= hook.pos.x;
    pry+= hook.pos.y;

    in_ix->pr.txt32_2i(prx, pry, sel->text);
  }

  // expanded list draw
  if(is.expanded && p) {
    // a border around the dropdown
    //glUseProgram(in_sl->id);
    //in_sl->hollowRect(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy- buttonDy);
    
    in_ix->glo.draw.quad.useProgram();
    in_ix->glo.draw.quad.setClipPlaneR(_clip);
    in_ix->glo.draw.quad.setColorv(colorBRD);
    in_ix->glo.draw.quad.setHollow(1.0f);
    in_ix->glo.draw.quad.setCoordsDi(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy);
    in_ix->glo.draw.quad.render();
    in_ix->glo.draw.quad.setHollow(-1.0f);


    //in_ix->pr.addClipPlaneD(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy- buttonDy);
    recti clp; clp.setD(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy- buttonDy);
    clp.intersectRect(_clip);
    in_ix->pr.setClipPlaneR(&clp);

    prx= p->x0;
    pry= (buttonDy- charDy)/ 2;
    if(pry< 0) pry= 0;
    pry+= p->y0;
    pry+= _scr->position;
    prx+= hook.pos.x;
    pry+= hook.pos.y;

    for(; p; p= (ixDropListData *)p->next) {
      in_ix->pr.txt32_2i(prx, pry, p->text);
      pry-= buttonDy;
    }

    in_ix->pr.delScissor();
  }
  */
}
#endif /// IX_USE_OPENGL
















