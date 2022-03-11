// Board: TTGO T-Display
// Arduino Select: ESP32 Dev Module

// ESP32 Library Version: 2.0.2
// TFT_eSPI Library Version: 2.4.32
// Button2 Library Version: 1.4.0

#include <TFT_eSPI.h>
#include "Button2.h"
#include "item.h"
#include "gengar.h"
#include "koopa.h"

#define STATUS_GAMEOVER 0
#define STATUS_RUN 1
#define STATUS_INIT 2

TFT_eSPI    tft = TFT_eSPI();        
TFT_eSprite spriteScreen = TFT_eSprite(&tft);
TFT_eSprite spriteHero = TFT_eSprite(&tft);
TFT_eSprite spriteHero1 = TFT_eSprite(&tft);
TFT_eSprite spriteHero2 = TFT_eSprite(&tft);
TFT_eSprite spriteEnemy0 = TFT_eSprite(&tft);
TFT_eSprite spriteEnemy1 = TFT_eSprite(&tft);
TFT_eSprite spriteGameOver = TFT_eSprite(&tft);
  
#define BUTTON_A_PIN  0    
#define BUTTON_B_PIN  35  
#define TIME_TO_SLEEP 10000  //10S
Button2 buttonA = Button2(BUTTON_A_PIN);
Button2 buttonB = Button2(BUTTON_B_PIN);

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;  

float initialSpeed;

float linesX[6];
int linesW[6];
float linesX2[6];
int linesW2[6];
float cloudX[2], cloudY[2];
float bumpsX[2];
int bumpType[2];

float enemyX[2];
int enemyColor[2];

float fSpeed;
int hero_Y;
float jumpSpeed;

int frames=0;

int bump_no;
float cloudfSpeed=0.4;
int gameStatus;
int score=0;

bool jumped = false;

int brightness_level[7]={35,70,100,130,160,200,230};
#define BRIGHTNESS_LEVEL 7
int brightness=1;

long last_available_time = 0;
long game_start_time = 0;

int JUMP_TOP, JUMP_BOTTOM;

int hero_W, hero_H;
int heroSelectedIdx = 0;
int heroType=0;

int hero_front_Type=1;

void initGame(){
  if(heroSelectedIdx == 0){
    hero_W = KOOPA_W;
    hero_H = KOOPA_H;  
    jumpSpeed = -1.8;  
    initialSpeed = 1.0;
  }
  else if(heroSelectedIdx == 1){  
    hero_W = GENGAR_W;
    hero_H = GENGAR_H;  
    jumpSpeed = -2.2;  
    initialSpeed = 1.2;
  }
  JUMP_TOP = GROUND_Y - ENEMY_H - hero_H - 10;
  JUMP_BOTTOM = GROUND_Y + 14 - hero_H;
  
  hero_Y = JUMP_BOTTOM;
  fSpeed = initialSpeed; 
  
  cloudX[0] = random(0, 80);
  cloudX[1] = random(100, 180);
  cloudY[0] = random(20, 40);
  cloudY[1] = random(20, 40);
  
  enemyX[0] = random(240, 310);
  enemyX[1] = random(400, 470);
  enemyColor[0] = RandomColor();
  enemyColor[1] = RandomColor();
    
  frames = 0;
  heroType = 0;
  bump_no = random(0, 2);
  cloudfSpeed = 0.4;
  score = 0;

  for(int i=0; i<6; i++){
    linesX[i] = random(i*40, (i+1)*40);
    linesW[i] = random(1, 14);
    linesX2[i] = random(i*40, (i+1)*40);
    linesW2[i] = random(1, 14);
  }

  for(int n=0; n<2; n++){
    bumpsX[n] = random(n*90, (n+1)*120);
    bumpType[n] = random(0, 2); //0 or 1
  }

  spriteHero.createSprite(hero_W, hero_H);
  spriteEnemy0.createSprite(ENEMY_W, ENEMY_H);
  spriteEnemy1.createSprite(ENEMY_W, ENEMY_H);

}

void pressA(Button2& btn) {
  last_available_time = millis();
  
  if(gameStatus == STATUS_RUN){
    if(!jumped) {
      jumped = true;
      heroType=0;  
    }
  }
  else if(gameStatus == STATUS_GAMEOVER){
    gameStatus = STATUS_INIT;
  }
  else if(gameStatus == STATUS_INIT){
    last_available_time = millis();
    game_start_time = millis();
    tft.fillScreen(TFT_BLACK);
    initGame();
    gameStatus = STATUS_RUN;
  }
  
}

void pressB(Button2& btn) {  
  last_available_time = millis();

  if(gameStatus == STATUS_INIT){    
    if(heroSelectedIdx==0) heroSelectedIdx = 1;
    else if(heroSelectedIdx==1) heroSelectedIdx = 0;
  }
  else{
    if(brightness++ > BRIGHTNESS_LEVEL - 1) { 
      brightness=0; 
    }  
    ledcWrite(pwmLedChannelTFT, brightness_level[brightness]); 
  }
}

void setup() {   
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);  
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, LOW);
  
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);

  buttonA.setPressedHandler(pressA);
  buttonB.setPressedHandler(pressB);
  
  tft.init();    
  //tft.setSwapBytes(true);
  tft.setSwapBytes(false);
  
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
  
  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, brightness_level[brightness]);

  spriteScreen.createSprite(240, 135);
  spriteScreen.setTextColor(TFT_YELLOW);

  gameStatus = STATUS_INIT;
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
      hero_Y += jumpSpeed; 
      if(hero_Y <= JUMP_TOP)  { 
        jumpSpeed = -jumpSpeed; 
      }
      else if(hero_Y >= JUMP_BOTTOM) {
        hero_Y = JUMP_BOTTOM;
        jumpSpeed = -jumpSpeed; 
        jumped = false;
      }
    }
    if(frames < 8 && !jumped) heroType = 1;
    if(frames > 8 && !jumped) heroType = 2;
  
    drawS(heroType);
    frames++;
    if(frames==16) frames=0;
 
    checkColision();    
  }

  if(gameStatus == STATUS_GAMEOVER){
    spriteScreen.fillSprite(TFT_BLACK);    
    spriteScreen.setTextSize(2); 
    spriteScreen.drawString("GAME OVER", 25, 25, 2);  
    spriteScreen.drawString("Score: " + String(score), 25, 75, 2);      
    spriteScreen.pushSprite(0, 0);

    spriteHero.deleteSprite();
    spriteEnemy0.deleteSprite();
    spriteEnemy1.deleteSprite();    
  }   

  if(gameStatus == STATUS_INIT){
    showHeroSelection();   
  }

}

void drawS(int heroType){ 
  spriteScreen.fillSprite(TFT_BLACK);
  spriteScreen.drawLine(0, GROUND_Y, 240, GROUND_Y, TFT_WHITE);
  
  for(int i=0; i<6; i++){
    spriteScreen.drawLine(linesX[i], GROUND_Y+3 ,linesX[i]+linesW[i], GROUND_Y+3, TFT_WHITE);
    linesX[i] = linesX[i] - fSpeed;
    if(linesX[i] < -14){
      linesX[i]=random(245, 280);
      linesW[i]=random(1, 14);
    }
    spriteScreen.drawLine(linesX2[i], GROUND_Y+14 , linesX2[i]+linesW2[i], GROUND_Y+14, TFT_WHITE);
    linesX2[i] = linesX2[i] - fSpeed;
    if(linesX2[i] < -14){
      linesX2[i]=random(245, 280);
      linesW2[i]=random(1, 14);
    }
  }

  // Draw Cloud
  for(int j=0; j<2; j++){
    spriteScreen.drawXBitmap(cloudX[j], cloudY[j], cloud, 38, 11, TFT_WHITE, TFT_BLACK);
    cloudX[j] = cloudX[j] - cloudfSpeed;
    if(cloudX[j] < -40){
      cloudX[j] = random(244, 364);
      cloudY[j] = random(20, 40);
    }
  }

  // Draw Bump
  for(int n=0; n<2; n++){
    spriteScreen.drawXBitmap(bumpsX[n], GROUND_Y-4, bump[bumpType[n]], 34, 5, TFT_WHITE, TFT_BLACK);
    bumpsX[n] = bumpsX[n] - fSpeed;
    if(bumpsX[n] < -40){
      bumpsX[n] = random(244, 364);
      bumpType[n] = random(0, 2); //0 or 1
    }
  }

  // Move Enemy or Create New Enemy
  for(int enemyNo=0; enemyNo<2; enemyNo++){
    enemyX[enemyNo] -= fSpeed;
    if(enemyX[enemyNo] < -20) {
      enemyX[enemyNo] = random(240,360) + 60 * fSpeed; //Create New Enemy
//      do{
//        enemyX[enemyNo] = random(240,480); //Create New Enemy
//      }while(fabs(enemyX[0]-enemyX[1]) < 40+hero_W*fSpeed);     
      enemyColor[enemyNo] = RandomColor();
    }
  }
  
  spriteEnemy0.drawXBitmap(0, 0, enemy[0], ENEMY_W, ENEMY_H, enemyColor[0], TFT_BLACK);
  spriteEnemy1.drawXBitmap(0, 0, enemy[1], ENEMY_W, ENEMY_H, enemyColor[1], TFT_BLACK);
  if(heroSelectedIdx==0){
    spriteHero.pushImage(0, 0, hero_W, hero_H, koopa[heroType]); 
  }
  else if(heroSelectedIdx==1){    
    spriteHero.pushImage(0, 0, hero_W, hero_H, gengar[heroType]);  
  }  
    
  spriteEnemy0.pushToSprite(&spriteScreen, enemyX[0], ENEMY_Y, TFT_BLACK);
  spriteEnemy1.pushToSprite(&spriteScreen, enemyX[1], ENEMY_Y, TFT_BLACK);
  spriteHero.pushToSprite(&spriteScreen, HERO_X, hero_Y, TFT_GREEN);

  score=(millis()-game_start_time)/120;
  spriteScreen.setTextSize(1); 
  spriteScreen.drawString(String(score), 200, 12, 2);
  spriteScreen.pushSprite(0, 0);

  fSpeed = initialSpeed + (score/100)*0.1;  
}

int RandomColor(){
  int r = random(10,255);
  int g = random(10,255);
  int b = random(10,255);
  return tft.color565(r, g, b);
}

void checkColision(){
  for(int enemyNo=0; enemyNo<2; enemyNo++){
    if( enemyX[enemyNo] < HERO_X + hero_W/2 && 
        enemyX[enemyNo] > HERO_X && 
        hero_Y > JUMP_BOTTOM - ENEMY_H + 5){
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

void showHeroSelection(){
  spriteScreen.fillSprite(TFT_BLACK);   
  
  // Draw Rect
  if(heroSelectedIdx==0){
    spriteScreen.drawRect(30, 27, 80, 80, TFT_YELLOW);  
    spriteScreen.drawRect(130, 27, 80, 80, TFT_DARKGREY);
  }
  else{
    spriteScreen.drawRect(30, 27, 80, 80, TFT_DARKGREY);  
    spriteScreen.drawRect(130, 27, 80, 80, TFT_YELLOW);
  }

  spriteHero1.createSprite(KOOPA_FRONT_W, KOOPA_FRONT_H);
  spriteHero2.createSprite(GENGAR_FRONT_W, GENGAR_FRONT_H);

  int X0,Y0;
  X0 = 70 - KOOPA_FRONT_W/2;
  Y0 = 95 - KOOPA_FRONT_H + 5;
  int X1,Y1;
  X1 = 170 - GENGAR_FRONT_W/2;
  Y1 = 95  - GENGAR_FRONT_H - 5;
    
  // Draw Heros
  if(heroSelectedIdx == 0){ //select koopa
    // Hero 0 koopa walks
    spriteHero1.pushImage(0, 0, KOOPA_FRONT_W, KOOPA_FRONT_H, koopa_front[hero_front_Type]);
    spriteHero1.pushToSprite(&spriteScreen, X0, Y0, TFT_GREEN);

    // Hero 1 gengar Stop 
	  spriteHero2.pushImage(0, 0, GENGAR_FRONT_W, GENGAR_FRONT_H, gengar_front[0]);
    spriteHero2.pushToSprite(&spriteScreen, X1, Y1/*, TFT_BLACK*/);
    
  }
  else if(heroSelectedIdx == 1){
    // Hero 0 koopa Stop
	  spriteHero1.pushImage(0, 0, KOOPA_FRONT_W, KOOPA_FRONT_H, koopa_front[0]);
    spriteHero1.pushToSprite(&spriteScreen, X0, Y0, TFT_GREEN);

    // Hero 1 gengar walks
    spriteHero2.pushImage(0, 0, GENGAR_FRONT_W, GENGAR_FRONT_H, gengar_front[hero_front_Type]);
    spriteHero2.pushToSprite(&spriteScreen, X1, Y1/*, TFT_BLACK*/);
  }

  if(frames < 8 && !jumped) hero_front_Type = 1;
  if(frames > 8 && !jumped) hero_front_Type = 2;

  frames++;
  if(frames==16) frames=0;
  
  spriteScreen.pushSprite(0, 0); 
   
  spriteHero1.deleteSprite();
  spriteHero2.deleteSprite();  
}
