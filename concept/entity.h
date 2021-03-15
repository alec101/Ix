#pragma once

/*
entity -> bone system -> mesh on each bone -> material on each mesh -> shader(_s_)-> texture(s) on each material

to be able to alter shaders, dunno how this is done... aditional effects that can happen oh each mesh of the entity
-modifying the material shader, a different shader, i think !!!

maybe a material -> draw multiple meshes ? dunno the order is very important too


or _maybe_ the shader is not bound to the material? another link in the system? where whould it go?


why only one system?

multiple systems? and whatever is best for an entity...

but, the classes must be done, the basic classes... the link between them will be very important for Ix - the definition of Ix
*/


class ixEntity {
public:



  ixEntity();
  ~ixEntity();
  void delData();
protected:

};























