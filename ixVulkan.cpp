#include "ix/ix.h"

#ifdef IX_USE_VULKAN

/*

-secondary buffers could be double, so you can build them while the GPU is drawing...
   atm, there's only a list that is called by the main draw buffers


// POSSIBILITY vvv - NEAH, FOUND A BETTER SOLUTION - NEW(CLUSTER), CFG, BUILD()
// ixvkResource - maybe ixvkBuffer: public VkoBuffer & ixvkImage: public VkoImage???????????????
//    the only other option woudl be to be derived from Vko objects, but this would mean they will not have anything in common
//    ... and it could be a better option, i am not sure about it... cuz atm im not set on this...
//             ^^^

  dynamic/static mesh buffers are the triky thing
  ATM, these clusters WILL DO.
  smart clusters will happen LATER. this is for open world games, and it's too far away imho

  -swapchains: currentExtent, in gettting surface capabilities, can be even ~0, in wayland;
               this tells me, that you cannot rely on that. it goes to 0,0 when window is minimized anyway
               ... WHAT IF, you have a last dx, last dy, and you just use that? does the surface really gets unavaible on minimize?
               it's kinda stoopid. what if you can actually still draw even with those error warnings? (there's 2 of them)
               in any case, the swapchain must be fully re-checked.

               for sure ix must set it using the customExtent, in swapchain.cfg, using osi.window.dx/dy

  // IMAGE BARIERS
  // Setting the old layout to VK_IMAGE_LAYOUT_UNDEFINED implies that the contents of the image subresource need not be preserved.
  //  Implementations may use this information to avoid performing expensive data transition operations.
  //  (AFTER PRESENTING, YOU DON'T CARE ABOUT THE IMAGE ANYMORE)


  -DYNAMIC UNIFORM BUFFER OBJECT
    Dynamic UBO descriptors also exist, given the VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC type.
    Basically, the idea is that, when you call vkCmdBindDescriptorSets, you provide a dynamic offset for any dynamic descriptors.
    The offset represents a byte offset from the buffer attached to that descriptor to where the UBO data you want to fetch lives.
    So you would have all of your data in a large buffer, and you would select which particular data you're using for a particular
    render call by invoking vkCmdBindDescriptorSets with a different dynamic offset.

  -dynamic ubo/sbo/anything that accepts it, can be used in the last 2 vars
    ix->vk.CmdBindDescriptorSets(*ix->vki.cmdOrtho, vk->pipelineLayout, 0, 1, &descriptorSet, 1, dynamic number[orarray]);



    the swapchain could be automated with 2 barriers
    one barrier to make the image ready to be written into, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    one barrier to make the image VK_ACCESS_MEMORY_READ_BIT, so the presentation engine will read from it and draw on monitor/window


    -moving buffers/images:
     resources could have the first access point, so you know to put the barrier as late as possible, not top of pipe




*/





// ##    ##  ##    ##  ##        ##    ##    ####    ##    ##
// ##    ##  ##    ##  ##        ##  ##    ##    ##  ###   ##
//  ##  ##   ##    ##  ##        ####      ########  ## ## ##
//   ####    ##    ##  ##        ##  ##    ##    ##  ##   ###
//    ##       ####    ########  ##    ##  ##    ##  ##    ##

///==========================================================///

///=========================-------------
// CONSTRUCTOR / DESTRUCTOR =============
///=========================-------------


ixVulkan::ixVulkan(Ix *in_ix): ix(in_ix), fi(0), draw(in_ix), swap(in_ix), render(in_ix), ortho(in_ix), defrag(in_ix) {
  q1= q2= null;
  qTool= null;
  qTransfer= null;
  qCompute= null;
  

  switchFamily= true;

  // ix private pools/cmd buffers, used for various operations
  poolTool= null;
  poolTrn= null;
  poolCompute= null;
  cmdTool= null;
  cmdTrn= null;
  cmdCompute= null;
  
  clusterIxDevice= clusterIxHost= null;
  clusterDevice= clusterHost= null;

  // stage buffers
  stageDevice= null;
  stageHost= null;

  ixStaticSetPool= null;  // main static set pool - fixed size

  //staticTexLayout= null;
  //staticTexPool= null;

  glb[0]= glb[1]= null;

  noTexture= null;

  /*
  screw barriers. there's semaphores delimiting things. i overthinked.
    barriers are lowest and fastest of sincronizations... but _sincronization_ . once you have a semaphore, there's nothing to sincronize anymore.

    sets must be smart unfortunately
    buffers/images must know their sets i think, so maybe seglists with the sets that use them
    or you go thru all sets and see if they use this current resource you wanna defrag... this ain't pretty
    and making the sets smart will mean to know to update/write themselfs.
    maybe the write will write 5 max at a time with one call... in the loop...

    and i think this is the true point where the low engine is done
    ;

  last thing i did: went thru all code and cut alot of vars, cuz ixvkResource would be bigger than the actual data that was in it -.-
    mem(), offset(), perfect examples to cut on data size, if needed. if more inline things like that can happen, it must happen imho.;
  even inline Ix *_ix() can be a thing everywhere!!!


    the size of objects must be trimmed, nicely.

    probly another pass thru whole vkobject, what can be cut, should be cut, cuz there's too much data in memory for these objects
    and ofc, the biggest problem will be the shader... but the shader DOES need another pass, for compute.
    */

}


ixVulkan::~ixVulkan() {
  if(ix->vk.device)
    ix->vk.DeviceWaitIdle(ix->vk.device);

  resClusters.delData();

  // MAYBE ALL BIG CLASSES SHOULD USE STUFF FROM ixVulkan
  // so all destroys happen here





}


void ixVulkan::shutdown() {
  if(swap.handle)
    swap.handle->destroy(); // swap must be destroyed before osi destroys the surfaces
}








///======-------------------------------
// FUNCS ===============================
///======-------------------------------


void ixVulkan::initAfterWindow() {
  #ifdef IX_BE_CHATTY
  bool chatty= true;
  #endif


  // vulkan device build
  
  /// gpu features
  ix->cfg.vk.gpuFeatures.depthBounds= 1;
  ix->cfg.vk.gpuFeatures.inheritedQueries= 1;
  ix->cfg.vk.gpuFeatures.vertexPipelineStoresAndAtomics= 1;
  ix->cfg.vk.gpuFeatures.wideLines= 1;
  /// gpuFeatures.geometryShader= 1;       geom enable
  /// gpuFeatures.tessellationShader= 1;   tese enable
  /// gpuFeatures.shaderStorageBufferArrayDynamicIndexing <<< to look into
  ix->cfg.vk.gpuFeatures.samplerAnisotropy= 1;

  ix->vk.cfg.gpuFeatures= &ix->cfg.vk.gpuFeatures;
  ix->vk.cfg.extensions.device.vk_KHR_swapchain.enable= 1;

  // physical device selection
  /// use specified physical device, if set
  if(ix->cfg.vk.physicalDeviceIndex!= ~0u)
    ix->vk.physicalDevice= &((*ix->vk.info.physicalDevice())[ix->cfg.vk.physicalDeviceIndex]);
      
  /// try to get the physical device from OSI (some systems can't match right atm)
  else if(ix->gpu->vkGPU!= null) {
    // OLD ix->vk.physicalDevice= (VkPhysicalDevice)ix->gpu->vkGPU;

    for(uint32_t a= 0; a< ix->vk.info.nrPhysicalDevices(); a++)
      if((*ix->vk.info.physicalDevice())[a].physicalDevice== (VkPhysicalDevice)(ix->gpu->vkGPU)) {
        ix->vk.physicalDevice= &((*ix->vk.info.physicalDevice())[a]);
        break;
      }
    
  /// use the first physical device on the system
  } else
    ix->vk.physicalDevice= &((*ix->vk.info.physicalDevice())[0]);

  if(!ix->vk.build()) { error.simple("Ix init failed: VKO device build failed"); return; }
    
  _handleQueues();

  // flag to use memory barriers for queue family switches
  switchFamily= (qTransfer->family!= qTool->family);
  if(ix->cfg.vk.resourcesUseSharedQueueFamilies)
    switchFamily= false;
  
  #ifdef IX_BE_CHATTY
  if(chatty) {
    printf("Vulkan mem info:\n");
    // mem heaps
    printf("-memoryHeapCount[%u]\n", ix->vk.info.memProp.memoryHeapCount);
    for(uint a= 0; a< ix->vk.info.memProp.memoryHeapCount; a++) {
      printf("  heap[%u]: size[%llu]", a, ix->vk.info.memProp.memoryHeaps[a].size);
      if(ix->vk.info.memProp.memoryHeaps[a].flags& VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)   printf(" [device]");
      if(ix->vk.info.memProp.memoryHeaps[a].flags& VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) printf(" [multi instance]");
      printf("\n");
    }
    // mem types
    printf("-memoryTypeCount[%u]\n", ix->vk.info.memProp.memoryTypeCount);
    for(uint a= 0; a< ix->vk.info.memProp.memoryTypeCount; a++) {
      printf("  type[%u]: heap[%u]", a, ix->vk.info.memProp.memoryTypes[a].heapIndex);
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)        printf(" [device]");
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)        printf(" [host visible]");
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)       printf(" [host coherent]");
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_HOST_CACHED_BIT)         printf(" [host cached]");
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)    printf(" [lazily alloc]");
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_PROTECTED_BIT)           printf(" [protected]");
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) printf(" [dev coherent AMD]");
      if(ix->vk.info.memProp.memoryTypes[a].propertyFlags& VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) printf(" [dev uncached AMD]");
      printf("\n");
    }
  } /// chatty
  #endif

  // create the memory clusters
  addCluster(&clusterIxDevice);
  addCluster(&clusterIxHost);
  addCluster(&clusterDevice);
  addCluster(&clusterHost);

  clusterIxDevice->cfg(ix->cfg.vk.size_clusterIxDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  clusterIxHost->cfg(ix->cfg.vk.size_clusterIxHost, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  clusterDevice->cfg(ix->cfg.vk.size_clusterResDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  clusterHost->cfg(ix->cfg.vk.size_clusterResHost, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // stage buffers
  stageDevice= new ixvkBuffer(clusterIxDevice);
    stageDevice->handle->cfgSize(ix->cfg.vk.size_stageBufferDevice);
    stageDevice->handle->cfgUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT| VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  stageDevice->build();

  stageHost= new ixvkBuffer(clusterIxHost);
    stageHost->handle->cfgSize(ix->cfg.vk.size_stageBufferHost);
    stageHost->handle->cfgUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT| VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  stageHost->build();
  
  // setup the pools
  poolTool= ix->vk.objects.addCommandPool();
  poolTool->configure(qTool->family);
  poolTool->build();

  poolTrn= ix->vk.objects.addCommandPool();
  poolTrn->configure(qTransfer->family);
  poolTrn->build();
  
  poolCompute= ix->vk.objects.addCommandPool();
  poolCompute->configure(qCompute->family);
  poolCompute->build();

  // setup the command buffers
  cmdTool= poolTool->addCommandBuffer();
  cmdTool->cfgLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  cmdTool->cfgUsage(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  cmdTool->build();

  cmdTrn= poolTrn->addCommandBuffer();
  cmdTrn->cfgLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  cmdTrn->cfgUsage(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  cmdTrn->build();

  cmdCompute= poolCompute->addCommandBuffer();
  cmdCompute->cfgLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
  cmdCompute->cfgUsage(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  cmdCompute->build();

  // main objects init

  render.init();
  swap.init();
  ortho.init();
  render.updateViewportAndScissor();
  defrag.init();      // depends on render.cmdMain[] - so always init AFTER render class


  /// glb buffer layout
  VkoDescriptorSetLayout *glbLayout= ix->vk.objects.addDescriptorSetLayout();
  glbLayout->cfgAddDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL);
  glbLayout->build();

  // <<< add more layouts here, for ix if needed

  /// ix static set pool - includes glb buffer
  ixStaticSetPool= new ixvkDescPool(this->ix);
  ixStaticSetPool->configure(glbLayout, 2);
  ixStaticSetPool->build();

  /// glb buffer creation
  for(uint a= 0; a< 2; a++) {
    glb[a]= new GlbBuffer(clusterIxDevice);
    //glb[a]->cfg(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

    //glb[a]->access.firstStage= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    //glb[a]->access.firstAccess= VK_ACCESS_SHADER_READ_BIT;


    glb[a]->handle->cfgSize(sizeof(GlbBuffer::Data), 0);
    glb[a]->handle->cfgUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT| VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    glb[a]->build();

    glb[a]->layout= glbLayout;
    ixStaticSetPool->addSet(&glb[a]->set);
    glb[a]->set->bind(0, glb[a]);
    glb[a]->set->update();
    /*
    VkDescriptorBufferInfo bufInfo= { glb[a]->handle->buffer, 0, VK_WHOLE_SIZE }; // buffer, offset, range

    VkWriteDescriptorSet setWrite= {
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // sType;
      null,                                   // pNext;
      glb[a]->set->set,                       // dstSet;
      0,                                      // dstBinding;
      0,                                      // dstArrayElement;
      1,                                      // descriptorCount;
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,      // descriptorType;
      null,                                   // pImageInfo;
      &bufInfo,                               // pBufferInfo;
      null                                    // pTexelBufferView;
    }; 

    ix->vk.UpdateDescriptorSets(ix->vk, 1, &setWrite, 0, null);
    */
  }
  




  

  // draw.init() moved to ix.cpp, after texSys
}




void ixVulkan::getToolCmdAndQueue(uint32 in_family, VkoQueue **out_queue, VkoCommandBuffer **out_cmd) {
  if(in_family== qTool->family)     { if(out_queue) *out_queue= qTool; if(out_cmd) *out_cmd= cmdTool; return; }
  if(in_family== qTransfer->family) { if(out_queue) *out_queue= qTransfer; if(out_cmd) *out_cmd= cmdTrn; return; }
  // COMPUTE MAKEME <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  if(in_family== qCompute->family)  { if(out_queue) *out_queue= qCompute; if(out_cmd) *out_cmd= cmdCompute; error.makeme(__FUNCTION__); return; }

  if(out_queue) *out_queue= null;
  if(out_cmd) *out_cmd= null;
}


void ixVulkan::addCluster(ixvkResCluster **out_cluster) {
  *out_cluster= new ixvkResCluster(ix);
  resClusters.add(*out_cluster);
}


void ixVulkan::delCluster(ixvkResCluster **out_cluster) {
  //(*out_cluster)->destroy();
  resClusters.del(*out_cluster);
}




void ixVulkan::cmdScissor(VkCommandBuffer in_cmd, recti *in_s) {
  VkRect2D r= {{ in_s->x0- ix->win->x0, in_s->y0- ix->win->y0 }, { (uint32)in_s->dx, (uint32)in_s->dy}};
  if(r.offset.x< 0) r.offset.x= 0;
  if(r.offset.y< 0) r.offset.y= 0;
  ix->vk.CmdSetScissor(in_cmd, 0, 1, &r);
}










// PRIVATE ===============================

VkoQueue *ixVulkan::_getQueue(uint32 n, VkQueueFlags in_type, VkQueueFlags in_mustNotHave) {
  VkoQueue *q= null;
  
  // loop thru all queues, an exact match must happen
  for(uint32 a= 0; a< ix->vk.nrQueues; a++)
    if(ix->vk.queue[a].typeFlags& in_type) {
      /// must not have these flags
      if(in_mustNotHave)
        if(ix->vk.queue[a].typeFlags& in_mustNotHave)
          continue;

      // found a queue with specified flags
      if(n== 0) {
        q= &ix->vk.queue[a];
        break;
      } else
        n--;
    }
  return q;
}


void ixVulkan::_handleQueues() {
  // main queue that will be used. this has to exist
  q1= _getQueue(0, VK_QUEUE_COMPUTE_BIT| VK_QUEUE_COMPUTE_BIT| VK_QUEUE_TRANSFER_BIT);
  if(q1== null) {
    error.detail("not one vulkan universal queue found. aborting.", __FUNCTION__);
    return;
  }

  // secondary queue, used as the main one
  q2= _getQueue(1, VK_QUEUE_COMPUTE_BIT| VK_QUEUE_COMPUTE_BIT| VK_QUEUE_TRANSFER_BIT);
  if(q2== null)         /// fallback to q1
    q2= q1;

  // tool queue (queue #3)
  qTool= _getQueue(2, VK_QUEUE_COMPUTE_BIT| VK_QUEUE_COMPUTE_BIT| VK_QUEUE_TRANSFER_BIT);
  if(qTool== null)      /// fallback to q2
    qTool= q2;

  // transfer queue
  qTransfer= _getQueue(0, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT| VK_QUEUE_COMPUTE_BIT);
  if(qTransfer== null)  /// fallback to an universal queue #4
    qTransfer= _getQueue(3, VK_QUEUE_TRANSFER_BIT);
  if(qTransfer== null)  /// fallback to qTool
    qTransfer= qTool;

  // compute queue
  qCompute= _getQueue(0, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
  if(qCompute== null)   /// fallback to an universal queue #5
    qCompute= _getQueue(4, VK_QUEUE_COMPUTE_BIT);
  if(qCompute== null)   /// fallback to q2
    qCompute= q2;
}










//   ####    ##    ##    ####    ######      ####    ##    ##    ####    ########  ##    ##
// ##        ##    ##  ##    ##  ##    ##  ##    ##  ##    ##  ##    ##     ##     ###   ##
//   ####    ## ## ##  ########  ######    ##        ########  ########     ##     ## ## ##
//       ##  ## ## ##  ##    ##  ##        ##    ##  ##    ##  ##    ##     ##     ##   ###
//  #####     ##  ##   ##    ##  ##          ####    ##    ##  ##    ##  ########  ##    ##

///========================================================================================///




void ixVulkan::Swap::init() {
  bool ret= false;
  /// swapchain
  handle= _ix->vk.objects.addSwapchain();
  handle->cfg.setSurface((VkSurfaceKHR)_ix->win->vkSurface);
  if(!handle->build()) goto Exit;
  
  /// depth/stencil image
  depthStencil= new ixvkImage(_ix->vki.clusterIxDevice);
  depthStencil->cfgFormat(VK_FORMAT_D24_UNORM_S8_UINT);
  depthStencil->cfgSize(VkExtent3D{handle->dx, handle->dy, 1}, 1, 1);
  depthStencil->cfgUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  depthStencil->access[0].qFamily= _ix->vki.q1->family;
  if(!depthStencil->build()) goto Exit;

  _ix->vk.DeviceWaitIdle(_ix->vk);
  _ix->vki.cmdTool->pool->reset();
  _ix->vki.cmdTool->startRecording();
  depthStencil->barrier(_ix, *_ix->vki.cmdTool, 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
                        //VK_QUEUE_FAMILY_IGNORED, _ix->vki.q1->family);
  _ix->vki.cmdTool->endRecording();
  _ix->vki.cmdTool->submit(*_ix->vki.qTool);
  _ix->vk.QueueWaitIdle(*_ix->vki.qTool);

  depthStencil->access[0].layout= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthStencil->access[0].qFamily= _ix->vki.q1->family;

  for(uint a= 0; a< 2; a++) {
    /// framebuffers
    framebuffer[a]= _ix->vk.objects.addFramebuffer();
    framebuffer[a]->cfgRenderPass(*_ix->vki.render.handle);
    framebuffer[a]->cfgAddAttachment(handle->images[a], handle->swapInfo.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    framebuffer[a]->cfgAddAttachment(*depthStencil, depthStencil->handle->createInfo.format, Img::vkGetAspectFromFormat((ImgFormat)(depthStencil->handle->createInfo.format)));
    framebuffer[a]->cfgDimensions(handle->dx, handle->dy);
    if(!framebuffer[a]->build()) goto Exit;
  }


  ret= true; // success
Exit:
  
  /// signal to rebuild the render command buffers
  _ix->vki.render.cmdBuild[0]= true;
  _ix->vki.render.cmdBuild[1]= true;

  if(!ret) error.detail("swapchain build failed", __FUNCTION__);
}



void ixVulkan::Swap::rebuild() {
  bool ret= false;
  _ix->vk.DeviceWaitIdle(_ix->vk);

  /// swapchain rebuild
  if(!handle->rebuild()) goto Exit;

  /// depth/stencil rebuild
  depthStencil->cfgSize(VkExtent3D{handle->dx, handle->dy, 1}, 1, 1);
  if(!depthStencil->rebuild()) goto Exit;

  _ix->vki.cmdTool->pool->reset();
  _ix->vki.cmdTool->startRecording();
  depthStencil->barrier(_ix, *_ix->vki.cmdTool, 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
                        //VK_QUEUE_FAMILY_IGNORED, _ix->vki.q1->family);
  _ix->vki.cmdTool->endRecording();
  _ix->vki.cmdTool->submit(*_ix->vki.qTool);
  depthStencil->access[0].layout= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthStencil->access[0].qFamily= _ix->vki.q1->family;

  /// framebuffers rebuild
  for(uint a= 0; a< 2; a++) {
    framebuffer[a]->changeAttachment(0, handle->images[a], handle->swapInfo.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    framebuffer[a]->changeAttachment(1, *depthStencil, depthStencil->handle->createInfo.format, Img::vkGetAspectFromFormat((ImgFormat)(depthStencil->handle->createInfo.format)));
    framebuffer[a]->cfgDimensions(handle->dx, handle->dy);
    if(!framebuffer[a]->rebuild()) goto Exit;
  }

  ret= true;  // success

Exit:

  /// signal to rebuild the render command buffers
  _ix->vki.render.cmdBuild[0]= true;
  _ix->vki.render.cmdBuild[1]= true;

  _ix->vk.DeviceWaitIdle(_ix->vk);
  if(!ret) error.detail("Swapchain rebuild failed", __FUNCTION__);
}


void ixVulkan::Swap::rebuildDepthStencil() {
  _ix->vk.DeviceWaitIdle(_ix->vk);

  for(uint a= 0; a< 2; a++) {
    framebuffer[a]->changeAttachment(1, *depthStencil, depthStencil->handle->createInfo.format, Img::vkGetAspectFromFormat((ImgFormat)(depthStencil->handle->createInfo.format)));
    if(!framebuffer[a]->rebuild()) error.detail("framebuffer rebuild failed on depth/stencil atachment change", __FUNCTION__);
  }
}





// ######    ########  ##    ##  ######    ########  ######
// ##    ##  ##        ####  ##  ##    ##  ##        ##    ##
// ######    ######    ##  ####  ##    ##  ######    ######
// ##    ##  ##        ##    ##  ##    ##  ##        ##    ##
// ##    ##  ########  ##    ##  ######    ########  ##    ##

///==========================================================///

void ixVulkan::RenderPass::delData() {


}





void ixVulkan::RenderPass::init() {
  // THIS NEEDS MORE CONFIGURATION
  bool ret= false;

  

  handle= _ix->vk.objects.addRenderPass();
  
  handle->addAttachment2(0, VK_FORMAT_B8G8R8A8_UNORM,        VK_SAMPLE_COUNT_1_BIT,
                            VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_STORE,      // att load+store
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencil load+store
                            VK_IMAGE_LAYOUT_UNDEFINED,       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);  // initial+final layout
  
  handle->addAttachment2(0, VK_FORMAT_D24_UNORM_S8_UINT,     VK_SAMPLE_COUNT_1_BIT,
                            VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_DONT_CARE,                  // att load+store
                            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,                  // stencil load+store
                            VK_IMAGE_LAYOUT_UNDEFINED,       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL); // initial+final layout
  
  handle->addSubpass(0, VK_PIPELINE_BIND_POINT_GRAPHICS);

  //INITIAL>> handle->addSubpassColorAttachment(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false, 0, VK_IMAGE_LAYOUT_UNDEFINED, false, 0, VK_IMAGE_LAYOUT_UNDEFINED);
  // ADDED DEPTH/STENCIL vvv
  handle->addSubpassColorAttachment(0, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false, 0, VK_IMAGE_LAYOUT_UNDEFINED, true, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  
  handle->addSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

  if(!handle->build()) goto Exit;


  /// double main command buffers
  for(uint a= 0; a< 2; a++) {
    pool[a]= _ix->vk.objects.addCommandPool();
    pool[a]->configure(_ix->vki.q1->family);
    if(!pool[a]->build()) goto Exit;

    fenStart[a]= _ix->vk.objects.addFence();
    fenStart[a]->cfgFlags(VK_FENCE_CREATE_SIGNALED_BIT);
    fenStart[a]->build();

    cmdMain[a]= pool[a]->addCommandBuffer();
    cmdMain[a]->cfgLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    //cmdMain[a]->setUsage(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT); // << maybe?
    cmdMain[a]->cfgAddWaitSemaphore(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    
    cmdMain[a]->cfgAddSignalSemaphore();
    cmdMain[a]->cfgFence(*fenStart[a]);
    cmdMain[a]->build();
  }

  ret= true;  // success
Exit:
  if(!ret) error.detail("Render object build failed", __FUNCTION__);
}




bool ixVulkan::RenderPass::startRender() {
  
  //uint32 si= _ix->vki.swap.handle->currentIndex;   // swap's index - this1 cannot be certain, there's a logic clusterf
  uint32 fi= _ix->vki.fi;               /// ix's frame index

  /// previous command buffer register
  cmdPrev= cmdMain[(fi== 0? 1: 0)];

  _ix->measureFPS();


  if(!_ix->vki.swap.handle->aquire(*cmdMain[fi]->waitSemaphores[0]))
    return false;

  // if defragJob happening, a new fence has to happen here <<<<<<<<<<<<<<<<<<<<<<<<,,
  //_ix->vki.defrag.doJobs(1);

  /// start only when cmdStart's fence is signaled
  _ix->vk.WaitForFences(_ix->vk, 1, &cmdMain[fi]->fence, true, ~0);
  _ix->vk.ResetFences(_ix->vk, 1, &cmdMain[fi]->fence);

  updateViewportAndScissor();   /// update the global viewport / scissor


  //VkClearColorValue colorClear = {{ 0.9f, 0.0f, 0.9f, 1.0f }};
  
  // global buffer update/upload

  _ix->vki.glb[fi]->data.cameraOrtho= _ix->cameraOrtho.cameraMat;
  _ix->vki.glb[fi]->data.cameraPersp= _ix->cameraPersp.cameraMat;
  _ix->vki.glb[fi]->data.vp.x= (float)_ix->win->x0;
  _ix->vki.glb[fi]->data.vp.y= (float)_ix->win->y0;
  _ix->vki.glb[fi]->upload(&_ix->vki.glb[fi]->data, 0, sizeof(_ix->vki.glb[fi]->data));

  

  /*  THE WAITING HAPPEN ON RENDERPASS I THINK, there's a dependency there
  VkImageSubresourceRange range= {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}; 
  VkImageMemoryBarrier b= { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, null,
                            VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_WRITE_BIT,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                            swap->handle->images[swap->handle->currentIndex], range};
  _ix->vk.CmdPipelineBarrier(*cmdMain[i], VK_PIPELINE_STAGE_f, in_dstStage, 0, 0, null, 0, null, 1, &b);
  */



  //_ix->vk.ResetCommandPool(_ix->vk, *pool[fi], 0);
  //cmdMain[fi]->reset();

  //cmdMain[fi]->_submitInfo.waitSemaphoreCount= ((_ix->vki.defrag.job> 0)? 2: 1);

  cmdMain[fi]->pool->reset();
  cmdMain[fi]->startRecording();

  

  // CLEAR IMAGE example
  //VkClearColorValue colorClear= {1.0, 1.0, 0.2, 0.3};
  //VkImageSubresourceRange subres= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  //_ix->vk.CmdClearColorImage(*_ix->vki.ortho.cmd[_ix->vki.fi], _ix->vki.swap.handle->images[_ix->vki.swap.handle->currentIndex], VK_IMAGE_LAYOUT_current i think, &colorClear, 1, &subres);


  VkRenderPassBeginInfo rpInfo;
    rpInfo.sType= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext= null;
    rpInfo.renderPass= *handle;
    rpInfo.framebuffer= *_ix->vki.swap.framebuffer[_ix->vki.swap.handle->currentIndex];
    rpInfo.renderArea= {{ 0, 0 }, { _ix->vki.swap.handle->dx, _ix->vki.swap.handle->dy }};
    VkClearValue rpColor[2] = { {0.3f, 0.2f, 0.6f, 1.0f}, {0.0f} };
    rpColor[1].depthStencil.depth= 1.0f;
    rpColor[1].depthStencil.stencil= 0;
    rpInfo.clearValueCount= 2;
    rpInfo.pClearValues= rpColor;
    
  _ix->vk.CmdBeginRenderPass(*cmdMain[fi], &rpInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);


  /// starting cmd buffer execute / update
  //if(flagUpdateCmdIxStart)
  //  updateCmdIxStart();       /// atm updating ixDraw glb descriptor set
  //_ix->vk.CmdExecuteCommands(*cmdMain[fi], 1, &cmdIxStart[fi]->buffer);




  return true;
}


void ixVulkan::RenderPass::endRender() {
  //uint32 si= _ix->vki.swap.handle->currentIndex;
  uint32 fi= _ix->vki.fi;

  _ix->vk.CmdEndRenderPass(*cmdMain[fi]);
  cmdMain[fi]->endRecording();


  // defrag + submit of main command buffer

  _ix->vki.defrag.doJobs();
  this;
  // _submitInfo can have 2 buffers to submit... so this could be done in defrag<<<<<
  cmdMain[fi]->submit(*_ix->vki.q1);      // defrag signal semaphore is done by doJobs()

  if(_ix->vki.defrag.cmdToSubmit) {       // there is a defrag cmdBuffer to submit
    _ix->vki.defrag.cmdToSubmit->submit(_ix->vki.defrag.qToSubmit);
    _ix->vki.defrag.cmdToSubmit->fence= 0;
    _ix->vki.defrag.cmdToSubmit= null;
  }
  cmdMain[fi]->_submitInfo.waitSemaphoreCount= 1;
  cmdMain[fi]->_submitInfo.signalSemaphoreCount= 1;

  /// depth/stencil image could be defragged, that would require an update to the frontbuffer before the submit
  if(_ix->vki.swap.flags.isUp(0x01)) {
    _ix->vki.swap.rebuildDepthStencil();
    _ix->vki.swap.flags.setDown(0x01);
  }

  // swapchain submit
  if(_ix->vki.swap.handle->currentIndex!= ~0)
    _ix->vki.swap.handle->queueShowCurrent(*_ix->vki.q1, *cmdMain[fi]->signalSemaphores[0]);


  // TEST TO MOVE THIS JUST AFTER DEPTH/STENCIL DEFRAG CHECK
  // defrag buffers to destroy

  _ix->vki.defrag.afterSubmitJobs();

  //if(_ix->vki.defrag.destroyBuffersList->nrNodes)
  //  _ix->vki.defrag.destroyBuffers(_ix->vki.defrag.fenceSignal[fi]);
  //_ix->vki.defrag.

  /// reset current stuff
  _ix->vki.ortho.currentPipeline= 0;


  // update the current frame index
  _ix->vki.fi= (_ix->vki.fi? 0: 1);
}





bool ixVulkan::RenderPass::startOrtho() {
  _ix->vki.ortho.cmd[_ix->vki.fi]->_inheritanceInfo.framebuffer= _ix->vki.swap.framebuffer[_ix->vki.swap.handle->currentIndex]->framebuffer;

  _ix->vki.ortho.cmd[_ix->vki.fi]->pool->reset();
  _ix->vki.ortho.cmd[_ix->vki.fi]->startRecording();
  return true;
}



void ixVulkan::RenderPass::endOrtho() {
  _ix->vki.ortho.cmd[_ix->vki.fi]->endRecording();
  _ix->vk.CmdExecuteCommands(*cmdMain[_ix->vki.fi], 1, &_ix->vki.ortho.cmd[_ix->vki.fi]->buffer);
}








// ######    ########  ########  ######      ####      ####
// ##    ##  ##        ##        ##    ##  ##    ##  ##
// ##    ##  ######    ######    ######    ########  ##  ####
// ##    ##  ##        ##        ##    ##  ##    ##  ##    ##
// ######    ########  ##        ##    ##  ##    ##    ######

///==========================================================///

ixVulkan::Defrag::Defrag(Ix *in_ix): _ix(in_ix), destroyBuffersList{ {10, sizeof(DBuffer), true}, {10, sizeof(DBuffer), true} },
                                      poolGfx{null, null}, poolCompute{null, null}, poolTrn{null, null},
                                      cmdGfx{null, null},  cmdCompute{null, null},  cmdTrn{null, null},
                                      semWait{null, null}, semSignal{null, null}, // semLink{null, null},
                                      fenceSignal{null, null}
{
  job= 0;
  _buf;
  cmdToSubmit= null;
  qToSubmit= 0;
  resToUpdate= null;
}


ixVulkan::Defrag::~Defrag() {
  /*
  for(uint a= 0; a< 2; ++a)
    cmdGfx[a]->_submitInfo.waitSemaphoreCount= 0,
    cmdGfx[a]->_submitInfo.signalSemaphoreCount= 0,
    cmdGfx[a]->_submitInfo.pWaitDstStageMask= null,
    cmdGfx[a]->_submitInfo.pWaitSemaphores= null,
    cmdGfx[a]->_submitInfo.pSignalSemaphores= null,
    cmdCompute[a]->_submitInfo.waitSemaphoreCount= 0,
    cmdCompute[a]->_submitInfo.signalSemaphoreCount= 0,
    cmdCompute[a]->_submitInfo.pWaitDstStageMask= null,
    cmdCompute[a]->_submitInfo.pWaitSemaphores= null,
    cmdCompute[a]->_submitInfo.pSignalSemaphores= null,
    cmdTrn[a]->_submitInfo.waitSemaphoreCount= 0,
    cmdTrn[a]->_submitInfo.signalSemaphoreCount= 0,
    cmdTrn[a]->_submitInfo.pWaitDstStageMask= null,
    cmdTrn[a]->_submitInfo.pWaitSemaphores= null,
    cmdTrn[a]->_submitInfo.pSignalSemaphores= null;
    */
}




void ixVulkan::Defrag::getQueueCmd(uint32 in_qfamily, VkoQueue **out_queue, VkoCommandBuffer **out_cmd) {
  if(in_qfamily== _ix->vki.qTool->family)     { if(out_queue) *out_queue= _ix->vki.qTool;     if(out_cmd) *out_cmd= cmdGfx[_ix->vki.fi];     return; }
  if(in_qfamily== _ix->vki.qCompute->family)  { if(out_queue) *out_queue= _ix->vki.qCompute;  if(out_cmd) *out_cmd= cmdCompute[_ix->vki.fi]; return; }
  if(in_qfamily== _ix->vki.qTransfer->family) { if(out_queue) *out_queue= _ix->vki.qTransfer; if(out_cmd) *out_cmd= cmdTrn[_ix->vki.fi];     return; }
  if(in_qfamily== VK_QUEUE_FAMILY_IGNORED)    { if(out_queue) *out_queue= _ix->vki.qTool;     if(out_cmd) *out_cmd= cmdGfx[_ix->vki.fi];     return; }
  if(out_queue) *out_queue= null;
  if(out_cmd) *out_cmd= null;
}





void ixVulkan::Defrag::init() {
  // wait[1] and signal[1] of cmdMain, are for defrag operation


  _buf= new ixvkBuffer(_ix->vki.clusterIxDevice);
  _buf->handle->cfgUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT| VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  //_buf->build(); NO BUILD, it's built when it's needed


  //stageWait= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  /// defrag signals job is done
  semSignal[0]= _ix->vk.objects.addSemaphore(); semSignal[0]->build();
  semSignal[1]= _ix->vk.objects.addSemaphore(); semSignal[1]->build();

  

  for(uint a= 0; a< 2; a++) {
    // semaphore link logic

    /// waits for defrag - it could be DRAW_INDIRECT, cuz that's 1 level lower, but not sure if any advantage.
    ///                    anything using a buffer must wait for the defrag
    _ix->vki.render.cmdMain[a]->cfgAddCustomWaitSemaphore(semSignal[(a== 0? 1: 0)], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    _ix->vki.render.cmdMain[a]->cfgAddSignalSemaphore();                 // signal[1] - signals for defrag work

    _ix->vki.render.cmdMain[a]->_submitInfo.signalSemaphoreCount--;
    _ix->vki.render.cmdMain[a]->_submitInfo.waitSemaphoreCount--;

    semWait[a]= _ix->vki.render.cmdMain[a]->signalSemaphores[_ix->vki.render.cmdMain[a]->nrSignalSemaphores- 1];

    fenceSignal[a]= _ix->vk.objects.addFence();
    //fenceSignal[a]->cfgFlags(VK_FENCE_CREATE_SIGNALED_BIT);
    fenceSignal[a]->build();
    
    // pools

    poolGfx[a]= _ix->vk.objects.addCommandPool();
    poolGfx[a]->configure(_ix->vki.q1->family);
    poolGfx[a]->build();

    poolCompute[a]= _ix->vk.objects.addCommandPool();
    poolCompute[a]->configure(_ix->vki.qCompute->family);
    poolCompute[a]->build();
    
    poolTrn[a]= _ix->vk.objects.addCommandPool();
    poolTrn[a]->configure(_ix->vki.qTransfer->family);
    poolTrn[a]->build();

    // command buffers

    cmdGfx[a]= poolGfx[a]->addCommandBuffer();
    cmdGfx[a]->cfgLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    cmdGfx[a]->cfgUsage(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    cmdGfx[a]->cfgAddCustomSignalSemaphore(semSignal[a]);
    cmdGfx[a]->cfgAddCustomWaitSemaphore(semWait[a]);
    cmdGfx[a]->build();

    cmdCompute[a]= poolCompute[a]->addCommandBuffer();
    cmdCompute[a]->cfgLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    cmdCompute[a]->cfgUsage(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    cmdCompute[a]->cfgAddCustomSignalSemaphore(semSignal[a]);
    cmdCompute[a]->cfgAddCustomWaitSemaphore(semWait[a]);
    cmdCompute[a]->build();

    cmdTrn[a]= poolTrn[a]->addCommandBuffer();
    cmdTrn[a]->cfgLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    cmdTrn[a]->cfgUsage(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    cmdTrn[a]->cfgAddCustomSignalSemaphore(semSignal[a]);
    cmdTrn[a]->cfgAddCustomWaitSemaphore(semWait[a]);
    cmdTrn[a]->build();
  }
  
  // DEFRAG one - it could be DRAW_INDIRECT, cuz that's 1 level lower, but not sure if any advantage.
  //              anything using a buffer must wait for the defrag


  //_ix->vki.render.cmdMain[0]->addCustomWaitSemaphore(semSignal[1], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);    
  //_ix->vki.render.cmdMain[1]->addCustomWaitSemaphore(semSignal[0], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

}


void ixVulkan::Defrag::doJobs(uint32 in_n) {
  // cmd1->defrag1->cmd2->defrag2
  // the one-job is as fast as humanly possible, ATM. In the future, it could be even more advanced, but i don't see if it's even possible

  // MULTIPLE JOBS: use QueueWaitIdle. no semaphores - do NOT fit within app frames, stop everything and do jobs.
  // THE ONE JOB:       cmdMain must NOT be sent already!!!!!!! - it is designed to fit between frames.

  // a secondary thread that will manage loading/downloading/uploading/defraging? so defrag would not conflict with loading? that could be it

  //if(job== 0) return;

  uint32 qres;
  VkoQueue *q;
  VkoCommandBuffer *cmd;
  uint fi= _ix->vki.fi;

  // special, hopely rare case where there is a job, this func is called, but no job must be done
  // an empty cmd buffer is sent just to signal semaphores;
  if(in_n== 0 || job== 0) {
    _ix->vki.render.cmdMain[fi]->_submitInfo.signalSemaphoreCount= 1;
    if(_ix->vki.render.cmdPrev) _ix->vki.render.cmdPrev->_submitInfo.waitSemaphoreCount= 1;


    // DEBUG VVV
    uint dbgnr= 0;
    for(ixvkResCluster *cluster= (ixvkResCluster *)_ix->vki.resClusters.first; cluster; cluster= (ixvkResCluster *)cluster->next)
      for(ixvkResClusterSegment *segment= (ixvkResClusterSegment *)cluster->segments.first; segment; segment= (ixvkResClusterSegment *)segment->next)
        for(ixvkResource *res= (ixvkResource *)segment->resources.first; res; res= (ixvkResource *)res->next)
          if(res->defragDelta)
            dbgnr++;
    dbgnr;
    // DEBUG ^^^


    return;

  // fast 1 job- with semaphore links
  } else if(in_n== 1) {
    _ix->vki.render.cmdMain[fi]->_submitInfo.signalSemaphoreCount= 2;
    _ix->vki.render.cmdPrev->_submitInfo.waitSemaphoreCount= 2;

    /// pass thru all clusters/segments/resources
    for(ixvkResCluster *cluster= (ixvkResCluster *)_ix->vki.resClusters.first; cluster; cluster= (ixvkResCluster *)cluster->next)
      for(ixvkResClusterSegment *segment= (ixvkResClusterSegment *)cluster->segments.first; segment; segment= (ixvkResClusterSegment *)segment->next)
        for(ixvkResource *res= (ixvkResource *)segment->resources.first; res; res= (ixvkResource *)res->next) {

          if(res== _buf)
            continue;  // skip defrag's buf

          // res with defrag job found
          if(res->defragDelta || res->defragDeltaInside) {
            /// special case - stage buffers move
            if(res->defragDelta && (res== _ix->vki.stageDevice || res== _ix->vki.stageHost)) {
              ((ixvkBuffer *)res)->handle->offset-= res->defragDelta;
              res->defragDelta= 0;
              ((ixvkBuffer *)res)->handle->rebuild();                     // this avoids changing the cluster
              _ix->vk.BindBufferMemory(_ix->vk, *((ixvkBuffer *)res)->handle, *res->segment->memory, ((ixvkBuffer *)res)->handle->offset);

            /// normal resources move
            } else {

              if(res->classT== ixClassT::VKBUFFER)
                qres= ((ixvkBuffer *)res)->access.qFamily;
              else if(res->classT== ixClassT::VKIMAGE)
                qres= ((ixvkImage *)res)->access[0].qFamily; // an image array with different queues will screw this <<<<<<<<<<<<<<<<< but this is a rare case
              getQueueCmd(qres, &q, &cmd);

              cmd->pool->reset();
              cmd->startRecording();
              cmd->cfgFence(*fenceSignal[fi]);

              /// buffer
              if(res->classT== ixClassT::VKBUFFER) {
                if(res->defragDelta) {

                  moveBuffer(*cmd, (ixvkBuffer *)res);
                  //res->defragDelta= 0;


                } else if(res->defragDeltaInside)
                  moveInside(*cmd, (ixvkBuffer *)res);

              /// image
              } else if(res->classT== ixClassT::VKIMAGE) {
                moveImage(*cmd, (ixvkImage *)res);
                //res->defragDelta= 0;
              }

              else error.window("unkown resource type", true);

              cmd->endRecording();
              cmdToSubmit= cmd;
              qToSubmit= q->queue;
              
            } /// normal resource

            if(res== _ix->vki.swap.depthStencil)
              _ix->vki.swap.flags.setUp(0x01);
            else
              resToUpdate= res;

            /// update segment freespace
            segment->updateFreespace();
            --job;
            return;
          } /// a defrag hast o happen on this resource
        } /// pass thru all clusters/segments/resources


  // slowest - multiple jobs, QueueWaitIdle inside - do not fit within a game frame
  } else {
    // use QueueWaitIdle, for big batches - no semaphores
    _ix->vki.render.cmdMain[fi]->_submitInfo.signalSemaphoreCount= 1;
    _ix->vki.render.cmdPrev->_submitInfo.waitSemaphoreCount= 1;

    /// pass thru all clusters/segments/resources
    for(ixvkResCluster *cluster= (ixvkResCluster *)_ix->vki.resClusters.first; cluster; cluster= (ixvkResCluster *)cluster->next)
      for(ixvkResClusterSegment *segment= (ixvkResClusterSegment *)cluster->segments.first; segment; segment= (ixvkResClusterSegment *)segment->next)
        for(ixvkResource *res= (ixvkResource *)segment->resources.first; res; res= (ixvkResource *)res->next) {

          // res with defrag job found
          if(res->defragDelta || res->defragDeltaInside) {
            /// special case - stage buffers move
            if(res->defragDelta && (res== _ix->vki.stageDevice || res== _ix->vki.stageHost)) {
              ((ixvkBuffer *)res)->handle->offset-= res->defragDelta;
              res->defragDelta= 0;
              ((ixvkBuffer *)res)->handle->rebuild();                     // this avoids changing the cluster
              _ix->vk.BindBufferMemory(_ix->vk, *((ixvkBuffer *)res)->handle, *res->segment->memory, ((ixvkBuffer *)res)->handle->offset);

            /// normal resources move
            } else {

              if(res->classT== ixClassT::VKBUFFER)
                qres= ((ixvkBuffer *)res)->access.qFamily;
              else if(res->classT== ixClassT::VKIMAGE)
                qres= ((ixvkImage *)res)->access[0].qFamily; // an image array with different queues will screw this <<<<<<<<<<<<<<<<< but this is a rare case
              getQueueCmd(qres, &q, &cmd);

              cmd->pool->reset();
              cmd->startRecording();

              /// buffer
              if(res->classT== ixClassT::VKBUFFER) {
                if(res->defragDelta)
                  moveBuffer(*cmd, (ixvkBuffer *)res);
                else if(res->defragDeltaInside)
                  moveInside(*cmd, (ixvkBuffer *)res);

              /// image
              } else if(res->classT== ixClassT::VKIMAGE)
                moveImage(*cmd, (ixvkImage *)res);
              else error.window("unkown resource type", true);

              cmd->endRecording();
              cmd->_submitInfo.signalSemaphoreCount= cmd->_submitInfo.waitSemaphoreCount= 0;
              cmd->submit(*q);
              cmd->_submitInfo.signalSemaphoreCount= cmd->_submitInfo.waitSemaphoreCount= 1;
              _ix->vk.QueueWaitIdle(*q);
            } /// normal resource

            afterSubmitJobs(false);  /// signal not to use fence

            /// update segment freespace
            segment->updateFreespace();
            --job, --in_n;
            if(job== 0 || in_n== 0) return;
            
          } /// a defrag hast o happen on this resource
        } /// pass thru all clusters/segments/resources
    } /// one job / multiple jobs
}


void ixVulkan::Defrag::afterSubmitJobs(bool in_useFence) {
  if(destroyBuffersList->nrNodes || resToUpdate) {
    uint32 fi= _ix->vki.fi;
    if(in_useFence) {
      _ix->vk.WaitForFences(_ix->vk, 1, &fenceSignal[fi]->fence, true, ~0);
      _ix->vk.ResetFences(_ix->vk, 1, &fenceSignal[fi]->fence);
    }

    // destroy old buffers that were moved
    while(destroyBuffersList[fi].nrNodes) {
      if(((DBuffer *)(destroyBuffersList[fi].first))->buf)
        _ix->vk.DestroyBuffer(_ix->vk, ((DBuffer *)destroyBuffersList[fi].first)->buf, _ix->vk);
      if(((DBuffer *)(destroyBuffersList[fi].first))->img)
        _ix->vk.DestroyImage(_ix->vk, ((DBuffer *)destroyBuffersList[fi].first)->img, _ix->vk);
      destroyBuffersList[fi].del(destroyBuffersList[fi].first);
    }

    // update sets of buffers that were moved
    if(resToUpdate) {
      /// re-create view if it had one
      if(resToUpdate->classT== ixClassT::VKIMAGE)
        if(((ixvkImage *)resToUpdate)->view)
          ((ixvkImage *)resToUpdate)->createView();
      resToUpdate->updateSets();
      resToUpdate= null;
    }
  }
}



void ixVulkan::Defrag::moveBuffer(VkCommandBuffer in_cmd, ixvkBuffer *out_b) {
  VkBuffer oldBuffer= out_b->handle->buffer;
  uint64 oldOffset= *out_b->offset();

  /// new buffer setup vars, get it ready for transfer
  out_b->handle->offset-= out_b->defragDelta;
  out_b->defragDelta= 0;

  out_b->handle->buffer= 0;
  out_b->handle->build();           // this avoids changing the cluster
  _ix->vk.BindBufferMemory(_ix->vk, *out_b->handle, *out_b->segment->memory, out_b->handle->offset);

  // copy
  uint64 size= *out_b->size(); //handle->createInfo.size; //memRequirements.size;
  //uint64 size= out_b->mem()->size;
  uint64 offset= 0;
  
  for(ixvkResource *r= (ixvkResource *)out_b->segment->resources.first; r; r= (ixvkResource *)r->next) {
  //for(uint a= 0; a< out_b->segment->resources.nrNodes; a++) {
    if(r== out_b) continue;
    uint64 s1= *out_b->offset();
    uint64 e1= s1+ out_b->mem()->size- 1;
    uint64 s2= *r->offset();
    uint64 e2= s2+ r->mem()->size- 1;
    bool intersection= false;
    if(s1>= s2 && s1< e2)
      intersection= true;
    if(e1>= s2 && e1< e2)
      intersection= true;
    //if(*out_b->offset()>= *r->offset() && (*out_b->offset()< *r->offset()+ r->mem()->size))
    //if(*out_b->offset()+ out_b->mem()->size>= *r->offset() && (*out_b->offset()+ out_b->mem()->size< *r->offset()+ r->mem()->size))
      if(intersection) error.window("intersection!", true);
    
  }





  while(size) {
    uint64 copySize= (size> *_ix->vki.stageDevice->size()? *_ix->vki.stageDevice->size(): size);
    VkBufferCopy region= { offset, 0,      copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, oldBuffer, _ix->vki.stageDevice->handle->buffer, 1, &region);
    _ix->vki.stageDevice->barrier(_ix, in_cmd, VK_ACCESS_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    that solved it. so the cmd commands were done in same time. BARRIERS<<<<<
      omg i found it ... and it's 03:16 in the morning
since this is a for, there has to be more barriers
    //out_b->barrier(_ix, in_cmd, VK_ACCESS_MEMORY_WRITE
                 region= { 0,      offset, copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, _ix->vki.stageDevice->handle->buffer, out_b->handle->buffer, 1, &region);


    /* WORKS:
    uint64 copySize= size;
    VkBufferCopy region= { offset, 0,      copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, oldBuffer, out_b->handle->buffer, 1, &region);
    */


    /*
    uint64 copySize= (size> *_ix->vki.stageDevice->size()? *_ix->vki.stageDevice->size(): size);
    VkBufferCopy region= { offset, 0,      copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, oldBuffer,             *_ix->vki.stageDevice, 1, &region);
                 region= { 0,      offset, copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, *_ix->vki.stageDevice, *out_b,                1, &region);
    */

    /* ORIG:
    uint64 copySize= (size> *_ix->vki.stageDevice->size()? *_ix->vki.stageDevice->size(): size);
    VkBufferCopy region= { offset, 0,      copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, oldBuffer,             *_ix->vki.stageDevice, 1, &region);
                 region= { 0,      offset, copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, *_ix->vki.stageDevice, *out_b,                1, &region);
    */

    offset+= copySize;
    size-= copySize;
  }

  // finalize
  DBuffer *d= (DBuffer *)destroyBuffersList[_ix->vki.fi].add();
  d->buf= oldBuffer;
  d->img= 0;
}




void ixVulkan::Defrag::moveImage(VkCommandBuffer in_cmd, ixvkImage *out_i) {
  VkImageLayout saveLayout= out_i->access[0].layout;
  uint32 saveFamily=        out_i->access[0].qFamily;
  VkImage oldImage=         out_i->handle->image;

  // SEMAPHORES WILL AVOID ANY NEED FOR BARRIERS.

  /// populate _defragBuf
  _buf->handle->cfgSize(out_i->handle->memRequirements.size, out_i->handle->offset);
  
  _buf->handle->build();        // HANDLE, NOT DIRECT BUILD
  _buf->segment= out_i->segment;
  _ix->vk.BindBufferMemory(_ix->vk, *_buf, *out_i->segment->memory, out_i->handle->offset);
  _buf->defragDelta= out_i->defragDelta;

  // move by buffers
 
  moveBuffer(in_cmd, _buf);
  
  out_i->handle->offset-= out_i->defragDelta;
  out_i->defragDelta= 0;
  

  out_i->handle->cfgInitialLayout(VK_IMAGE_LAYOUT_PREINITIALIZED);
  out_i->handle->image= 0;
  out_i->handle->build();       // HANDLE, NOT DIRECT BUILD
  _ix->vk.BindImageMemory(_ix->vk, *out_i, *out_i->segment->memory, out_i->handle->offset);


  /// layout change
  out_i->access[0].layout= VK_IMAGE_LAYOUT_PREINITIALIZED;
  out_i->barrierRange(_ix, in_cmd, 0, out_i->handle->createInfo.arrayLayers, saveLayout, 0, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
  out_i->access[0].layout= saveLayout;

  DBuffer *d= (DBuffer *)destroyBuffersList[_ix->vki.fi].add();
  d->buf= _buf->handle->buffer;
  d->img= oldImage;

  _buf->handle->buffer= 0;
  _buf->segment= null;
  _buf->defragDelta= 0;
}









void ixVulkan::Defrag::moveInside(VkCommandBuffer in_cmd, ixvkBuffer *out_b) {
  //if(out_b->defragOffset== ~0)                { error.detail("a whole buffer move is needed, not inside move", __FUNCTION__); return; }
  if(out_b->defragSize== 0)                         { error.detail("defrag size is 0", __FUNCTION__); return; }
  if(out_b->defragDeltaInside== 0)                  { error.detail("defrag delta is 0", __FUNCTION__); return; }
  if(out_b->defragDeltaInside> out_b->defragOffset) { error.detail("defrag delta is bigger than the offset, the moving will be out of bounds", __FUNCTION__); return; }

  //out_b->barrierRange(in_cmd, out_b->defragOffset- out_b->defragDeltaInside, out_b->defragSize+ out_b->defragDeltaInside,
  //                    out_b->access.lastAccess, VK_ACCESS_TRANSFER_READ_BIT,
  //                    out_b->access.lastStage,  VK_PIPELINE_STAGE_TRANSFER_BIT);

  // copy
  
  uint64 size= out_b->defragSize;
  uint64 offset= 0;
  while(size) {
    uint64 copySize= (size> _ix->vki.stageDevice->handle->memRequirements.size? _ix->vki.stageDevice->handle->memRequirements.size: size);
    VkBufferCopy region= { offset+ out_b->defragOffset, offset, copySize };
    _ix->vk.CmdCopyBuffer(in_cmd, *out_b, *_ix->vki.stageDevice, 1, &region);

    region.srcOffset= offset, region.dstOffset= offset+ out_b->defragOffset- out_b->defragDeltaInside;
    _ix->vk.CmdCopyBuffer(in_cmd, *_ix->vki.stageDevice, *out_b, 1, &region);

    offset+= size;
    size-= copySize;
  }

  //out_b->barrierRange(_ix, in_cmd, out_b->defragOffset- out_b->defragDeltaInside, out_b->defragSize+ out_b->defragDeltaInside,
  //                   VK_ACCESS_TRANSFER_WRITE_BIT,   out_b->access.firstAccess,
  //                    VK_PIPELINE_STAGE_TRANSFER_BIT, out_b->access.firstStage);

  // finalize

  out_b->defragDeltaInside= 0;
  out_b->defragSize= 0;
  out_b->defragOffset= 0;
}





























//   ####    ######    ########  ##    ##    ####
// ##    ##  ##    ##     ##     ##    ##  ##    ##
// ##    ##  ######       ##     ########  ##    ##
// ##    ##  ##    ##     ##     ##    ##  ##    ##
//   ####    ##    ##     ##     ##    ##    ####

///================================================///

void ixVulkan::Ortho::init() {
  
  for(uint a= 0; a< 2; a++) {
    pool[a]= _ix->vk.objects.addCommandPool();
    pool[a]->configure(_ix->vki.q1->family);
    pool[a]->build();

    cmd[a]= pool[a]->addCommandBuffer();
    cmd[a]->cfgLevel(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    cmd[a]->cfgUsage(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT| VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
    cmd[a]->cfgInheritance(*_ix->vki.render.handle, 0, 0, false, 0, 0);
    cmd[a]->build();
  }
}











void ixvkResource::updateSets() {
  for(Set *s= (Set *)_sets.first; s; s= (Set *)s->next)
    s->set->update();
}





// ######    ##    ##  ########  ########  ########  ######
// ##    ##  ##    ##  ##        ##        ##        ##    ##
// ######    ##    ##  ######    ######    ######    ######
// ##    ##  ##    ##  ##        ##        ##        ##    ##
// ######      ####    ##        ##        ########  ##    ##

///==========================================================///


ixvkBuffer::ixvkBuffer(ixvkResCluster *in_p): ixvkResource(in_p), handle(null) {
  classT= ixClassT::VKBUFFER;
  //type= 0;
  handle= in_p->_ix->vk.objects.addBuffer();

  /// family handling
  if(in_p->_ix->cfg.vk.resourcesUseSharedQueueFamilies) {
    handle->cfgSharingMode(VK_SHARING_MODE_CONCURRENT);
    handle->cfgAddFamily(in_p->_ix->vki.q1->family);
    if(in_p->_ix->vki.qTransfer->family!= in_p->_ix->vki.q1->family)
      handle->cfgAddFamily(in_p->_ix->vki.qTransfer->family);
    if(in_p->_ix->vki.qCompute->family!= in_p->_ix->vki.q1->family)
      handle->cfgAddFamily(in_p->_ix->vki.qCompute->family);
  }

  //handle->cfgUsage <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< MUST MAYBE AUTO_MAKE TRANSFER_DST+SRC????????

  //offset= &handle->offset;
  //sizeMem= &handle->memRequirements.size;
}

ixvkBuffer::~ixvkBuffer() {
  if(handle) { cluster->_ix->vk.objects.delBuffer(handle); handle= null; }
}


bool ixvkBuffer::build() {
  if(!handle->build()) { error.detail(cluster->_ix->vk.errorText, __FUNCTION__); return false; }
  return cluster->allocRes(this);
}


void ixvkBuffer::destroy() {
  if(handle) handle->destroy();
  cluster->delResource(this, false);
}



/// barrier for whole buffer
void ixvkBuffer::barrier(Ix *in_ix, VkCommandBuffer in_cmd, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_oldFamily, uint32 in_newFamily) {
  VkBufferMemoryBarrier barrier;
    barrier.sType= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext= null;
    barrier.buffer= *handle;
    barrier.offset= 0;                // = handle->offset; // "this is relative to the base offset as bound to the buffer"
    barrier.size= VK_WHOLE_SIZE; //handle->memRequirements.size;
    barrier.srcAccessMask= in_srcAccess;
    barrier.dstAccessMask= in_dstAccess;
    barrier.srcQueueFamilyIndex= in_oldFamily;
    barrier.dstQueueFamilyIndex= in_newFamily;
  in_ix->vk.CmdPipelineBarrier(in_cmd, in_srcStage, in_dstStage, 0, 0, null, 1, &barrier, 0, null);
}

/// barrier for a specific range of the buffer
/// <in_offset> specs: "this is relative to the base offset as bound to the buffer"
void ixvkBuffer::barrierRange(Ix *in_ix, VkCommandBuffer in_cmd, VkDeviceSize in_offset, VkDeviceSize in_size, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_oldFamily, uint32 in_newFamily) {
  VkBufferMemoryBarrier barrier;
    barrier.sType= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext= null;
    barrier.buffer= *handle;
    barrier.offset= in_offset;              // "this is relative to the base offset as bound to the buffer"
    barrier.size= in_size;
    barrier.srcAccessMask= in_srcAccess;
    barrier.dstAccessMask= in_dstAccess;
    barrier.srcQueueFamilyIndex= in_oldFamily;
    barrier.dstQueueFamilyIndex= in_newFamily;
  in_ix->vk.CmdPipelineBarrier(in_cmd, in_srcStage, in_dstStage, 0, 0, null, 1, &barrier, 0, null);
}



bool ixvkBuffer::upload(void *in_data, VkDeviceSize in_offset, VkDeviceSize in_size) {
  const char *err= null;
  int errL;
  bool needStageBuffer= !(segment->memory->typeFlags& VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  void *bufMap;
  VkoQueue *q= null;                /// currently used queue
  VkoCommandBuffer *cmd= null;      /// cmdBuffer compatible with current queue
  Ix *_ix= this->_ix();

  if(in_data== null) IXERR("<in_data> is null");
  if(in_size== 0) IXERR("<in_size> is 0");
  if(_ix->vk.device== VK_NULL_HANDLE) IXERR("ix's vulkan device is not built");

  /// queue release + save current queue
  if(access.qFamily!= VK_QUEUE_FAMILY_IGNORED)
    if(_ix->vki.switchFamily && (access.qFamily!= _ix->vki.qTransfer->family)) {
      _ix->vki.getToolCmdAndQueue(access.qFamily, &q, &cmd);
      cmd->pool->reset();
      cmd->startRecording();    
      barrierRange(_ix, *cmd, in_offset, in_size, VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                  q->family,                                             _ix->vki.qTransfer->family);
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }


  if(needStageBuffer) {
    // copy data - in chunks of what the stage buffer can handle
    _ix->vki.cmdTrn->pool->reset();
    _ix->vki.cmdTrn->startRecording();

    /// barrier if the buffer is already being used
    if(access.qFamily!= VK_QUEUE_FAMILY_IGNORED)
      barrierRange(_ix, *_ix->vki.cmdTrn, in_offset, in_size, VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                              VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                   (q? q->family: VK_QUEUE_FAMILY_IGNORED), (q? _ix->vki.qTransfer->family: VK_QUEUE_FAMILY_IGNORED));

    VkDeviceSize copyOffset= 0, bytesToCopy= in_size;
    
    while(bytesToCopy> 0) {
      VkDeviceSize size= (bytesToCopy> _ix->vki.stageHost->handle->createInfo.size? _ix->vki.stageHost->handle->createInfo.size: bytesToCopy);  /// size, as big as the stage can handle
      /// [host]->[stage buffer]
      _ix->vk.MapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory, _ix->vki.stageHost->handle->offset, size, 0, &bufMap);
      ixMemcpy(bufMap, (uint8 *)in_data+ copyOffset, size);
      _ix->vk.UnmapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory);
      /// [stage buffer]->[device]
      VkBufferCopy region= { 0, in_offset+ copyOffset, size }; // src_offset, dst_offset, size
      _ix->vk.CmdCopyBuffer(*_ix->vki.cmdTrn, *_ix->vki.stageHost->handle, *handle, 1, &region);


      bytesToCopy-= size;
      copyOffset+= size;

      if(bytesToCopy) {
        _ix->vki.cmdTrn->endRecording();
        _ix->vki.cmdTrn->submit(*_ix->vki.qTransfer);
        _ix->vk.QueueWaitIdle(*_ix->vki.qTransfer);
        _ix->vki.cmdTrn->pool->reset();
        _ix->vki.cmdTrn->startRecording();
      }
    } /// while there's data to copy

    barrierRange(_ix, *_ix->vki.cmdTrn, in_offset, in_size, VK_ACCESS_TRANSFER_WRITE_BIT,   VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT,
                                                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                            _ix->vki.qTransfer->family,     (q? q->family: _ix->vki.q1->family));

    _ix->vki.cmdTrn->endRecording();
    _ix->vki.cmdTrn->submit(*_ix->vki.qTransfer);
    _ix->vk.QueueWaitIdle(*_ix->vki.qTransfer);

    /// queue aquire
    if(!q)
      _ix->vki.getToolCmdAndQueue(_ix->vki.q1->family, &q, &cmd);

    if(_ix->vki.switchFamily && (q->family!= _ix->vki.qTransfer->family)) {
      cmd->pool->reset();
      cmd->startRecording();
      barrierRange(_ix, *cmd, in_offset, in_size, VK_ACCESS_TRANSFER_WRITE_BIT,   VK_ACCESS_MEMORY_READ_BIT| VK_ACCESS_MEMORY_WRITE_BIT,
                                                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                  _ix->vki.qTransfer->family,     q->family);
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }

    access.qFamily= q->family;

  // no need for stage buffer
  } else {
    _ix->vk.MapMemory(_ix->vk, *segment->memory, handle->offset+ in_offset, in_size, 0, &bufMap);
    ixMemcpy(bufMap, in_data, in_size);
    _ix->vk.UnmapMemory(_ix->vk, *segment->memory);
  }

Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);
    return false;
  } else return true;
}


bool ixvkBuffer::download(void *out_data, VkDeviceSize in_offset, VkDeviceSize in_size) {
  const char *err= null;
  int errL;
  bool needStageBuffer= !(segment->memory->typeFlags& VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  void *deviceMap;
  VkoQueue *q= null;                /// currently used queue
  VkoCommandBuffer *cmd= null;      /// cmdBuffer compatible with current queue
  Ix *_ix= this->_ix();

  if(out_data== null) IXERR("<out_data> is null. must be allocated first");
  if(in_size== 0) IXERR("<in_size> is 0");
  if(_ix->vk.device== VK_NULL_HANDLE) IXERR("ix's vulkan device is not built");

  /// queue release + save current queue
  if(access.qFamily!= VK_QUEUE_FAMILY_IGNORED)
    if(_ix->vki.switchFamily && (access.qFamily!= _ix->vki.qTransfer->family)) {
      _ix->vki.getToolCmdAndQueue(access.qFamily, &q, &cmd);
      cmd->pool->reset();
      cmd->startRecording();    
      barrierRange(_ix, *cmd, in_offset, in_size, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                  q->family,  _ix->vki.qTransfer->family);
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }

  
  if(needStageBuffer) {
    // copy data - in chunks of what the stage buffer can handle
    _ix->vki.cmdTrn->pool->reset();
    _ix->vki.cmdTrn->startRecording();

    /// barrier if the buffer is already being used
    if(access.qFamily!= VK_QUEUE_FAMILY_IGNORED)
      barrierRange(_ix, *_ix->vki.cmdTrn, in_offset, in_size, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                   (q? q->family: VK_QUEUE_FAMILY_IGNORED), (q? _ix->vki.qTransfer->family: VK_QUEUE_FAMILY_IGNORED));


    //for(VkDeviceSize copyOffset= 0, bytesToCopy= in_size; bytesToCopy> 0; bytesToCopy-= size, copyOffset+= size) {
    VkDeviceSize copyOffset= 0, bytesToCopy= in_size;
    
    while(bytesToCopy> 0) {
      VkDeviceSize copySize= MIN(_ix->vki.stageHost->handle->createInfo.size, bytesToCopy); /// amount to copy at once, as big as the stage can handle
      
      /// device->stage buffer
      VkBufferCopy region= { in_offset+ copyOffset, 0, copySize }; // src_offset, dst_offset, size
      _ix->vk.CmdCopyBuffer(*_ix->vki.cmdTrn, *handle, *_ix->vki.stageHost->handle, 1, &region);
      /// stage buffer->host
      _ix->vk.MapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory, _ix->vki.stageHost->handle->offset, copySize, 0, &deviceMap);
      ixMemcpy((uint8 *)out_data+ copyOffset, deviceMap, copySize);
      _ix->vk.UnmapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory);

      bytesToCopy-= copySize;
      copyOffset+= copySize;
      if(bytesToCopy) {
        _ix->vki.cmdTrn->endRecording();
        _ix->vki.cmdTrn->submit(*_ix->vki.qTransfer);
        _ix->vk.QueueWaitIdle(*_ix->vki.qTransfer);
        _ix->vki.cmdTrn->pool->reset();
        _ix->vki.cmdTrn->startRecording();
      }
    }

    barrierRange(_ix, *_ix->vki.cmdTrn, in_offset, in_size, VK_ACCESS_TRANSFER_READ_BIT,    VK_ACCESS_MEMORY_WRITE_BIT,
                                                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                            _ix->vki.qTransfer->family,     (q? q->family: _ix->vki.q1->family));

    _ix->vki.cmdTrn->endRecording();
    _ix->vki.cmdTrn->submit(*_ix->vki.qTransfer);
    _ix->vk.QueueWaitIdle(*_ix->vki.qTransfer);

    /// queue aquire
    if(!q)
      _ix->vki.getToolCmdAndQueue(_ix->vki.q1->family, &q, &cmd);

    if(_ix->vki.switchFamily && (q->family!= _ix->vki.qTransfer->family)) {
      cmd->pool->reset();
      cmd->startRecording();
      barrierRange(_ix, *cmd, in_offset, in_size, VK_ACCESS_TRANSFER_READ_BIT,    VK_ACCESS_MEMORY_WRITE_BIT,
                                                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                  _ix->vki.qTransfer->family,     q->family);
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }
    access.qFamily= q->family;

  // no need for stage buffer
  } else {
    _ix->vk.MapMemory(_ix->vk, *segment->memory, handle->offset+ in_offset, in_size, 0, &deviceMap);
    ixMemcpy(out_data, deviceMap, in_size);
    _ix->vk.UnmapMemory(_ix->vk, *segment->memory);
  }

Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);
    return false;
  } else return true;
}


// ########  ##    ##    ####      ####    ########
//    ##     ###  ###  ##    ##  ##        ##
//    ##     ## ## ##  ########  ##  ####  ######
//    ##     ##    ##  ##    ##  ##    ##  ##
// ########  ##    ##  ##    ##    ######  ########

///================================================///


ixvkImage::ixvkImage(ixvkResCluster *in_parent): ixvkResource(in_parent), handle(null) {
  classT= ixClassT::VKIMAGE;
  //type= 1;
  handle= in_parent->_ix->vk.objects.addImage();

  /// family handling
  if(in_parent->_ix->cfg.vk.resourcesUseSharedQueueFamilies) {
    handle->cfgSharingMode(VK_SHARING_MODE_CONCURRENT);
    handle->cfgAddFamily(in_parent->_ix->vki.q1->family);
    if(in_parent->_ix->vki.qTransfer->family!= in_parent->_ix->vki.q1->family)
      handle->cfgAddFamily(in_parent->_ix->vki.qTransfer->family);
    if(in_parent->_ix->vki.qCompute->family!= in_parent->_ix->vki.q1->family)
      handle->cfgAddFamily(in_parent->_ix->vki.qCompute->family);
  }
  
  //offset= &handle->offset;
  //sizeMem= &handle->memRequirements.size;
  access= null;                   // INIT 1



  view= VK_NULL_HANDLE;

  viewInfo= {};
  viewInfo.sType= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.pNext= nullptr;
  viewInfo.flags= 0;
}



ixvkImage::~ixvkImage() {
  if(handle) { _ix()->vk.objects.delImage(handle); handle= null; }
  if(view) { _ix()->vk.DestroyImageView(_ix()->vk, view, _ix()->vk); view= 0; }
  if(access) { delete[] access; access= null; }    // DEALLOC 1
  if(segment) cluster->delResource(this, false);
}





void ixvkImage::cfgSize(VkExtent3D in_size, uint32 in_layers, uint32 in_mips) {
  handle->createInfo.extent= in_size,
  handle->createInfo.arrayLayers= in_layers;
  if(in_mips== ~0u) {
    /// manual compute mipmaps
    handle->createInfo.mipLevels= 1;
    for(uint b= MAX(MAX(in_size.width, in_size.height), in_size.depth); b!= 1; b/= 2)
      handle->createInfo.mipLevels++;

  } else
    handle->createInfo.mipLevels= in_mips;

  if(access) { delete[] access; access= null; }
  access= new Access[in_layers];    // ALLOC 1
}


bool ixvkImage::build() {
  for(uint a= 0; a< handle->createInfo.arrayLayers; a++)
    access[a].layout= handle->createInfo.initialLayout;

  if(!handle->build()) { error.detail(_ix()->vk.errorText, __FUNCTION__, __LINE__); return false; }

  // if the cluster segment size is left 0, this signals that this class must fill it in, with the size as big as this image mem requirements size
  if(cluster->segmentSize== 0) {
    cluster->segmentSize= handle->memRequirements.size;
  }

  return cluster->allocRes(this);
}


void ixvkImage::destroy() {
  if(handle) handle->destroy();
  if(segment) cluster->delResource(this, false);
}


void ixvkImage::createView(Tex *in_t) {
  viewInfo.image= handle->image;
  if(in_t) {
    viewInfo.viewType= (VkImageViewType)in_t->type;
    viewInfo.format=   (VkFormat)in_t->format;

    viewInfo.components.r= (VkComponentSwizzle)in_t->swizzR;
    viewInfo.components.g= (VkComponentSwizzle)in_t->swizzG;
    viewInfo.components.b= (VkComponentSwizzle)in_t->swizzB;
    viewInfo.components.a= (VkComponentSwizzle)in_t->swizzA;

    viewInfo.subresourceRange.aspectMask=     Img::vkGetAspectFromFormat(in_t->format);
    viewInfo.subresourceRange.baseMipLevel=   0;
    viewInfo.subresourceRange.levelCount=     in_t->nrLevels;
    viewInfo.subresourceRange.baseArrayLayer= 0;
    viewInfo.subresourceRange.layerCount=     1;
  }

  if(view) { _ix()->vk.DestroyImageView(_ix()->vk, view, _ix()->vk); view= 0; }

  if(!error.vkCheck(_ix()->vk.CreateImageView(_ix()->vk, &viewInfo, _ix()->vk, &view))) 
    error.detail("vkCreateImageView failed", __FUNCTION__);
}



/// one layer barrier
void ixvkImage::barrier(Ix *in_ix, VkCommandBuffer in_cmd, uint32 in_layer, VkImageLayout in_newLayout, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_oldFamily, uint32 in_newFamily) {
  VkImageSubresourceRange range;
  range.aspectMask= Img::vkGetAspectFromFormat((ImgFormat)(handle->createInfo.format));
  range.baseMipLevel= 0;
  range.levelCount= handle->createInfo.mipLevels;
  range.baseArrayLayer= in_layer;
  range.layerCount= 1;

  VkImageMemoryBarrier b;
  b.sType= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  b.pNext= null;
  b.image= handle->image;
  b.subresourceRange= range;
  b.oldLayout= access[in_layer].layout;
  b.newLayout= in_newLayout;
  b.srcAccessMask= in_srcAccess;
  b.dstAccessMask= in_dstAccess;
  b.srcQueueFamilyIndex= in_oldFamily;
  b.dstQueueFamilyIndex= in_newFamily;
  in_ix->vk.CmdPipelineBarrier(in_cmd, in_srcStage, in_dstStage, 0, 0, null, 0, null, 1, &b);
  //currentLayout= in_newLayout; <<< CANNOT HAPPEN DUE QUEUE SWITCH REQUIRES 2X CALLS ON THIS FUNC
}


/// multiple layers barrier
void ixvkImage::barrierRange(Ix *in_ix, VkCommandBuffer in_cmd, uint32 in_fromLayer, uint32 in_nrLayers, VkImageLayout in_newLayout, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_newFamily) {
  VkImageSubresourceRange range;
  range.aspectMask= Img::vkGetAspectFromFormat((ImgFormat)(handle->createInfo.format));
  range.baseMipLevel= 0;
  range.levelCount= handle->createInfo.mipLevels;
  range.baseArrayLayer= in_fromLayer;
  range.layerCount= in_nrLayers;

  VkImageMemoryBarrier b;
  b.sType= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  b.pNext= null;
  b.image= handle->image;
  b.subresourceRange= range;
  b.oldLayout= access[in_fromLayer].layout;
  b.newLayout= in_newLayout;
  b.srcAccessMask= in_srcAccess;
  b.dstAccessMask= in_dstAccess;
  b.srcQueueFamilyIndex= (in_newFamily== VK_QUEUE_FAMILY_IGNORED? VK_QUEUE_FAMILY_IGNORED: access[in_fromLayer].qFamily);
  b.dstQueueFamilyIndex= in_newFamily;
  in_ix->vk.CmdPipelineBarrier(in_cmd, in_srcStage, in_dstStage, 0, 0, null, 0, null, 1, &b);  
}

void ixvkImage::barrierRange2(Ix *in_ix, VkCommandBuffer in_cmd, VkImageSubresourceRange *in_range, VkImageLayout in_newLayout, VkAccessFlags in_srcAccess, VkAccessFlags in_dstAccess, VkPipelineStageFlags in_srcStage, VkPipelineStageFlags in_dstStage, uint32 in_newFamily) {
  VkImageMemoryBarrier b;
  b.sType= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  b.pNext= null;
  b.image= handle->image;
  b.subresourceRange= *in_range;
  b.oldLayout= access[in_range->baseArrayLayer].layout;
  b.newLayout= in_newLayout;
  b.srcAccessMask= in_srcAccess;
  b.dstAccessMask= in_dstAccess;
  b.srcQueueFamilyIndex= access[in_range->baseArrayLayer].qFamily;
  b.dstQueueFamilyIndex= in_newFamily;
  in_ix->vk.CmdPipelineBarrier(in_cmd, in_srcStage, in_dstStage, 0, 0, null, 0, null, 1, &b);
}










///  textures must fit in the stage buffer
/// making the stage buffer 20mb would fit 2048x2048 textures sizes
bool ixvkImage::upload(Tex *in_tex, uint32 in_layer, VkImageLayout in_layout, uint32 in_level, uint32 in_nlevels) {
  // depth/stencil cannot be done with transfer queue!!!
  // "Because depth or stencil aspect buffer to image copies may require format conversions on some implementations, they are not supported on queues that do not support graphics"

  /// vars
  cchar *err= null;
  int errL;
  void *bufMap;                               /// memory map pointer
  VkoQueue *q= null;                          /// currently used queue
  VkoCommandBuffer *cmd= null;                /// cmdBuffer compatible with current queue
  VkoQueue *copyQ;                            /// copy queue
  VkoCommandBuffer *copyCmd;                  /// copy command buffer
  uint32 aspect= Img::vkGetAspectFromFormat(in_tex->format);
  uint32 mx= in_tex->dx, my= in_tex->dy, mz= in_tex->depth;     /// mipmap level sizes
  Ix *_ix= this->_ix();

  if(in_tex->size> _ix->vki.stageHost->handle->createInfo.size)
    IXERR("Texture too big to fit in the stage buffer. Enlarge the stage buffer in engine configuration");

  /// copy queue - it can't be transfer on depth/stencil
  if((aspect& VK_IMAGE_ASPECT_DEPTH_BIT) || (aspect& VK_IMAGE_ASPECT_STENCIL_BIT))
    _ix->vki.getToolCmdAndQueue(access[in_layer].qFamily, &copyQ, &copyCmd);
  else 
    copyQ= _ix->vki.qTransfer, copyCmd= _ix->vki.cmdTrn;

  /// mipmap level req vars
  if(in_nlevels== 0)
    in_nlevels= in_tex->nrLevels- in_level;

  if(in_level!= 0)
    for(uint a= 0; a< in_level; a++)
      mx/= 2, my/= 2, mz/= 2;
  
  /// queue release + save current queue
  if(access[in_layer].qFamily!= VK_QUEUE_FAMILY_IGNORED)
    if(_ix->vki.switchFamily && (access[in_layer].qFamily!= copyQ->family)) {
      _ix->vki.getToolCmdAndQueue(access[in_layer].qFamily, &q, &cmd);
      cmd->pool->reset();
      cmd->startRecording();
      barrier(_ix, *cmd, in_layer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT,           VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                                         q->family,                            copyQ->family);                                                                    
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }

  // copy data - using stage buffer - non-linear image cannot be directly mapped
  if((!(segment->memory->typeFlags& VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) || (handle->createInfo.tiling!= VK_IMAGE_TILING_LINEAR)) {
    copyCmd->pool->reset();
    copyCmd->startRecording();

    /// switching layout to transfer optimal [+queue aquire if needed]
    barrier(_ix, *copyCmd, in_layer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT,           VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        (q? q->family: VK_QUEUE_FAMILY_IGNORED), (q? copyQ->family: VK_QUEUE_FAMILY_IGNORED));
    access[in_layer].layout= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    /// special case, it's a new image, starting with ~0 as family. no aquire but cmd+q has to exist at this point
    if(access[in_layer].qFamily== VK_QUEUE_FAMILY_IGNORED)
      _ix->vki.getToolCmdAndQueue(_ix->vki.q1->family, &q, &cmd);

    /// copy vars
    uint32 copyOffset= 0;
    uint32 copySize= 0;

    VkBufferImageCopy copyRegion;
      copyRegion.bufferRowLength= 0;
      copyRegion.bufferImageHeight= 0;
      copyRegion.imageSubresource.aspectMask= aspect;
      copyRegion.imageSubresource.layerCount= 1;
      copyRegion.imageOffset= { 0, 0, 0 };

    for(uint a= in_level; a< in_nlevels; a++)
      copySize+= in_tex->levSize[a];

    // copy [ixTex]->[stage buffer]    
    if(_ix->vk.MapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory, _ix->vki.stageHost->handle->offset, copySize, 0, &bufMap)!= VK_SUCCESS)
      IXERR("vkMapMemory failed.");
    ixMemcpy((uint8 *)bufMap, in_tex->bitmap+ in_tex->levFrom[in_level], copySize);
    _ix->vk.UnmapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory);

    // copy [stage buffer]->[vkImage]
    for(uint a= in_level; a< in_nlevels; a++, mx/= 2, my/= 2, mz/= 2) {
      mx= MAX(mx, 1), my= MAX(my, 1), mz= MAX(mz, 1);
      copyRegion.bufferOffset= copyOffset;
      copyRegion.imageSubresource.baseArrayLayer= in_layer;
      copyRegion.imageSubresource.mipLevel= a;
      copyRegion.imageExtent= { mx, my, mz };

      _ix->vk.CmdCopyBufferToImage(*copyCmd, *_ix->vki.stageHost->handle, *handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

      copyOffset+= in_tex->levSize[a];
    }

    /// final barier + queue release if needed
    barrier(_ix, *copyCmd, in_layer, in_layout, VK_ACCESS_TRANSFER_WRITE_BIT,   VK_ACCESS_MEMORY_WRITE_BIT,
                                                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                copyQ->family,                  (q? q->family: copyQ->family));
    copyCmd->endRecording();
    copyCmd->submit(*copyQ);
    _ix->vk.QueueWaitIdle(*copyQ);

    /// queue aquire if needed
    if(q) {
      cmd->pool->reset();
      cmd->startRecording();
      barrier(_ix, *cmd, in_layer, in_layout, VK_ACCESS_TRANSFER_WRITE_BIT,   VK_ACCESS_MEMORY_WRITE_BIT,
                                              VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                              copyQ->family,                  q->family);
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }


  // copy data - direct map - tilling must be VK_IMAGE_TILING_LINEAR
  /// sync would be made with a fence... after the fence you do the upload
  } else {
    if(access[in_layer].qFamily!= VK_QUEUE_FAMILY_IGNORED)
      _ix->vki.getToolCmdAndQueue(access[in_layer].qFamily, &q, &cmd);
    else
      _ix->vki.getToolCmdAndQueue(_ix->vki.qTool->family, &q, &cmd);

    /// layout change
    cmd->pool->reset();
    cmd->startRecording();
    barrier(_ix, *cmd, in_layer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT,           VK_ACCESS_MEMORY_WRITE_BIT,
                                                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);  // <<<NOT SURE
    access[in_layer].layout= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    cmd->endRecording();
    cmd->submit(*q);
    _ix->vk.QueueWaitIdle(*q);


    VkSubresourceLayout layout; // i think layout.arrayPitch is if you wanna map the whole level
    VkImageSubresource subRes;
    subRes.aspectMask= aspect;
    subRes.arrayLayer= in_layer;
    subRes.mipLevel= in_level;

    for(uint a= in_level; a< in_nlevels; a++) {
      subRes.mipLevel= a;
      _ix->vk.GetImageSubresourceLayout(_ix->vk, *handle, &subRes, &layout );
      _ix->vk.MapMemory(_ix->vk, *segment->memory, handle->offset+ layout.offset, layout.size, 0, &bufMap);  /// i hope i didn't get this wrong. linear image layout: mips/arrays/depths/y/x
      
      uint32 texRowSize= in_tex->dx* (in_tex->bpp/ 8);

      for(uint b= 0; b< in_tex->depth; b++) {
        uint8 *mapRow= (uint8 *)bufMap;
        uint8 *texRow= in_tex->bitmap+ in_tex->levFrom[a];
        for(uint c= 0; c< in_tex->dy; c++) {
          ixMemcpy(mapRow, texRow, texRowSize);
          mapRow+= layout.rowPitch;
          texRow+= texRowSize;
        }
        bufMap= (uint8 *)bufMap+ layout.depthPitch;
      }
      _ix->vk.UnmapMemory(_ix->vk, *segment->memory);
    }
    
    /// layout change
    cmd->pool->reset();
    cmd->startRecording();
    
    barrier(_ix, *cmd, in_layer, in_layout, VK_ACCESS_MEMORY_WRITE_BIT,        VK_ACCESS_MEMORY_WRITE_BIT,
                                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);     // <<< NOT SURE
    cmd->endRecording();
    cmd->submit(*q);
    _ix->vk.QueueWaitIdle(*q);
  }

  access[in_layer].qFamily= (q? q->family: copyQ->family);
  access[in_layer].layout= in_layout;

Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);

    return false;
  } else return true;
}



///  textures must fit in the stage buffer
/// making the stage buffer 20mb would fit 2048x2048 textures sizes
bool ixvkImage::download(Tex *out_tex, uint32 in_layer, uint32 in_level, uint32 in_nlevels) {
  // depth/stencil cannot be done with transfer queue!!!
  // "Because depth or stencil aspect buffer to image copies may require format conversions on some implementations, they are not supported on queues that do not support graphics"

  /// vars
  Ix *_ix= this->_ix();
  cchar *err= null;
  int errL;
  void *bufMap;                               /// memory map pointer
  VkoQueue *q= null;                          /// currently used queue
  VkoCommandBuffer *cmd= null;                /// cmdBuffer compatible with current queue
  VkoQueue *copyQ;                            /// copy queue
  VkoCommandBuffer *copyCmd;                  /// copy command buffer
  uint32 aspect= Img::vkGetAspectFromFormat((ImgFormat)handle->createInfo.format);
  uint32 mx= handle->createInfo.extent.width,
         my= handle->createInfo.extent.height,
         mz= handle->createInfo.extent.depth;     /// mipmap level sizes
  uint16 pixSize;
  VkImageLayout saveLayout= access[in_layer].layout;

  /// mipmap level req vars
  if(in_nlevels== 0)
    in_nlevels= handle->createInfo.mipLevels- in_level;

  if(in_level!= 0)
    for(uint a= 0; a< in_level; a++)
      mx/= 2, my/= 2, mz/= 2;
  mx= MAX(mx, 1), my= MAX(my, 1), mz= MAX(mz, 1);

  // populate out_tex params
  out_tex->delData();
  out_tex->type= (Img::Type)handle->createInfo.imageType;
  out_tex->format= (ImgFormat)handle->createInfo.format;
  out_tex->compressed= out_tex->compressed= Img::isFormatCompressed(out_tex->format);
  out_tex->dx= mx;
  out_tex->dy= my;
  out_tex->depth= mz;
  out_tex->nrLevels= in_nlevels;
  out_tex->size;
  out_tex->computePixelInfo();

  pixSize= out_tex->bpp/ 8;
  for(uint32 tmx= mx, tmy= my, tmz= mz, a= 0; a< in_nlevels; a++, tmx/= 2, tmy/= 2, tmz/= 2) {
    tmx= MAX(tmx, 1), tmy= MAX(tmx, 1), tmz= MAX(tmz, 1);
    out_tex->levFrom[a]= (a== 0? 0: out_tex->levFrom[a- 1]+ out_tex->levSize[a- 1]);
    out_tex->levSize[a]= (tmx* pixSize)* tmy* tmz;
    out_tex->size+= out_tex->levSize[a];
  }

  if(out_tex->size> _ix->vki.stageHost->handle->createInfo.size)
    IXERR("Texture too big to fit in the stage buffer. Enlarge the stage buffer in engine configuration");

  out_tex->bitmap= new uint8[out_tex->size];
  
  /// copy queue - it can't be transfer on depth/stencil
  if((aspect& VK_IMAGE_ASPECT_DEPTH_BIT) || (aspect& VK_IMAGE_ASPECT_STENCIL_BIT))
    _ix->vki.getToolCmdAndQueue(access[in_layer].qFamily, &copyQ, &copyCmd);
  else 
    copyQ= _ix->vki.qTransfer, copyCmd= _ix->vki.cmdTrn;

  /// queue release + save current queue
  if(access[in_layer].qFamily!= VK_QUEUE_FAMILY_IGNORED)
    if(_ix->vki.switchFamily && (access[in_layer].qFamily!= copyQ->family)) {
      _ix->vki.getToolCmdAndQueue(access[in_layer].qFamily, &q, &cmd);
      cmd->pool->reset();
      cmd->startRecording();
      barrier(_ix, *cmd, in_layer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT,           VK_ACCESS_TRANSFER_READ_BIT,
                                                                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                                         q->family,                            copyQ->family);                                                                    
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }

  // copy data - using stage buffer - non-linear image cannot be directly mapped
  if((!(segment->memory->typeFlags& VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) || (handle->createInfo.tiling!= VK_IMAGE_TILING_LINEAR)) {
    copyCmd->pool->reset();
    copyCmd->startRecording();

    /// barrier if the image is already being used - or the second barrier that signals queue aquisition
    if(access[in_layer].qFamily!= VK_QUEUE_FAMILY_IGNORED)
      barrier(_ix, *copyCmd, in_layer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT,           VK_ACCESS_TRANSFER_READ_BIT,
                                                                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         (q? q->family: VK_QUEUE_FAMILY_IGNORED), (q? copyQ->family: VK_QUEUE_FAMILY_IGNORED));
    access[in_layer].layout= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    // copy [vkImage]->[stage buffer]
    uint32 copyOffset= 0;
    VkBufferImageCopy copyRegion;
      copyRegion.bufferRowLength= 0;
      copyRegion.bufferImageHeight= 0;
      copyRegion.imageSubresource.aspectMask= aspect;
      copyRegion.imageSubresource.layerCount= 1;
      copyRegion.imageOffset= { 0, 0, 0 };
    
    for(uint a= 0; a< in_nlevels; a++, mx/= 2, my/= 2, mz/= 2) {
      mx= MAX(mx, 1), my= MAX(my, 1), mz= MAX(mz, 1);
      
      copyRegion.bufferOffset= copyOffset;
      copyRegion.imageSubresource.baseArrayLayer= in_layer;
      copyRegion.imageSubresource.mipLevel= a+ in_level;
      copyRegion.imageExtent= { mx, my, mz };

      _ix->vk.CmdCopyImageToBuffer(*copyCmd, *handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *_ix->vki.stageHost->handle, 1, &copyRegion);

      copyOffset+= out_tex->levSize[a];
    }

    // copy [stage buffer]->[ixTex]
    if(_ix->vk.MapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory, _ix->vki.stageHost->handle->offset, out_tex->size, 0, &bufMap)!= VK_SUCCESS)
      IXERR("vkMapMemory failed.");
    ixMemcpy(out_tex->bitmap, bufMap, out_tex->size);
    _ix->vk.UnmapMemory(_ix->vk, *_ix->vki.stageHost->segment->memory);

    /// final barier + queue release if needed
    barrier(_ix, *copyCmd, in_layer, saveLayout, VK_ACCESS_TRANSFER_READ_BIT,    VK_ACCESS_MEMORY_WRITE_BIT,
                                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                 copyQ->family,                  (q? q->family: copyQ->family));
    copyCmd->endRecording();
    copyCmd->submit(*copyQ);
    _ix->vk.QueueWaitIdle(*copyQ);

    /// queue aquire if needed
    if(q) {
      cmd->pool->reset();
      cmd->startRecording();
      barrier(_ix, *cmd, in_layer, saveLayout, VK_ACCESS_TRANSFER_READ_BIT,    VK_ACCESS_MEMORY_WRITE_BIT,
                                               VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                               copyQ->family,                  q->family);
      cmd->endRecording();
      cmd->submit(*q);
      _ix->vk.QueueWaitIdle(*q);
    }


  // copy data - direct map - tilling must be VK_IMAGE_TILING_LINEAR
  /// sync would be made with a fence... after the fence you do the upload
  } else {

    if(access[in_layer].qFamily!= VK_QUEUE_FAMILY_IGNORED)
      _ix->vki.getToolCmdAndQueue(access[in_layer].qFamily, &q, &cmd);
    else
      _ix->vki.getToolCmdAndQueue(_ix->vki.qTool->family, &q, &cmd);

    /// layout change
    cmd->pool->reset();
    cmd->startRecording();
    barrier(_ix, *cmd, in_layer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_WRITE_BIT,           VK_ACCESS_MEMORY_READ_BIT,
                                                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);      // <<< NOT SURE
    access[in_layer].layout= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    cmd->endRecording();
    cmd->submit(*q);
    _ix->vk.QueueWaitIdle(*q);

    
    VkSubresourceLayout layout; // i think layout.arrayPitch is if you wanna map the whole level
    VkImageSubresource subRes;
    subRes.aspectMask= aspect;
    subRes.arrayLayer= in_layer;
    subRes.mipLevel= in_level;

    for(uint a= 0; a< in_nlevels; a++) {
      subRes.mipLevel= a+ in_level;
      _ix->vk.GetImageSubresourceLayout(_ix->vk, *handle, &subRes, &layout );
      _ix->vk.MapMemory(_ix->vk, *segment->memory, handle->offset+ layout.offset, layout.size, 0, &bufMap);  /// i hope i didn't get this wrong. linear image layout: mips/arrays/depths/y/x

      uint32 texRowSize= out_tex->dx* (out_tex->bpp/ 8);

      for(uint b= 0; b< out_tex->depth; b++) {
        uint8 *mapRow= (uint8 *)bufMap;
        uint8 *texRow= out_tex->bitmap+ out_tex->levFrom[a];
        for(uint c= 0; c< out_tex->dy; c++) {
          //ixMemcpy(mapRow, texRow, texRowSize);
          ixMemcpy(texRow, mapRow, texRowSize);
          mapRow+= layout.rowPitch;
          texRow+= texRowSize;
        }
        bufMap= (uint8 *)bufMap+ layout.depthPitch;
      } /// for each depth
      _ix->vk.UnmapMemory(_ix->vk, *segment->memory);
    } /// for each level
    
    /// layout change
    cmd->pool->reset();
    cmd->startRecording();
    barrier(_ix, *cmd, in_layer, saveLayout, VK_ACCESS_MEMORY_READ_BIT,         VK_ACCESS_MEMORY_WRITE_BIT,
                                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);        // << NOT SURE
    cmd->endRecording();
    cmd->submit(*q);
    _ix->vk.QueueWaitIdle(*q);
  } /// stage buffer or no stage buffer

  access[in_layer].layout= saveLayout;
  access[in_layer].qFamily= (q? q->family: copyQ->family);

Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);
    out_tex->delData();

    return false;
  } else return true;
}
































// RESOURCE

//   ####    ##        ##    ##    ####    ########  ########  ######
// ##    ##  ##        ##    ##  ##           ##     ##        ##    ##
// ##        ##        ##    ##    ####       ##     ######    ######
// ##    ##  ##        ##    ##        ##     ##     ##        ##    ##
//   ####    ########    ####     #####       ##     ########  ##    ##

///====================================================================///



ixvkResCluster::ixvkResCluster(Ix *in_ix): ixClass(ixClassT::VKCLUSTER), _ix(in_ix) /*, add(in_ix, this)*/ {
  delData();
}

ixvkResCluster::~ixvkResCluster() {
  delData();
}

void ixvkResCluster::delData() {
  while(segments.first)
    segments.del(segments.first);

  segmentSize= 0;
  memReq=  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  memPref= 0;
  memTypeBits= ~0;
  
  defragBehaviour= 0; // WIP
}






inline ixvkResClusterSegment *ixvkResCluster::_getSegment(VkMemoryRequirements *in_mem) {
  // find a segment with enough freespace
  ixvkResClusterSegment *s= (ixvkResClusterSegment *)segments.first;

  /// no alignment required
  if(in_mem->alignment<= 1) {
    for(; s; s= (ixvkResClusterSegment *)s->next)
      if(s->freeSize>= in_mem->size)
        break;

  /// alignment required
  } else {
    for(; s; s= (ixvkResClusterSegment *)s->next) {
      //uint32 m= s->freeOffset% in_mem.alignment;                      /// [m] holds the rest of the division by alignment
      //uint32 algnOffset= s->freeOffset+ (m? in_mem.alignment- m: 0);  /// aligned offset
      //uint32 delta= algnOffset- s->freeOffset;                        /// difference between [alligned offset] and [offset]

      uint32 delta= _getAlignOffsetDelta(s->freeOffset, in_mem->alignment);
      if(((s->freeSize- delta)>= in_mem->size) && (s->freeSize> delta))
        break;
    }
  }

  // no segment with freespace - add one
  if(s== null) {
    s= new ixvkResClusterSegment(this);
    if(!s->build()) { error.window("ixvkResClusterSegment *ixvkResCluster::_getSegment(): ixvkResClusterSegment::build() failed."); return null; }
    segments.add(s);

    if(s->freeSize< in_mem->size) {
      error.window("ixvkResClusterSegment *ixvkResCluster::_getSegment(): CRITICAL MEMORY CLUSTER ERROR: requested memory size is bigger than configured cluster segment size");
      return null;
    }
  }


  return s;
}





void ixvkResCluster::delResource(ixvkResource *out_r, bool in_del) {
  ixvkResClusterSegment *s= out_r->segment;

  /// remove current jobs that his resource had (can have 2 - inside/wholesize)
  if(out_r->defragDelta)       _ix->vki.defrag.job--;
  if(out_r->defragDeltaInside) _ix->vki.defrag.job--;

  // delete the only resource allocated - no defrag
  if(s->resources.nrNodes== 1 && s->resources.first== out_r) {
    s->freeOffset= 0;
    s->freeSize= segmentSize;

    if(in_del) out_r->segment->resources.del(out_r);
    else       out_r->segment->resources.release(out_r);

  // delete the last resource in the segment - no defrag
  } else if(out_r== s->resources.last) {
    ixvkResource *lastPrev= (ixvkResource *)(s->resources.last->prev);
    s->freeOffset= (*lastPrev->offset())+ lastPrev->mem()->size;
    s->freeSize= segmentSize- s->freeOffset;
    
    if(in_del) out_r->segment->resources.del(out_r);
    else       out_r->segment->resources.release(out_r);

  // middle of pack delete ... defrag has to happen
  } else {

    /// s->freeOffset + s->freeSize must be computed when defrag happens

    ixvkResource *tmp= (ixvkResource *)out_r->prev;

    ixvkResource *p= (ixvkResource *)out_r->next;
    if(in_del) out_r->segment->resources.del(out_r);
    else       out_r->segment->resources.release(out_r);
    
    /// defrag data compute, for all resources past the deleted one
    for(; p; p= (ixvkResource *)p->next) {
      if(p== _ix->vki.defrag._buf)
        continue;          // defrag's buffer must be skipped

      bool itAlreadyHasJob= (p->defragDelta? true: false);

      if(p->prev) {
        ixvkResource *prv= (ixvkResource *)p->prev;
        uint32 alignment= (uint32)p->mem()->alignment;
        VkDeviceSize newOffset= _getAlignedOffset(*(prv->offset())- prv->defragDelta+ prv->mem()->size, alignment);
        p->defragDelta+= (uint32)(*(p->offset())- newOffset);

      } else
        p->defragDelta=  (uint32)*p->offset();        // move to beginning

      /// this logic is to avoid going thru all resources and counting all jobs, but i fear that might be necesary, in the end...
      if(itAlreadyHasJob && !p->defragDelta)          // somehow it don't need no job anymore
        _ix->vki.defrag.job--;
      if(!itAlreadyHasJob && p->defragDelta)          // it has a need for a job now
        _ix->vki.defrag.job++;

    } /// for each resource after deleted
  }
}



bool ixvkResCluster::allocRes(ixvkResource *out_r) {
  ixvkResClusterSegment *s= null;

  /// find a segment with enough freespace
  s= _getSegment(out_r->mem());
  if(s== null) { error.detail("Fatal Error: requested memory is too big for the cluster segment size, or vukan alloc failed", __FUNCTION__); return false; }

  if(out_r->classT== ixClassT::VKBUFFER) {
    if(!((ixvkBuffer *)out_r)->handle->isMemoryCompatible(s->memory)) { error.detail("segment memory is not compatible with new buffer", __FUNCTION__); return false; }
  } else if(out_r->classT== ixClassT::VKIMAGE) {
    if(!((ixvkImage *)out_r)->handle->isMemoryCompatible(s->memory)) { error.detail("segment memory is not compatible with new buffer", __FUNCTION__); return false; }
  }
  
  out_r->segment= s;
    
  /// resource offset in cluster/segment

  *out_r->offset()= _getAlignedOffset(s->freeOffset, out_r->mem()->alignment);

  // bind memory
  if(out_r->classT== ixClassT::VKBUFFER)     _ix->vk.BindBufferMemory(_ix->vk, *((ixvkBuffer *)out_r)->handle, *s->memory, *out_r->offset());
  else if(out_r->classT== ixClassT::VKIMAGE) _ix->vk.BindImageMemory(_ix->vk,  *((ixvkImage *)out_r)->handle,  *s->memory, *out_r->offset());
  
  /// update segment free space
  s->freeSize-= ((*out_r->offset())- s->freeOffset);    /// substract difference between aligned/not aligned offset (thrash)
  s->freeSize-= out_r->mem()->size;                            /// substract the buffer size
  s->freeOffset= (*out_r->offset())+ out_r->mem()->size;

  /// defrag data compute
  ixvkResource *last= ((ixvkResource *)s->resources.last);
  if(last)
    if(last->defragDelta) {
      uint32 alignment= out_r->mem()->alignment;
      VkDeviceSize newOffset= _getAlignedOffset((*last->offset())- last->defragDelta+ last->mem()->size, alignment);
      out_r->defragDelta= (uint32)((*out_r->offset())- newOffset);
    } else
      out_r->defragDelta= 0;
  out_r->defragDeltaInside= 0;
  out_r->defragOffset= 0;
  out_r->defragSize= 0;
  if(out_r->defragDelta!= 0) _ix->vki.defrag.job++;

  s->resources.add(out_r);

  return true;
}


















// RESOURCE CLUSTER

//   ####    ########    ####    ##    ##  ########  ##    ##  ########
// ##        ##        ##        ###  ###  ##        ###   ##     ##
//   ####    ######    ##  ####  ## ## ##  ######    ## ## ##     ##
//       ##  ##        ##    ##  ##    ##  ##        ##   ###     ##
//  #####    ########    ######  ##    ##  ########  ##    ##     ##

///====================================================================///

ixvkResClusterSegment::ixvkResClusterSegment(ixvkResCluster *in_parent): ixClass(ixClassT::VKCLUSTERSEGMENT), cluster(in_parent) {
  //_ix= parent->_ix;

  memory= null;

  delData();
}


ixvkResClusterSegment::~ixvkResClusterSegment() {
  delData();
}


void ixvkResClusterSegment::delData() {
  while(resources.first)
    resources.del(resources.first);

  if(memory)
    memory->destroy();

  freeSize= 0;
  freeOffset= 0;
}


bool ixvkResClusterSegment::build() {
  const char *err= null;
  int errL;

  memory= cluster->_ix->vk.objects.addMemory();
  memory->configure(cluster->segmentSize, cluster->memReq, cluster->memPref, cluster->memTypeBits);
  if(!memory->build()) IXERR("failed. aborting");

  freeSize= cluster->segmentSize;
  freeOffset= 0;
  
Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);
    return false;
  } else return true;
}


void ixvkResClusterSegment::updateFreespace() {
  freeOffset= (*((ixvkResource *)resources.last->prev)->offset())+ ((ixvkResource *)resources.last->prev)->mem()->size;
  freeSize= cluster->segmentSize- freeOffset;
}

#endif // IX_USE_VULKAN















// ######    ########    ####      ####    ######    ########  ######    ########    ####    ######
// ##    ##  ##        ##        ##    ##  ##    ##     ##     ##    ##     ##     ##    ##  ##    ##
// ##    ##  ######      ####    ##        ######       ##     ######       ##     ##    ##  ######
// ##    ##  ##              ##  ##    ##  ##    ##     ##     ##           ##     ##    ##  ##    ##
// ######    ########   #####      ####    ##    ##  ########  ##           ##       ####    ##    ##

///====================================================================///
// POOL+SET



// AMD advice:
// Place all Descriptors in one giant Descriptor Set
// -layout (set=0, binding=N) uniform texture2D textures[hugeNumber]
// Leave the one giant Descriptor Set always bound
// -No more vkCmdBindDescriptorSets() calls for each draw/dispatch
// -Instead use Push Constant(s) via vkCmdPushConstants() for per-draw indexes into binding array


ixvkDescPool::ixvkDescPool(Ix *in_ix): VkoDynamicSetPool(&in_ix->vk) {
  _ix= in_ix;
  _ix->vk.objects.addCustomDynamicSetPool(this);
}

ixvkDescPool::~ixvkDescPool() {
  VkoDynamicSetPool::~VkoDynamicSetPool();
}


void ixvkDescPool::addSet(ixvkDescSet **out_s) {
  *out_s= new ixvkDescSet;

  //layout->descriptors.nrNodes;  ->number of descriptors in this set - direct number of bindings - bindings can have count 0
  if(layout->descriptors.nrNodes) {
    (*out_s)->binds= new ixClass *[layout->descriptors.nrNodes];
    for(uint32 a= 0; a< layout->descriptors.nrNodes; ++a)
      (*out_s)->binds[a]= nullptr;
  }
  addCustomSet(*out_s);
  (*out_s)->_pool= this;
}






ixvkDescSet::ixvkDescSet(): VkoDynamicSet() {
  binds= null;
  _pool= null;
}

ixvkDescSet::~ixvkDescSet() {
  if(binds) { delete[] binds; binds= null; }
}


void ixvkDescSet::bind(uint32 in_nr, ixClass *in_res) {
  // any class can be binded. this can be easily expanded.
  if(in_nr> _pool->layout->descriptors.nrNodes) { error.detail("bind out of bounds", __FUNCTION__); return; }
  
  if(binds[in_nr]) _unlink(in_nr);
  binds[in_nr]= in_res;
  _link(in_nr);
}


void ixvkDescSet::unbind(uint32 in_nr) {
  if(binds[in_nr]== nullptr) { error.detail("trying to unbind a null bind", __FUNCTION__); return; }
  _unlink(in_nr);
  binds[in_nr]= nullptr;
}


void ixvkDescSet::_link(uint32 in_nr) {
  ixvkResource *r= null;
  if(binds[in_nr]->classT== ixClassT::VKBUFFER)
    r= (ixvkResource *)(binds[in_nr]);
  else if(binds[in_nr]->classT== ixClassT::TEXTURE)
    r= (ixvkResource *)(((ixTexture *)(binds[in_nr]))->vkd.img);

  if(r)
    ixvkResource::Set *s= new ixvkResource::Set(this, r);
}


void ixvkDescSet::_unlink(uint32 in_nr) {
  ixvkResource *r= null;
  if(binds[in_nr]->classT== ixClassT::VKBUFFER)
    r= (ixvkResource *)(binds[in_nr]);
  else if(binds[in_nr]->classT== ixClassT::TEXTURE)
    r= (ixvkResource *)(((ixTexture *)(binds[in_nr]))->vkd.img);

  if(r)
    for(ixvkResource::Set *s= (ixvkResource::Set *)r->_sets.first; s; s= (ixvkResource::Set *)s->next)
      if(s->set== this) {
        r->_sets.del(s);
        return;
      }
}


void ixvkDescSet::update() {
  VkWriteDescriptorSet w;
  VkDescriptorImageInfo i;
  VkDescriptorBufferInfo b;
  w.sType= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  w.pNext= nullptr;
  w.dstSet= set;
  w.dstArrayElement= 0;                   // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< ARRAY ELEMENT CAN BE AN ISSUE, I CAN'T FORESEE 
  w.descriptorCount= 1;
  w.pBufferInfo= &b;                  // always pointing <<< hopely no issues
  w.pImageInfo= &i;                   // always pointing <<< hopely no issues
  w.pTexelBufferView= nullptr;

  VkoDescriptorLayout *d= (VkoDescriptorLayout *)_pool->layout->descriptors.first;
  for(uint32 a= 0; a< nbinds(); ++a, d= (VkoDescriptorLayout *)d->next)
    if(binds[a]!= nullptr) {
      w.dstBinding= a;
      w.descriptorType= d->type;

      /// basic BUFFER bind
      if(binds[a]->classT== ixClassT::VKBUFFER) {
        b.offset= 0;
        b.range= VK_WHOLE_SIZE;
        b.buffer= ((ixvkBuffer *)binds[a])->handle->buffer;
        w.pBufferInfo= &b;
        w.pImageInfo= null;

      /// TEXTURE bind
      } else if(binds[a]->classT== ixClassT::TEXTURE) {
        w.pBufferInfo= null;
        w.pImageInfo= &i;
        ixTexture *tex= (ixTexture *)binds[a];
        i.sampler= tex->vkd.sampler->sampler;
        i.imageView= tex->vkd.img->view;

        if(tex->affinity< 2)
          //i.imageView= tex->segment->view,
          i.imageLayout= tex->vkd.img->access[0].layout;
        
        else if(tex->affinity== 64)
          //i.imageView= tex->segment->view,
          i.imageLayout= tex->vkd.img->access[tex->layer->index].layout;
        else error.detail(str8().f("unknown texture affinity[%d] for bind[%d]", tex->affinity, a), __FUNCTION__);

      /*
      } else if(binds[a]->classT== ixClassT::VKIMAGE) {
        // I DON'T THINK YOU CAN BIND JUST AN IMAGE HERE, YOU NEED SAMPLER/VIEW ALSO, AND THOSE ARE NOT IN THE IMAGE.
        error.makeme(__FUNCTION__); 
      */

      } else {
        error.detail(str8().f("unknown class binded in slot[%d]", a), __FUNCTION__);
      }
      


      _ix()->vk.UpdateDescriptorSets(_ix()->vk, 1, &w, 0, nullptr);
    }
  
}



void ixvkDescSet::updateStandardMat() {
  ixTexture **map= (ixTexture **)binds;
  VkWriteDescriptorSet w;
  VkDescriptorImageInfo i[4];
  w.sType= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  w.pNext= null;
  w.dstSet= set;
  w.dstArrayElement= 0;
  w.dstBinding= 0;
  w.descriptorCount= 4;   // will update binding+1/+2/+3 also
  w.descriptorType= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  w.pImageInfo= i;
  w.pBufferInfo= null;
  w.pTexelBufferView= null;
  for(uint a= 0; a< 4; a++)
    if(binds[a]) {
      i[a].sampler= map[a]->vkd.sampler->sampler;
      i[a].imageView= map[a]->vkd.img->view; //segment->view;
      i[a].imageLayout= map[a]->vkd.img->access[map[a]->layer->index].layout;
    } else {
      i[a].sampler= _ix()->vki.noTexture->vkd.sampler->sampler;
      i[a].imageView= _ix()->vki.noTexture->vkd.img->view;
      i[a].imageLayout= _ix()->vki.noTexture->vkd.img->access[0].layout;
    }

  _ix()->vk.UpdateDescriptorSets(_ix()->vk, 1, &w, 0, nullptr);
}




