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
*/

class ixBone {


public:




  ixBone();
  ~ixBone();
  void delData();


};














