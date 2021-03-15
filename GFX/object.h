#pragma once



class ixObject {
  Ix *_ix;
public:


  //uint32 nrMaterials;
  //uint32 nrMeshes;
  //ixMaterial **mats;      // list of pointers to the actual objects
  //ixMesh **meshes;        // list of pointers to the actual objects

  //chainList mats; class Mat: public chainData { ixMaterial *handle; };
  chainList meshes; class Mesh: public chainData { public: ixMesh *mesh; ixMaterial *mat; };
  void addMesh(ixMesh *in_mesh, ixMaterial *in_mat);
  //void addMat(const ixMaterial *in_m);

  // constr / destr

  ixObject(Ix *in_ix);
  ~ixObject();
  void delData();

protected:



};















