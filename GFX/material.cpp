#include "ix/ix.h"

std::mutex ixMaterial::Mesh::mtx;     // no constructor auto-loading of mats or this will be a problem


// ##    ##    ####    ########  ########  ######    ########    ####    ##
// ###  ###  ##    ##     ##     ##        ##    ##     ##     ##    ##  ##
// ## ## ##  ########     ##     ######    ######       ##     ########  ##
// ##    ##  ##    ##     ##     ##        ##    ##     ##     ##    ##  ##
// ##    ##  ##    ##     ##     ########  ##    ##  ########  ##    ##  ########

///==============================================================================///


// material private
namespace _ixMat {

};



ixMaterial::ixMaterial(Ix *in_parent): ixClass(ixClassT::MATERIAL), _ix(in_parent) {
  map[0]= map[1]= map[2]= map[3]= null;

  _ix->res.mat.setPool->addSet(&set);
}


ixMaterial::~ixMaterial() {
  delData();
}


void ixMaterial::delData() {
}



void ixMaterial::updateSet() {
  for(uint32 a= 0; a< 4; ++a)
    if(map[a])
      set->bind(a, map[a]);
  //set->bind

  set->updateStandardMat();
  /*
  VkWriteDescriptorSet w;
  VkDescriptorImageInfo i[4];
  w.sType= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  w.pNext= null;
  w.dstSet= set->set;
  w.dstArrayElement= 0;
  w.dstBinding= 0;
  w.descriptorCount= 4;   // will update binding+1/+2/+3 also
  w.descriptorType= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  w.pImageInfo= i;
  w.pBufferInfo= null;
  w.pTexelBufferView= null;
  for(uint a= 0; a< 4; a++)
    if(map[a]) {
      i[a].sampler= map[a]->vkd.sampler->sampler;
      i[a].imageView= map[a]->segment->view;
      i[a].imageLayout= map[a]->vkd.img->access[map[a]->layer->index].layout;
    } else {
      i[a].sampler= _ix->vki.noTexture->vkd.sampler->sampler;
      i[a].imageView= _ix->vki.noTexture->vkd.imgView;
      i[a].imageLayout= _ix->vki.noTexture->vkd.img->access[0].layout;
    }

  _ix->vk.UpdateDescriptorSets(_ix->vk, 1, &w, 0, nullptr);
  */
}





void ixMaterial::addMesh(ixMesh *in_m) {
  Mesh *m= new Mesh;
  m->mesh= in_m;
  in_m->_matLink= m;
  Mesh::mtx.lock();
  meshes.add(m);
  Mesh::mtx.unlock();
}


void ixMaterial::delMesh(ixMesh *in_m) {
  Mesh::mtx.lock();
  meshes.del(in_m->_matLink);
  in_m->_matLink= null;
  Mesh::mtx.unlock();
}













// MATERIAL

//   ####    ##    ##    ####    ########  ########  ##    ##
// ##         ##  ##   ##           ##     ##        ###  ###
//   ####      ####      ####       ##     ######    ## ## ##
//       ##     ##           ##     ##     ##        ##    ##
//  #####       ##      #####       ##     ########  ##    ##

///==========================================================///

ixMatSys::ixMatSys(Ix *in_ix): _ix(in_ix) {
  cleanup_job= 0;
  fileMaterialDB= "/gfx/materialDB.mat";
  setPool= null;
}


ixMatSys::~ixMatSys() {
  delData();
}



void ixMatSys::delData() {
  mats.delData();
}


void ixMatSys::_init() {
  setPool= new ixvkDescPool(_ix);
  setPool->configure(_ix->res.tex.vkd.standard4mapsLayout, 30);
  setPool->build();
}




ixMaterial *ixMatSys::add() {
  ixMaterial *m= new ixMaterial(_ix);
  mats.add(m);

  return m;
}








bool ixMatSys::cleanup_ONE() {
  cleanup_job= 0;
  return true;  // DISABLED <<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // must be very fast
  if(!cleanup_job) return false;
  
  for(ixMaterial *m= (ixMaterial *)mats.first; m; m= (ixMaterial *)m->next) {
    if(!m->meshes.nrNodes)
      error.makeme(__FUNCTION__);
      // i mean, remove mats that are not used? what's the benefit of that?
  }
  cleanup_job= false;

}

void ixMatSys::cleanup_ALL() {
  while(cleanup_job)
    cleanup_ONE();
}







