#include "ix/ix.h"



// maybe just altering the glyph abit, the outline could happen way easier...
// https://blog.mapbox.com/drawing-text-with-signed-distance-fields-in-mapbox-gl-b0933af6f817
// for certain tex offset might not be any use (the func in the shader), you can just use texture func, i think, will see... it could be faster

//so split the intensity in two. half for the outline, half for the actual glypth;
// you can select the color of overlay, this way, and color of the font
// probly 25% larger or 50% larger per char, and that's that;
// but again, even with this method, chars will shadow over prev chars -.-, that's the main issue here


// VULKAN conversion:
/*
 -vertex shader can compute everything about the character, write in a buffer
 - ^^^ ? or as many computations as possible? maybe? in any case, if it's a way to reduce things alot, it would be great.

 -fragment would have only clipping, and just execute the draw
 -
;
  print must have such buffer sys also
  maybe an optimized print class, and let the old one be, for other stuff
  and the optimized one would have all it's pages loaded, maybe tex array? or array of textures?
  imagearray would probly be best... you have an order of chars to print, all pages with coresponding image layer
  you'd have the advanced coord, based on orientation


  - the buffer could house only x0,y0, dx, dy, and compute everything from that, it's size would be cut in half
    or x0,y0,xe,ye, depending what has the least computations
*/

/*
thinking.

methods of printing that have to be implemented. You choose the best method for your scenario:

m1
RENDER TO TEXTURE has to happen, but it should not be the only way to render. it should be a feature.
the texture you handle yourself, you just direct print to re-render to it if needed.

m2
command buffer render, in a way, dono what name it would have. You create the buffer, next frame you just use it, not destroy/recreate it;
this method must be tested for speed. It could ROCK in speed, in theory, if the GPU already has the commands to do, and just repeats them. it depends on the driver.
comand buffer reuse? dono what a name would be.

m3

all font pages in mem, you send what chars to print.
problem: you cannot use a UBO, or 2 or 10, because cpu cannot wait on the gpu , to reform the buffer. Must think in vulkan.
so you send stuff in what? push constants? the max chars you can print at a time?
another way for m3, would be with compute shaders. It is an unknown how you send data to them, that is MY problem... that has to be rectivied soon


OUTLINE:
 - the quad can be made bigger from the shader, if needed
 - a condition to not go over the glyph's location in texture could be added, due you do know it
 - the only problem with outline from shader is the char-by-char draw would outline over the prev char

 SHADOW:
 - draw double the amount of vertices, first half is hadow, second half is normal
;





>>> readting alot: multiple draw calls do not mean slow speed, so m2 with command buffers that get updated only when needed could rock, i think <<<
;
*/

// >>> so char by char is not that bad, especially when text changes non-stop, i think; the buffer method is hard as shit, with possible garbage and everything tied to it; <<<




#ifdef IX_USE_VULKAN

namespace _ixpr {


// mk1: char by char method, it must exist i think, texts that update on every frame have to use this.
class vkM1: public ixPrintShader {
public:
  
  struct PConsts {
    vec4 pos;               // changed frequently
    vec4 color1;            // main color
    vec4 color2;            // outline / shadow
    uint32 flags;           // byte meaning: 0= perspective camera, 1= ortho camera
    int outline;            // [0= disable] [1-5= outline size] character outline
    float scale;
  } push;

  ixvkDescPool *poolDesc;

  void txt(const void *text, int type, int start, int end, int startBytes, int endBytes);

  void printChar(uint32 in_unicode);
  vkM1(Ix *in_ix);
  //void vkInit(Ix *in_ix, ixPrintShader **out_s);

protected:
  VkDescriptorSet _currentSet1;   // currently bound sets
  

  void _prePrintInit();
  void _afterPrint();
  void _create(_ixFPage *);
  void _delete(_ixFPage *);
  friend class ixPrint;
  friend class _ixFPage;
};







_ixpr::vkM1::vkM1(Ix *in_ix): ixPrintShader(in_ix) {
  _currentSet1= VK_NULL_HANDLE;
  // _currentSet2 is _prevTex;

  //vk= _ix->vk.objects.addShader();
  vk->loadModuleVert(Ix::Config::shaderDIR()+ "vkPrintM1.vert.spv");
  vk->loadModuleFrag(Ix::Config::shaderDIR()+ "vkPrintM1.frag.spv");

  /// set 0, binding 0, global buffer
  vk->addDescriptorSetFromExisting(_ix->vki.glb[_ix->vki.fi]->layout);
  /// set 1, binding 0 & 1, buffer+ texture for the font
  vk->addDescriptorSet();
    vk->addDescriptor(1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1, VK_SHADER_STAGE_VERTEX_BIT);    // page buffer
    vk->addDescriptor(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);  // page texture
  vk->descSet[1]->build();

  vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
  //vk->setFrontFace(VK_FRONT_FACE_CLOCKWISE);
  vk->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
  vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
  vk->addPushConsts(sizeof(PConsts), 0, VK_SHADER_STAGE_ALL);
  vk->setRenderPass(*_ix->vki.render.handle);
  //vk->setSubpass(0);
  vk->setDynamicViewports(1);

  vk->addColorBlendAttachement(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                                      VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ZERO,                VK_BLEND_OP_ADD,
                                      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

  if(!vk->build()) error.window(_ix->vk.errorText, __FUNCTION__);

  /// pipeline starting settings
  _ix->vk.QueueWaitIdle(*_ix->vki.qTool);
  _ix->vki.cmdTool->pool->reset();
  _ix->vki.cmdTool->startRecording();
    _ix->vk.CmdBindPipeline(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipeline);
    _ix->vk.CmdSetViewport(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.viewport);
    _ix->vk.CmdSetScissor(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.scissor);
  _ix->vki.cmdTool->endRecording();
  _ix->vki.cmdTool->submit(*_ix->vki.qTool);
  _ix->vk.QueueWaitIdle(*_ix->vki.qTool);

  poolDesc= new ixvkDescPool(_ix);
  poolDesc->configure(vk->descSet[1], _ix->cfg.vk.printDynamicSetSegmentSize);
  poolDesc->build();
}


void _ixpr::vkM1::_create(_ixFPage *in_page) {

  /// buffer composition - 20484 bytes
  struct BufferData {
    vec4 ch[1024];
    vec2 texSize;
    // OLD, DELETE AFTER TESTING
    //vec4 tex[1024];
    //int32 chDx[1024];
    //int32 chDy;
  };

  BufferData *buffer= new BufferData;

  /// nr of chars in the page
  uint32 nrChars= pagesList[in_page->id].max- pagesList[in_page->id].min+ 1;

  // populate buffer data
  

  for(uint32 a= 0; a< nrChars; a++) {
    buffer->ch[a].x= (float)in_page->ch[a].x0;
    buffer->ch[a].y= (float)in_page->ch[a].y0;
    buffer->ch[a].z= (float)in_page->ch[a].dx;
    buffer->ch[a].w= (float)in_page->ch[a].dy;
    // TO DEL
    //buffer->chDx[a]= in_page->ch[a].dx;
    //buffer->tex[a].x= in_page->ch[a].texX0;
    //buffer->tex[a].w= in_page->ch[a].texY0;   // HAD TO INVERSE IT
    //buffer->tex[a].z= in_page->ch[a].texXe;
    //buffer->tex[a].y= in_page->ch[a].texYe;   // <<<<<<< HAD TO INVERSE IT, UNTIL THE FONT TEXTURE IS BUILT FOR VULKAN
  }
  buffer->texSize.x= (float)in_page->texDx;
  buffer->texSize.y= (float)in_page->texDy;
  //buffer->chDy= in_page->size->size;    to del<,<<

  /// create vulkan buffer
  in_page->vkData= new ixvkBuffer(_ix->vki.clusterIxDevice);
  in_page->vkData->handle->cfgUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT| VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  in_page->vkData->handle->cfgSize(sizeof(BufferData));
  
  in_page->vkData->build();

  // upload data to vulkan buffer
  in_page->vkData->upload(buffer, 0, sizeof(BufferData));

  poolDesc->addSet(&in_page->vkSet);
  in_page->vkSet->bind(0, in_page->vkData);
  in_page->vkSet->bind(1, in_page->tex);
  in_page->vkSet->update();


  if(buffer) delete buffer;
}


void _ixpr::vkM1::_delete(_ixFPage *out_page) {
  if(_ix) {
    
    if(out_page->vkSet) {
      poolDesc->delSet(out_page->vkSet);
      out_page->vkSet= null;
    }
  
    if(out_page->vkData) {
      
      // OFC, YOU CAN JUST PUT A _ix->vk.DeviceWaitIdle(), but destroying any font during a program would insert a pause...
      error.detail("these buffers get destroyed before any work with them stops. must think more on it", __FUNCTION__, __LINE__);

      error.makeme(__FUNCTION__);

      _ix->vki.clusterIxDevice->delResource(out_page->vkData, true);
      out_page->vkData= null;
    }
  }
}




void _ixpr::vkM1::_prePrintInit() {
  _prevTex= nullptr;
  VkCommandBuffer cmd= _ix->vki.ortho.cmd[_ix->vki.fi]->buffer;
  
  _ix->vk.CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 0, 1, &_ix->vki.glb[_ix->vki.fi]->set->set, 0, null);
  //_ix->vk.CmdPushConstants(cmd, vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);
  push.scale= _print->style->scale;
}

void _ixpr::vkM1::printChar(uint32 in_unicode) {
  _ixFSize *fs= (_ixFSize *)_print->style->selFont;
  _ixFPage *fp= (_ixFPage *)fs->pages.first;
  VkCommandBuffer cmd= _ix->vki.ortho.cmd[_ix->vki.fi]->buffer;

  /* IT WOULD HAVE TO BE ON EVERY FUNC. this is too much i think
  if(c>= 9644 && c<= 9835) {
    if     (c== 9786) c= 1;
    else if(c== 9787) c= 2;
    else if(c== 9829) c= 3;
    else if(c== 9830) c= 4;
    else if(c== 9827) c= 5;
    else if(c== 9824) c= 6;
    else if(c== 9688) c= 8;
    else if(c== 9675) c= 9;
    else if(c== 9689) c= 10;
    else if(c== 9794) c= 11;
    else if(c== 9792) c= 12;
    else if(c== 9834) c= 13;
    else if(c== 9835) c= 14;
    else if(c== 9788) c= 15;
    else if(c== 9658) c= 16;
    else if(c== 9668) c= 17;
    else if(c== 9644) c= 22;
    else if(c== 9650) c= 30;
    else if(c== 9660) c= 31;
  }
  else if(c>= 8226 && c<= 8735) {
    if     (c== 8226) c= 7;
    else if(c== 8597) c= 18;
    else if(c== 8252) c= 19;
    else if(c== 8593) c= 24;
    else if(c== 8595) c= 25;
    else if(c== 8594) c= 26;
    else if(c== 8592) c= 27;
    else if(c== 8735) c= 28;
    else if(c== 8596) c= 29;
  }
  else if(c== 182) c= 20;
  else if(c== 167) c= 21;
  */
  //1= 9786;
  //2= 9787;
  //3= 9829;
  //4= 9830;
  //5= 9827;
  //6= 9824;
  //7= 8226;
  //8= 9688;
  //9= 9675;
  //10= 9689;
  //11= 9794;
  //12= 9792;
  //13= 9834;
  //14= 9835;
  //15= 9788;
  //16= 9658;
  //17= 9668;
  //18= 8597;
  //19= 8252;
  //20= 182;
  //21= 167;
  //22= 9644;
  //23= 8616;
  //24= 8593;
  //25= 8595;
  //26= 8594;
  //27= 8592;
  //28= 8735;
  //29= 8596;
  //30= 9650;
  //31= 9660;


  /// search for the page the unicode char belongs to
  for(; fp; fp= (_ixFPage *)fp->next)
    if(pagesList[fp->id].min<= in_unicode && in_unicode<= pagesList[fp->id].max)
      break;

  /// if no fp is found, there is no page that has this character
  if(!fp)
    fp= (_ixFPage *)fs->pages.first,
    in_unicode= '?';                                 /// this should be a character that is on all fonts for sure
    
  _ixFChar *ch= &fp->ch[in_unicode- pagesList[fp->id].min];    /// shortcut
  
  bool comb= Str::isComb(in_unicode);

  /// bind the set
  if(_prevTex!= &fp->vkSet->set) {
    _prevTex= &fp->vkSet->set;
    _ix->vk.CmdBindDescriptorSets(cmd , VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipelineLayout, 1, 1, (VkDescriptorSet *)_prevTex, 0, null);
  }

  /// advance cursor position based on text orientation
  if(_print->style->orientation== IX_TXT_RIGHT) {       // >  to right orientation
    if(in_unicode== ' ') {
      _print->pos.x+= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize)* _print->style->scale;
      return;
    }
    push.pos.set(_print->pos.x+ ch->start* _print->style->scale, _print->pos.y, _print->pos.z, 1.0f);
    if(!comb)_print->pos.x+= ch->end* _print->style->scale;

  } else if(_print->style->orientation== IX_TXT_UP) {   // ^  to up orientation
    if(in_unicode== ' ') {
      _print->pos.y+= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize)* _print->style->scale;
      return;
    }

    push.pos.set(_print->pos.x+ ch->start, _print->pos.y, _print->pos.z, 1.0f);
    if(!comb)_print->pos.y+= ch->dy;

  } else if(_print->style->orientation== IX_TXT_DOWN) { // v  to down orientation
    if(in_unicode== ' ') {
      _print->pos.y-= (_print->style->spaceSize== 0.0f? ch->dy: _print->style->spaceSize)* _print->style->scale;
      return;
    }
    if(!comb)_print->pos.y-= ch->dy;
    push.pos.set(_print->pos.x+ ch->start, _print->pos.y, _print->pos.z, 1.0f);

  } else if(_print->style->orientation== IX_TXT_LEFT) { // <  to left orientation
    if(in_unicode== ' ') {
      _print->pos.x-= (_print->style->spaceSize== 0.0f? ch->end: _print->style->spaceSize)* _print->style->scale;
      return;
    }
    if(!comb)_print->pos.x-= ch->end;
    push.pos.set(_print->pos.x+ ch->start, _print->pos.y, _print->pos.z, 1.0f);
  }

  uint32 id= (in_unicode- pagesList[fp->id].min);
  id*= 4;   /// 4 vertex per char

  _ix->vk.CmdPushConstants(cmd, vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, pos), sizeof(push.pos), &push.pos);
  _ix->vk.CmdDraw(cmd, 4, 1, id, 0);
}

void _ixpr::vkM1::_afterPrint() {
  _prevTex= null;
}


void _ixpr::vkM1::txt(const void *s, int type, int start, int end, int startBytes, int endBytes) {
  if(!s) return;
  if(_print== null) return;
  if(!_print->style->selFont) return;                    /// if no font is selected, just return

  VkCommandBuffer cmd= _ix->vki.ortho.cmd[_ix->vki.fi]->buffer;

  _prePrintInit();

  // shadow draw - print everything with shadow color - SLOW
  if(!_print->justDraw) {
    if(_print->style->drawMode& 0x02) {
      vec2 oldPos(_print->pos);
      vec4 oldColor(_print->style->color1);

      _print->justDraw= true;
      _print->style->color1= _print->style->color2;      
      _print->pos+= _print->style->shadowPos;
      txt(s, type, start, end, startBytes, endBytes);

      _print->pos= oldPos;
      _print->style->color1= oldColor;

      _print->justDraw= false;  /// just draw must always be false, it's used to force a draw whitout checks for effects
    }
  }

  /// push consts

  push.color1= _print->style->color1;
  push.color2= _print->style->color2;
  push.flags&= ~(0x0001);
  push.flags|= 0x0002;

  if(_print->style->drawMode& 0x01)
    push.outline= _print->style->outlineSize,
    push.flags|= 0x0004;
  else
    push.flags&= ~0x0004;

  if(_print->style->drawMode& 0x04) push.flags|= 0x0008;
  else                                push.flags&= ~0x0008;

  _ix->vk.CmdPushConstants(cmd, vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

  uint32 c;       // will hold the current unicode to be printed

  // UTF-8 print
  if(type== 8) {
    const uint8 *t= (const uint8*)s+ startBytes;
    endBytes-= startBytes;

    if(start)                         /// skip [start] characters from the string
      while(*t && start)
        if(((*t++)& 0xc0)!= 0x80)     /// 0xc0= 11000000, first two bits  0x80= 10000000, test for 10xxxxxx the last 6 bits wont matter, as they are not tested so test to 10000000
          start--, end--;
    
    // for each character
    for(; *t && end && (endBytes> 0); end--) {
      if(*t < 128)                    /// character uses 1 byte (ascii 0-127)
        c= *t++, endBytes--;
      else {
        if((*t& 0xe0) == 0xc0)        /// character uses 2 bytes
          c= *t++ & 0x1f, endBytes-= 2,
          c<<= 6, c+= *t++ & 0x3f;
        else if((*t& 0xf0) == 0xe0)   /// character uses 3 bytes
          c= *t++ & 0x0f, endBytes-= 3,
          c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f;
        else if((*t& 0xf8) == 0xf0)   /// character uses 4 bytes
          c= *t++ & 0x07, endBytes-= 4,
          c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f;
        // the last 2 bytes are not used, but printed if in the future unicode will expand
        else if((*t& 0xfc) == 0xf8)   /// character uses 5 bytes
          c= *t++ & 0x03, endBytes-= 5,
          c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f;
        else if((*t& 0xfe) == 0xfc)   /// character uses 6 bytes
          c= *t++ & 0x01, endBytes-= 6,
          c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f, c<<= 6, c+= *t++ & 0x3f;
        else error.simple("invalid utf8 string");
      } /// unpack the unicode value

      printChar(c);
    }	/// for each character


  // UTF-16 print
  } else if(type== 16) {
    const uint16 *t16= (const uint16 *)((int8 *)s+ startBytes);
    endBytes-= startBytes;
    if(start)          /// skip [start] characters from the string
      while(*t16 && start)
        t16+= (Str::isHighSurrogate(*t16)? 2: 1), start--, end--;
    
    /// for each character
    for(; *t16 && end && (endBytes> 0); end--) {
      if(Str::isHighSurrogate(*t16))
        c= (*t16<< 10)+ *(t16+ 1)+ Str::UTF16_SURROGATE_OFFSET,
        t16+= 2, endBytes-= 4;
      else 
        c= *t16++, endBytes-= 2;

      printChar(c);
    }	/// for each character


  // UTF-32 print
  } else if(type== 32) {
    const uint32 *t32= (uint32 *)((int8 *)s+ startBytes);
    endBytes-= startBytes;

    if(start)         /// skip [start] characters from the string
      while(*t32 && start)
        t32++, start--, end--;
    
    /// for each character
    for(; *t32 && end && (endBytes> 0); end--, endBytes-= 4) {
      c= *t32++;
      printChar(c);
    }	/// for each character
  }

  if(!_print->justDraw)
    _afterPrint();
}











// vulkan

// ######  ##    ##  ######  ########
//   ##    ####  ##    ##       ##
//   ##    ## ## ##    ##       ##
//   ##    ##  ####    ##       ##
// ######  ##    ##  ######     ##

void vkInit(Ix *in_ix, ixPrintShader **out_s) {

  // there have to be multiple shaders made i think



  if(in_ix->renVulkan()) {
    if(*out_s== null) {
      *out_s= new _ixpr::vkM1(in_ix);
    }
  }
}





}; // namespace _ixpr

#endif // IX_USE_VULKAN


