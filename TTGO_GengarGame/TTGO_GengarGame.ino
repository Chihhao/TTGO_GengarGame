// Board: TTGO T-Display
// Arduino Select: ESP32 Dev Module

// ESP32 Library Version: 2.0.2
// TFT_eSPI Library Version: 2.4.32
// Button2 Library Version: 1.4.0

#include <TFT_eSPI.h>   
#include "initPicture.h" 
#include "Button2.h";
#include "item.h"

#define STATUS_GAMEOVER 0
#define STATUS_RUN 1
#define STATUS_INIT 2

TFT_eSPI    tft = TFT_eSPI();        
TFT_eSprite spriteScreen = TFT_eSprite(&tft);
TFT_eSprite spriteDino = TFT_eSprite(&tft);
TFT_eSprite spriteEnemy0 = TFT_eSprite(&tft);
TFT_eSprite spriteEnemy1 = TFT_eSprite(&tft);
TFT_eSprite spriteGameOver = TFT_eSprite(&tft);

#define BUTTON_A_PIN  0    //按鍵A
#define BUTTON_B_PIN  35   //按鍵B
#define TIME_TO_SLEEP 12000  //12S
Button2 buttonA = Button2(BUTTON_A_PIN);
Button2 buttonB = Button2(BUTTON_B_PIN);

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;  

//const int DINO_W=33, DINO_H=35;
const int DINO_W=32, DINO_H=42;
const int ENEMY_W=18, ENEMY_H=38;

const float INITIAL_SPEED = 1.0;

float linesX[6];
int linesW[6];
float linesX2[6];
int linesW2[6];
float clouds[2]={random(0,80),random(100,180)};
float bumps[2];
int bumpsF[2];

float eX[2];
int eColor[2];

float fSpeed;
int dinoX=30;
int dinoY=58;
float jumpDir=-1.4;

int frames=0;
int dino_pose=0;
float cloudfSpeed=0.4;
int gameStatus;
int score=0;

bool jumped = false;

int brightness_level[7]={35,70,100,130,160,200,230};
#define BRIGHTNESS_LEVEL 7
int brightness=1;

long last_available_time = 0;
long game_start_time = 0;

void initGame(){
  gameStatus = STATUS_INIT;  
  
  clouds[0] = random(0,80);
  clouds[1] = random(100,180);
  
  eX[0] = random(240,310);
  eX[1] = random(400,470);
  eColor[0] = RandomColor();
  eColor[1] = RandomColor();
  
  fSpeed=INITIAL_SPEED; 
  dinoX=30;
  dinoY=58;
  jumpDir=-1.8;  //ori:-1.4
  frames=0;
  dino_pose=0;
  cloudfSpeed=0.4;
  score=0;

  for(int i=0; i<6; i++){
    linesX[i]=random(i*40,(i+1)*40);
    linesW[i]=random(1,14);
    linesX2[i]=random(i*40,(i+1)*40);
    linesW2[i]=random(1,14);
  }

  for(int n=0; n<2; n++){
    bumps[n]=random(n*90,(n+1)*120);
    bumpsF[n]=random(0,2);
  }

  tft.pushImage(0, 0, 240, 135, initPicture); 
}

void pressA(Button2& btn) {
  last_available_time = millis();
  
  if(gameStatus == STATUS_RUN){
    if(!jumped) {
      jumped=true;
      dino_pose=0;  
    }
  }
  else if(gameStatus == STATUS_GAMEOVER){
    initGame();    
  }
  else if(gameStatus == STATUS_INIT){
    last_available_time = millis();
    game_start_time = millis();
    tft.fillScreen(TFT_BLACK);    
    gameStatus = STATUS_RUN;
  }
  
}

void pressB(Button2& btn) {
  last_available_time = millis();
  if(brightness++ > BRIGHTNESS_LEVEL - 1) { brightness=0; }  
  ledcWrite(pwmLedChannelTFT, brightness_level[brightness]); 
}

void setup() {   
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);  
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, LOW);
  
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);

  buttonA.setPressedHandler(pressA);
  buttonB.setPressedHandler(pressB);
  
  tft.init();    
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

//  spriteDino.setColorDepth(16);
//  spriteScreen.setColorDepth(16);
    
  spriteScreen.createSprite(240, 100);
  spriteDino.createSprite(DINO_W, DINO_H);
  spriteEnemy0.createSprite(ENEMY_W, ENEMY_H);  //int ENEMY_W=18, ENEMY_H=38;
  spriteEnemy1.createSprite(ENEMY_W, ENEMY_H);

  //spriteScreen.setTextColor(TFT_WHITE);  


    
  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, brightness_level[brightness]);

  initGame();
}

void loop() {
  buttonA.loop();  
  buttonB.loop(); 
  
  if(gameStatus != STATUS_RUN){
    if( millis() - last_available_time > TIME_TO_SLEEP) {
      doSleep();
    }
  }
  
  if(gameStatus == STATUS_RUN){
    if(jumped){ 
      dinoY = dinoY + jumpDir; 
      if(dinoY<=2)  { jumpDir = -jumpDir; }
      if(dinoY>=58) { jumpDir = -jumpDir; jumped = false;}
    }
    if(frames<8 && !jumped) dino_pose=1;
    if(frames>8 && !jumped) dino_pose=2;
  
    drawS(dino_pose);
    frames++;
    if(frames==16) frames=0;
 
    checkColision();    
  }

  if(gameStatus == STATUS_GAMEOVER){
    spriteGameOver.createSprite(240, 135);
    spriteGameOver.fillSprite(TFT_BLACK);
    spriteGameOver.setTextColor(TFT_YELLOW);
    spriteGameOver.setTextSize(2); 
    spriteGameOver.drawString("GAME OVER", 25, 25, 2);  
    spriteGameOver.drawString("Score: " + String(score), 25, 75, 2);      
    spriteGameOver.pushSprite(0, 0);
    spriteGameOver.deleteSprite();
  }
   
}

void drawS(int dino_pose){ 
  spriteScreen.fillSprite(TFT_BLACK);
  spriteScreen.drawLine(0, 84, 240, 84, TFT_WHITE);
  
  for(int i=0; i<6; i++){
    spriteScreen.drawLine(linesX[i],87 ,linesX[i]+linesW[i], 87, TFT_WHITE);
    linesX[i]=linesX[i]-fSpeed;
    if(linesX[i]<-14){
      linesX[i]=random(245,280);
      linesW[i]=random(1,14);
    }
    spriteScreen.drawLine(linesX2[i],98 ,linesX2[i]+linesW2[i], 98, TFT_WHITE);
    linesX2[i]=linesX2[i]-fSpeed;
    if(linesX2[i]<-14){
      linesX2[i]=random(245,280);
      linesW2[i]=random(1,14);
    }
  }
  
  for(int j=0; j<2; j++){
    spriteScreen.drawXBitmap(clouds[j], 20, cloud, 38, 11, TFT_WHITE, TFT_BLACK);
    clouds[j]=clouds[j]-cloudfSpeed;
    if(clouds[j]<-40){
      clouds[j]=random(244,364);
    }
  }

  for(int n=0; n<2; n++){
    spriteScreen.drawXBitmap(bumps[n], 80, bump[bumpsF[n]], 34, 5, TFT_WHITE, TFT_BLACK);
    bumps[n]=bumps[n]-fSpeed;
    if(bumps[n]<-40){
      bumps[n]=random(244,364);
      bumpsF[n]=random(0,2);
    }
  }
  
  for(int enemyNo=0; enemyNo<2; enemyNo++){
    eX[enemyNo] -= fSpeed;
    if(eX[enemyNo] < -20) {
      do{
        eX[enemyNo] = random(240,400); //產生新敵人
      }while(abs(eX[0]-eX[1])< 40+40*fSpeed);     
      eColor[enemyNo] = RandomColor();
    }
  }
  
  spriteEnemy0.drawXBitmap(0, 0, enemy[0], ENEMY_W, ENEMY_H, eColor[0], TFT_BLACK);
  spriteEnemy1.drawXBitmap(0, 0, enemy[1], ENEMY_W, ENEMY_H, eColor[1], TFT_BLACK);  
  //spriteDino.drawXBitmap(0, 0, dino[dino_pose], DINO_W, DINO_H, TFT_WHITE, TFT_BLACK);
  //spriteDino.drawXBitmap(0, 0, dino, DINO_W, DINO_H, TFT_PURPLE);
  spriteDino.pushImage(0, 0, DINO_W, DINO_H, dino[dino_pose], TFT_GREEN);  
    
  spriteEnemy0.pushToSprite(&spriteScreen, eX[0], 56, TFT_BLACK);
  spriteEnemy1.pushToSprite(&spriteScreen, eX[1], 56, TFT_BLACK);
  //spriteDino.pushToSprite(&spriteScreen, dinoX, dinoY, TFT_BLACK);
  spriteDino.pushToSprite(&spriteScreen, dinoX, dinoY, TFT_GREEN);

  score=(millis()-game_start_time)/120;
  spriteScreen.drawString(String(score), 204, 0, 2);
  spriteScreen.pushSprite(0, 17);

  fSpeed = INITIAL_SPEED + (score/100)*0.1;
  
}

int RandomColor(){
  int r = random(10,255);
  int g = random(10,255);
  int b = random(10,255);
  return tft.color565(r, g, b);
}

void checkColision(){
  for(int enemyNo=0; enemyNo<2; enemyNo++){
    if( eX[enemyNo] < dinoX+DINO_W/2 && 
        eX[enemyNo] > dinoX && 
        dinoY > 25){
      gameStatus = STATUS_GAMEOVER;
    }
  }
}

void doSleep(){
    pinMode(4,OUTPUT);     
    digitalWrite(4,LOW); // Should force backlight off    
    tft.writecommand(ST7789_DISPOFF);// Switch off the display    
    tft.writecommand(ST7789_SLPIN);// Sleep the display driver
    esp_deep_sleep_start();
}

 
