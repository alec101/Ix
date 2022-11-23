#pragma once

/*
there could be a top object over this, or this might be the top object


this can be a backbone for any object
the bone object can have multiple bones linked
each bone can have multiple mesh3d linked to it



bone->mesh->material

THE TOP OBJECT
object? entity? bone? thing(the:)? 

ENTITY seems to be the winner
dictionary entity:
"a thing with distinct and independent existence."
"Church and empire were fused in a single entity"


           /->anything
so:  entity
           \->bone->mesh->material   - the gfx tree



ENGINE NAME
- i like id because is small and can be incorporated into anything, therefore something small can be nice
- if not small, it needs to have a _good abreviation_

ix / Ix / IX

ix seems to be the winner. 


ixEntity i guess cannot go in gfx...
ixPrint.h?
ixMesh3D.h? would this be better?
the directory must be ixGfx / ixBla, tho
the files, i don't think so...





need further research on bones
this chainlist i don't think can be done... bones must have children and parents (the system)

ha! maybe a familyList!!! this can be very nice to do! i think it will work on the window system, too!!!!
or a treeList
**********************************************************************************************************




IDEAL: 
compute shader that writes the matrix for each bone, you insert what animation, what key, and what time
... i must go into compute for this
it will be a small shader execute before the draw


ATM, i should go with:
buffer for each model, and you do them with the CPU
maybe some double buffer thing, you push model data, and do stuff, same with the other

so update the buffer(the exact small part of it), and the barrier will be key
update+draw, update+draw, i think


*/





/*
class ixBone: public ixClass {
  Ix *_ix;
public:

  str8 name;              // ye, noway heh... if you gonna name bones, you're not gonna have a smooth thing goin'

  ixBone *parent;
  //chainList parent;       // parent bone
  chainList children;     // children bones

  ixMesh *mesh;   // mesh it affects
  vec3i boneIDs; // BONES that this particular vertex is affected by <<<<<<<<<<<<< THIS MUST BE IN THE MESH
  // it should be vec4, maybe affected by up to 4 bones? cuz 
  vec4 weights;   // weight of every of the 4 bones it affects this vertex


  int32 id;         // direct ID to the shader array

  mat4 animatedTransform;     // current pos and rotation, model origin, not parent origin -> this gets directly uploaded to the shader, at index position <id>

  // private
  mat4 localBindTransform;    // in relation to the parent bone
  mat4 inverseBindTransform;  // inverse of the <transform> (final) matrix

  // probly total crap vvv
  void setBone(int32 in_index, str8 *in_name, mat4 *in_bindLocalTransform) { id= in_index; name= *in_name; localBindTransform= *in_bindLocalTransform; }
  void addChild(ixBone *in_bone) { children.add(in_bone); }
  mat4 getAnimatedTransform() { return animatedTransform; }
  void setAnimatedTransform(mat4 *in_mat) { animatedTransform= *in_mat; }
  mat4 getInverseBindTransform() { return inverseBindTransform; }

  // funcs

  void calcInverseBindTransform(const mat4 *in_parentBindTransform);

  // constr / destr

  ixBone(Ix *in_ix);
  ~ixBone();
  void delData();


private:

  
};
*/




/*
// ANIMATOR for bones
// Updates the animation time and determines the current pose, before setting the bone transforms in the AnimatedModel
class ixAnimatorBones {
  Ix *_ix;
public:


  ixAnimBone *currentAnim;
  float animationTime;


  ixAnimatorBones();
  ~ixAnimatorBones();
  void delData();

protected:

};
*/






#define IX_MAX_BONES 50

// 

struct ixBoneTransform {
  vec3 position;    // position
  vec4 rotation;    // quaternion - easy to interpolate between rotations (slurp method), easy to comvert to/from matrices
  
  inline ixBoneTransform() {}
  inline ixBoneTransform(const vec3 &in_pos, const vec4 &in_rotQuaternion): position(in_pos), rotation(in_rotQuaternion) {}

  // from quaternion & pos to a normal mat4
  inline void getLocalTransform(mat4 *out_ret) const {
    out_ret->translate(position);
    (*out_ret)*= rotation.quaternionToMat4();
	}

  inline void interpolate(const ixBoneTransform &frameA, const ixBoneTransform &frameB, float progression) {
    position.interpolate(frameA.position, frameB.position, progression);
    rotation.quaternionInterpolate(frameA.rotation, frameB.rotation, progression);
	}
};


struct ixKeyFrame {
  ixBoneTransform *bones;  // array, nrBones in size - one for each bone - in relation to the parent bone, not to the root bone
  float timeStamp;

  static void createAndCopyKeyFrames(ixKeyFrame **out_dst, ixKeyFrame *in_src, int32 in_nrKeyFrames, int32 in_nrBones);
  static void createKeyFrames(ixKeyFrame **out_dst, int32 in_nrKeyFrames, int32 in_nrBones);
};



class ixAnimMK1data: public ixClass {
public:
  ixKeyFrame *keyFrames;
  int32 nrKeyFrames;
  int32 nrBones;
  float length;                   // in seconds

  void create(float in_lengthInSec, ixKeyFrame *in_srcKF, int32 in_nrKeyFrames, int32 in_nrBones);

  ixAnimMK1data();
  ixAnimMK1data(float lengthInSec, ixKeyFrame *in_srcKF, int32 in_nrKeyFrames, int32 in_nrBones);
  ~ixAnimMK1data();
  void delData();
};



/* NOPE
// so animMK1 is in one place, no need for 100
class ixAnimMK1 { 


public:

  struct Data {
    ixKeyFrame *keyFrames;
    int32 nrKeyFrames;
    int32 nrBones;
    float length;                   // in seconds

    void create(float in_lengthInSec, ixKeyFrame *in_srcKF, int32 in_nrKeyFrames, int32 in_nrBones);

    ixAnimMK1data();
    ixAnimMK1data(float lengthInSec, ixKeyFrame *in_srcKF, int32 in_nrKeyFrames, int32 in_nrBones);
    ~ixAnimMK1data();
    void delData();
  } data;

  // ANIMATOR
  struct AnimMK1 {
    ixSkeleton *_parent;

    ixAnimMK1data *currentAnim;   // THIS CAN BE ixClass, AND YOU SEE WHAT TYPE IT IS FROM ixClassT
    float animationTime;

    mat4 currentPose[IX_MAX_BONES];
    int32 nrBones;

    void setAnimation(ixAnimMK1data *in_anim, float in_time);
    void update();
    void increaseAnimationTime();
    void calculateCurrentAnimationPose();
    void applyPoseToBones(ixBone *in_bone, mat4 *in_parentTransform);
    void getPrevAndNextFrames(ixKeyFrame *out_kf);
    float calculateProgression(ixKeyFrame *in_prevFrame, ixKeyFrame *in_nextFrame);
    void interpolatePoses(ixKeyFrame *in_prevFrame, ixKeyFrame *in_nextFrame, float in_progression);
  } anim1;
  // END ANIMATOR


};
*/

struct ixAnimData {



};


class ixAnimator {
  Ix *_ix;
public:


};







enum class ixBoneN: int32 { 
  ROOT,
  ARM_LEFT_HIGH,
  ARM_LEFT_LOW,


  NOT_ASSIGNED= -1
};

static const char *ixBoneNames[]= {
  "Root",
  "Arm Left high",
  "Arm Left low"
};




class ixBone: public ixClass2 {
public:

  ixBoneN idName;             // maybe

  ixBone *parent;             // pointer to parent bone
  ixBone *children;           // pointer to children bones
  uint32 nchildren;           // number of children bones

  /* MESH data, not here, idk why i put this here<<<<<<<<<
  vec3i boneIDs; // BONES that this particular vertex is affected by <<<<<<<<<<<<< THIS MUST BE IN THE MESH
  // it should be vec4, maybe affected by up to 4 bones? cuz 
  vec3 weights;   // weight of every of the 3 bones it affects this vertex
  the mesh data must be heavily modified when created, basically re-created, so it fits the order choosen
  */

  // int32 id;                   // direct ID to the shader array <<< KINDA POINTLESS, BUT IT COULD HELP

  //mat4 animatedTransform;     // current pos and rotation, model origin, not parent origin -> this gets directly uploaded to the shader, at index position <id>

  // pointers to the actual data in skeleton class
  mat4 *poseFinal;            // array that can be directly sent to the gpu with the bone poses;
  mat4 *poseInvFinal;         // inverted poseFinal (I HOPE IT'S NOT LOCAL ONE)
  mat4 *poseLocal;            // based on parent bone

  // funcs

  void calcInverseBindTransform(const mat4 *in_parentBindTransform);
  cchar *getName() { return ixBoneNames[(int)idName]; }

  // constr / destr

  ixBone();           // no destructors, ixSkelly should handle if needed, no alloc here anyway
private:
};

// the ideea... create seglist with these skeletons, these must be unique per your unit... everything else is a constant
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

class ixSkeleton: public ixClass {
public:
  

  uint32 nbones;        // number of bones in the whole skeleton
  ixBone *bones;        // [nbones len] bones[0] is root;
  mat4 *posesFinal;     // [nbones len] array with each bone's pose; this will be DIRECTLY SENT TO THE SHADER
  mat4 *posesInvFinal;  // [nbones len] array with each bone's inverted poseFinal
  mat4 *posesLocal;     // [nbones len] array with each bone's local pose
  
  // ixMesh *mesh;         // mesh of this skelly   <<< MAYBE

  // allocates memory (or not) for bone data, populates all pointers to the data
  // <in_nbones> number of bones in the skelly
  // <in_memPreAllocated> - pass pre-allocated memory, so no allocs will happen
  bool alloc_boneData(uint32 in_nbones, uint8 *in_memPreAllocated= null);
  static uint32 sizeof_boneData(uint32 in_nbones);
  
  // allocates memory for one or multiple skellys, what this func really does, is that the memory for the whole skelly(s) is linear, only one alloc for all data
  //   animations will require many skellys, having them in a bulk space, is way better
  // YOU ARE RESPONSIBLE TO delete[] THE WHOLE MEM, STARTING WITH FIRST OBJECT delete []out_skelly
  // <out_skelly> - return pointer
  // <in_nbones> - [1- 1000] how many bones the skelly will have
  // <in_nskellys> - number of skellys to alloc/construct, <out_skelly> will have the first one in the linear array
  // <in_memAlreadyAlloc> - if it's not null, the func will use this memory location for skelly construction, make sure the amount of memory you allocate is enough
  static bool alloc(ixSkeleton **out_skelly, uint32 in_nbones, uint32 in_nskellys= 1, uint8 *in_memAlreadyAlloc= null);

  // returns how much memory is required for a skelly that has <in_nbones> number of bones (includes the sizeof ixSkeleton and all ixBones inside)
  static uint32 sizeof_this(uint32 in_nbones);


  // if you alloc mem for a skelly, and want all the data in one place, pass to it where to put the bone data in <in_memBonesLocation>
  // if you want to automatically alloc bones, give from start the number of bones
  ixSkeleton(uint32 in_nbones= 0, uint8 *in_memBonesLocation= null);
  ~ixSkeleton();
  void delData();

protected:
  // skeleton data, trying to optimize for max speed; avoiding to have a chainlist with such data because of it
  // ixBone array children order (list is arranged on creation):
  // root0, [child0..childn], [child0_child0..child0_childn], [childn_child0..childn_child0] etc
  uint8 *data;          // memory alloc for all bones, posesFinal, posesInvFinal, poseLocal; each of those will point to parts in this data;
  uint32 dataSize;

  // 0x0001- mem is handled externally (no deletes or anything will happen in destructor)
  ixFlags32 flags;
};
























// ANIMATOR MUST BE MERGED IN HERE
struct AnimMK1 {
  //ixAnimMK1 *currentAnim;   // THIS CAN BE ixClass, AND YOU SEE WHAT TYPE IT IS FROM ixClassT
  //float animationTime;


  ixAnimMK1data *currentAnim;   // THIS CAN BE ixClass, AND YOU SEE WHAT TYPE IT IS FROM ixClassT
  float animationTime;

  //screw perfect stuff, make a thing work, see what is needed for it to work, THEN arrange it. I'm fully goin to the future and that ain't good.

  mat4 currentPose[IX_MAX_BONES];
  int32 nrBones;

  void setAnimation(ixAnimMK1data *in_anim, float in_time);
  void update();
  void increaseAnimationTime();
  void calculateCurrentAnimationPose();
  void applyPoseToBones(ixBone *in_bone, mat4 *in_parentTransform);
  void getPrevAndNextFrames(ixKeyFrame *out_kf);
  float calculateProgression(ixKeyFrame *in_prevFrame, ixKeyFrame *in_nextFrame);
  void interpolatePoses(ixKeyFrame *in_prevFrame, ixKeyFrame *in_nextFrame, float in_progression);

  
  // must have create bone, createHumanoidMK1, createHumanoidMK2(), the laters should create all the bones for a humanoid, the mk2, with toes too, maybe;

  //you have rotation for the top half of the body; and a maximum rotation of legs of 90 degrees, maybe no rotation for 180, and you walk backwards;
  //but all these things will work great i think, with low poly count of meshes, really low, but good stilized, i guess...


};

// END ANIMATOR
  



