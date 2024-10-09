// Board: ESP32 Dev Module
// CPU Freq: 240MHz
// Flash Freq: 80Hz
// Flash Mode: QIO
// Flash Size: 4MB (32Mb)
// Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
// PSRAM: Disabled

// Library:
// ESP32 Library Version: 2.0.2
// TFT_eSPI Library Version: 2.4.32
// Button2 Library Version: 1.4.0

// Screen Size: 240*135

#include <TFT_eSPI.h>
#include "EEPROM.h"
#include "Button2.h"

#include "item.h"
#include "gengar.h"
#include "koopa.h"
#include "goku.h"

#define BUTTON_A_PIN  0
#define BUTTON_B_PIN  35
#define PIN_BAT_ADC   34
#define PIN_POWER_EN  14

#define VERSION "V2"
#define MY_WIDTH  TFT_HEIGHT  // 240
#define MY_HEIGHT TFT_WIDTH   // 135  
#define TIME_TO_SLEEP          12000  // 12s
#define TIME_TO_ADD_SKILL_NO   10000  // 10s
#define TIME_TO_RESUME_GIANT   2000   // 2s
#define PWM_FRQ 5000
#define PWM_RES 8
#define PWM_CHN 0
#define SKILL_INIT_NO 5

TFT_eSPI    tft = TFT_eSPI();        
TFT_eSprite spriteScreen = TFT_eSprite(&tft);
TFT_eSprite spriteHeroRun = TFT_eSprite(&tft);
TFT_eSprite spriteHeroJump = TFT_eSprite(&tft);
TFT_eSprite spriteHero0 = TFT_eSprite(&tft);
TFT_eSprite spriteHero1 = TFT_eSprite(&tft);
TFT_eSprite spriteHero2 = TFT_eSprite(&tft);
TFT_eSprite spriteEnemy0 = TFT_eSprite(&tft);
TFT_eSprite spriteEnemy1 = TFT_eSprite(&tft);
TFT_eSprite spriteFireball = TFT_eSprite(&tft);
TFT_eSprite spriteGameOver = TFT_eSprite(&tft);
Button2 buttonA = Button2(BUTTON_A_PIN);
Button2 buttonB = Button2(BUTTON_B_PIN);

float initialSpeed;

float linesX[6], linesX2[6]; // 地板紋理 X 座標
int linesW[6], linesW2[6];   // 地板紋理 X 長度
float cloudX[2], cloudY[2];  // 雲朵 X/Y 座標
float bumpsX[2];             // 地板凸起 X 座標
int bumpType[2];             // 地板凸起類別 (大/小)
float enemyX[2];             // 敵人 X 座標
int enemyColor[2];
bool enemyVisible[2];
int fireBallX;

float heroSpeed;
int hero_Y;
float jumpSpeed;

int frames=0;
int flyingTime=0;

float cloudSpeed=0.4;
float fireBallSpeed=1.0;
int score=0;

typedef enum { GAMEOVER, RUN, INIT } GameState;
GameState gameState = INIT;

typedef enum { HERO_KOOPA, HERO_GENGAR, HERO_GOKU } Hero;
Hero hero = HERO_KOOPA;

typedef enum { ACT_RUN, ACT_JUMP, ACT_FALL, ACT_FLY, ACT_GIANT } HeroAction;
HeroAction heroAction = ACT_RUN;

bool fireBallTrigger = false;

long last_available_time = 0;
long game_start_time = 0;
long last_skill_add_time=0;
long last_giant_time=0;

int JUMP_TOP, JUMP_BOTTOM;

int hero_run_W, hero_run_H;
int hero_jump_W, hero_jump_H;
int hero_colision_diff;

int hero_front_Type=1;

int batChargeAnimation = 4;
unsigned long timestamp_charging_animation = 0;

struct EEPROMDATA{ int maxScore[3]; } eepromData;

int skillNo;  // 技能點數

void initGame(){    
  frames = 0;
  score = 0;
  heroSpeed = initialSpeed; 
  heroAction = ACT_RUN;
  fireBallTrigger = false;
  skillNo = SKILL_INIT_NO;
  
  switch(hero){
	case HERO_KOOPA:{
      hero_run_W = KOOPA_RUN_W;
      hero_run_H = KOOPA_RUN_H;  
      hero_jump_W = KOOPA_JUMP_W;
      hero_jump_H = KOOPA_JUMP_H;
      hero_colision_diff = 5;
	  break;
	}
	case HERO_GENGAR:{
      hero_run_W = GENGAR_RUN_W;
      hero_run_H = GENGAR_RUN_H; 
      hero_jump_W = GENGAR_JUMP_W;
      hero_jump_H = GENGAR_JUMP_H;  
      hero_colision_diff = 5;
	  break;
	}
	case HERO_GOKU:{
      hero_run_W = GOKU_RUN_W;
      hero_run_H = GOKU_RUN_H;  
      hero_jump_W = GOKU_JUMP_W;
      hero_jump_H = GOKU_JUMP_H; 
      hero_colision_diff = 10;
	  break;
	}
  }
  
  switch(hero){
	case HERO_KOOPA:{
      jumpSpeed = 1.8;
      initialSpeed = 1.0;
      JUMP_TOP = GROUND_Y - ENEMY_H - hero_jump_H - 10;
      JUMP_BOTTOM = GROUND_Y + 14 - hero_jump_H;
	  break;
	}
	case HERO_GENGAR:{
      jumpSpeed = 2.2;
      initialSpeed = 1.2;
      JUMP_TOP = GROUND_Y - ENEMY_H - hero_jump_H - 15;
      JUMP_BOTTOM = GROUND_Y + 14 - hero_jump_H;
	  break;
	}
	case HERO_GOKU:{
      jumpSpeed = 2.0;
      initialSpeed = 1.4;
      JUMP_TOP = GROUND_Y - ENEMY_H - hero_jump_H - 15;
      JUMP_BOTTOM = GROUND_Y + 14 - hero_jump_H - 4;
	  break;
	}
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
  enemyVisible[0] = true;
  enemyVisible[1] = true;
  
  fireBallX = FIREBALL_X;  // 火球位置恢復原點

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
  
  last_skill_add_time = millis();
}

// Enter / Jump Button
void pressA(Button2& btn) {
  last_available_time = millis();

  switch(gameState){
    case RUN:{      
      if(heroAction == ACT_RUN) {
        heroAction = ACT_JUMP;
        break;
      }
      if(hero == HERO_GOKU){
        if(heroAction == ACT_JUMP || heroAction == ACT_FALL){
          if(skillNo > 0){
            heroAction = ACT_FLY;
            skillNo--;
          }
        } 
      }
      break;
    } 
    case GAMEOVER:{
      gameState = INIT;
      break;
    }
    case INIT:{
      game_start_time = millis();
      tft.fillScreen(TFT_BLACK);
      initGame();
      gameState = RUN;
      break;   
    }   
  }  
}

// Select / Skill Button
void pressB(Button2& btn) {  
  last_available_time = millis();

  switch(gameState){
    case RUN:{
      switch(hero){
        case HERO_KOOPA:{
          if(heroAction==ACT_RUN && !fireBallTrigger){
            if(skillNo > 0){
              fireBallTrigger = true;
              skillNo--;
            }
          }
          break;
        }
        case HERO_GENGAR:{
          if(heroAction==ACT_RUN){
            if(skillNo > 0){
              heroAction = ACT_GIANT;
              last_giant_time = millis();
              skillNo--;
            }
          }
          else if(heroAction==ACT_GIANT){
            heroAction=ACT_RUN;
          } 
          break;
        }
        case HERO_GOKU:{
          if(heroAction == ACT_RUN) {
            heroAction = ACT_JUMP;
          }
          else if(heroAction == ACT_JUMP || heroAction == ACT_FALL){
            if(skillNo > 0){
              heroAction = ACT_FLY;
              skillNo--;
            }
          } 
          break;
        }
      }
      break;
    }
    case GAMEOVER:{
      gameState = INIT;
      break;
    }
    case INIT:{
      switch(hero){
        case HERO_KOOPA:{
		  hero = HERO_GENGAR;
          break;
        }
        case HERO_GENGAR:{
		  hero = HERO_GOKU;
          break;
        }
        case HERO_GOKU:{
		  hero = HERO_KOOPA;
          break;
        }
      }
      break;   
    }   
  }
}

void setup() {
  Serial.begin(112500);
  delay(10);

  //從EEPROM載入設定
  Serial.println("enable EEPROM");   
  EEPROM.begin(4096); 
  // initializeFirstTime();  
  EEPROM_readAnything(eepromData);  

  // 設定深度睡眠喚醒
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);  
  //esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, LOW);
  
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pinMode(PIN_BAT_ADC, INPUT);
  pinMode(PIN_POWER_EN, OUTPUT);
  digitalWrite(PIN_POWER_EN, HIGH);

  buttonA.setPressedHandler(pressA);
  buttonB.setPressedHandler(pressB);
  
  tft.init();
  tft.setSwapBytes(false);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  // 設定螢幕亮度
  ledcSetup(PWM_CHN, PWM_FRQ, PWM_RES);
  ledcAttachPin(TFT_BL, PWM_CHN);
  ledcWrite(PWM_CHN, 127);   
  
  spriteScreen.createSprite(MY_WIDTH, MY_HEIGHT);
  spriteScreen.setTextColor(TFT_YELLOW); 

  gameState = INIT;  
}

void loop() {  
  buttonA.loop();  
  buttonB.loop(); 
  
  // 待機過長則進入睡眠
  if(gameState != RUN){
    if( millis() - last_available_time > TIME_TO_SLEEP) {
      doSleep();
    }
  }
  
  if(gameState == RUN){	  
	switch(heroAction){
	  case ACT_FLY:{
        if(flyingTime++ > 30) {      
          heroAction = ACT_FALL;
          flyingTime = 0;
          if(frames < 8) { 
            frames = 15 - frames; 
          }        
        }		
	    break;
	  }
	  case ACT_JUMP:{
        hero_Y -= jumpSpeed; 
        if(hero_Y <= JUMP_TOP) {
          heroAction = ACT_FALL; 
        }
	    break;
	  }
	  case ACT_FALL:{
        hero_Y += jumpSpeed;        
        if(hero_Y >= JUMP_BOTTOM) {
          heroAction = ACT_RUN;
          hero_Y = JUMP_BOTTOM;          
        }  
	    break;
	  }
	} 

    drawSprite();
    checkColision();
  }

  if(gameState == GAMEOVER){  
    showScore(); 
    spriteEnemy0.deleteSprite();
    spriteEnemy1.deleteSprite();
  }   

  if(gameState == INIT){
    showHeroSelection();   
  }

  // 增加技能值
  if( millis() - last_skill_add_time > TIME_TO_ADD_SKILL_NO) {
    last_skill_add_time = millis();
    skillNo++;
  }

  // 確認是否從巨人狀態恢復
  if(heroAction==ACT_GIANT){
    if( millis() - last_giant_time > TIME_TO_RESUME_GIANT) {
      heroAction = ACT_RUN; 
    }
  }

}

void drawSprite(){ 
  int runFrameNo = 0;
  if(heroAction == ACT_RUN || heroAction == ACT_GIANT){    
    if(frames < 8) runFrameNo = 0;
    if(frames > 8) runFrameNo = 1;
    if(++frames >= 16) frames = 0;
  }
  heroSpeed = initialSpeed + (score/100)*0.1; 
  
  spriteScreen.fillSprite(TFT_BLACK);

  // 畫地板
  spriteScreen.drawLine(0, GROUND_Y, MY_WIDTH, GROUND_Y, TFT_WHITE);
  
  // 畫地板紋理
  for(int i=0; i<6; i++){
    spriteScreen.drawLine(linesX[i], GROUND_Y+3 ,linesX[i]+linesW[i], GROUND_Y+3, TFT_WHITE);
    linesX[i] -= heroSpeed;
    if(linesX[i] < -14){
      linesX[i]=random(245, 280);
      linesW[i]=random(1, 14);
    }
    spriteScreen.drawLine(linesX2[i], GROUND_Y+14 , linesX2[i]+linesW2[i], GROUND_Y+14, TFT_WHITE);
    linesX2[i] -= heroSpeed;
    if(linesX2[i] < -14){
      linesX2[i]=random(245, 280);
      linesW2[i]=random(1, 14);
    }
  }
  
  // 畫地板凸起物
  for(int n=0; n<2; n++){
    spriteScreen.drawXBitmap(bumpsX[n], GROUND_Y-4, bump[bumpType[n]], 34, 5, TFT_WHITE, TFT_BLACK);
    bumpsX[n] -= heroSpeed;
    if(bumpsX[n] < -40){
      bumpsX[n] = random(244, 364);
      bumpType[n] = random(0, 2); //0 or 1
    }
  }

  // 畫雲朵
  for(int j=0; j<2; j++){
    spriteScreen.drawXBitmap(cloudX[j], cloudY[j], cloud, 38, 11, TFT_WHITE, TFT_BLACK);
    cloudX[j] -= cloudSpeed;
    if(cloudX[j] < -40){
      cloudX[j] = random(244, 364);
      cloudY[j] = random(20, 40);
    }
  }

  // 畫敵人
  for(int enemyNo=0; enemyNo<2; enemyNo++){
    enemyX[enemyNo] -= heroSpeed;
    if(enemyX[enemyNo] < -20) {
      enemyX[enemyNo] = random(240,360) + 60 * heroSpeed; //Create New Enemy  
      enemyColor[enemyNo] = RandomColor();
      enemyVisible[enemyNo] = true;
    }
  }  
  if(enemyVisible[0]){  
    spriteEnemy0.drawXBitmap(0, 0, enemy[0], ENEMY_W, ENEMY_H, enemyColor[0], TFT_BLACK);
    spriteEnemy0.pushToSprite(&spriteScreen, enemyX[0], ENEMY_Y, TFT_BLACK);
  }
  if(enemyVisible[1]){  
    spriteEnemy1.drawXBitmap(0, 0, enemy[1], ENEMY_W, ENEMY_H, enemyColor[1], TFT_BLACK);
    spriteEnemy1.pushToSprite(&spriteScreen, enemyX[1], ENEMY_Y, TFT_BLACK);
  }
  
  // 畫主角
  if(heroAction == ACT_JUMP ||  heroAction == ACT_FALL){
	  spriteHeroJump.createSprite(hero_jump_W, hero_jump_H);	  
	  if(hero == HERO_KOOPA){
		spriteHeroJump.pushImage(0, 0, hero_jump_W, hero_jump_H, koopa_jump);
	  }
	  else if(hero == HERO_GENGAR){
		spriteHeroJump.pushImage(0, 0, hero_jump_W, hero_jump_H, gengar_jump);  
	  }  
	  else if(hero == HERO_GOKU){
		spriteHeroJump.pushImage(0, 0, hero_jump_W, hero_jump_H, goku_jump);  
	  }	  
	  spriteHeroJump.pushToSprite(&spriteScreen, HERO_X, hero_Y, TFT_GREEN);
  }
  else if(heroAction == ACT_GIANT){
    spriteHeroRun.createSprite(hero_run_W*2, hero_run_H*2);    
    spriteHeroRun.pushImage(0, 0, hero_run_W*2, hero_run_H*2, gengar_giant[runFrameNo]);
    spriteHeroRun.pushToSprite(&spriteScreen, HERO_X-hero_run_W/2, hero_Y-hero_run_H, TFT_GREEN);
  }   
  else{
	  spriteHeroRun.createSprite(hero_run_W, hero_run_H);
	  if(hero == HERO_KOOPA){
		  spriteHeroRun.pushImage(0, 0, hero_run_W, hero_run_H, koopa_run[runFrameNo]);		
	  }
	  else if(hero == HERO_GENGAR){    
		  spriteHeroRun.pushImage(0, 0, hero_run_W, hero_run_H, gengar_run[runFrameNo]);  		
	  }  
	  else if(hero == HERO_GOKU){    
		  spriteHeroRun.pushImage(0, 0, hero_run_W, hero_run_H, goku_run);  		
	  }	  
	  spriteHeroRun.pushToSprite(&spriteScreen, HERO_X, hero_Y, TFT_GREEN);
  }   

  // 畫火球
  if(fireBallTrigger){    
    spriteFireball.createSprite(FIREBALL_W, FIREBALL_H);
    spriteFireball.pushImage(0, 0, FIREBALL_W, FIREBALL_H, koopa_fireball);  
    spriteFireball.pushToSprite(&spriteScreen, fireBallX, FIREBALL_Y, TFT_BLACK);
    spriteFireball.deleteSprite();
    
    fireBallX += fireBallSpeed;
    if(fireBallX >= MY_WIDTH){      
      fireBallTrigger = false;
      fireBallX = FIREBALL_X;  // 火球位置恢復原點
    }
  }

  // 畫分數
  score=(millis()-game_start_time)/120;
  spriteScreen.setTextSize(1); 
  spriteScreen.drawString(String(score), 200, 12, 2);

  // 畫技能點
  spriteScreen.setTextSize(1);   
  spriteScreen.drawString(String(skillNo), 12, 12, 2);
  
  // 印出
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
  if(heroAction == ACT_GIANT) return;
  bool colision = false; 

  if(fireBallTrigger){
    for(int enemyNo=0; enemyNo<2; enemyNo++){      
      if( enemyX[enemyNo] < fireBallX + FIREBALL_W - 15 && 
          enemyVisible[enemyNo] == true){
        enemyVisible[enemyNo]=false;
        fireBallTrigger = false;
        fireBallX = FIREBALL_X;  // 火球位置恢復原點
      }
    }
  }

  for(int enemyNo=0; enemyNo<2; enemyNo++){   
    if(!enemyVisible[enemyNo]) continue;
    if( enemyX[enemyNo] < HERO_X + hero_jump_W/2 && 
        enemyX[enemyNo] > HERO_X && 
        hero_Y > JUMP_BOTTOM - ENEMY_H + hero_colision_diff){
      colision = true;
    }
  }


  if(colision){
    delay(500); 
    gameState = GAMEOVER;
  }
}

void doSleep(){
    pinMode(TFT_BL, OUTPUT);     
    digitalWrite(TFT_BL, !TFT_BACKLIGHT_ON);  
    tft.writecommand(ST7789_DISPOFF);// Switch off the display    
    tft.writecommand(ST7789_SLPIN);// Sleep the display driver
    esp_deep_sleep_start();
}

double mapf(double x, double in_min, double in_max, double out_min, double out_max){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double getBatteryVolts(){
  int bat = analogRead(PIN_BAT_ADC); 
  double adc_ratio = ((double)bat/4096.0) * 2;  
  double volts = adc_ratio * 3.3;
  return volts;
}

int getBatteryPersentage(double volts){
  double persentage = mapf(volts, 3.2, 3.7, 0, 100);
  if(persentage >= 100){ persentage = 100; }
  if(persentage <= 0){ persentage = 0; }
  return  (int)persentage;
}

void showHeroSelection(){
  // spriteScreen.fillSprite(TFT_BLACK);     
  spriteScreen.fillRect(0 ,27, MY_WIDTH, MY_HEIGHT-27, TFT_BLACK);  // clear
  
  if(frames < 8) hero_front_Type = 1;
  if(frames > 8) hero_front_Type = 2;
  if(++frames==16) frames=0;
  
  { // 畫電池
    double dBatVolts = getBatteryVolts();
    int dBatPeresntage = getBatteryPersentage(dBatVolts);  
    
    // 畫電池外框 
    const int _RIGHT = 0;
    const int _TOP = 6;
    const int _WIDTH = 43;
    const int _HEIGHT = 15;
    const int _HEAD = 5;
    int _LEFT = MY_WIDTH - _RIGHT - _WIDTH;
    int _BOTTOM = _TOP + _HEIGHT;  
    spriteScreen.drawRect(_LEFT, _TOP, _WIDTH, _HEIGHT, TFT_DARKGREY);   
    spriteScreen.drawRect(_LEFT-_HEAD+1, _TOP+_HEAD, _HEAD, _HEAD, TFT_DARKGREY);
    spriteScreen.drawLine(_LEFT, _TOP+_HEAD ,_LEFT, _TOP+_HEAD+_HEAD-1, TFT_BLACK);   
  
    // 畫電池裡面
    const int _C_LEFT = _LEFT + 2;
    const int _C_TOP = _TOP + 2;
    const int _C_WITDH = _WIDTH / 5 - 1;
    const int _C_HEIGHT = _HEIGHT - 4;

    if(dBatVolts>4.0){  // 充電中
      if(millis() - timestamp_charging_animation > 500){  
        timestamp_charging_animation = millis();
        if(batChargeAnimation==4) {
          spriteScreen.fillRect(_LEFT+1, _TOP+1, _WIDTH-2, _HEIGHT-2, TFT_BLACK);  //clear
        }
        for(int i=4; i>=batChargeAnimation; i--){
          spriteScreen.fillRect(_C_LEFT + (_C_WITDH+1)*i, _C_TOP, _C_WITDH, _C_HEIGHT, TFT_GREEN);
        }        
        if(--batChargeAnimation<0) {
          batChargeAnimation=4;          
        }        
      } 
    }
    else{
      spriteScreen.fillRect(_LEFT+1, _TOP+1, _WIDTH-2, _HEIGHT-2, TFT_BLACK);  //clear      
      
      int level;
      if(dBatPeresntage<=0)       { level = 5; }
      else if(dBatPeresntage<=20) { level = 4; }
      else if(dBatPeresntage<=40) { level = 3; }
      else if(dBatPeresntage<=60) { level = 2; }
      else if(dBatPeresntage<=80) { level = 1; }
      else                        { level = 0; }
    
      if(level < 5){
        for(int i=4; i>=level; i--){
          spriteScreen.fillRect(_C_LEFT + (_C_WITDH+1)*i, _C_TOP, _C_WITDH, _C_HEIGHT, TFT_GREEN);
        }   
      }      
    }

    // Words
//    spriteScreen.fillRect(0 ,0, 180, 27, TFT_BLACK);  // clear
//    spriteScreen.setTextSize(1); 
//    if(dBatVolts>4.0){
//      spriteScreen.drawString("Battery : (Charging)", 3, 5, 2);                            
//    }
//    else{
//      spriteScreen.drawString("Battery : " + String(dBatPeresntage) +"% (" + String(dBatVolts, 1) + "V)" , 
//                              3, 5, 2);  
//    }
  }

  // 畫頂部文字
  spriteScreen.fillRect(0 ,0, 190, 27, TFT_BLACK);  // clear
  spriteScreen.setTextSize(1); 
  if(hero == HERO_KOOPA){ spriteScreen.drawString("Select : Koopa", 3, 5, 2); }
  if(hero == HERO_GENGAR){ spriteScreen.drawString("Select : Gengar", 3, 5, 2); }
  if(hero == HERO_GOKU){ spriteScreen.drawString("Select : Goku", 3, 5, 2); }
  spriteScreen.drawString(VERSION, 170, 5, 2);
  
  // 畫底部文字
  spriteScreen.drawString("github.com/Chihhao/TTGO_GengarGame", 0, 114, 2);  
  
  // 畫框框
  spriteScreen.drawRect(1, 27, 80, 80, hero==HERO_KOOPA?TFT_YELLOW:TFT_DARKGREY);  
  spriteScreen.drawRect(81, 27, 79, 80, hero==HERO_GENGAR?TFT_YELLOW:TFT_DARKGREY);
  spriteScreen.drawRect(160, 27, 80, 80, hero==HERO_GOKU?TFT_YELLOW:TFT_DARKGREY);


  
  // 畫角色
  int X0 = 40 - KOOPA_FRONT_W/2;
  int Y0 = 95 - KOOPA_FRONT_H + 5;
  int X1 = 119 - GENGAR_FRONT_W/2;
  int Y1 = 95  - GENGAR_FRONT_H - 5;
  int X2 = 199 - GOKU_FRONT_W/2 - 1;
  int Y2 = 95 - GOKU_FRONT_H + 2;   
  spriteHero0.createSprite(KOOPA_FRONT_W, KOOPA_FRONT_H);
  spriteHero1.createSprite(GENGAR_FRONT_W, GENGAR_FRONT_H);
  spriteHero2.createSprite(GOKU_FRONT_W, GOKU_FRONT_H);
  if(hero==HERO_KOOPA){ 
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
  else if(hero==HERO_GENGAR){
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
  else if(hero==HERO_GOKU){
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
    
  // 印出
  spriteScreen.pushSprite(0, 0); 
   
  spriteHero0.deleteSprite();
  spriteHero1.deleteSprite();  
  spriteHero2.deleteSprite(); 
}

void showScore(){	
	// 清空
    spriteScreen.fillSprite(TFT_BLACK);    
    spriteScreen.setTextSize(2); 
    
	// 畫此次分數
    spriteScreen.drawString("Score : " + String(score), 40, 8, 2);  

	// 更新最高分 (EEPROM)
	int _idx;
	if(hero==HERO_KOOPA) {_idx=0;}
	if(hero==HERO_GENGAR){_idx=1;}
	if(hero==HERO_GOKU)  {_idx=2;}
    if(score > eepromData.maxScore[_idx]) {
      eepromData.maxScore[_idx] = score;
      EEPROM_writeAnything(eepromData);
    }
   
    // 畫框框
    spriteScreen.drawRect(1, 50, 80, 80, hero==HERO_KOOPA?TFT_YELLOW:TFT_DARKGREY);  
    spriteScreen.drawRect(81, 50, 79, 80, hero==HERO_GENGAR?TFT_YELLOW:TFT_DARKGREY);
    spriteScreen.drawRect(160, 50, 80, 80, hero==HERO_GOKU?TFT_YELLOW:TFT_DARKGREY);

    // 畫最高分
    spriteScreen.drawString("Max", 18, 57, 2);  
    spriteScreen.drawString("Max", 98, 57, 2); 
    spriteScreen.drawString("Max", 178, 57, 2); 
    char s0[5],s1[5],s2[5];
    sprintf(s0, "%4d", eepromData.maxScore[0]);
    sprintf(s1, "%4d", eepromData.maxScore[1]);
    sprintf(s2, "%4d", eepromData.maxScore[2]);
    spriteScreen.drawString(s0, 10, 89, 2);  
    spriteScreen.drawString(s1, 90, 89, 2); 
    spriteScreen.drawString(s2, 170, 89, 2); 
	
	// 印出
    spriteScreen.pushSprite(0, 0);
}

//https://forum.arduino.cc/index.php?topic=41497.0
template <class T> int EEPROM_writeAnything(const T& value){
   int ee=0;
   const byte* p = (const byte*)(const void*)&value;
   int i;
   for (i = 0; i < sizeof(value); i++){
       EEPROM.write(ee++, *p++);
       vTaskDelay(1); //避免觸發看門狗
   }
   EEPROM.commit(); 
   return i;
}

//https://forum.arduino.cc/index.php?topic=41497.0
template <class T> int EEPROM_readAnything(T& value){
  int ee=0;
   byte* p = (byte*)(void*)&value;
   int i;
   for (i = 0; i < sizeof(value); i++){
       *p++ = EEPROM.read(ee++);
   }
   return i;
}

void initializeFirstTime(){
  eepromData.maxScore[0]=0;
  eepromData.maxScore[1]=0;
  eepromData.maxScore[2]=0;
  EEPROM_writeAnything(eepromData);
  delay(2);
}
