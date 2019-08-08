// ****************************************************************************
// ***  Wof_Cam.ino                                    A. Vreugdenhil @2019 ***
// ***                                                                      ***
// ***  W/o\F = Walk of Fame                        a.vreugdenhil@hccnet.nl ***
// ***                                                                      ***
// ***  Doel: Mbv een ESP32-CAM een 2-servo walker bouwen.                  ***
// ***        Tijdens de Roborama 2019/11 een lijn gaan volgen.             ***
// ***        [ We gaan niet voor het spel, maar voor de Knikkers ;-) ]     ***
// ***                                                                      ***
// ***  dd 20190723 Let's Walk. Omzetting vanuit PlatformIO naar Arduino    ***
// ***     20190726 Servo aansturing is goed, bochtenwerk ook.              ***
// ***     20190727 Lijn is mooi te zien. Via netwerk loopt eigen deel vast :-(
// ***              Fish-Eye camera heeft 90 graden gedraait beeld !!       ***
// ***              Het accu-Pack moet wel naar voren geschoven worden !!!  ***
// ***                                                                      ***
// ***                                                                      ***
// ***                                                                      ***
// ***                                                                      ***
// ***                                                                      ***
// ****************************************************************************

const char* ssid = "AV_Retro";              // E-Tech Wireless Router WLRT03
const char* password = "C8656FA14B";        //     Werkt, kan goed inloggen.

const char* apssid = "WoF_ESP32-CAM";
const char* appassword = "12345678";         
    //AP password require at least 8 characters.

#include "esp_wifi.h"
#include "esp_camera.h"
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// We werken met camera model AI_THINKER / ESP32-CAM
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

void startCameraServer();

// ****************************************************************************
// *** Lijst met Servo-Standen voor het Looppatroon ***************************
// **************************************************************************** 
static const int Servo_stand[26] = { 00, 65, 73, 82, 90, 98, 106, 115, 106, 98, 
            90, 82, 73, 65, 73, 82, 90, 98, 106, 115, 106, 98, 90, 82, 73, 65};
static const int Lijn_corr[15] = { 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, -1, -1, -1, 0, 
                                                                            0};                    

// ****************************************************************************
// *** Lijst met gebruikte Globale variabele en Constanten ********************
// ****************************************************************************                   
uint8_t lijn_sensor[32];
uint8_t Filter_lijn[32];
uint8_t VPoot, APoot, Verschil, Voor, Richting, Lus, Teller, Tel2, LedTeller, LedAan, px;
int Lijn_begin, Lijn_eind, Lijn_midden, correctie;

const int Led0 = 4;                         // Flash_light
const int Led1 = 12;                        // Led Oog-1
const int Led2 = 13;                        // Led Oog-2
const int ServoPin1 = 15;                   // Servo VPoot = Voor Poot
const int ServoPin2 = 14;                   // Servo APoot = Achter Poot

// ****************************************************************************
// *** Laad een beeld in het geheugen *****************************************
// **************************************************************************** 
esp_err_t camera_capture(){
  camera_fb_t * s_state = esp_camera_fb_get();
  if (!s_state) {
    ESP_LOGE(TAG, "Camera Capture Failed");
    return ESP_FAIL;
  }
  Lus = 0;
  // Hier gaan we een aantal "metingen" in het array --lijn_sensor[x]-- zetten   
  int ih = 60;                                // 120 hoog, 60 = halverwege                    
  for (int iw = 0; iw < 160 ; iw += 5){       // 160 / 5 = 32 metingen op rij       
    lijn_sensor[Lus] = (s_state->buf[iw + (ih * s_state->width)]);
    Lus = Lus + 1;      
  }  
  //**************************************************************************
  //Geef het geheugen voor de framebuffer weeer vrij.
  esp_camera_fb_return(s_state);
  return ESP_OK;
}

// ****************************************************************************
// *** Een Stap zetten ********************************************************
// **************************************************************************** 
void Stap() {
  if (Voor == 1)
    Teller = Teller + 1;
  else 
    Teller = Teller - 1;

  if (Teller == 0)
    Teller = 12;
  else if (Teller == 13) 
    Teller = 1;

  if (Voor == 1)
    Tel2 = Teller + 4;
  else  
    Tel2 = Teller + 10;

  VPoot = Servo_stand[Teller];                          // Kijk in Loop-Tabel.
  APoot = Servo_stand[Tel2];
}

// ****************************************************************************
// *** Print alleen de lijn ***************************************************
// ****************************************************************************
void Lijn() {
  Serial.print("**  "); 
  for (int Tell = 0; Tell < 32; Tell += 1)   
  { 
    px = lijn_sensor[Tell];
     if (px < 26)
        Serial.print(" ");
      else if (px < 51)
        Serial.print(".");
      else if (px < 77)
        Serial.print(":");
      else if (px < 102)
        Serial.print("-");
      else if (px < 128)
        Serial.print("=");
      else if (px < 154)
        Serial.print("+");
      else if (px < 179)
        Serial.print("*");
      else if (px < 205)
        Serial.print("#");
      else if (px < 230)
        Serial.print("%");
      else
        Serial.print("@"); 
  }
  Serial.println("  **");
}
  
// ****************************************************************************
// *** Filter en omzetting naar Richting **************************************
// ****************************************************************************
void Filter_a() {
// zoek lijn op, loop van 5 - 25 en kijk of het pixelnivo onder de 230 ligt 
  Lijn_begin = 0;
  Lijn_eind = 0;
  
  for (int i=6;i<25;i++) { 
    if (lijn_sensor[i] < 230)
      if (Lijn_begin == 0){
        Lijn_begin = i;
      }        
    else 
      if (!Lijn_begin == 0) {  
        Lijn_eind = i; 
      }   
  }
  int i = Lijn_eind - Lijn_begin;
  
  if (i < 3) 
    Lijn_midden = Lijn_begin;   
  else if (i > 2)
    Lijn_midden = (Lijn_eind - Lijn_begin) / 2; 
  
  Lijn_midden = Lijn_midden + Lijn_corr[Teller]; 
  // Correctie voor de ServoStand cq beweging van de Walker.
  // Lijn_midden --> Een laag getal is LINKS, een hoog is RECHTS.
}

// ****************************************************************************
// *** Filter op basis van contrast tussen 2 pixels ***************************
// ****************************************************************************
void Filter_b(){
  Lijn_begin = 0;
  Lijn_eind = 0;
  
  for (int i=5;i<25;i++) {
    int y = lijn_sensor[i] - lijn_sensor[i+1]; 
    if (abs(y) < 80)  
      Serial.print("-");  
    else
      Serial.print("@");
      if (Lijn_begin = 0)
        Lijn_begin = i;
      else
        Lijn_eind = i;    // dit kan niet te veel bij begin vandaan liggen. 
  }                       // ander historie mee gaan nemen.
  Serial.println(); 
  
  Lijn_midden = (Lijn_eind - Lijn_begin) / 2; 
  Lijn_midden = Lijn_midden + Lijn_corr[Teller]; 
  
  Serial.print("Lijn_Midden ");
  Serial.println(Lijn_midden);
  
  // Correctie voor de ServoStand cq beweging van de Walker.
  // Lijn_midden --> Een laag getal is LINKS, een hoog is RECHTS.    
}

// ****************************************************************************
// *** Bepaal de richting ahv Lijn_Midden  ************************************
// ****************************************************************************
void Richting_ber(){
  // VPoot = VPoot + 10;  // +10 = bocht naar links 
  // APoot = APoot - 10; 
   
  if (Lijn_midden < 15) {
    correctie = (Lijn_midden - 16) * 2;
    if (correctie < -10) {
      correctie = -10;
    }    
  }
  else if (Lijn_midden > 17) {
    correctie = (Lijn_midden - 16) * 2;
    if (correctie < 10){
      correctie = 10;
    } 
  else correctie = 0;     
  }
  VPoot = VPoot + correctie;
  APoot = APoot - correctie; 
  Serial.print("Correctie  ");
  Serial.println(correctie);    
}

// ****************************************************************************
// *** Motoren Init.  *********************************************************
// ****************************************************************************
void initIO() 
{
  ledcSetup(3, 2000, 8); // 2000 hz PWM, 8-bit resolution
  ledcSetup(4, 2000, 8); // 2000 hz PWM, 8-bit resolution
  ledcSetup(5, 2000, 8); // 2000 hz PWM, 8-bit resolution
  ledcSetup(6, 50, 16); // 50 hz PWM, 16-bit resolution
  ledcSetup(7, 50, 16); // 50 hz PWM, 16-bit resolution
  ledcAttachPin(Led0, 3); 
  ledcAttachPin(Led1, 4); 
  ledcAttachPin(Led2, 5); 
  ledcAttachPin(ServoPin1, 6); 
  ledcAttachPin(ServoPin2, 7);   
}

// ****************************************************************************
// *** Grootte Setup Routine  *************************************************
// **************************************************************************** 
void setup() 
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  // prevent brownouts by silencing them
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE,
  config.frame_size = FRAMESIZE_QQVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();   // Moet dit nog ??

  initIO();                 // Start de servo's en IO op.
  Voor = 1;
  Richting = 5;
  
  WiFi.begin(ssid, password);
  delay(500);

  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      if ((StartTime+10000) < millis()) break;
  } 
 
  startCameraServer();
    
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("' to connect");
  
  WiFi.softAP((WiFi.softAPIP().toString()+"_"+(String)apssid).c_str(), appassword);    

  for (int i=0;i<5;i++) 
  {
    ledcWrite(3,50);  // flash led
    delay(100);
    ledcWrite(3,0);
    delay(100);    
  } 

  ledcWrite(3,255); // Zet Flash_LED 100% AAN                          
  camera_capture();
  ledcWrite(3,0);   // Zet Flash_LED UIT, anders wordt het een beetje heet :-(   

  Lijn_begin = 0;
  Lijn_eind = 0;
  Lijn_midden = 0;

   // zoek lijn op
  for (int i=6;i<25;i++) { 
    if (lijn_sensor[i] < 230)
      if (Lijn_begin == 0){
        Lijn_begin = i;
      }        
    else 
      if (!Lijn_begin == 0) {  
        Lijn_eind = i; 
      }   
  }
  int i = Lijn_eind - Lijn_begin;
  
  if (i < 3) 
    Lijn_midden = Lijn_begin;   
  else if (i > 2)
    Lijn_midden = (Lijn_eind - Lijn_begin) / 2; 
}
// ****************************************************************************
// *** Einde Setup Routine  ***************************************************
// ****************************************************************************

// ****************************************************************************
// *** Grootte HoofdLus  ******************************************************
// **************************************************************************** 
void loop() {
  delay(35);   // normaal 35
  ledcWrite(3,255);  // Zet Flash_LED 100% AAN                          
  camera_capture();
  ledcWrite(3,0);    // Zet Flash_LED UIT, anders wordt het een beetje heet :-(   
  
  Lijn();   // Print alleen de lijn. Lijn[0] is de Linker kant. Later uit vinken !!!!!
  //Filter_a(); // Filter de Zwarte Lijn uit de data.  
  Filter_b();  // Lijn detectie obv pixel-contrast verschil                 
  Stap();   // Berekening om een volgende stap te maken. 
  Richting_ber();    // wijzig hier VPoot en APoot nog een beetje aan de hand
                     // van richting cq camera om de bochten te maken    
   
  ledcWrite(6,((VPoot - 65) * 40) + 4200);   // 6200, 5200, 4200   
  //                              --> 30 graden links, midden, 30 graden rechts
  ledcWrite(7,((APoot - 65) * 40) + 5300);   // 7300, 6300, 5300   
  //                              --> 30 graden links, midden, 30 graden rechts
}
// ****************************************************************************
// *** Einde Grootte HoofdLus *************************************************
// ****************************************************************************

/*
// ****************************************************************************
int lijn_sensor[32];  =  De 32 waardes die we uit de lijn halen.
// ****************************************************************************
int VPoot, APoot, Verschil, Voor, Richting, Lus, Teller;
Vpoot = stand Voorpoot servo
APoot = stand Achterpoot servo
Verschil = Verschil tussen voor- en achter-poot in tabel
Voor --> 1 = vooruit / 2 = achteruit
Richting --> 1 = linksaf, 5 = vooruit, 9 = rechtsaf. (?) 
                                              Dit om bochten te kunnen maken.
Lus = algemene teller
Teller = basis volgorde van de stapbeweging in de Tabel.
// ****************************************************************************
 */
