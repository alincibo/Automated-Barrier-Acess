#include "esp_camera.h"
#include <WiFi.h>

//  MAC 3C:8A:1F:AC:DF:8C.       IP reserved in local network:  192.168.1.40

// Select camera model
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// LED si senzor miscare
#define LED_GPIO_NUM   4
#define FLASH_LED_PIN     4   // Camera flash (GPIO 4)
#define MOTION_SENSOR_PIN 13  // MH-Flying Fish senzor pe pinul GPIO 13

// ===========================
// Enter your WiFi credentials
// ===========================

//const char* ssid = "DIGI-pX3h";
//const char* password = "gYFJnVq3vD";

const char* ssid = "netis";
const char* password = "password";

// ===========================
// Enter IP address and port
// ===========================

const char* serverName = "192.168.1.131"; // IP server FastAPI
const int serverPort = 8000;
const char* serverPath = "/upload";


void startCameraServer();
void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200);  //viteza de raspuns in serial monitor
  Serial.setDebugOutput(true);
  Serial.println();


  // initialize the flash LED pin
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  pinMode(MOTION_SENSOR_PIN, INPUT); // pin input la senzor

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;  //calitatea la care se face poza pe camera, inainte de a fi trimisa
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 9; // JPEG quality Range: 0 (best quality, largest file) to 63 (worst quality, smallest file)
  config.fb_count = 1; //Frame buffer count (number of images the camera can store in memory at once


  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  
    s->set_vflip(s, 1);    // rortim 180 imaginea fata de setarile din fabrica
    s->set_hmirror(s, 1);  // oglindire a imaginii, 1 da, 0 nu

  if (s->id.PID == OV3660_PID) { //setam cateva efecte
  
    s->set_brightness(s, 1);   // luminozitate +1
    s->set_saturation(s, -2);  // saturatie -2
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_UXGA); //formatul in care se face poza la rezolutie maxima 1600x1200
  }

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

   blinkFlash(7, 100);// Flash de 7 ori la conectare cu delay de 100 milisec

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void blinkFlash(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(duration);
    digitalWrite(FLASH_LED_PIN, LOW);
    if (i < times - 1) delay(duration);
  }
}


void loop() {
  static int lastState = LOW;
  int currentState = digitalRead(MOTION_SENSOR_PIN);

  // Trigger only once when motion is detected (rising edge: LOW -> HIGH)
  if (lastState == LOW && currentState == HIGH) {
    blinkFlash(2, 100);

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Capture failed");
      delay(5000);
      lastState = currentState;
      return;
    }

    WiFiClient client;
    if (!client.connect(serverName, serverPort)) {
      Serial.println("Connection to server failed");
      esp_camera_fb_return(fb);
      delay(5000);
      lastState = currentState;
      return;
    }

    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    String startRequest = "--" + boundary + "\r\n";
    startRequest += "Content-Disposition: form-data; name=\"file\"; filename=\"esp32.jpg\"\r\n";
    startRequest += "Content-Type: image/jpeg\r\n\r\n";
    String endRequest = "\r\n--" + boundary + "--\r\n";

    int contentLength = startRequest.length() + fb->len + endRequest.length();

    client.print(String("POST ") + serverPath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + serverName + "\r\n");
    client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
    client.print("Content-Length: " + String(contentLength) + "\r\n");
    client.print("Connection: close\r\n\r\n");

    client.print(startRequest);
    client.write(fb->buf, fb->len);
    client.print(endRequest);

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }
    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }

    esp_camera_fb_return(fb);

    // Wait for motion to end before allowing another capture
    while (digitalRead(MOTION_SENSOR_PIN) == HIGH) {
      delay(50);
    }
  }

  lastState = currentState;
  delay(50); // Polling interval
}