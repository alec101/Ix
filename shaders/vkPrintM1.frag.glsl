#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location= 0) in  vec2 in_UV;
layout(location= 0) out vec4 out_color;
layout(set= 1, binding= 1) uniform sampler2D texSampler;

// push constants
layout(push_constant) uniform PConst {
  vec4 pos;               // changed frequently
  vec4 color1;            // main color
  vec4 color2;            // outline color
  int flags;              // each byte meaning: 0= persp camera, 1= ortho camera, 2= outline, 3= solid background
  int outline;            // 0= no outline, [1-5]= outline distance
} p;


// ********************************************************************
void main() {
  // output color
  out_color= p.color1;
  out_color.a*= texture(texSampler, in_UV).r;     // MULTIPLY IS OK? OR A BLEND/MIX?

  // character outline
  if((p.flags& 0x0004)> 0) {
    vec4 shadow= vec4(p.color2.rgb, 0);

    // i know this is retarded, but textureOffset accepts only compile-time constant as the offset -.-
    if(p.outline== 1) {
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-1, -1)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 1, -1)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 1,  1)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-1,  1)).r);
      shadow.a/= 1.41; // corners have lower weight, due distance is bigger from texel              (<<< IS THIS OK? - ATM OK)
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-1,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 1,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0, -1)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0,  1)).r);
    } else if(p.outline== 2) {
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-2, -2)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 2, -2)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 2,  2)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-2,  2)).r);
      shadow.a/= 1.41; // corners have lower weight, due distance is bigger from texel              (<<< IS THIS OK? - ATM OK)
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-2,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 2,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0, -2)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0,  2)).r);
    } else if(p.outline== 3) {
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-3, -3)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 3, -3)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 3,  3)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-3,  3)).r);
      shadow.a/= 1.41; // corners have lower weight, due distance is bigger from texel              (<<< IS THIS OK? - ATM OK)
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-3,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 3,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0, -3)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0,  3)).r);
    } else if(p.outline== 4) {
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-4, -4)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 4, -4)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 4,  4)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-4,  4)).r);
      shadow.a/= 1.41; // corners have lower weight, due distance is bigger from texel              (<<< IS THIS OK? - ATM OK)
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-4,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 4,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0, -4)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0,  4)).r);
    } else if(p.outline== 5) {
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-5, -5)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 5, -5)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 5,  5)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-5,  5)).r);
      shadow.a/= 1.41; // corners have lower weight, due distance is bigger from texel              (<<< IS THIS OK? - ATM OK)
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2(-5,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 5,  0)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0, -5)).r);
      shadow.a= max(shadow.a, textureOffset(texSampler, in_UV, ivec2( 0,  5)).r);
    }

    // mix shadow/out_color, with out_color's alpha being the weight
    if(shadow.a> 0) {
      if(out_color.a> 0)
        out_color= mix(shadow, out_color, out_color.a);
      else out_color= shadow;
    }

  // solid background
  } else if((p.flags& 0x0008)> 0) {
    out_color= mix(p.color2, out_color, out_color.a);
  }
}
