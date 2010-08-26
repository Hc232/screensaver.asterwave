// include file for screensaver template
#ifndef __WATER_H_
#define __WATER_H_

#include "waterfield.h" 
//#include "sphere.h"

void SetAnimation();

struct WaterSettings
{
  //C_Sphere * sphere;
  WaterField * waterField;
  int effectType;
  bool isSingleEffect;
  int frame;
  int nextEffectTime;
  int nextTextureTime;
  int effectCount;
  float scaleX;
  bool isWireframe;
  bool isTextureMode;
  char szTexturePath[1024];
  char szTextureSearchPath[1024];
};

#endif 