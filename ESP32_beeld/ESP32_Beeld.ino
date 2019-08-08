// ****************************************************************************
// ***  ESP32_Beeld.ino                                A. Vreugdenhil @2019 ***
// ***                                                                      ***
// ***  HCC!Robotica                                a.vreugdenhil@hccnet.nl ***
// ***                                                                      ***
// ***  Doel: Mbv een ESP32-CAM een beeld sensor bouwen.                    ***
// ***                                                                      ***
// ***                          https://github.com/AVTech1/ESP32-Cam-Walker ***
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
const int Led0 = 4;                         // Flash_light

// ****************************************************************************
// *** Laad een beeld in het geheugen *****************************************
// **************************************************************************** 
esp_err_t camera_beeld(){
  camera_fb_t * s_state = esp_camera_fb_get();
  if (!s_state) {
    ESP_LOGE(TAG, "Camera Capture Failed");
    return ESP_FAIL;
  }
  // int pixels_to_skip = s_state->width / 80;

  for (int ih = 0; ih < s_state->height; ih += 8)
  {
    for (int iw = 0; iw < s_state->width * 2; iw += 5)
    {
      uint8_t pixel = (s_state->buf[iw + (ih )]);
      if (pixel < 26)
        printf(" ");
      else if (pixel < 51)
        printf(".");
      else if (pixel < 77)
        printf(":");
      else if (pixel < 102)
        printf("-");
      else if (pixel < 128)
        printf("=");
      else if (pixel < 154)
        printf("+");
      else if (pixel < 179)
        printf("*");
      else if (pixel < 205)
        printf("#");
      else if (pixel < 230)
        printf("%%");
      else
        printf("@");
      }
      printf("\n");
    }
  //**************************************************************************
  //Geef het geheugen voor de framebuffer weeer vrij.
  esp_camera_fb_return(s_state);
  return ESP_OK;
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

  // Setup onboard Flash-LED on GPIO4
  ledcSetup(3, 2000, 8); // 2000 hz PWM, 8-bit resolution
  ledcAttachPin(Led0, 3); 

  for (int i=0;i<5;i++) 
  {
    ledcWrite(3,100);  // flash led
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
  delay(500);   // normaal 35
  ledcWrite(3,255);  // Zet Flash_LED 100% AAN                          
  camera_beeld();
  ledcWrite(3,0);    // Zet Flash_LED UIT, anders wordt het een beetje heet :-(   
}
// ****************************************************************************
// *** Einde Grootte HoofdLus *************************************************
// ****************************************************************************
