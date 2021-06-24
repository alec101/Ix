#pragma once

/* DESCRIPTOR INDEXING <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap50.html#VK_EXT_descriptor_indexing
research in descriptor indexing:
-it's only 56% supported (march 2021), for older hardware they seem not to bother updating, even tho this extension rocks it seems;
-everything new-ish do support it
-anything supporting 1.2 do support at least the core bindless descriptors, not all the functionality
-even phones start having them (march 2021)
-it do rock it seems.
-so it is the future i think
-descriptor hell would be total over with them, and you'd just have indexes for material in the freaking shader.. how kewl is that?
 you just do a cmd to draw tons of meshes, and they know to fit their mats, ffs, how hard was it to get to here
*/


class ixMesh;

class ixMaterial: public ixClass {
  Ix *_ix;
public:
  /*
  the material will be important for high/low resolutions
    it has to load low/high
    to know if high is loaded
    
    the loading for high, must be done in the background somehow.
    low must be avaible on render, it has to

    or at least the very very low must be loaded.
    maybe a trick with the mipmaps can be done, not sure.


    >>>descriptor indexing are the shit, tho. the answer to the descriptor madness. problem is, ofc, older drivers are not updated for them, only the new ones...
    even phones start to have it (march 2021)


    the material will have the set/pool
    textures that are static will still have the staticPool+sets, so a fast way for the descriptors for singular textures still exist.
    the materials will deal only with afinity64

    descriptor indexing should be a future, but not sure when, atm i should do a standard thing, with these dynamic pools, cuz they're also good
    */

  
  
  
  // ixvkDescriptorSet <<<<<<<<<<<<<<; MAKEME
  
  
  
  
  
  ixvkDescSet *set;

  // try to order the list and keep all mats that have same shader in one place
  //     then sort those mats so all that have same textures are kept togheder too
  //ixShader *shader;         // shader used. shader::id have to be populated

  //this is the issue i think. i don't see how to link the shader with the mat, without heavy shader customization, and i don't think i wanna do that
  //  so a material would have only the colors and maps on it, nothing more.
  //  that chainlist with meshes is triky too, ofc in theory you'd want to render all meshes on one mat, but will see.
  //  atm i think the mat will be kept simple.;
  
  vec4 diffuse;             // main color, alpha could be used for transparency
  vec3 ambient;             // ambient, multiplied by the global ambient (global should be used for effects)
  vec3 specular;            // shining/specular 
  vec3 emission;            // self illuminations

  // texture slot 0 default would be diffuse 1
  // texture slot 1 default would be diffuse 2 with alpha channel as blend between the two
  // texture slot 2 default would be bump mapping, with 3 unused channels
  ixTexture *map[4];          // texture using slot 0; default would be diffuse 1

  // this list will be used for drawing, so a MUTEX have to happen when adding/removing stuff, somewhere
  chainList meshes; class Mesh: public chainData { public: static std::mutex mtx; ixMesh *mesh; }; // all meshes that have this mat
  void addMesh(ixMesh *in_mesh);
  void delMesh(ixMesh *in_mesh);

  void updateSet();

  // THIS COULD BECOME A STANDARD, there's no way to pass the ix engine to an array, even if you'd think it would be something trivial
  static ixMaterial *allocArray(uint32 arraySize, Ix *in_engine);

  ixMaterial(Ix *in_parent);
  ~ixMaterial();
  void delData();

protected:
  ixMaterial();
  uint32 _fileID;   // index in the material lib file
};




class ixMatSys {
  Ix *_ix;
public:

  ixvkDescPool *setPool;

  str8 fileMaterialDB;  // [def: "/gfx/materialDB.mat"] .MAT file name

  chainList mats;               // [ixMaterial:chainData] all materials loaded
  ixMaterial *add();


  bool loadMAT(cchar *in_file= null);   // leave filename null to use current fileMaterialDB string
  bool saveMAT(cchar *in_file= null);   // leave filename null to use current fileMaterialDB string

  // cleanup sys

  uint32 cleanup_job;           // number of jobs that need being done
  bool cleanup_ONE();
  void cleanup_ALL();

  

  ixMatSys(Ix *in_ix);
  ~ixMatSys();
  void delData();

private:
  void _init();
  bool _loadMATdataV0_0_1(FILE *in_f);
  friend class Ix;
};














