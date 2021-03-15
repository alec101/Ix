#include "RGB.h"


// RGBAb rest of struct //
///--------------------///
RGBAb::RGBAb(const RGBAb &o): r(o.r),      g(o.g),      b(o.b),      a(o.a) {}
RGBAb::RGBAb(const RGBb  &o): r(o.r),      g(o.g),      b(o.b),      a(255) {}
RGBAb::RGBAb(const RGBAf &o): r(f2b(o.r)), g(f2b(o.g)), b(f2b(o.b)), a(f2b(o.a)) {}
RGBAb::RGBAb(const RGBf  &o): r(f2b(o.r)), g(f2b(o.g)), b(f2b(o.b)), a(255) {}

RGBAb& RGBAb::operator= (const RGBAb &o) { r= o.r;        g= o.g;        b= o.b;        a= o.a;      return *this; }
RGBAb& RGBAb::operator= (const RGBb  &o) { r= o.r;        g= o.g;        b= o.b;        a= 255;      return *this; }
RGBAb& RGBAb::operator= (const RGBAf &o) { r= f2b(o.r);   g= f2b(o.g);   b= f2b(o.b);   a= f2b(o.a); return *this; }
RGBAb& RGBAb::operator= (const RGBf  &o) { r= f2b(o.r);   g= f2b(o.g);   b= f2b(o.b);   a= 255;      return *this; }

// RGBAf rest of struct //
///--------------------///
RGBAf::RGBAf(const RGBAf &o): r(o.r),      g(o.g),      b(o.b),      a(o.a) {}
RGBAf::RGBAf(const RGBf  &o): r(o.r),      g(o.g),      b(o.b),      a(1.0f) {}
RGBAf::RGBAf(const RGBAb &o): r(b2f(o.r)), g(b2f(o.g)), b(b2f(o.b)), a(b2f(o.a)) {}
RGBAf::RGBAf(const RGBb  &o): r(b2f(o.r)), g(b2f(o.g)), b(b2f(o.b)), a(1.0f) {}

RGBAf& RGBAf::operator= (const RGBAf &o) { r= o.r;      g= o.g;      b= o.b;      a= o.a;      return *this; }
RGBAf& RGBAf::operator= (const RGBf  &o) { r= o.r;      g= o.g;      b= o.b;      a= 1.0f;     return *this; }
RGBAf& RGBAf::operator= (const RGBAb &o) { r= b2f(o.r); g= b2f(o.g); b= b2f(o.b); a= b2f(o.a); return *this; }
RGBAf& RGBAf::operator= (const RGBb  &o) { r= b2f(o.r); g= b2f(o.g); b= b2f(o.b); a= 1.0f;     return *this; }

// RGBb rest of struct //
///-------------------///
RGBb::RGBb(const RGBb  &o): r(o.r),      g(o.g),      b(o.b) {}
RGBb::RGBb(const RGBAb &o): r(o.r),      g(o.g),      b(o.b) {}
RGBb::RGBb(const RGBf  &o): r(f2b(o.r)), g(f2b(o.g)), b(f2b(o.b)) {}
RGBb::RGBb(const RGBAf &o): r(f2b(o.r)), g(f2b(o.g)), b(f2b(o.b)) {}

RGBb& RGBb::operator= (const RGBb  &o) { r= o.r;      g= o.g;      b= o.b;      return *this; }
RGBb& RGBb::operator= (const RGBAb &o) { r= o.r;      g= o.g;      b= o.b;      return *this; }
RGBb& RGBb::operator= (const RGBf  &o) { r= f2b(o.r); g= f2b(o.g); b= f2b(o.b); return *this; }
RGBb& RGBb::operator= (const RGBAf &o) { r= f2b(o.r); g= f2b(o.g); b= f2b(o.b); return *this; }

// RGBf rest of struct //
///-------------------///
RGBf::RGBf(const RGBf  &o): r(o.r),      g(o.g),      b(o.b) {}
RGBf::RGBf(const RGBAf &o): r(o.r),      g(o.g),      b(o.b) {}
RGBf::RGBf(const RGBb  &o): r(b2f(o.r)), g(b2f(o.g)), b(b2f(o.b)) {}
RGBf::RGBf(const RGBAb &o): r(b2f(o.r)), g(b2f(o.g)), b(b2f(o.b)) {}

RGBf& RGBf::operator= (const RGBf  &o) { r= o.r;      g= o.g;      b= o.b;      return *this; }
RGBf& RGBf::operator= (const RGBAf &o) { r= o.r;      g= o.g;      b= o.b;      return *this; }
RGBf& RGBf::operator= (const RGBb  &o) { r= b2f(o.r); g= b2f(o.g); b= b2f(o.b); return *this; }
RGBf& RGBf::operator= (const RGBAb &o) { r= b2f(o.r); g= b2f(o.g); b= b2f(o.b); return *this; }



