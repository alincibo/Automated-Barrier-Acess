## Overview
An intelligent parking barrier system that uses computer vision for license plate recognition and automated access control. The system operates on a local network, utilizing three main components: an ESP32-CAM for plate capture, a processing server, and an ESP32-S3 for barrier control.

## System Architecture

### 1. License Plate Detection (ESP32-CAM)
- Motion detection triggers image capture
- High-resolution (UXGA 1600x1200) image capture
- Local WiFi transmission
- Flash LED for low-light conditions
- PIR motion sensor integration

### 2. Processing Server (Python/FastAPI)
- Image preprocessing pipeline
- OCR text extraction
- Authorization validation
- Real-time response system
- Local network API endpoints

### 3. Barrier Control (ESP32-S3)
- Servo-controlled barrier
- OLED status display
- RGB status indicators
- Automated open/close cycles

## Hardware Requirements

### ESP32-CAM Module
- AI-THINKER ESP32-CAM board
- PIR motion sensor (GPIO 13)
- Flash LED (GPIO 4)
- PSRAM for image storage

### Processing Server
- Computer with Python support and Tesseract OCR installed
- Local network connection: A router configured wirh address reservation for each device connected

### ESP32-S3 Control Unit
- ESP32-S3 development board
- SSD1306 OLED Display (128x64)
- Servo motor
- NeoPixel RGB LED (already present on the ESP32 S3 board
- Local network connectivity
  
### Wifi Router

## Software Dependencies

### Server Requirements
```bash
fastapi
uvicorn
opencv-python
pytesseract
pillow
numpy

    

ESP32 library requeirements: 

    
// ESP32-CAM
ESP32 Camera Driver
WiFi.h

// ESP32-S3
Adafruit_GFX
Adafruit_SSD1306
ESP32Servo
Adafruit_NeoPixel
AsyncTCP
ESPAsyncWebServer

    

Network Configuration
Local Network Setup

    Dedicated local network
    Static IP assignments:
        ESP32-CAM: 192.168.1.XXX
        Server: 192.168.1.XXX
        ESP32-S3: 192.168.1.XXX
    Port configurations:
        Server: 8000
        Barrier Control: 8080

Installation & Setup
1. Server Setup

    
# Clone repository
git clone [repository-url]

# Install dependencies
pip install -r requirements.txt

# Start server
uvicorn main:app --host 0.0.0.0 --port 8000

    

2. ESP32-CAM Configuration

    Install Arduino IDE and ESP32 board support
    Configure WiFi credentials
    Set server IP and port
    Flash ESP32-CAM
    Position camera for optimal plate capture

3. ESP32-S3 Barrier Control Setup

    Configure pins for peripherals
    Set WiFi credentials
    Configure server endpoints
    Flash ESP32-S3
    Connect servo and display

Operation Flow

    Motion Detection & Capture
        PIR sensor detects vehicle
        ESP32-CAM captures image
        Image sent to processing server

    Image Processing
        Server receives image
        Preprocessing (rotation (skew), grayscale)
        OCR extracts plate number
        Authorization check

    Access Control
        Server sends result to ESP32-S3
        Display shows plate number
        LED indicates status
        Barrier operates accordingly

API Endpoints
Image Upload (ESP32-CAM → Server)

    
POST /upload
Content-Type: multipart/form-data


Barrier Control (Server → ESP32-S3)

    
POST /plate
Content-Type: application/x-www-form-urlencoded
Body: plate=ABC123&authorized=1

    
Debug & Monitoring
Serial Monitor Outputs

    ESP32-CAM: 115200 baud
    ESP32-S3: 115200 baud
    Server: Console logging

Status Indicators

    ESP32-CAM: Flash LED patterns
    ESP32-S3: RGB LED states
    OLED display messages

Error Handling

    Network connectivity monitoring
    Image capture verification
    OCR validation
    Barrier operation safety checks
    Comprehensive error logging

Security Considerations

    Local network operation only
    No external network access
    Whitelist-based authorization

