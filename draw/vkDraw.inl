#pragma once


// POINT

void vkPoint::cmdPushColor(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color), 16, &push.color); }
void vkPoint::cmdPushPos(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, pos),   16, &push.pos); }
void vkPoint::cmdPushSize(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, size),   4, &push.size); }
void vkPoint::cmdPushFlags(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4, &push.flags); }
void vkPoint::cmdPushAll(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkPoint::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 1, 1, 0, 0); }

// LINE

void vkLine::cmdUpdateWidth(VkCommandBuffer in_c) { _ix->vk.CmdSetLineWidth(in_c, push.size+ ((push.flags& 0x0008)? 2.0f: 0.0f)); }
void vkLine::cmdPushColor(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color),  16,    &push.color); }
void vkLine::cmdPushPos(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, pos[0]), 16* 2, &push.pos[0]); }
void vkLine::cmdPushSize(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, size),   4,     &push.size); }
void vkLine::cmdPushFlags(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4,     &push.flags); }
void vkLine::cmdPushAll(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkLine::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 2, 1, 0, 0); }

// TRIANGLE

void vkTriangle::cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t) {
  if(!in_t) in_t= _ix->vki.noTexture;
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sl->vk->pipelineLayout, 1, 1, &in_t->vkd.set->set, 0, null);
}
void vkTriangle::cmdPushColor(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color),   16,    &push.color); }
void vkTriangle::cmdPushPos(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, vert[0]), 16* 3, &push.vert[0]); }
void vkTriangle::cmdPushTex(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, tex[0]),  16* 3, &push.tex[0]); }
void vkTriangle::cmdPushFlags(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),   4,     &push.flags); }
void vkTriangle::cmdPushAll(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkTriangle::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 3, 1, 0, 0); }

// QUAD

void vkQuad::cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t) {
  if(!in_t) in_t= _ix->vki.noTexture;
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sl->vk->pipelineLayout, 1, 1, &in_t->vkd.set->set, 0, null);
}
void vkQuad::cmdPushColor(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color), 16, &push.color); }
void vkQuad::cmdPushPos(VkCommandBuffer in_c)    { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, x0),    20, &push.x0); }
void vkQuad::cmdPushTex(VkCommandBuffer in_c)    { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, tx0),   20, &push.tx0); }
void vkQuad::cmdPushHollow(VkCommandBuffer in_c) { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, hollow), 4, &push.hollow); }
void vkQuad::cmdPushFlags(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4, &push.flags); }
void vkQuad::cmdPushAll(VkCommandBuffer in_c)    { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
void vkQuad::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 4, 1, 0, 0); }

// CIRCLE

inline void vkCircle::cmdTexture(VkCommandBuffer in_cmd, ixTexture *in_t) {
  if(!in_t) in_t= _ix->vki.noTexture;
  _ix->vk.CmdBindDescriptorSets(in_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sl->vk->pipelineLayout, 1, 1, &in_t->vkd.set->set, 0, null);
}
inline void vkCircle::cmdPushColor(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, color), 16, &push.color); }
inline void vkCircle::cmdPushPos(VkCommandBuffer in_c)     { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, x),     16, &push.x); }
inline void vkCircle::cmdPushTex(VkCommandBuffer in_c)     { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, tx0),   20, &push.tx0); }
inline void vkCircle::cmdPushHollow(VkCommandBuffer in_c)  { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, hollow), 4, &push.hollow); }
inline void vkCircle::cmdPushFlags(VkCommandBuffer in_c)   { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, offsetof(PConsts, flags),  4, &push.flags); }
inline void vkCircle::cmdPushAll(VkCommandBuffer in_c)     { _ix->vk.CmdPushConstants(in_c, sl->vk->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push); }
inline void vkCircle::cmdDraw(VkCommandBuffer in_c) { _ix->vk.CmdDraw(in_c, 4, 1, 0, 0); }


