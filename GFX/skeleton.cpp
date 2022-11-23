#include "ix/ix.h"


/*

https://github.com/TheThinMatrix/OpenGL-Animation

IN THE SHADER:

const int MAX_JOINTS= 50;
const int MAX_WEIGHTS= 3;

in vec3 in_position;
in vec2 in_textureCoords;
in vec3 in_normal;
in ivec3 in_jointIndices;
in vec3 in_weights;

out vec2 pass_textureCoords;
out vec3 pass_normal;

uniform mat4 jointTransforms[MAX_JOINTS];
uniform mat4 projectionViewMatrix;

void main(void) {
  vec4 totalLocalPos= vec4(0.0);
  vec4 totalNormal= vec4(0.0);
  for(int i=0; i< MAX_WEIGHTS; i++) {
    vec4 localPosition= boneTransforms[in_boneIndices[i]]* vec4(in_position, 1.0);
    totalLocalPos+= localPosition* in_weights[i];
    vec4 worldNormal= boneTransforms[in_jointIndices[i]]* vec4(in_normal, 0.0);
    totalNormal+= worldNormal* in_weights[i];
  }

  gl_position= projectionViewMatrix* totalLocalPos;
  pass_normal= totalNormal.xyz;
  pass_textureCoords= in_textureCoords;
}

*/






/*
1.
nop. an animator must happen
it would calculate every mesh's animation
the mesh would have only minimal and output vals, of the anim

the animator would know what type of animation the mesh has
it would process every mesh in game, outputting stuff to every mesh's stuff

in a way the mesh would not change in size in any way. you would always change the mat4's, and stuff
you would only update stuff to the gpu, once the animator is done

the compute shaders could help, i bet, but i cannot see how stuff is in the end, to actually think how to incorporate compute shaders ATM


2.
ixEditor - this would incorporate this mesh edit/save/load/watever
it would have texture edit and such, also
you would need to switch to diff modes
maybe you would be able to create as many windows as you would want / need
any window can be anything
must be an easy wat to switch the window's mode;

maybe level / world editors, later, not sure, if this will get to that
ixEditor would be the main executable, as few executables as possible, no need to clog with tons of programs
one for textures, one for meshes, one for watever
;


3.
a basic type of animations, a rotate one, mainly, maybe translate/scale too
those would have a max rotation velocity, you'd ask for a direction, the animator would compute how to reach that, based of the max speed/accel 

4.
Meshes cannot have animations tied to them in the file. multiple meshes can have same animation.
therefore, animation files must happen, that can apply to multiple meshes
meshes must have bones, for the bone animations, or other vars for simple animations, or watever other animations;



atm, the basic bone animation must happen, i see i mostly did from that tutorial, (check link, there is one); i must complete that, then massive change to make ixEdit happen;



5. this bone file, must be renamed, there's nothing big in the bones, they're just data;


*/

/*
this "bone" file, doesn't fit anymore, it seems
let's face it, bones will be a simple parent/children thing, in the standard bone animation
so things must be re-thinked

mkay.
https://ogldev.org/www/tutorial38/tutorial38.html
this file could be skelton or something, but still i need to figure stuff out, no point in thinking too much about it.
it seems assimp stores bone data bit weird
so each bone has a number of vertices it affects, but the data in the gpu is different... each vertex has weights and for what bones;
the bone, in the gpu is a matrix, nothing more, it seems atm.
so a linear list of these matrices must happen;
the added weights for every bone must be 1.0

the optimization... i mean, i think the whole skelton, with the inverse mats and the output could just be in the cpu cache, the whole thing, if you think about it
it's not that big...
ofc the bone data must be properly smaller as possible, no ix engine pointers and crap, that i put in there
the skeleton must handle the bone data;

im liking the skelton, not that bad to have it;
so you'd have all this data;
the anims... can be named poses? maybe?
an anim must handle a number of bones, not all;

im guessing a standardized thing can happen
with bone names, maybe? im not sure if this would help in any way, tho;
such naming things can be a per game thing, imho... you'd have naming conventions for bastion, for the humanoids, different creatures, maybe
tbh, only humanoids need naming conventions, maybe...
the stranger creatures im gonna think will have own animations that cannot be shared with anoy other skelton...


only humanoids could have a special naiming convention... maybe... 
still, extra objects that would stick to a certain bone... there would be special soket bones? so you'd have a weapon somewhere, right?... 
the weapons must be different and many...

things must not be too complicated, they must work flawlesly, and the art must be simple, but working good, everything must be smooth;


the skeleton data is pure mat4* nr bones... they call them joints and other crap, but i think skelly+bones will be great.
there could be a template anim that would work with names, you could apply such anims;... not sure atm, but importing stuff will have diff bone indexes for sure
that's why the naming conventions;
ofc these conventions could be a standard in IX maybe? diff types of skellys? not sure... i mean ix must apply a good same array of ID's for many anims...
stuff must be nicely done;
*/















// ix Skeleton class
ixSkeleton::ixSkeleton(uint32 in_nbones, uint8 *in_memBonesLocation): ixClass(ixClassT::SKELETON) {
  // ixBone array children order (list is arranged on creation):
  // root0, [c0..cn], [c0c0..c0cn], [cnc0..cnc0] etc
  // c0 - children 0, cn - children N;

  flags= 0;
  
  if(in_nbones) {
    alloc_boneData(in_nbones, in_memBonesLocation);

  } else {
    data= null;
  
    delData();
  }
}


ixSkeleton::~ixSkeleton() {
  delData();
}


void ixSkeleton::delData() {
  //mesh= null;

  nbones= 0;
  bones= null;
  posesFinal= posesInvFinal= posesLocal= null;
 
  // dealloc mem only if it's handled by this class
  if(!flags.isUp(0x0001))
    if(data) {
      delete[] data;
      data= null;
      dataSize= 0;
    }
}






// returns how much memory is required for a skelly that has <in_nbones> number of bones
uint32 ixSkeleton::sizeof_this(uint32 in_nbones) {
  return (uint32)sizeof(ixSkeleton)+ ixSkeleton::sizeof_boneData(in_nbones);
}


uint32 ixSkeleton::sizeof_boneData(uint32 in_nbones) {
  uint32 ret= (uint32)(sizeof(ixBone)+ sizeof(mat4)* 3); /// ixBone+ posesFinal+ posesInvFinal+ posesLocal
  return ret* in_nbones;
}


bool ixSkeleton::alloc_boneData(uint32 in_nbones, uint8 *in_memPreAllocated) {
  cchar *err= null;
  int errL;

  if(in_nbones== 0)   IXERR("<in_nbones> is 0");
  if(in_nbones> 1000) IXERR("<in_nbones> too big (>1k), something is off");

  if(data) delData();

  nbones= in_nbones;                   /// number of bones
  dataSize= sizeof_boneData(nbones);   /// data size

  // alloc mem
  if(in_memPreAllocated== null) {
    data= new uint8[dataSize];
    flags.setUp(0x0001);

  // just point data to pre-allocated mem  
  } else {
    data= in_memPreAllocated;
    flags.setDown(0x0001);
  }

  /// pointers populate
  bones= new(data) ixBone();
  posesFinal=    (mat4 *)((uint8 *)bones+         sizeof(ixBone)* nbones);
  posesInvFinal= (mat4 *)((uint8 *)posesFinal+    sizeof(mat4)*   nbones);
  posesLocal=    (mat4 *)((uint8 *)posesInvFinal+ sizeof(mat4)*   nbones);
  
Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);

    return false;
  } else
    return true;
}



bool ixSkeleton::alloc(ixSkeleton **out_skelly, uint32 in_nbones, uint32 in_nskellys, uint8 *in_memAlreadyAlloc) {
  const char *err= null;
  int errL= 0;
  uint8 *data= null, *dataBones= null;
  uint32 skellySize= sizeof_this(in_nbones);

  if(out_skelly== null) IXERR("<out_skelly> is null");
  if(in_nbones== 0)     IXERR("<in_nbones> is zero");
  if(in_nbones> 1000)   IXERR("<in_nbones> is over 1k, something is wrong");
  if(in_nskellys<= 0)   IXERR("<in_nskellys> is zero or negative");

  if(in_memAlreadyAlloc)
    data= in_memAlreadyAlloc;
  else
    data= new uint8[skellySize* in_nskellys];

  for(; in_nskellys> 0; in_nskellys--) {
    dataBones= data+ sizeof(ixSkeleton);
    *out_skelly= new (data) ixSkeleton(in_nbones, dataBones);   /// apply constructor on all skellys - this whole loop is for this
    data+= skellySize;
  }

Exit:
  if(err) {
    error.detail(err, __FUNCTION__, errL);
    return false;
  } else
    return true;
}











// ix

// ######      ####    ##    ##  ########
// ##    ##  ##    ##  ####  ##  ##
// ######    ##    ##  ## ## ##  ######
// ##    ##  ##    ##  ##   ###  ##
// ######      ####    ##    ##  ########

// class

ixBone::ixBone(): ixClass2(ixClassT::BONE) {
  idName= ixBoneN::NOT_ASSIGNED;

  parent= null;
  children= null;
  nchildren= 0;

  poseFinal= null;
  poseInvFinal= null;
  poseLocal= null;
}








// ix
// KEYFRAMES
// struct
void ixKeyFrame::createAndCopyKeyFrames(ixKeyFrame **out_dst, ixKeyFrame *in_src, int32 in_nrKeyFrames, int32 in_nrBones) {
  *out_dst= new ixKeyFrame[in_nrKeyFrames];
  for(int32 a= 0; a< in_nrKeyFrames; a++) {
    (*out_dst)[a].timeStamp= in_src[a].timeStamp;
    (*out_dst)[a].bones= new ixBoneTransform[in_nrBones];
    for(int32 b= 0; b< in_nrBones; b++) {
      (*out_dst)[a].bones[b].position= in_src[a].bones[b].position;
      (*out_dst)[a].bones[b].rotation= in_src[a].bones[b].rotation;
    }
  }
}


void ixKeyFrame::createKeyFrames(ixKeyFrame **out_dst, int32 in_nrKeyFrames, int32 in_nrBones) {
  *out_dst= new ixKeyFrame[in_nrKeyFrames];
  for(int32 a= 0; a< in_nrKeyFrames; a++)
    (*out_dst)[a].bones= new ixBoneTransform[in_nrBones];
}



// ix
//   ####    ##    ##  ##  ##    ##    ##    ##  ##    ##    ##
// ##    ##  ####  ##  ##  ###  ###    ###  ###  ##  ##     ###
// ########  ## ## ##  ##  ########    ########  ####        ##
// ##    ##  ##  ####  ##  ## ## ##    ## ## ##  ##  ##      ##
// ##    ##  ##    ##  ##  ##    ##    ##    ##  ##    ##  ######
// class

ixAnimMK1data::ixAnimMK1data(): ixClass(ixClassT::ANIM_MK1) {
  keyFrames= null;      // INIT 0
  delData();
}

ixAnimMK1data::ixAnimMK1data(float in_lengthInSec, ixKeyFrame *in_srcKF, int32 in_nrKeyFrames, int32 in_nrBones): ixClass(ixClassT::ANIM_MK1) {
  create(in_lengthInSec, in_srcKF, in_nrKeyFrames, in_nrBones);
}

ixAnimMK1data::~ixAnimMK1data() {
  delData();
}


void ixAnimMK1data::delData() {
  if(keyFrames) { delete[] keyFrames; keyFrames= null; }
  nrKeyFrames= 0;
  nrBones= 0;
  length= 0.0f;
}


void ixAnimMK1data::create(float in_lengthInSec, ixKeyFrame *in_srcKF, int32 in_nrKeyFrames, int32 in_nrBones) {
  if(keyFrames) { delete[] keyFrames; keyFrames= null; }
  ixKeyFrame::createAndCopyKeyFrames(&keyFrames, in_srcKF, in_nrKeyFrames, in_nrBones);
  nrKeyFrames= in_nrKeyFrames;
  nrBones= in_nrBones;
  length= in_lengthInSec;
}

















// "This is called during setup, after the bones hierarchy has been set
void ixBone::calcInverseBindTransform(const mat4 *in_parentBindTransform) {
  // NOTHING TESTED, EVERYTHING IS KINDA RUSHED, MUST GET ANOTHER PASS
  mat4 bindTransform= (*in_parentBindTransform)* (*poseLocal); // localBindTransform;
  
  poseInvFinal->invert(bindTransform);                          //  inverseBindTransform.invert(bindTransform);

  for(uint a= 0; a< nchildren; a++)
    children[a].calcInverseBindTransform(&bindTransform);       // p->calcInverseBindTransform(&bindTransform);
}















// ANIM MK1 ===========================================================================
// =========---------------------------------------------------------------------------


void AnimMK1::setAnimation(ixAnimMK1data *in_anim, float in_time) {
  currentAnim= in_anim; animationTime= in_time;
}

void AnimMK1::update() {
  if(currentAnim== null) return;
  increaseAnimationTime();
  calculateCurrentAnimationPose();
  // applyPoseToBones(); << error, commented
}


void AnimMK1::increaseAnimationTime() {
  animationTime+= (float)(osi.present/ 1000000000);
  if(animationTime> currentAnim->length)
    animationTime-= currentAnim->length;
}


void AnimMK1::calculateCurrentAnimationPose() {
  ixKeyFrame kf[2];
  getPrevAndNextFrames(kf);
  float progression= calculateProgression(&kf[0], &kf[1]);
  return interpolatePoses(&kf[0], &kf[1], progression);
}


void AnimMK1::applyPoseToBones(ixBone *in_bone, mat4 *in_parentTransform) {
  // EVERYTHING GOT COMMENTED, TOO MANY ERRORS, MUST FIX THIS , MUST COMPILE FIRST
  //the ID of the bone must be placed here, altho, it's in the in_bone... will think;
  //  THIS GET NAME... GETS THE NAME OF THE ANIM? for sure no way this is done with names;

  // mat4 currentLocalTransform= currentPose.get(name), so current name...;
  // mat4 currentTransform= (*in_parentTransform)* currentLocalTransform;

  //for(uint a= 0; a< in_bone->nchildren; a++)
  //  applyPoseToBones(&in_bone->children[a], &currentTransform);
  
  //currentTransform*= *(in_bone->poseInvFinal); // inverseBindTransform;
  
  //in_bone->setAnimatedTransform(&currentTransform);  probly this would apply the pose to the bone;
}

void AnimMK1::getPrevAndNextFrames(ixKeyFrame *out_kf) {
  for(int a= 1; a< currentAnim->nrKeyFrames; a++) {
    out_kf[1]= currentAnim->keyFrames[a];
    if(out_kf[1].timeStamp> animationTime)
      break;
    out_kf[0]= currentAnim->keyFrames[a];
  }
}


float AnimMK1::calculateProgression(ixKeyFrame *in_prevFrame, ixKeyFrame *in_nextFrame) {
  float totalTime= in_nextFrame->timeStamp- in_prevFrame->timeStamp;
  float currentTime= animationTime- in_prevFrame->timeStamp;
  return currentTime/ totalTime;
}
   

void AnimMK1::interpolatePoses(ixKeyFrame *in_prevFrame, ixKeyFrame *in_nextFrame, float in_progression) {
  ixBoneTransform curTransform;
  for(int a= 0; a< nrBones; a++) {
    curTransform.interpolate(in_prevFrame->bones[a], in_nextFrame->bones[a], in_progression);
    curTransform.getLocalTransform(&currentPose[a]);
  }
}













