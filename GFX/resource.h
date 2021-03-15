#pragma once

/*

CONCLUSION AFTER ALL THINKING:
 - the data is or isn't in cpu memory
 - the fileName of the resource is a pointer, specially handled, in case the resource is shared, so there is a small memory imprint                                  
 - there is a copy of the res, on each ix - things are simple this way, and memory imprint is small this way
 - there is only a sharedList on each res, if shares happen
 
 - there has to exist a bigger class that pools many resources. there could be shares between these big classes
 - there should be more than one method to share things - beginShare / endShare, a bigger class that pools resources


hmmm


base has id, then pointer to the thingies?
or... not pointer, maybe reference, even...
well, asset->data.vars

ixAssetData = base class for ixTexture, etc
ixAsset->ixAssetData
ixAsset is on every ix engine

ixAssetData can be the same if shared
ixAsset must be the smallest thing bossible. chainlist, id, that's it




VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
OK. seems i got this.

you want a texture? you make a pointer, ask for one, from the ix engine.
the asset base class is HIDDEN, you don't need to know it exists
ask for a texture to be destroyed? - you ask on an ix engine. if it is used on another engine, it's not totally destroyed
so there is the need to be known only that it's used on another engine <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

what happens when you change data?
there has to be a shareList

there can't be too many chainlists, cuz they do have int64 next and prev
there has to be minimum number of vars on an asset


so

ixTexture
ixTexture::assetBase
assetBase = chainlist on the ixEngine
shareList? would this be a thing?


THIS MIGHT BE NOT NEEDED

any asset can have a data pointer, that can or can not be, if you want it to be in the cpu memory too
every asset can have a sharelist, that has ONLY a pointer to the shared ix

and that's that?

this is way simple.

having 2 parts, one on the engine, one a shared pointer to a texture class... the saved mem size is just extremly small
to save mem space, the fileName, can be a pointer to the str, therefore, there can be only one filename, this way.
so the filename is specially handled, as it will probably be the biggest memory waste, having fileName on every texture share
"graph/trees/bigtrees/oakTreeVeryLarge.png" - this has 41 bytes + str8 internal vars. i think it's a good mem save

there can be a var set, so every shared asset is binded on all engines...
but ofc, having 2 threads might be faster... ? or not?
it's a triky thing...
i don't think this is possible

this is for the future, tests must happen, but, the base for sharing must be done NOW









there can be  beginShare(Ix *array, nrElements); (this can be with chainList too, so you create a chainList, with the ixEngines, and pass to this func)

                assets are created

              endShare;






seeing this, i see the need for an "asset pool" class - it can be shared with another "asset pool"



THERE HAS TO BE A HIGHER CLASS THAT TIES ASSETS TOGHEDER









*/




// this can be the base for every asset - it looks good, it might work so you can dynamically load everything in any engine, and set shares for any asset
class Ix;

class ixResource: public chainData {
  Ix *_ix;
public:
  str8 fileName;

  //virtual bool load(cchar *in_file)= 0;    // loads the asset from file

  //virtual bool upload()= 0;                // loads asset in all ix engines (the main and the shared ones) - only loads on the engines that don't have it
  //virtual bool download()= 0;
  //virtual bool check()= 0;                 // checks if asset is still ok on the grcard, and reloads it if it's not
  //virtual bool unload()= 0;               // unloads an asset from grcard. if param is null, unloads from all engines.

  // share system - SEEMS IT WILL GO AWAY
  //void beginShare();  // concept
  //void endShare();    // concept
  //void share(Ix *);   // concept

  // constructors / destructors

  ixResource(Ix *in_ix): _ix(in_ix)/*, _share(null)*/ {}
  virtual ~ixResource() { delData(); }
  virtual void delData();


protected:
  friend class Ix;
  friend class ixTexture;

  // vvv SEEMS THIS WILL GO AWAY
  /*
  struct _ixResShare: public circleList {
    Ix *ix;           // not sure if needed
    ixResource *res;  // this resource - it will be chained with the other resources that is shared with (the classes will be the same)
    _ixResShare(): circleList(), res(null) {}
  } *_share;
  */
  // ^^^ SEEMS THIS WILL GO AWAY

  //virtual cchar *_getError();
  //void _printErr(cchar *in_func, int in_line);
};



/*
class ixResourceSys {
public:

  void unloadAll(Ix *engine);
  void checkAll(Ix *engine);


private:

  friend class Ix;


};
*/

// SCRAPE I THINK VVV
// ERROR numbers:
// 0 - OK, no error
// 1 - Unknown error
// 2 - Memory allocation failed
// 3 - Cannot open file
// 4 - File read error
// 5 - File write error
// 6 - renderer not set / unknown renderer



