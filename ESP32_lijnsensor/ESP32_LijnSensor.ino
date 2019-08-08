// ****************************************************************************
// ***  ESP32_LijnSensor.ino                           A. Vreugdenhil @2019 ***
// ***                                                                      ***
// ***  ESP32_LijnSensor                            a.vreugdenhil@hccnet.nl ***
// ***                                                                      ***
// ***  Doel: Mbv een ESP32-CAM een lijnsensor bouwen.                      ***
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
// *** Lijst met gebruikte Globale variabele en Constanten ********************
// ****************************************************************************                   
uint8_t lijn_sensor[32];
uint8_t Teller, Tel2, px, Lus;

// ****************************************************************************
// *** Laad een beeldlijn in het geheugen *************************************
// **************************************************************************** 
esp_err_t camera_capture_lijn(){
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
// *** Print alleen de lijn ***************************************************
// ****************************************************************************
void Lijn() { 
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
  Serial.println();
}
  
// ****************************************************************************
// *** Filter op basis van pixel-grijswaarde **********************************
// ****************************************************************************
void Filter_a() {
// zoek lijn op, loop van 5 - 25 en kijk of het pixelnivo onder de 230 ligt 

  for (int i=6;i<25;i++) { 
    if (lijn_sensor[i] < 230)
       Serial.print("@");
    else 
      Serial.print("-");
  }
}

// ****************************************************************************
// *** Filter op basis van contrast tussen 2 pixels ***************************
// ****************************************************************************
void Filter_c(){  
  
  for (int i=5;i<25;i++) {
    int y = lijn_sensor[i] - lijn_sensor[i+1]; 
    if (abs(y) < 80)  
      Serial.print("-");  
    else
      Serial.print("@");     
  }                     
  Serial.println(); 
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

  // drop down frame size for higher initial frame rate
  // sensor_t * s = esp_camera_sensor_get();   // Moet dit nog ??
  
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
    ledcWrite(3,50);  // flash led Flitsen ten teken dat alles opgestart is.
    delay(100);
    ledcWrite(3,0);
    delay(100);    
  } 
}
// ****************************************************************************
// *** Einde Setup Routine  ***************************************************
// ****************************************************************************

// ****************************************************************************
// *** Grootte HoofdLus  ******************************************************
// **************************************************************************** 
void loop() {
  delay(350);           // normaal 35
  ledcWrite(3,255);     // Zet Flash_LED 100% AAN                          
  camera_capture_lijn();  // Neem een beeld op met de camera module.
  ledcWrite(3,0);       // Zet Flash_LED UIT, anders wordt het een beetje heet :-(   

  Lijn();               // Print de lijn als "grijs-tint". 
  // Filter_a();           // Filter de Zwarte Lijn uit de data.  
  Filter_c();           // Lijn detectie obv pixel-contrast verschil                 

}
// ****************************************************************************
// *** Einde Grootte HoofdLus *************************************************
// ****************************************************************************
