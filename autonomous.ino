#include <Arduboy.h>
#define WX 64
#define WY 32
unsigned char *vram; // =arduboy.getBuffer()

#define KEY_XM 0
#define KEY_XP 1
#define KEY_YM 2
#define KEY_YP 3
#define KEY_B  4
#define KEY_A  5
#define KEYS   6
int curkeys = 0;
int button[KEYS]={ LEFT_BUTTON, RIGHT_BUTTON, UP_BUTTON, DOWN_BUTTON, B_BUTTON, A_BUTTON};
boolean keypressed[KEYS];
Arduboy arduboy;

int key_rate    = 60; // keys/sec
int frame_rate  = 2;  // frames/sec
int keys_per_frame = key_rate/frame_rate; // keys/frames

int ri=0;
/* 
  histP[i] = XXXX XXXD
  P       = player        : 0=own 1=enemy
  D       = dir    to walk: 0=x 1=y
  XXXXXXX = number to walk: -31 to +31
*/
#define HISTMAX 100 // oh my memory
// 0=player 1=COM h=head t=tail
uint8_t hist[2][HISTMAX];
int  x[2][2],  y[2][2];// {x,y}[p][c] =            current position of player p+1 of {head,tail}[c]
int dx[2][2], dy[2][2];// {x,y}[p][c] = ={-1,0,+1} current position of player p+1 of {head,tail}[c] 
int ih[2][2];          // ih[p][c] =         history buffer pointer of player p+1 of {head,tail}[c]
int ax, ay; // position of apple

uint8_t isWall(int x, int y);
void addHist(uint8_t *hist, int *i, int d, uint8_t isy);
void incHist(uint8_t *hist, int *i);
void decHist(uint8_t *hist, int *i);
int16_t getLeft(uint8_t *hist, int *i);
void gameover(int iswin);

void setup(){
  arduboy.begin();
  arduboy.initRandomSeed();
  vram = arduboy.getBuffer();
  initGame();
  arduboy.setFrameRate(key_rate);
}
void initGame(){
  resetGame();
}
void resetGame(){
  ax=random(0,WX);
  ay=random(0,WY);

  x[0][0]=WX/4*1; y[0][0]=WY/2; //P1 head
  x[1][0]=WX/4*3; y[1][0]=WY/2; //P1 tail
  x[0][1]=WX/4*1; y[0][1]=WY/2; //P2 head
  x[1][1]=WX/4*3; y[1][1]=WY/2; //P2 tail
  
  dx[0][0]=+1; dy[0][0]=0; //P1 tail
  dx[1][0]=-1; dy[1][0]=0; //P1 head
  dx[0][1]=+1; dy[0][1]=0; //P2 tail
  dx[1][1]=-1; dy[1][1]=0; //P2 head
  
  for(int i=0;i<HISTMAX;i++){
    hist[0][i]=0x00;
    hist[1][i]=0x00;
  }
  hist[0][0]=0x01; // x+1
  hist[1][0]=0x7F; // x-1
  
  ih[0][0]=0;
  ih[0][1]=0;
  ih[1][0]=0;
  ih[1][1]=0;
  
  arduboy.clear();
  arduboy.drawRect(0, 0, WIDTH, HEIGHT, 1);
  arduboy.display();
}


void loop(){
  if (!(arduboy.nextFrame())) return;
  for(int k=0;k<KEYS;k++){
    keypressed[k] |= arduboy.pressed(button[k]); // add key
  }
  if(curkeys++ == keys_per_frame){
    loopGame();
    curkeys = 0;
  }
  for(int k=0;k<KEYS;k++) keypressed[k]=0;
}

void loopGame(){
  uint8_t iy=0, by=0;
  int gotApple = 2; // 2 = nobody, 0=P1, 1=P2
  
  //input for P1 (key -> dx,dy)
  if(!(keypressed[KEY_XM]||keypressed[KEY_XP]||keypressed[KEY_YM]||keypressed[KEY_YP])){
    incHist(hist[0],&ih[0][0]);
  }else{
    if(keypressed[KEY_XM]){
      if(dx[0][0]<0){
        incHist(hist[0],&ih[0][0]);
      }else if(dx[0][0]==0){
        addHist(hist[0],&ih[0][0],-1,0);
        dx[0][0]=-1;
        dy[0][0]= 0;
      }
    }
    if(keypressed[KEY_XP]){
      if(dx[0][0]>0){
        incHist(hist[0],&ih[0][0]);
      }else if(dx[0][0]==0){
        addHist(hist[0],&ih[0][0],+1,0);
        dx[0][0]=+1;
        dy[0][0]= 0;
      }
    }
    if(keypressed[KEY_YM]){
      if(dy[0][0]<0){
        incHist(hist[0],&ih[0][0]);
      }else if(dy[0][0]==0){
        addHist(hist[0],&ih[0][0],-1,1);
        dx[0][0]= 0;
        dy[0][0]=-1;
      }
    }
    if(keypressed[KEY_YP]){
      if(dy[0][0]>0){
        incHist(hist[0],&ih[0][0]);
      }else if(dy[0][0]==0){
        addHist(hist[0],&ih[0][0],+1,1);
        dx[0][0]= 0;
        dy[0][0]=+1;
      }
    }
  }
  
  //judge P1
  if(x[0][0]+dx[0][0]==ax && y[0][0]+dy[0][0]==ay){
    gotApple = 0;
  }else if(isWall(x[0][0]+dx[0][0], y[0][0]+dy[0][0])){
    gameover(0);
  }
  
  //input for P2 (AI -> dx,dy)
  if(x[1][0]+dx[1][0]==ax && x[1][0]+dy[1][0]==ay){
    // if got apple
    gotApple = 1;
    incHist(hist[1],&ih[1][0]);
  }else if(isWall(x[1][0]+dx[1][0], y[1][0]+dy[1][0])){
    // if p2 can go straight
    incHist(hist[1],&ih[1][0]);
  }else{
    // when p2 can not go straight
    int d  = random(0,1)*2-1; // randomize the trying order =-1 or +1 
    int md = -d;
    if(dx[1][0]==0){
      if     (!isWall(x[1][0]+ d, y[1][0])){addHist(hist[1],&ih[1][0],d,0); x[1][0]= d; y[1][0]= 0;}
      else if(!isWall(x[1][0]+md, y[1][0])){addHist(hist[1],&ih[1][0],d,0); x[1][0]=md; y[1][0]= 0;}
      else{
      //  gameover(1);
      }
    }else{ //dy[1][0]==0
      if     (!isWall(x[1][0], y[1][0]+ d)){addHist(hist[1],&ih[1][0],d,1); x[1][0]= 0; y[1][0]= d;}
      else if(!isWall(x[1][0], y[1][0]+md)){addHist(hist[1],&ih[1][0],d,1); x[1][0]= 0; y[1][0]=md;}
      else{
      //  gameover(1);
      }
    }
  }
  
  for(int p=0;p<2;p++){
    //draw head
    iy = (y[p][0]*2+dy[p][0]*1) / 8;
    by = (y[p][0]*2+dy[p][0]*1) % 8;
    vram[iy*WIDTH + x[p][0]*2+dx[p][0]*1] |= 1<<by;
    iy = (y[p][0]*2+dy[p][0]*2) / 8;
    by = (y[p][0]*2+dy[p][0]*2) % 8;
    vram[iy*WIDTH + x[p][0]*2+dx[p][0]*2] |= 1<<by;
    
    //draw tail
    /*
    iy = (y[p][0]*2+dy[p][0]*1) / 8;
    by = (y[p][0]*2+dy[p][0]*1) % 8;
    vram[iy*WIDTH + x[p][0]*2+dx[p][0]*1] &= ~(1<<by);
    iy = (y[p][0]*2+dy[p][0]*2) / 8;
    by = (y[p][0]*2+dy[p][0]*2) % 8;
    vram[iy*WIDTH + x[p][0]*2+dx[p][0]*2] &= ~(1<<by);
*/
//  for(int p=0;p<2;p++){
  }
  for(int p=0;p<1;p++){
  //move head
    x[p][0] += dx[p][0];
    y[p][0] += dy[p][0];
    
    if(p==gotApple){
      ax=random(0,WX);
      ay=random(0,WY);
      while(!isWall(ax,ay)){
        ax=random(0,WX);
        ay=random(0,WY);
      }
    }else{
      //move tail
      x[p][1] += dx[p][1];
      y[p][1] += dy[p][1];
      // new direction of tail
      if(getLeft(hist[p],&ih[p][1])==0) ih[p][1] = (ih[p][1]+1) % HISTMAX;
    }
  }
  
  //display apple
  iy = (ay*2) / 8;
  by = (ay*2) % 8;
  vram[iy*WIDTH + ax*2] |= 1<<by;  
  if(1){
    int i=0;
    for(i=0;i<8;i++) vram[i]=1<<i;
    vram[i++]=(uint8_t)keypressed[0];
    vram[i++]=(uint8_t)keypressed[1];
    vram[i++]=(uint8_t)keypressed[2];
    vram[i++]=(uint8_t)keypressed[3];
  }

  arduboy.display();
}
uint8_t isWall(int _x, int _y){
  return arduboy.getPixel(_x*2,_y*2);
}

void addHist(uint8_t *hist, int *i, int d, uint8_t isy){
  *i = (*i+1) % HISTMAX;
  hist[*i] =  (((uint8_t)d)<<1)|isy;
}
void incHist(uint8_t *hist, int *i){
  int8_t s = ((int8_t)hist[*i])>>1;
  if(s>0)s++;else s--;
  hist[*i] =  (((uint8_t)s)<<1)|hist[*i]&1;
}
void decHist(uint8_t *hist, int *i){
  int8_t s = ((int8_t)hist[*i])>>1;
  if(s>0)s--;else s++;
  hist[*i] =  (((uint8_t)s)<<1)|hist[*i]&1;
}
int16_t getLeft(uint8_t *hist, int *i){
   return (int16_t)((int8_t)hist[*i])>>1;
}

void gameover(int iswin){
  resetGame();
}





