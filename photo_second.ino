/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-cam-projects-ebook/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include "esp_camera.h"
#include "ESP32_OV5640_AF.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include <FS.h>
#include "base64.hpp"
#include <WiFi.h>
#include "HTTPClient.h"
#include <LittleFS.h>
#include "base64.hpp"
#include <NTPClient.h>
#include <WiFiUdp.h>



const char* ssid = "****";
const char* password = "****";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);


#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif

OV5640 ov5640 = OV5640();

#define FILE_PHOTO "photo.jpg"
#define FILE_PHOTO_PATH "/photo.jpg"
#define FORMAT_LITTLEFS_IF_FAILED true



void setup() {


#ifdef TWOPART
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED, "/lfs2", 5, "part2")){
    Serial.println("part2 Mount Failed");
    return;
    }
    LittleFS.end();

    Serial.println( "Done with part2, work with the first lfs partition..." );
#endif

    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.println( "SPIFFS-like write file to new path and delete it w/folders" );

    deleteFile2(LittleFS, "/photo.jpg");
    listDir(LittleFS, "/", 3);
  
    Serial.println( "Test complete" ); 
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  Serial.begin(115200);
  Serial.println();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  // Init filesystem
  //ESP_MAIL_DEFAULT_FLASH_FS.begin();
   
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
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SXGA;
  config.jpeg_quality = 5;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  ov5640.start(sensor);

  if (ov5640.focusInit() == 0) {
    Serial.println("OV5640_Focus_Init Successful!");
  }

  if (ov5640.autoFocusMode() == 0) {
    Serial.println("OV5640_Auto_Focus Successful!");
  }
  
  capturePhotoSaveLittleFS();
  //sendPhoto();
    timeClient.begin();
  
}

void loop() {

listDir(LittleFS, "/", 3);
capturePhotoSaveLittleFS();
delay(3600000); //standalone only
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO_PATH );
  unsigned int pic_sz = f_pic.size();
  Serial.println("Photo is ok");
 
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to LittleFS
void capturePhotoSaveLittleFS( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");
  uint8_t rc = ov5640.getFWStatus();
  Serial.printf("FW_STATUS = 0x%x\n", rc);

  if (rc == -1) {
    Serial.println("Check your OV5640");
  } else if (rc == FW_STATUS_S_FOCUSED) {
    Serial.println("Focused!");
  } else if (rc == FW_STATUS_S_FOCUSING) {
    Serial.println("Focusing!");
  } else {
  }

  camera_fb_t* fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed2");
    esp_camera_fb_return(fb);
    return;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Non-JPEG data not implemented");
    esp_camera_fb_return(fb);
    return;
  }
    if(!fb) {
      Serial.println("Camera capture failed3");
      delay(1000);
      ESP.restart();
    }
delay(2000);
    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO_PATH);
    File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO_PATH);
      Serial.print(" - Size: ");
      Serial.print(fb->len);
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    sendPhoto();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in LittleFS
    ok = checkPhoto(LittleFS);
  } while ( !ok );
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void writeFile2(fs::FS &fs, const char * path, const char * message){
    if(!fs.exists(path)){
    if (strchr(path, '/')) {
            Serial.printf("Create missing folders of: %s\r\n", path);
      char *pathStr = strdup(path);
      if (pathStr) {
        char *ptr = strchr(pathStr, '/');
        while (ptr) {
          *ptr = 0;
          fs.mkdir(pathStr);
          *ptr = '/';
          ptr = strchr(ptr+1, '/');
        }
      }
      free(pathStr);
    }
    }

    Serial.printf("Writing file to: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void deleteFile2(fs::FS &fs, const char * path){
    Serial.printf("Deleting file and empty folders on path: %s\r\n", path);

    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }

    char *pathStr = strdup(path);
    if (pathStr) {
        char *ptr = strrchr(pathStr, '/');
        if (ptr) {
            Serial.printf("Removing all empty folders on path: %s\r\n", path);
        }
        while (ptr) {
            *ptr = 0;
            fs.rmdir(pathStr);
            ptr = strrchr(pathStr, '/');
        }
        free(pathStr);
    }
}


void sendPhoto( void ) {
File file = LittleFS.open("/photo.jpg");
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
  const auto filesize = file.size();


    HTTPClient http;
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("Content-Length", String(filesize, DEC));

timeClient.update();

        const auto A = timeClient.getEpochTime();
        String str =  String(A, DEC);
        Serial.println(str);
        str = "https://75gaklt2va.execute-api.us-east-1.amazonaws.com:443/dev/iot-images-esp32/"+str+".jpeg";


    http.begin(str);
    int http_code;

    Serial.println("We are ready");
    http_code = http.sendRequest("PUT", &file, filesize);
    Serial.printf("\n time-keep: %lus", (millis() - A) / 1000);
    String response = http.getString();   
    Serial.println("Http_code: "+http_code);
    Serial.println("Response: "+response);          
 
    http.end();
    file.close();
}
