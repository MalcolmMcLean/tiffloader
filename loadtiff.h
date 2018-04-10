#ifndef loadtiff_h
#define loadtiff_h

#include <stdio.h>


/*
  By Malcolm McLean


  To use
  int width, height;
  unsigned char *buff;
  int format;

  FILE *fp = fopen("tiffile.tiff", "rb");
  if(!fp)
     errorhandling();
  buff = floadtiff(fp, &width, &height, &format);
  fclose(fp);
  
  if(buff == 0)
     printf("TIFF file unreadable\n");

  switch(format)
  {
     case FMT_ERROR: internal error;
     case FMT_GREY: buff is 8 bit greyscale
     case FMT_CMYK: buff is 32 bit CMYK values
     case FMT_RGB: buff is 24 bit RGB values
  }
   
     width is image width, height is image height in pixels
     format is the image format
  */

#define FMT_ERROR 0
#define FMT_RGB 1
#define FMT_CMYK 2
#define FMT_GREY 3



unsigned char *floadtiff(FILE *fp, int *width, int *height, int *format);

#endif
