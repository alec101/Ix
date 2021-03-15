#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"



ixProgressBar::ixProgressBar(): ixBaseWindow() {
  _type= _IX_PROGRESSBAR;
  usage._parent= this;
  position= 0;
  usage.setPercentageBar(0, 100);
}


ixProgressBar::~ixProgressBar() {
  delData();
}


void ixProgressBar::delData() {
  ixBaseWindow::delData();
}



// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ usage class █
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

void ixProgressBar::Usage::setPercentageBar(int32 in_val1, int32 in_val2, ixOrientation in_o) {
  barType= 0;
  value1= in_val1;
  value2= in_val2;
  orientation= in_o;
  _parent->resetPosition();
}


void ixProgressBar::Usage::setCustomAmountBar(int32 in_val1, int32 in_val2, ixOrientation in_o) {
  barType= 1;
  value1= in_val1;
  value2= in_val2;
  orientation= in_o;
  _parent->resetPosition();
}


void ixProgressBar::Usage::setTimeBar(int32 in_val1, int32 in_val2, ixOrientation in_o) {
  barType= 2;
  value1= in_val1;
  value2= in_val2;
  orientation= in_o;
  _parent->resetPosition();
}




// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ ixProgressBar funcs █
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀


void ixProgressBar::resetPosition() {
  if(usage.orientation== RIGHT) {
    position= usage.value1;
  } else if(usage.orientation== LEFT) {
    position= usage.value2;
  } else if(usage.orientation== UP) {
    position= usage.value1;
  } else if(usage.orientation== DOWN) {
    position= usage.value2;
  }
  _computeFillingRect();
}


void ixProgressBar::move(int32 x0, int32 y0) {
  ixBaseWindow::move(x0, y0);
  _computeFillingRect();
}


void ixProgressBar::moveDelta(int32 dx, int32 dy) {
  ixBaseWindow::moveDelta(dx, dy);
  _computeFillingRect();
}


void ixProgressBar::resize(int32 dx, int32 dy) {
  ixBaseWindow::resize(dx, dy);
  _computeFillingRect();
}


void ixProgressBar::resizeDelta(int32 dx, int32 dy) {
  ixBaseWindow::resizeDelta(dx, dy);
  _computeFillingRect();
}


void ixProgressBar::setPos(int32 x0, int32 y0, int32 dx, int32 dy) {
  ixBaseWindow::setPos(x0, y0, dx, dy);
  _computeFillingRect();
}


void ixProgressBar::setPosition(int32 in_pos) {
  position= in_pos;
  _asurePositionInBounds();
  _computeFillingRect();
}


void ixProgressBar::_asurePositionInBounds() {
  if(usage.value1< usage.value2) {
    if(position< usage.value1) position= usage.value1;
    if(position> usage.value2) position= usage.value2;
  } else {
    if(position< usage.value2) position= usage.value2;
    if(position> usage.value1) position= usage.value1;
  }
}


void ixProgressBar::_computeFillingRect() {
  int32 nrUnits;                /// number of units between value1 and value2
  int32 posFromV1, posFromV2;   /// number of units from value1 to pos, number of units from value2 to pos

  // need number of units between val1 and val2
  // need number of units from val1 to position
  // need number of units from val2 to position
  if(usage.value1<= usage.value2) {
    nrUnits= usage.value2- usage.value1;
    posFromV1= position- usage.value1;
    posFromV2= usage.value2- position;

  } else {
    nrUnits= usage.value1- usage.value2;
    posFromV1= usage.value1- position;
    posFromV2= position- usage.value2;
  }


  if(usage.orientation== RIGHT) {
    //    x   - posFromV1
    // pos.dx - nrUnits
    // x= (posFromV1* pos.dx)/ nrUnits;
    _filledRect.setD(pos.x0, pos.y0, (posFromV1* pos.dx)/ nrUnits, pos.dy);
    // dx COULD BE COMPUTED AS FLOAT, AND USE mlib::round FOR A MORE EXACT POSITION, CLOSER TO THE LEFT OR RIGHT PIXEL

  } else if(usage.orientation== LEFT) {
    // x      - posFromV2
    // pos.dx - nrUnits
    _filledRect.set(pos.xe- ((posFromV2* pos.dx)/ nrUnits), pos.y0, pos.xe, pos.ye);

  } else if(usage.orientation== UP) {
    _filledRect.set(pos.x0, pos.ye- (posFromV1* pos.dy)/ nrUnits, pos.xe, pos.ye);

  } else if(usage.orientation== DOWN) {
    _filledRect.setD(pos.x0, pos.y0, pos.dx, ((posFromV2* pos.dy)/ nrUnits));
  }
}





//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixProgressBar::_update(bool in_mIn,bool updateChildren) {
  recti r; getVDcoordsRecti(&r);
  // update it's children first
  if(updateChildren)
    if(_updateChildren((in_mIn? r.inside(in.m.x, in.m.y): false)))
      return true;


//  this func will update the time bar, nothing more, i thnk;


  // FUNCTION CORE


  // THIS IS AN INFO BAR, BASICALLY. I DON'T THINK THERE'S ANYTHING TO PUT IN HERE




  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(in_mIn, false);
}












// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW

#ifdef IX_USE_VULKAN
void ixProgressBar::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {

  // bars definetly need texturing
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  if(!_clip.exists()) return;
  if(!_inBounds(in_ix)) return;     // nothing to draw, the ix+gpu don't draw this window

  //int32 charDy= ixPrint::getCharDy(font.selFont);
  ixWSgenericStyle *s= (ixWSgenericStyle *)(in_style? in_style: parent->style);
  ixTexture *t= s->parent->getTexture(in_ix);

  in_ix->pr.style= &font;
  
  // this is the non-textured variant, done with purely rectangles and colors
  // ======================================================================== //

  in_ix->vk.CmdBindPipeline(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipeline);
  in_ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, in_ix->vki.draw.quad.sl->vk->pipelineLayout, 0, 1, &in_ix->vki.glb[in_ix->vki.fi]->set->set, 0, null);
  in_ix->vki.cmdScissor(in_cmd, &_clip);
  in_ix->vki.draw.quad.cmdTexture(in_cmd, t);
  
  /// border draw
  in_ix->vki.draw.quad.push.color= colorBRD;
  in_ix->vki.draw.quad.push.hollow= 2.0f;
  in_ix->vki.draw.quad.setPosDi(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, 0, pos.dx, pos.dy);
  in_ix->vki.draw.quad.flagTexture(false);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  /// filled draw
  in_ix->vki.draw.quad.push.color= color;
  in_ix->vki.draw.quad.push.hollow= -1.0f;
  in_ix->vki.draw.quad.setPosDi(_filledRect.x0+ hook.pos.x, _filledRect.y0+ hook.pos.y, 0, _filledRect.dx, _filledRect.dy);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  
  if(usage.txtShowValue) {
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // text decimals should happen in future. atm everything's an int, that's not great

    str8 txt;
    
    if(usage.barType== 0) {
      txt.fromInt(position);
      txt+= "%";
    } else if(usage.barType== 1) {
      txt.fromInt(position);

    } else if(usage.barType== 2) {
      // NOT TESTED
      int32 hours= position/ 3600;
      int32 minutes= position/ 60;
      int32 seconds= position% 60; // i think <<

      if(hours) {
        txt+= str8().fromInt(hours);
        txt+= ":";
      }
      if(minutes) {
        if(minutes< 10)
          txt+= '0';
        txt+= str8().fromInt(minutes);
        txt+= ":";
      }

      if(seconds< 10)
        txt+= '0';
      txt+= str8().fromInt(seconds); txt+= "s";
    }

    int32 len= ixPrint::getTextLen(txt, 0, font.selFont);
    int32 charDy= ixPrint::getCharDy(font.selFont);
    //in_ix->pr.setFont(&font);
    in_ix->pr.style= &font;

    if(usage.orientation== RIGHT || usage.orientation== LEFT) {
      if(usage.txtPosition== 0) {             // centered text
        in_ix->pr.txt2i(pos.x0+ ((pos.dx- len)/ 2)+ hook.pos.x, pos.y0+ ((pos.dy- charDy)/ 2)+ hook.pos.y, txt);


      } else if(usage.txtPosition== 1) {      // left/ down
        in_ix->pr.txt2i(pos.x0+ hook.pos.x, pos.y0+ ((pos.dy- charDy)/ 2)+ hook.pos.y, txt);



      } else if(usage.txtPosition== 2) {      // right/ up
        in_ix->pr.txt2i(pos.xe- len+ hook.pos.x, pos.y0+ ((pos.dy- charDy)/ 2)+ hook.pos.y, txt);

      } /// text position
    } /// text orientation
  }

}
#endif /// IX_USE_VULKAN



#ifdef IX_USE_OPENGL
void ixProgressBar::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {

  // bars definetly need texturing
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  /// get all required assets
  //ixWinSys::ixWSshader *sl= in_ix->wsys.getShader(in_ix);
  //if(!sl) return;           // at this point, there's an error    
  
  int32 charDy= ixPrint::getCharDy(font.selFont);
  //int32 prx, pry;
  

  in_ix->pr.style= &font;
  
  // this is the non-textured variant, done with purely rectangles and colors
  // ======================================================================== //

  in_ix->glo.draw.quad.useProgram();
  /// border draw
  in_ix->glo.draw.quad.setColorv(colorBRD);
  in_ix->glo.draw.quad.setHollow(2.0f);
  in_ix->glo.draw.quad.setCoordsDi(pos.x0+ hook.pos.x, pos.y0+ hook.pos.y, pos.dx, pos.dy);
  in_ix->glo.draw.quad.render();

  /// filled draw
  in_ix->glo.draw.quad.setColorv(color);
  in_ix->glo.draw.quad.setHollow(-1.0f);
  in_ix->glo.draw.quad.setCoordsDi(_filledRect.x0+ hook.pos.x, _filledRect.y0+ hook.pos.y, _filledRect.dx, _filledRect.dy);
  in_ix->glo.draw.quad.render();
}

#endif





