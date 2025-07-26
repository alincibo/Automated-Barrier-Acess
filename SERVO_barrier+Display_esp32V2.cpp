//	 RESERVED IP: 192.168.1.150	 MAC: 98:3d:ae:ec:14:f0	    hostname: esp32s3-EC14F0

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// === LED CONFIGURATION ===
#define PIN 48         // 48 este
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// === WIFI CONFIGURATION ===
const char* ssid = "netis";
const char* password = "password";
AsyncWebServer server(8080); // Recommended port

String receivedPlate = "";
bool receivedAuthorized = false;

// === OLED CONFIGURATION ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C
#define OLED_SDA      10 
#define OLED_SCL      11
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === SERVO CONFIGURATION ===
#define SERVO_PIN      14
#define SERVO_OPEN     90
#define SERVO_CLOSED   0
Servo barrierServo;


// Prototip pentru moveServoSmooth dacă nu există deja
void moveServoSmooth(Servo &servo, int fromAngle, int toAngle, int stepDelay = 20);//viteza de miscare a servo-ului

void setup() {
  Serial.begin(115200);

  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Setează culoarea rosu la pornire
  pixels.show();   

  // Set up EXRAN I2C  custom pins
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(" OLED not found");
    while (true); // Halt
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  // Initialize servo
  barrierServo.setPeriodHertz(50);  // Standard 50Hz servo
  barrierServo.attach(SERVO_PIN);
  barrierServo.write(SERVO_CLOSED);


  // Connect to WiFi
  WiFi.begin(ssid, password);
  display.setTextSize(2);
  display.setCursor(0, 30);
  display.print("Connecting WiFi...");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("WiFi OK:");
  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print(WiFi.localIP());
  display.display();
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());

  // NOU: Handler POST compatibil cu x-www-form-urlencoded
  server.on("/plate", HTTP_POST, [](AsyncWebServerRequest *request){
    String plate = "";
    String authorized = "";
    if (request->hasParam("plate", true)) {
      plate = request->getParam("plate", true)->value();
    }
    if (request->hasParam("authorized", true)) {
      authorized = request->getParam("authorized", true)->value();
    }
    Serial.print("[ESP32] Plate: ");
    Serial.println(plate);
    Serial.print("[ESP32] Authorized: ");
    Serial.println(authorized);

    receivedPlate = plate;
    receivedAuthorized = (authorized == "1");

    showPlate(receivedPlate);
    if (receivedAuthorized) {
      showStatus("AUTHORIZED");
      openBarrier();
    } else {
      showStatus("DENIED");
    }
    request->send(200, "text/plain", "OK");
    Serial.println("[ESP32] Am primit si am raspuns la un POST /plate!");
  });

  server.begin();

  // DEBUG: Mesaje pe Serial Monitor
  Serial.println("=====================================");
  Serial.println("[ESP32] HTTP server started!");
  Serial.print  ("[ESP32] Acceseaza: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8080/plate");
  Serial.println("=====================================");
}

// === FUNCTION: Display plate ===
void showPlate(const String& plate) {
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 30);
  display.print(plate);
  display.display();
}

// === FUNCTION: Display status ===
void showStatus(const String& status) {
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(status);
  display.display();
}


// === FUNCTION: Move servo smoothly ===
void moveServoSmooth(Servo &servo, int fromAngle, int toAngle, int stepDelay) {
  if (fromAngle < toAngle) {
    for (int pos = fromAngle; pos <= toAngle; pos++) {
      servo.write(pos);
      delay(stepDelay);
    }
  } else {
    for (int pos = fromAngle; pos >= toAngle; pos--) {
      servo.write(pos);
      delay(stepDelay);
    }
  }
}

// === LED helper functions ===
void setNeoPixelColor(uint32_t color) {
  pixels.setPixelColor(0, color);
  pixels.show();
}

void flashColor(uint32_t color, int times, int intervalMs) {
  for (int i = 0; i < times; i++) {
    setNeoPixelColor(color);
    delay(intervalMs);
    setNeoPixelColor(0); // Off
    delay(intervalMs);
  }
}

// === FUNCTION: Open and close barrier slowly ===
void openBarrier() {
  Serial.println("[DEBUG] openBarrier START");
  int currentPos = barrierServo.read();  // Read current position
  Serial.println("Deschid bariera...");

  moveServoSmooth(barrierServo, currentPos, SERVO_OPEN, 5);// viteza de miscare a servo-ului
  Serial.println("Bariera deschisa!");

  // Flash verde la deschidere
  flashColor(pixels.Color(0, 255, 0), 7, 250);

  // Bariera ramane deschisa 5 secunde
    delay(5000);
  

  Serial.println("Inchid bariera...");

  // Flash rosu la inchidere
  flashColor(pixels.Color(255, 0, 0), 5, 300);

  moveServoSmooth(barrierServo, SERVO_OPEN, SERVO_CLOSED, 5);
  Serial.println("Bariera inchisa!");

  // Revine la rosu dupa ciclu
  setNeoPixelColor(pixels.Color(255, 0, 0));
  Serial.println("[DEBUG] openBarrier END");
}



void loop() {
}