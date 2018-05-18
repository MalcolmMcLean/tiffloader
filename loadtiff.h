#ifndef loadtiff_h
#define loadtiff_h

#include <stdio.h>


/*
  By Malcolm McLean


  To use
  int width, height;
  unsigned char *data;
  int format;

  FILE *fp = fopen("tiffile.tiff", "rb");
  if(!fp)
     errorhandling();
  data = floadtiff(fp, &width, &height, &format);
  fclose(fp);
  
  if(data == 0)
     printf("TIFF file unreadable\n");
   
     data format given by format - it's 8 bit channels
       with alpha (if any) last.
     alpha is premultiplied = composted on black. To get
        the image composted on white, call floadtiffwhite() 
     width is image width, height is image height in pixels
  */

#define FMT_ERROR 0
#define FMT_RGBA 1
#define FMT_CMYK 2
#define FMT_CMYKA 3
#define FMT_GREYALPHA 4
#define FMT_RGB 5
#define FMT_GREY 6

unsigned char *floadtiffwhite(FILE *fp, int *width, int *height, int *format);
unsigned char *floadtiff(FILE *fp, int *width, int *height, int *format);

#endif
