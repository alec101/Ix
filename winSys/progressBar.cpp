#include "ix/ix.h"
#include "ix/winSys/_privateDef.h"

/* TODO
 - bars need texturing
 - draw computes the text length non-stop; must optimize; maybe the text should be computed on update, and you don't update non-stop;

*/

ixProgressBar::ixProgressBar(): usage(this), ixBaseWindow(&is, &usage) {
  _type= ixeWinType::progressBar;
  position= 0.0f;
  usage.setPercentageBar(0.0f, 100.0f);
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

void ixProgressBar::Usage::setPercentageBar(float in_val1, float in_val2, ixOrientation in_o) {
  barType= 0;
  value1= in_val1;
  value2= in_val2;
  orientation= in_o;
  ((ixProgressBar *)_win)->resetPosition();
}


void ixProgressBar::Usage::setCustomAmountBar(float in_val1, float in_val2, ixOrientation in_o) {
  barType= 1;
  value1= in_val1;
  value2= in_val2;
  orientation= in_o;
  ((ixProgressBar *)_win)->resetPosition();
}


void ixProgressBar::Usage::setTimeBar(float in_val1, float in_val2, ixOrientation in_o) {
  barType= 2;
  value1= in_val1;
  value2= in_val2;
  orientation= in_o;
  ((ixProgressBar *)_win)->resetPosition();
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


void ixProgressBar::move(float x0, float y0) {
  ixBaseWindow::move(x0, y0);
  _computeFillingRect();
}


void ixProgressBar::moveDelta(float dx, float dy) {
  ixBaseWindow::moveDelta(dx, dy);
  _computeFillingRect();
}


void ixProgressBar::resize(float dx, float dy) {
  ixBaseWindow::resize(dx, dy);
  _computeFillingRect();
}


void ixProgressBar::resizeDelta(float dx, float dy) {
  ixBaseWindow::resizeDelta(dx, dy);
  _computeFillingRect();
}


void ixProgressBar::setPos(float x0, float y0, float dx, float dy) {
  ixBaseWindow::setPos(x0, y0, dx, dy);
  _computeFillingRect();
}


void ixProgressBar::setPosition(float in_pos) {
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
  float nrUnits;                /// number of units between value1 and value2
  float posFromV1, posFromV2;   /// number of units from value1 to pos, number of units from value2 to pos

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
    //_filledRect.set(pos.xe- ((posFromV2* pos.dx)/ nrUnits), pos.y0, pos.xe, pos.ye);
    _filledRect.set(pos.x0+ pos.dx- ((posFromV2* pos.dx)/ nrUnits), pos.y0, pos.x0+ pos.dx, pos.y0+ pos.dy);      // NOT TESTED <<<<<<<<<<

  } else if(usage.orientation== UP) {
    //_filledRect.set(pos.x0, pos.ye- (posFromV1* pos.dy)/ nrUnits, pos.xe, pos.ye);
    _filledRect.set(pos.x0, pos.y0+ pos.dy- ((posFromV1* pos.dy)/ nrUnits), pos.x0+ pos.dx, pos.y0+ pos.dy);      // NOT TESTED <<<<<<<<<<<<

  } else if(usage.orientation== DOWN) {
    _filledRect.setD(pos.x0, pos.y0, pos.dx, ((posFromV2* pos.dy)/ nrUnits));
  }
}





//  ##    ##    ######      ######        ####      ########    ########
//  ##    ##    ##    ##    ##    ##    ##    ##       ##       ##
//  ##    ##    ######      ##    ##    ##    ##       ##       ######      func
//  ##    ##    ##          ##    ##    ########       ##       ##
//    ####      ##          ######      ##    ##       ##       ########

bool ixProgressBar::_update(bool updateChildren) {
  if(!is.visible) return false;

  //rectf posvd;
  //bool insideThis;

  // update it's children first
  if(updateChildren)
    if(_updateChildren())
      return true;

  //getPosVD(&posvd);
  //insideThis= posvd.inside(mx, my);

  //  this func will update the time bar, nothing more, i thnk;

  // FUNCTION CORE

  // THIS IS AN INFO BAR, BASICALLY. I DON'T THINK THERE'S ANYTHING TO PUT IN HERE

  // base window update, at this point - no childrens tho, those were updated first
  return ixBaseWindow::_update(false);
}












// DDDDDD      RRRRRR        AAAA      WW      WW
// DD    DD    RR    RR    AA    AA    WW      WW
// DD    DD    RRRRRR      AA    AA    WW  WW  WW     func
// DD    DD    RR    RR    AAAAAAAA    WW  WW  WW
// DDDDDD      RR    RR    AA    AA      WW  WW

#ifdef IX_USE_VULKAN
void ixProgressBar::_vkDraw(VkCommandBuffer in_cmd, Ix *in_ix, ixWSsubStyleBase *in_style) {
  char textBuf[1024], textBuf2[1024];
  str8 txt, txt2;

  textBuf[0]= 0, textBuf2[0]= 0;
  txt.wrap(textBuf, 1024);
  txt2.wrap(textBuf2, 1024);

  // bars definetly need texturing
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  //>>> this func computes text length non-stop, it's a hit, still, on speed. must optimize somehow. <<<


  if(!_clip.exists()) return;
  if(!_inBounds(in_ix)) return;     // nothing to draw, the ix+gpu don't draw this window
  rectf posvd; getPosVD(&posvd);

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
  in_ix->vki.draw.quad.setPosR(posvd);
  in_ix->vki.draw.quad.flagTexture(false);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  /// filled draw
  in_ix->vki.draw.quad.push.color= color;
  in_ix->vki.draw.quad.push.hollow= -1.0f;
  in_ix->vki.draw.quad.setPosD(hook.pos.x+ _filledRect.x0, hook.pos.y+ _filledRect.y0, 0, _filledRect.dx, _filledRect.dy);
  in_ix->vki.draw.quad.cmdPushAll(in_cmd);
  in_ix->vki.draw.quad.cmdDraw(in_cmd);

  
  if(usage.txtShowValue) {
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // text decimals should happen in future. atm everything's an int, that's not great
    
    
    if(usage.barType== 0) {
      txt.fromFloat(position, 1);
      //txt.fromInt(position);
      txt+= "%";
    } else if(usage.barType== 1) {
      txt.fromFloat(position, 1);

    } else if(usage.barType== 2) {
      // NOT TESTED
      int32 hours=   (int32)position/ 3600;
      int32 minutes= (int32)position/ 60;
      int32 seconds= (int32)position% 60; // i think <<

      if(hours) {
        txt+= txt2.fromInt(hours);
        txt+= ":";
      }
      if(minutes) {
        if(minutes< 10)
          txt+= '0';
        txt+= txt2.fromInt(minutes);
        txt+= ":";
      }

      if(seconds< 10)
        txt+= '0';
      txt+= txt2.fromInt(seconds); txt+= "s";
    }

    float len= font.getTextLen(txt);
    float charDy= font.getCharDy();
    in_ix->pr.style= &font;

    if(usage.orientation== RIGHT || usage.orientation== LEFT) {
      if(usage.txtPosition== 0) {             // centered text
        in_ix->pr.txt2f(posvd.x0+ ((posvd.dx- len)/ 2.0f), posvd.y0+ ((posvd.dy- charDy)/ 2.0f), txt);


      } else if(usage.txtPosition== 1) {      // left/ down
        in_ix->pr.txt2f(posvd.x0, posvd.y0+ ((posvd.dy- charDy)/ 2.0f), txt);



      } else if(usage.txtPosition== 2) {      // right/ up
        in_ix->pr.txt2f(posvd.xe- len, posvd.y0+ ((posvd.dy- charDy)/ 2.0f), txt);

      } /// text position
    } /// text orientation
  }

}
#endif /// IX_USE_VULKAN



#ifdef IX_USE_OPENGL
void ixProgressBar::_glDraw(Ix *in_ix, ixWSsubStyleBase *in_style) {
  error.makeme();
}

#endif





