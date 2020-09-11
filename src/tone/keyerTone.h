#ifndef KEYERTONE_H
#define KEYERTONE_H
// Different hardware might need different tone control capabilities,
// so the approach is to use a global object that implements
// .tone and .noTone.

#include "M5Tone.h"
M5Tone derivedFromToneBase;


// make sure your derived object is named derivedFromToneBase
// and implements toneBase's .tone and .noTone
toneBase &toneControl{derivedFromToneBase};

#endif