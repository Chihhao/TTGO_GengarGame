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
#include "goku.h"

#define STATUS_GAMEOVER 0
#define STATUS_RUN 1
#define STATUS_INIT 2

TFT_eSPI    tft = TFT_eSPI();        
TFT_eSprite spriteScreen = TFT_eSprite(&tft);
TFT_eSprite spriteHeroRun = TFT_eSprite(&tft);
TFT_eSprite spriteHeroJump = TFT_eSprite(&tft);
TFT_eSprite spriteHero0 = TFT_eSprite(&tft);
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

float heroSpeed;
int hero_Y;
float jumpSpeed;

int frames=0;
int flyingTime=0;

float cloudSpeed=0.4;
int gameStatus;
int score=0;

typedef enum {
    Running, Jumping, Falling, Flying    
}HeroStatus;
HeroStatus heroStatus = Running;

int brightness_level[7]={35,70,100,130,160,200,230};
#define BRIGHTNESS_LEVEL 7
int brightness=1;

long last_available_time = 0;
long game_start_time = 0;

int JUMP_TOP, JUMP_BOTTOM;

int hero_run_W, hero_run_H;
int hero_jump_W, hero_jump_H;
int hero_colision_diff;
int heroSelectedIdx = 0;
const int NO_HEROS = 3;


int hero_front_Type=1;

#include "EEPROM.h"
#define EEPROM_SIZE 4
long historyHighScore;

void initGame(){  
  
  frames = 0;
  score = 0;
  heroSpeed = initialSpeed; 
  heroStatus = Running;
  
  if(heroSelectedIdx == 0){
    hero_run_W = KOOPA_RUN_W;
    hero_run_H = KOOPA_RUN_H;  
    hero_jump_W = KOOPA_JUMP_W;
    hero_jump_H = KOOPA_JUMP_H;
    hero_colision_diff = 5;
  }
  else if(heroSelectedIdx == 1){  
    hero_run_W = GENGAR_RUN_W;
    hero_run_H = GENGAR_RUN_H; 
    hero_jump_W = GENGAR_JUMP_W;
    hero_jump_H = GENGAR_JUMP_H;  
    hero_colision_diff = 5;
  }
  else if(heroSelectedIdx == 2){  
    hero_run_W = GOKU_RUN_W;
    hero_run_H = GOKU_RUN_H;  
    hero_jump_W = GOKU_JUMP_W;
    hero_jump_H = GOKU_JUMP_H; 
    hero_colision_diff = 10;
  }

  if(heroSelectedIdx == 0){
    jumpSpeed = 1.8;
    initialSpeed = 1.0;
    JUMP_TOP = GROUND_Y - ENEMY_H - hero_jump_H - 10;
    JUMP_BOTTOM = GROUND_Y + 14 - hero_jump_H;
  }
  else if(heroSelectedIdx == 1){  
    jumpSpeed = 2.2;
    initialSpeed = 1.2;
    JUMP_TOP = GROUND_Y - ENEMY_H - hero_jump_H - 15;
    JUMP_BOTTOM = GROUND_Y + 14 - hero_jump_H;
  }
  else if(heroSelectedIdx == 2){  
    jumpSpeed = 2.0;
    initialSpeed = 1.4;
    JUMP_TOP = GROUND_Y - ENEMY_H - hero_jump_H - 15;
    JUMP_BOTTOM = GROUND_Y + 14 - hero_jump_H - 4;
  }
  
  hero_Y = JUMP_BOTTOM;
  
  cloudX[0] = random(0, 80);
  cloudX[1] = random(100, 180);
  cloudY[0] = random(20, 40);
  cloudY[1] = random(20, 40);
  
  enemyX[0] = random(240, 310);
  enemyX[1] = random(400, 470);
  enemyColor[0] = RandomColor();
  enemyColor[1] = RandomColor();

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

  spriteEnemy0.createSprite(ENEMY_W, ENEMY_H);
  spriteEnemy1.createSprite(ENEMY_W, ENEMY_H);
}

void pressA(Button2& btn) {
  last_available_time = millis();
  
  if(gameStatus == STATUS_RUN){
    if(heroStatus == Running) {
      heroStatus = Jumping;
    }
    else if(heroSelectedIdx==2 && heroStatus!=Flying){
      heroStatus = Flying;
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
    if(++heroSelectedIdx >= NO_HEROS) {
      heroSelectedIdx = 0;     
    }
  }
  else{
    if(brightness++ > BRIGHTNESS_LEVEL - 1) { 
      brightness=0; 
    }  
    ledcWrite(pwmLedChannelTFT, brightness_level[brightness]); 
  }
}

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  historyHighScore = EEPROMReadlong(0);
  
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
    if(heroStatus == Flying){
      if(flyingTime++ > 30) {      
        heroStatus = Falling;
        flyingTime = 0;
        if(frames < 8) { 
          frames = 15 - frames; 
        }        
      }
    }    
    else if(heroStatus == Jumping){ 
      hero_Y -= jumpSpeed; 
      if(hero_Y <= JUMP_TOP) {
        heroStatus = Falling; 
      }
    }
    else if(heroStatus == Falling){        
      hero_Y += jumpSpeed;        
      if(hero_Y >= JUMP_BOTTOM) {
        heroStatus = Running;
        hero_Y = JUMP_BOTTOM;          
      }       
    }

    drawSprite();
    checkColision();    
  }

  if(gameStatus == STATUS_GAMEOVER){    
    spriteScreen.fillSprite(TFT_BLACK);    
    spriteScreen.setTextSize(2); 
    //spriteScreen.drawString("GAME OVER", 25, 25, 2);  
    spriteScreen.drawString("Score : " + String(score), 25, 25, 2);  
    if(score > historyHighScore) {
      historyHighScore = score;
      EEPROMWritelong(0, historyHighScore);
    }
    spriteScreen.drawString("Max : " + String(historyHighScore), 25, 75, 2);      
    spriteScreen.pushSprite(0, 0);
    spriteEnemy0.deleteSprite();
    spriteEnemy1.deleteSprite();
    
  }   

  if(gameStatus == STATUS_INIT){
    showHeroSelection();   
  }

}

void drawSprite(){ 
  int runFrameNo = 0;
  if(heroStatus == Running){    
    if(frames < 8) runFrameNo = 0;
    if(frames > 8) runFrameNo = 1;
    if(++frames >= 16) frames = 0;
  }
  heroSpeed = initialSpeed + (score/100)*0.1; 
  
  spriteScreen.fillSprite(TFT_BLACK);

  // Draw Ground
  spriteScreen.drawLine(0, GROUND_Y, 240, GROUND_Y, TFT_WHITE);
  
  // Draw Texture of Ground
  for(int i=0; i<6; i++){
    spriteScreen.drawLine(linesX[i], GROUND_Y+3 ,linesX[i]+linesW[i], GROUND_Y+3, TFT_WHITE);
    linesX[i] = linesX[i] - heroSpeed;
    if(linesX[i] < -14){
      linesX[i]=random(245, 280);
      linesW[i]=random(1, 14);
    }
    spriteScreen.drawLine(linesX2[i], GROUND_Y+14 , linesX2[i]+linesW2[i], GROUND_Y+14, TFT_WHITE);
    linesX2[i] = linesX2[i] - heroSpeed;
    if(linesX2[i] < -14){
      linesX2[i]=random(245, 280);
      linesW2[i]=random(1, 14);
    }
  }
  
  // Draw Bumps of Ground
  for(int n=0; n<2; n++){
    spriteScreen.drawXBitmap(bumpsX[n], GROUND_Y-4, bump[bumpType[n]], 34, 5, TFT_WHITE, TFT_BLACK);
    bumpsX[n] = bumpsX[n] - heroSpeed;
    if(bumpsX[n] < -40){
      bumpsX[n] = random(244, 364);
      bumpType[n] = random(0, 2); //0 or 1
    }
  }

  // Draw Clouds
  for(int j=0; j<2; j++){
    spriteScreen.drawXBitmap(cloudX[j], cloudY[j], cloud, 38, 11, TFT_WHITE, TFT_BLACK);
    cloudX[j] = cloudX[j] - cloudSpeed;
    if(cloudX[j] < -40){
      cloudX[j] = random(244, 364);
      cloudY[j] = random(20, 40);
    }
  }

  // Draw Enemys 
  for(int enemyNo=0; enemyNo<2; enemyNo++){
    enemyX[enemyNo] -= heroSpeed;
    if(enemyX[enemyNo] < -20) {
      enemyX[enemyNo] = random(240,360) + 60 * heroSpeed; //Create New Enemy  
      enemyColor[enemyNo] = RandomColor();
    }
  }  
  spriteEnemy0.drawXBitmap(0, 0, enemy[0], ENEMY_W, ENEMY_H, enemyColor[0], TFT_BLACK);
  spriteEnemy1.drawXBitmap(0, 0, enemy[1], ENEMY_W, ENEMY_H, enemyColor[1], TFT_BLACK);  
  spriteEnemy0.pushToSprite(&spriteScreen, enemyX[0], ENEMY_Y, TFT_BLACK);
  spriteEnemy1.pushToSprite(&spriteScreen, enemyX[1], ENEMY_Y, TFT_BLACK);
    
  
  // Draw Hero
  if(heroStatus == Jumping ||  heroStatus == Falling){
	  spriteHeroJump.createSprite(hero_jump_W, hero_jump_H);
	  if(heroSelectedIdx==0){
		spriteHeroJump.pushImage(0, 0, hero_jump_W, hero_jump_H, koopa_jump);
	  }
	  else if(heroSelectedIdx==1){
		spriteHeroJump.pushImage(0, 0, hero_jump_W, hero_jump_H, gengar_jump);  
	  }  
	  else if(heroSelectedIdx==2){
		spriteHeroJump.pushImage(0, 0, hero_jump_W, hero_jump_H, goku_jump);  
	  }	  
	  spriteHeroJump.pushToSprite(&spriteScreen, HERO_X, hero_Y, TFT_GREEN);
  }
  else{
	  spriteHeroRun.createSprite(hero_run_W, hero_run_H);
	  if(heroSelectedIdx==0){
		spriteHeroRun.pushImage(0, 0, hero_run_W, hero_run_H, koopa_run[runFrameNo]);		
	  }
	  else if(heroSelectedIdx==1){    
		spriteHeroRun.pushImage(0, 0, hero_run_W, hero_run_H, gengar_run[runFrameNo]);  		
	  }  
	  else if(heroSelectedIdx==2){    
		spriteHeroRun.pushImage(0, 0, hero_run_W, hero_run_H, goku_run);  		
	  }	  
	  spriteHeroRun.pushToSprite(&spriteScreen, HERO_X, hero_Y, TFT_GREEN);
  }   

  // Draw Score
  score=(millis()-game_start_time)/120;
  spriteScreen.setTextSize(1); 
  spriteScreen.drawString(String(score), 200, 12, 2);

  // Show Sprite
  spriteScreen.pushSprite(0, 0);

  // Delete Sprite
  spriteHeroRun.deleteSprite();
  spriteHeroJump.deleteSprite();
  
}

int RandomColor(){
  int r = random(10,255);
  int g = random(10,255);
  int b = random(10,255);
  return tft.color565(r, g, b);
}

void checkColision(){
  bool colision = false; 

  for(int enemyNo=0; enemyNo<2; enemyNo++){      
    if( enemyX[enemyNo] < HERO_X + hero_jump_W/2 && 
        enemyX[enemyNo] > HERO_X && 
        hero_Y > JUMP_BOTTOM - ENEMY_H + hero_colision_diff){
      colision = true;
    }
  }

  if(colision){
    delay(500); 
    gameStatus = STATUS_GAMEOVER;
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
  
  if(frames < 8) hero_front_Type = 1;
  if(frames > 8) hero_front_Type = 2;
  if(++frames==16) frames=0;
  
  // Draw Rect
  if(heroSelectedIdx==0){
    spriteScreen.drawRect(1, 27, 80, 80, TFT_YELLOW);  
    spriteScreen.drawRect(81, 27, 79, 80, TFT_DARKGREY);
    spriteScreen.drawRect(160, 27, 80, 80, TFT_DARKGREY);
  }
  else if(heroSelectedIdx==1){
    spriteScreen.drawRect(1, 27, 80, 80, TFT_DARKGREY);  
    spriteScreen.drawRect(81, 27, 79, 80, TFT_YELLOW);
    spriteScreen.drawRect(160, 27, 80, 80, TFT_DARKGREY);
  }
  else if(heroSelectedIdx==2){
    spriteScreen.drawRect(1, 27, 80, 80, TFT_DARKGREY);  
    spriteScreen.drawRect(81, 27, 79, 80, TFT_DARKGREY);
    spriteScreen.drawRect(160, 27, 80, 80, TFT_YELLOW);
  }

  int X0 = 40 - KOOPA_FRONT_W/2;
  int Y0 = 95 - KOOPA_FRONT_H + 5;

  int X1 = 119 - GENGAR_FRONT_W/2;
  int Y1 = 95  - GENGAR_FRONT_H - 5;

  int X2 = 199 - GOKU_FRONT_W/2 - 1;
  int Y2 = 95 - GOKU_FRONT_H + 2; 
	
  // Draw Heros
  spriteHero0.createSprite(KOOPA_FRONT_W, KOOPA_FRONT_H);
  spriteHero1.createSprite(GENGAR_FRONT_W, GENGAR_FRONT_H);
  spriteHero2.createSprite(GOKU_FRONT_W, GOKU_FRONT_H);
  if(heroSelectedIdx == 0){ 
    // Hero 0 koopa walks
    spriteHero0.pushImage(0, 0, KOOPA_FRONT_W, KOOPA_FRONT_H, koopa_front[hero_front_Type]);
    spriteHero0.pushToSprite(&spriteScreen, X0, Y0, TFT_GREEN);

    // Hero 1 gengar Stop 
    spriteHero1.pushImage(0, 0, GENGAR_FRONT_W, GENGAR_FRONT_H, gengar_front[0]);
    spriteHero1.pushToSprite(&spriteScreen, X1, Y1/*, TFT_BLACK*/);
	
    // Hero 2 koopa Stop
    spriteHero2.pushImage(0, 0, GOKU_FRONT_W, GOKU_FRONT_H, goku_front[0]);
    spriteHero2.pushToSprite(&spriteScreen, X2, Y2, TFT_GREEN);    
  }
  else if(heroSelectedIdx == 1){
    // Hero 0 koopa Stop
    spriteHero0.pushImage(0, 0, KOOPA_FRONT_W, KOOPA_FRONT_H, koopa_front[0]);
    spriteHero0.pushToSprite(&spriteScreen, X0, Y0, TFT_GREEN);

    // Hero 1 gengar walks
    spriteHero1.pushImage(0, 0, GENGAR_FRONT_W, GENGAR_FRONT_H, gengar_front[hero_front_Type]);
    spriteHero1.pushToSprite(&spriteScreen, X1, Y1/*, TFT_BLACK*/);
	
    // Hero 2 koopa Stop
    spriteHero2.pushImage(0, 0, GOKU_FRONT_W, GOKU_FRONT_H, goku_front[0]);
    spriteHero2.pushToSprite(&spriteScreen, X2, Y2, TFT_GREEN);   
  }
  else if(heroSelectedIdx == 2){
    // Hero 0 koopa Stop
    spriteHero0.pushImage(0, 0, KOOPA_FRONT_W, KOOPA_FRONT_H, koopa_front[0]);
    spriteHero0.pushToSprite(&spriteScreen, X0, Y0, TFT_GREEN);

    // Hero 1 gengar Stop 
    spriteHero1.pushImage(0, 0, GENGAR_FRONT_W, GENGAR_FRONT_H, gengar_front[0]);
    spriteHero1.pushToSprite(&spriteScreen, X1, Y1/*, TFT_BLACK*/);
	
    // Hero 0 koopa walks
    spriteHero2.pushImage(0, 0, GOKU_FRONT_W, GOKU_FRONT_H, goku_front[hero_front_Type]);
    spriteHero2.pushToSprite(&spriteScreen, X2, Y2, TFT_GREEN);
  } 
    
  spriteScreen.pushSprite(0, 0); 
   
  spriteHero0.deleteSprite();
  spriteHero1.deleteSprite();  
  spriteHero2.deleteSprite(); 
}

void EEPROMWritelong(int address, long value) {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
      EEPROM.commit();
}

//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to adress + 3.
long EEPROMReadlong(long address) {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
