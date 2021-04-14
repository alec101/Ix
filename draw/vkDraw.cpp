#include "ix/ix.h"



#ifdef IX_USE_VULKAN

/*

LINE:
https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
distance(p1, p2, (x0, y0))= |(y2- y1)x0- (x2- x1)y0 + x2y1- y2x1| / sqrt((y2- y1)^2 + (x2- x1)^2)
*/


/*
the draw sys could be optimized, and it must...
        would a draw sys like that old draw i knew as a kid work? 
        a uber shader that would accept commands...
        or probly the best would be a buffer for all types of primitives... the quad with the buffer, etc
*/









// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkDRAW class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀



vkDraw::vkDraw(Ix *in_ix): _ix(in_ix), point(in_ix), line(in_ix), triangle(in_ix), quad(in_ix), circle(in_ix) {
}


vkDraw::~vkDraw() {
}


void vkDraw::delData() {
}


void vkDraw::init() {
  point._init();
  line._init();
  triangle._init();
  quad._init();
  circle._init();


  // pipelines starting settings
  _ix->vk.QueueWaitIdle(*_ix->vki.qTool);
  _ix->vki.cmdTool->pool->reset();
  _ix->vki.cmdTool->startRecording();

    /// point
    _ix->vk.CmdBindPipeline(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, point.sl->vk->pipeline);
    _ix->vk.CmdSetViewport(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.viewport);
    _ix->vk.CmdSetScissor(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.scissor);
    
    /// line
    _ix->vk.CmdBindPipeline(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, line.sl->vk->pipeline);
    _ix->vk.CmdSetViewport(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.viewport);
    _ix->vk.CmdSetScissor(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.scissor);

    /// triangle
    _ix->vk.CmdBindPipeline(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.sl->vk->pipeline);
//    _ix->vk.CmdBindDescriptorSets(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.sl->vk->pipelineLayout, 1, 1, &_ix->vki.noTexture->vkData.set->set, 0, null);
    _ix->vk.CmdSetViewport(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.viewport);
    _ix->vk.CmdSetScissor(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.scissor);

    /// quad
    _ix->vk.CmdBindPipeline(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.sl->vk->pipeline);
//    _ix->vk.CmdBindDescriptorSets(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.sl->vk->pipelineLayout, 1, 1, &_ix->vki.noTexture->vkData.set->set, 0, null);
    _ix->vk.CmdSetViewport(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.viewport);
    _ix->vk.CmdSetScissor(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.scissor);
    
    /// circle
    _ix->vk.CmdBindPipeline(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, circle.sl->vk->pipeline);
//    _ix->vk.CmdBindDescriptorSets(*_ix->vki.cmdTool, VK_PIPELINE_BIND_POINT_GRAPHICS, circle.sl->vk->pipelineLayout, 1, 1, &_ix->vki.noTexture->vkData.set->set, 0, null);
    _ix->vk.CmdSetViewport(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.viewport);
    _ix->vk.CmdSetScissor(*_ix->vki.cmdTool, 0, 1, &_ix->vki.render.scissor);


  _ix->vki.cmdTool->endRecording();
  _ix->vki.cmdTool->submit(*_ix->vki.qTool);
  _ix->vk.QueueWaitIdle(*_ix->vki.qTool);

}

/*
void vkDraw::bindGlbBuffer(VkCommandBuffer in_cmd) {
  VkDescriptorSet s= _ix->vki.glb[_ix->vki.fi]->set->set;

  _ix->vk.CmdBindPipeline(in_cmd,       VK_PIPELINE_BIND_POINT_GRAPHICS, point.sl->vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, point.sl->vk->pipelineLayout,    0, 1, &s, 0, null);
  _ix->vk.CmdBindPipeline(in_cmd,       VK_PIPELINE_BIND_POINT_GRAPHICS, line.sl->vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, line.sl->vk->pipelineLayout,     0, 1, &s, 0, null);
  _ix->vk.CmdBindPipeline(in_cmd,       VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.sl->vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle.sl->vk->pipelineLayout, 0, 1, &s, 0, null);
  _ix->vk.CmdBindPipeline(in_cmd,       VK_PIPELINE_BIND_POINT_GRAPHICS, quad.sl->vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, quad.sl->vk->pipelineLayout,     0, 1, &s, 0, null);
  _ix->vk.CmdBindPipeline(in_cmd,       VK_PIPELINE_BIND_POINT_GRAPHICS, circle.sl->vk->pipeline);
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, circle.sl->vk->pipelineLayout,   0, 1, &s, 0, null);
}
*/





///=========-------------------------------------///
// IX POINT ===================================== //
///=========-------------------------------------///


vkPoint::vkPoint(Ix *in_ix): _ix(in_ix), parent(&in_ix->vki.draw) {
}


void vkPoint::_init() {
  sl= new ixShader(_ix);
  sl->vk->loadModuleVert(Ix::Config::shaderDIR()+ "ixDraw/vkPoint.vert.spv");
  sl->vk->loadModuleFrag(Ix::Config::shaderDIR()+ "ixDraw/vkPoint.frag.spv");

  sl->vk->addDescriptorSetFromExisting(_ix->vki.glb[_ix->vki.fi]->layout);  /// set 0, binding 0 - global buffer
  //sl->vk->addDescriptorSetFromExisting(_ix->texSys.vkData.staticLayout);    /// set 1, binding 0 - texture
  
  sl->vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_FALSE);
  sl->vk->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
  sl->vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
  //sl->vk->setCullModeFlags(VK_CULL_MODE_NONE);

  sl->vk->addPushConsts(sizeof(PConsts), 0, VK_SHADER_STAGE_ALL);
  sl->vk->setRenderPass(*_ix->vki.render.handle);
  //vk->setSubpass(0);
  sl->vk->setDynamicViewports(1);
  

  sl->vk->addColorBlendAttachement(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                                         VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ZERO,                VK_BLEND_OP_ADD,
                                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

  if(!sl->vk->build()) error.window(_ix->vk.errorText, __FUNCTION__);
}


///========--------------------------------------///
// IX LINE ====================================== //
///========--------------------------------------///


vkLine::vkLine(Ix *in_ix): _ix(in_ix), parent(&in_ix->vki.draw) {
}


void vkLine::_init() {
  sl= new ixShader(_ix);
  sl->vk->loadModuleVert(Ix::Config::shaderDIR()+ "ixDraw/vkLine.vert.spv");
  sl->vk->loadModuleFrag(Ix::Config::shaderDIR()+ "ixDraw/vkLine.frag.spv");

  sl->vk->addDescriptorSetFromExisting(_ix->vki.glb[_ix->vki.fi]->layout);  /// set 0, binding 0 - global buffer
  
  sl->vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_FALSE);
  sl->vk->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);

  sl->vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
  sl->vk->setCullModeFlags(VK_CULL_MODE_NONE);

  sl->vk->addPushConsts(sizeof(PConsts), 0, VK_SHADER_STAGE_ALL);
  sl->vk->setRenderPass(*_ix->vki.render.handle);
  //vk->setSubpass(0);
  sl->vk->setDynamicViewports(1);
  sl->vk->addDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);

  sl->vk->addColorBlendAttachement(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                                         VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ZERO,                VK_BLEND_OP_ADD,
                                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

  if(!sl->vk->build()) error.window(_ix->vk.errorText, __FUNCTION__);
}




///============----------------------------------///
// IX TRIANGLE ================================== //
///============----------------------------------///



vkTriangle::vkTriangle(Ix *in_ix): _ix(in_ix), parent(&in_ix->vki.draw) {
}


void vkTriangle::_init() {
  sl= new ixShader(_ix);
  sl->vk->loadModuleVert(Ix::Config::shaderDIR()+ "ixDraw/vkTriangle.vert.spv");
  sl->vk->loadModuleFrag(Ix::Config::shaderDIR()+ "ixDraw/vkTriangle.frag.spv");

  sl->vk->addDescriptorSetFromExisting(_ix->vki.glb[_ix->vki.fi]->layout);  /// set 0, binding 0 - global buffer
  sl->vk->addDescriptorSetFromExisting(_ix->res.tex.vkd.staticLayout);    /// set 1, binding 0 - texture

  sl->vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
  sl->vk->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
  //sl->vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
  sl->vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
  sl->vk->addPushConsts(sizeof(PConsts), 0, VK_SHADER_STAGE_ALL);
  sl->vk->setRenderPass(*_ix->vki.render.handle);
  //vk->setSubpass(0);
  sl->vk->setDynamicViewports(1);
  
  sl->vk->addColorBlendAttachement(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                                         VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ZERO,                VK_BLEND_OP_ADD,
                                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

  if(!sl->vk->build()) error.window(_ix->vk.errorText, __FUNCTION__);
}





///========--------------------------------------///
// IX QUAD ====================================== //
///========--------------------------------------///


vkQuad::vkQuad(Ix *in_ix): _ix(in_ix), parent(&in_ix->vki.draw) {
  
}


void vkQuad::_init() {
  sl= new ixShader(_ix);
  sl->vk->loadModuleVert(Ix::Config::shaderDIR()+ "ixDraw/vkQuad.vert.spv");
  sl->vk->loadModuleFrag(Ix::Config::shaderDIR()+ "ixDraw/vkQuad.frag.spv");

  sl->vk->addDescriptorSetFromExisting(_ix->vki.glb[_ix->vki.fi]->layout);  /// set 0, binding 0 - global buffer
  sl->vk->addDescriptorSetFromExisting(_ix->res.tex.vkd.staticLayout);    /// set 1, binding 0 - texture

  sl->vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
  //sl->vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
  sl->vk->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
  sl->vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
  sl->vk->setCullModeFlags(VK_CULL_MODE_NONE);

  sl->vk->addPushConsts(sizeof(PConsts), 0, VK_SHADER_STAGE_ALL);
  sl->vk->setRenderPass(*_ix->vki.render.handle);
  //vk->setSubpass(0);
  sl->vk->setDynamicViewports(1);
  
  sl->vk->addColorBlendAttachement(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                                         VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ZERO,                VK_BLEND_OP_ADD,
                                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

  if(!sl->vk->build()) error.window(_ix->vk.errorText, __FUNCTION__);
}









// ▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄
// █ vkCIRCLE class █████████████████████████████████████████████████████████████████████████
// ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀


vkCircle::vkCircle(Ix *in_ix): _ix(in_ix), parent(&in_ix->vki.draw) {
}



void vkCircle::_init() {
  sl= new ixShader(_ix);
  sl->vk->loadModuleVert(Ix::Config::shaderDIR()+ "ixDraw/vkCircle.vert.spv");
  sl->vk->loadModuleFrag(Ix::Config::shaderDIR()+ "ixDraw/vkCircle.frag.spv");

  sl->vk->addDescriptorSetFromExisting(_ix->vki.glb[_ix->vki.fi]->layout);  /// set 0, binding 0 - global buffer
  sl->vk->addDescriptorSetFromExisting(_ix->res.tex.vkd.staticLayout);    /// set 1, binding 0 - texture

  sl->vk->setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_FALSE);
  sl->vk->setFrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE);
  sl->vk->setCullModeFlags(VK_CULL_MODE_BACK_BIT);
  sl->vk->addPushConsts(sizeof(PConsts), 0, VK_SHADER_STAGE_ALL);
  sl->vk->setRenderPass(*_ix->vki.render.handle);
  //vk->setSubpass(0);
  sl->vk->setDynamicViewports(1);
  
  sl->vk->addColorBlendAttachement(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                                         VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ZERO,                VK_BLEND_OP_ADD,
                                         VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

  if(!sl->vk->build()) error.window(_ix->vk.errorText, __FUNCTION__);
}








#endif /// IX_USE_VULKAN


