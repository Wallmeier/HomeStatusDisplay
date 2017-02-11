#ifndef _FHEM_STATUS_DISPLAY_TYPES_H_
#define _FHEM_STATUS_DISPLAY_TYPES_H_

#include "LedSwitcher.h"

#define NUMBER_OF_ELEMENTS(array)  (sizeof(array)/sizeof(array[0]))

enum deviceType
{
  TYPE_WINDOW,
  TYPE_DOOR,
  TYPE_LIGHT,
  TYPE_ALARM  
};

struct deviceMapping
{
  String name;
  deviceType type;
  int ledNumber;
};

struct colorMapping
{
  String msg;
  deviceType type;
  LedSwitcher::ledState state;
};

#endif