#pragma once

// common/util stuff should go here

#ifndef _IXSAFEFILEOP
#define _IXSAFEFILEOP 1
static inline const char *_IXFILEREADERROR() { static const char *_ixFRE= "file read failed"; return _ixFRE; }
static inline const char *_IXFILEWRITEERROR() { static const char *_ixFWE= "file write failed"; return _ixFWE; }
#define IXFREAD(a, b, c, d)  if(fread(a, b, c, d)!= (c))  { err= _IXFILEREADERROR(), errL= __LINE__; goto Exit; }
#define IXFWRITE(a, b, c, d) if(fwrite(a, b, c, d)!= (c)) { err= _IXFILEWRITEERROR(), errL= __LINE__; goto Exit; }
#endif



struct ixFlags32 {
  uint32 flags;
  ixFlags32(): flags(0) {}
  ixFlags32(uint32 in_f): flags(in_f) {}

  inline uint32 &operator= (uint32 in_f) { return (flags= in_f); }  // set bits exact
  inline void set(uint32 in_f, bool in_enable) { if(in_enable) flags|= in_f; else flags&= ~(in_f); }    // set bit depending on <in_f>
  inline void setUp(uint32 in_f) { flags|= in_f; }                  // set bit up
  inline void setDown(uint32 in_f) { flags&= ~(in_f); }             // set bit down

  inline bool operator==(uint32 in_f) const { return flags==in_f; }      // exact match
  inline bool isUp(uint32 in_f) const { return (flags& in_f)> 0; }        // is bit up
  inline operator uint32 &() { return flags; }
};

struct ixFlags8 {
  uint8 flags;
  ixFlags8(): flags(0) {}

  inline uint8 &operator= (uint8 in_f) { return (flags= in_f); }   // set bits exact
  inline void set(uint8 in_f, bool in_enable) { if(in_enable) flags|= in_f; else flags&= ~(in_f); }    // set bit depending on <in_f>
  inline void setUp(uint8 in_f) { flags|= in_f; }                  // set bit up
  inline void setDown(uint8 in_f) { flags&= ~(in_f); }             // set bit down

  inline bool operator==(uint8 in_f)  { return flags==in_f; }      // exact match
  inline bool isUp(uint8 in_f) { return (flags& in_f)> 0; }        // is bit up
  inline operator uint8 &() { return flags; }
};


struct recti;

struct rectf {
  union {
    struct { float left, right, top, bottom; };
    struct { float l,    r,     t,   b; };
    struct { float x0,   xe,    y0,  ye; };
    float v[4];               /// allignment needs 16 bits anyways, this will help when retrieving data from __m128
  };
  float dx, dy;

  inline void delData() { x0= y0= xe= ye= dx= dy= 0.0f; }

  rectf():        l(0.0f), r(0.0f), t(0.0f), b(0.0f), dx(0.0f), dy(0.0f) {}
  rectf(float n): l(0.0f), r(n), t(n), b(0.0f), dx(n), dy(n) {}    // this makes a square, origin in 0,0, or if n is 0, clears the values
  rectf(float in_x0, float in_y0, float in_dx, float in_dy) { x0= in_x0, y0= in_y0, dx= in_dx, dy= in_dy; compEndpoints(); }
  rectf(float in_dx, float in_dy): dx(in_dx), dy(in_dy) { x0= 0.0f, y0= 0.0f; xe= dx, ye= dy; }
  rectf(const rectf &r): x0((float)r.x0), y0((float)r.y0), dx((float)r.dx), dy((float)r.dy), xe((float)r.xe), ye((float)r.ye) {}
  inline rectf &set(float in_x0, float in_y0, float in_xe, float in_ye) { x0= in_x0, y0= in_y0, xe= in_xe, ye= in_ye; compDeltas(); return *this; }
  inline rectf &setD(float in_x, float in_y, float in_dx, float in_dy) { x0= in_x, y0= in_y, dx= in_dx, dy= in_dy; xe= x0+ dx, ye= y0+ dy; return *this; }
  inline rectf &operator=(float n) { l= r= b= t= dx= dy= n; return *this; }
  inline rectf &operator=(const rectf &o) { l= o.l, r= o.r, t= o.t, b= o.b, dx= o.dx, dy= o.dy; return *this; }
  inline rectf &operator=(const recti &o);

  inline bool operator==(const rectf &o) const { return ((l== o.l) && (r== o.r) && (t== o.t) && (b== o.b)); }
  inline bool operator!=(const rectf &o) const { return ((l!= o.l) || (r!= o.r) || (t!= o.t) || (b!= o.b)); }
  inline bool exists() const { return (dx> 0.0f) && (dy> 0.0f); }

  // funcs

  #ifdef IX_USE_VULKAN
  inline VkRect2D &toVkRect2D(VkRect2D *out_v) const { *out_v= {{ mlib::roundf(x0), mlib::roundf(y0) }, {(uint32)mlib::roundf(dx), (uint32)mlib::roundf(dy)}}; return *out_v; }
  #endif

  inline bool inside(float in_x, float in_y) const { return ((in_x>= x0) && (in_x<= xe) && (in_y>= y0) && (in_y<= ye)); }
  inline bool intersect(const rectf &o) const { return (l< o.r && r> o.l && t< o.b && b> o.t); }
  inline rectf &intersectRect(const rectf &r) { if(r.x0> x0) x0= r.x0; if(r.y0> y0) y0= r.y0; if(r.xe< xe) xe= r.xe; if(r.ye< ye) ye= r.ye; compDeltas(); if(!exists()) delData(); return *this; }

  inline void compDeltas() { dx= xe- x0, dy= ye- y0; }
  inline void compEndpoints() { xe= x0+ dx, ye= y0+ dy; }
  inline void move(float in_x, float in_y) { compDeltas(); x0= in_x, y0= in_y; xe= x0+ dx, ye= y0+ ye; }
  inline void moveD(float in_dx, float in_dy) { x0+= in_dx, xe+= in_dx, y0+= in_dy, ye+= in_dy; }
  inline void resize(float in_dx, float in_dy)  { dx= in_dx, dy= in_dy;   xe= x0+ dx, ye= y0+ dy; }
  inline void resizeD(float in_dx, float in_dy) { dx+= in_dx, dy+= in_dy; xe= x0+ dx, ye= y0+ dy; }
};



// x0, y0 are inside the rectangle
// xe, ye are outside of the rectangle
// dx, dy are directly computed: xe- x0 and ye- y0
struct recti {
  union {
    struct { int32 x0,   y0,  dx,  dy,  xe,    ye; };       // difference from VkRect2D: size is still int32, if it's negative, the rect shouldn't exist; (no intersection for example)
    struct { int32 l,    t,   _z1, _z2, r,     b; };
    struct { int32 left, top, _z3, _z4, right, bottom; };
    int32 v[6];
    //struct { int32 left, right, top, bottom; };
    //struct { int32 l, r, t, b; };
    //struct { int32 x0, xe, y0, ye; };
    //int32 v[4];
  };
  //int32 dx, dy;
  
  recti():        x0(0), y0(0), dx(0), dy(0), xe(0), ye(0) {}
  recti(int32 n): x0(n), y0(n), dx((uint32)n), dy((uint32)n), xe(n), ye(n) {}
  recti(int32 in_x0, int32 in_y0, uint32 in_dx, uint32 in_dy): x0(in_x0), y0(in_y0), dx(in_dx), dy(in_dy) { compEndpoints(); }
  recti(uint32 in_dx, uint32 in_dy): x0(0), y0(0), dx(in_dx), dy(in_dy), xe(in_dx), ye(in_dy) {}
  recti(const recti &r): x0(r.x0), y0(r.y0), dx(r.dx), dy(r.dy), xe(r.xe), ye(r.ye) {}
  recti(const rectf &r): x0(mlib::roundf(r.x0)), y0(mlib::roundf(r.y0)), dx(mlib::roundf(r.dx)), dy(mlib::roundf(r.dy)), xe(mlib::roundf(r.xe)), ye(mlib::roundf(r.ye)) {}

  inline recti &set(int32 in_x0, int32 in_y0, int32 in_xe, int32 in_ye)  { x0= in_x0, y0= in_y0, xe= in_xe, ye= in_ye; compDeltas();    return *this; }
  inline recti &setD(int32 in_x, int32 in_y, uint32 in_dx, uint32 in_dy) { x0= in_x,  y0= in_y,  dx= in_dx, dy= in_dy; compEndpoints(); return *this; }
  inline recti &operator=(int32 n) { x0= y0= 0, xe= ye= dx= dy= n; return *this; }    // this makes a square, origin in 0,0, or if n is 0, clears the values
  inline recti &operator=(const recti &o) { x0= o.x0, y0= o.y0, dx= o.dx, dy= o.dy, xe= o.xe, ye= o.ye; return *this; }

  inline bool operator==(const recti &o) const { return ((x0== o.x0) && (y0== o.y0) && (dx== o.dx) && (dy== o.dy)); }
  inline bool operator!=(const recti &o) const { return ((x0!= o.x0) || (y0!= o.y0) || (dx!= o.dx) || (dy!= o.dy)); }

  #ifdef IX_USE_VULKAN
  inline VkRect2D &toVkRect2D(VkRect2D *out_v) const { *out_v= {{ x0, y0 }, {(uint32)dx, (uint32)dy}}; return *out_v; }
  #endif

  inline bool intersect(const recti &r)  const { return (x0< r.xe && xe> r.x0 && ye> r.y0 && y0< r.ye); } // xe, ye ouside the rect
  //inline bool intersect2(const recti &o) const { return (l<= o.r && r>= o.l && t>= o.b && b<= o.t); }   // xe, ye inside the rect
  //inline void intersectRect(const recti &r, recti *out) const { out->x0= max(r.x0, x0), out->xe= min(r.xe, xe), out->y0= max(r.y0, y0), out->ye= min(r.ye, ye); out->compDeltas(); if(out->dx<= 0|| out->dy<= 0) out->delData(); }
  inline recti &intersectRect(const recti &r) { if(r.x0> x0) x0= r.x0; if(r.y0> y0) y0= r.y0; if(r.xe< xe) xe= r.xe; if(r.ye< ye) ye= r.ye; compDeltas(); if(!exists()) delData(); return *this; }

  inline bool inside(int32 in_x, int32 in_y) const { return ((in_x>= x0) && (in_x< xe) && (in_y>= y0) && (in_y< ye)); }    // 0,0 inclusive, e,e exclusive
  //inline bool inside(int32 in_x, int32 in_y) const { return ((in_x>= x0) && (in_x<= xe) && (in_y>= y0) && (in_y<= ye)); }   // 0,0 inclusive, e,e inclusive

  inline void compDeltas()    { dx= (xe- x0< 0? 0: xe- x0), dy= ye- y0; } // xe, ye outside the rect
  inline void compEndpoints() { xe= x0+ dx, ye= y0+ dy; } // xe, ye outside the rect
  inline void move(int32 in_x, int32 in_y) { /*compDeltas();*/ x0= in_x, y0= in_y; xe= x0+ dx, ye= y0+ dy; }
  inline void moveD(int32 in_dx, int32 in_dy) { x0+= in_dx, xe+= in_dx, y0+= in_dy, ye+= in_dy; }
  inline void resize(uint32 in_dx, uint32 in_dy) { dx= in_dx, dy= in_dy; xe= x0+ dx, ye= y0+ dy; }
  inline void resizeD(uint32 in_dx, uint32 in_dy) { dx+= in_dx, dy+= in_dy; xe= x0+ dx, ye= y0+ dy; }

  inline bool exists() const { return (dx> 0) && (dy> 0); }

  inline void delData() { x0= y0= xe= ye= dx= dy= 0; }
  
};

rectf &rectf::operator=(const recti &o) { l= (float)o.l, r= (float)o.r, b= (float)o.b, t= (float)o.t, dx= (float)o.dx, dy= (float)o.dy; return *this; }




namespace ixUtil {

bool isWhitespace(uint32 c);                                      // returns true if specified character is whitespace
char *skipWhitespace(char *in_string);                            // skips whitespace characters and returns the resulting string
char *readWordOrWordsInQuotes(char *in_string, str8 *out_string); // returns the advance of in_string; ',' breaks the word, but not '.' ATM. there should be a readNumber too
char *readWordsInBrackets(char *in_string, str8 *out_string);     // reads words in brackets (if there are any) and returns the advanced in_string
bool _getBool(str8 *s);

void parseGenericTxtCommand(str8 *in_line, str8 *out_command, str8 *out_v1= null, str8 *out_v2= null, str8 *out_v3= null, str8 *out_v4= null);

inline bool intersect1D(int32 x1_start, int32 x1_end, int32 x2_start, int32 x2_end) { return (x1_start< x2_end) && (x1_end> x2_start); }

void changePathSeparator(char *out_str); // changes '\\' to '/' in a filepath
void changePathSeparator(str8 *out_str); // changes '\\' to '/' in a filepath

// RANDOM number generation
extern const uint32 randMax;
extern uint32 randSeed_thread0;                 // a main thread seed, beware of thread safety if using multiple threads. use a seed for each thread
extern uint32 randSeed_thread1;                 // secondary thread seed, beware of thread safety if using multiple threads. use a seed for each thread
extern uint32 randSeed_thread2;                 // 3rd thread seed, beware of thread safety if using multiple threads. use a seed for each thread
extern uint32 randSeed_thread3;                 // 4th thread seed, beware of thread safety if using multiple threads. use a seed for each thread

void randInit();    // creates the 4 seeds, based on osi.getNano()
int32 rand32(uint32 *inout_seed= null, int32 in_min= INT32_MIN, int32 in_max= INT32_MAX);   // [0 -> ~0] <inout_seed> will be randomized as well; using randSeed_thread0 if seed is null
float randfnorm(uint32 *inout_seed= null);      // [0.0f -> 1.0f] 0-1000000 precision

} /// namespace ixUtil



