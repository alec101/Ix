#include "ix/ix.h"





ixObject::ixObject(Ix *in_ix): _ix(in_ix) {
  //nrMaterials= nrMaterials= 0;
  //meshes= null;
  //mats= null;
}


ixObject::~ixObject() {
  delData();
}


void ixObject::delData() {
  //if(meshes) { delete[] meshes; meshes= null; }
  //if(mats) { delete[] mats; mats= null; }
  //nrMaterials= nrMeshes= 0;

}



void ixObject::addMesh(ixMesh *in_mesh, ixMaterial *in_mat ) {
  Mesh *m= new Mesh;
  m->mesh= in_mesh;
  m->mat= in_mat;

  m->mat->addMesh(m->mesh);
  m->mesh->mat= m->mat;


  meshes.add(m);
}




















