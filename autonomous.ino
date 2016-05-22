#include <Arduboy.h>
#define WX 64
#define WY 32

Arduboy arduboy;
/*
  histP[i] = DD XXXXXX
  P      = player        : 0=own 1=enemy
  DD     = dir    to walk: 00=right 01=up 10=left 11=down
  XXXXXX = pixels to walk: 0 -- 31,
*/
//uint8_t hist0[500]; // oh my memory
//uint8_t hist1[500]; // oh my memory
//int8_t dx0=0

void setup(){
  arduboy.begin();
  arduboy.clear();
  unsigned char *vram = arduboy.getBuffer();
//  arduboy.drawRect(0, 0, WIDTH-1, HEIGHT-1, 1);
  for(int i=0;i<1024;i++) vram[i] = i;
  arduboy.display();
  arduboy.setFrameRate(30);
}


void loop(){
  if (!(arduboy.nextFrame())) return;
//  if(arduboy.pressed(RIGHT_BUTTON));
}
