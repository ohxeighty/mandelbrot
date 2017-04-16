//
//  pixelColor.c
//  Andrew Yu & Alex Huang
//  Return appropriate colour for mandelbrot 
//  16/04/17
//


#include <stdio.h>
#include <stdlib.h>
#include "pixelColor.h"
#define MAX_INTENSITY 255 
//765 
unsigned char stepsToRed (int steps){
   return steps; 
}
unsigned char stepsToBlue (int steps){
   unsigned char returnValue = steps*3 - MAX_INTENSITY;
   if(returnValue < 0){
      returnValue = 0;
   } else if(returnValue > MAX_INTENSITY){
      returnValue = MAX_INTENSITY; 
   }
   return returnValue; 
}
unsigned char stepsToGreen (int steps){
      unsigned char returnValue = steps*3 - (MAX_INTENSITY * 2);
   if(returnValue < 0){
      returnValue = 0;
   } else if(returnValue > MAX_INTENSITY*2){
      returnValue = MAX_INTENSITY; 
   }
   return returnValue; 
}