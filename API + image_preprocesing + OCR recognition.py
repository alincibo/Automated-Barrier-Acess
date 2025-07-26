from fastapi import FastAPI, UploadFile, File      # Quick framework for web APIs (REST); UploadFile/File for files upload
from datetime import datetime                      # timestamp for folders
import shutil                                     # Operations with folders (copy, move)
import os                                         # Operation with directories and folders (create folder etc.)
import pytesseract                                # Wrapper Python for Tesseract OCR
from PIL import Image, ImageOps                   # Pillow - image process (open, convert, etc.)
import cv2                                        # OpenCV - advanced image process (deskew, crop, binarizare, etc.)
import numpy as np                                # Operations for arrays (used by OpenCV)
import math                                       # Math Functions (calculate image angles)
import requests                                   # Sends HTTP requests to other servers (ex: ESP32)


app = FastAPI()

UPLOAD_DIR = "received_images"
os.makedirs(UPLOAD_DIR, exist_ok=True)


def skew_angle(edges):
    # CLACULATE THE ANGLE TO ROTATE (skew) the image
    # Scope: it corrests the plante number text in the image, in order for the OCR-ul to work as expected.
    (h, w) = edges.shape[:2]
    threshold = max(h, w)
    while True:
        lines = cv2.HoughLines(edges, 1, np.pi / 180, threshold)
        if lines is None or len(lines) == 0:
            threshold -= threshold // 4
            continue
        angles = []
        for line in lines:
            for rho, theta in line:
                angle = (theta - np.pi / 2.0)
                angle = math.degrees(angle) % 360
                if angle > 180:
                    angle = angle - 360
                if angle > 60 or angle < -60:
                    continue
                angles.append(angle)
        if not len(angles):
            threshold -= threshold // 4
            continue
        break
    angle = np.mean(angles)
    return angle

def rotate(image, angle):
    (h, w) = image.shape[:2]
    center = (w // 2, h // 2)
    M = cv2.getRotationMatrix2D(center, angle, 1.0)
    return cv2.warpAffine(
        image, M, (w, h), flags=cv2.INTER_CUBIC,
        borderMode=cv2.BORDER_CONSTANT,
        borderValue=255)

def crop_to_largest_contour(image):
    """Finds and crops to the largest contour in the image."""
    contours, _ = cv2.findContours(image, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not contours:
        return image  # fallback: return original if nothing found
    largest = max(contours, key=cv2.contourArea)
    x, y, w, h = cv2.boundingRect(largest)
    cropped = image[y:y+h, x:x+w]
    return cropped

@app.post("/upload")
async def upload_image(file: UploadFile = File(...)):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    image_path = f"{UPLOAD_DIR}/image_{timestamp}.jpg"
    processed_path = f"{UPLOAD_DIR}/processed_{timestamp}.png"

    # Save the original image
    with open(image_path, "wb") as buffer:
        shutil.copyfileobj(file.file, buffer)

    print(f"[INFO] Image saved: {image_path}")

    try:
        # Read the image with OpenCV
        img = cv2.imread(image_path)
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

        # Auto binarization (Otsu)
        _, thresh = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

        # --- Deskew the image ---
        edges = cv2.Canny(thresh, 50, 150, apertureSize=3)
        angle = skew_angle(edges)
        deskewed = rotate(thresh, angle)

        # --- Zoom in (crop) to the largest contour (likely the plate) ---
        cropped = crop_to_largest_contour(deskewed)
        cv2.imwrite(processed_path, cropped)

        # OCR with Tesseract for deskewed image
        pil_img = Image.fromarray(cropped)
        custom_config = r'--oem 3 --psm 4 -c tessedit_char_whitelist=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'

        text = pytesseract.image_to_string(pil_img, config=custom_config)
        print(text)
        cleaned_text = text.strip()
        plate_number = cleaned_text.replace(" ", "")
        plate_number = ''.join(filter(str.isalnum, plate_number))

        print(f"[OCR] Number recognized: {plate_number}")

        # WHITELIST
        whitelist = {"IS14DTJ", "IS19TDI", "BC02BAC","BT96MRD"}
        authorized = 1 if plate_number in whitelist else 0

        print(f"[DEBUG] Sending to ESP32: plate={plate_number}, authorized={authorized}")

        try:
            esp32_url = "http://192.168.1.150:8080/plate"
            payload = {
                "plate": plate_number,
                "authorized": str(authorized)
            }
            print(f"[DEBUG] Sending to ESP32: {payload}")
            response = requests.post(esp32_url, data=payload, timeout=2)
            print(f"[ESP32] Status: {response.status_code}, Response: {response.text}")
        except Exception as esp32_err:
            print(f"[EROARE ESP32] {esp32_err}")

        return {"message": "Imagine analizată", "numar": plate_number}

    except Exception as e:
        print(f"[EROARE OCR] {e}")
        return {"error": str(e)}



#source venv/bin/activate
#pip list
#ipconfig getifaddr en0  - find the MACBOOK IP
#ifconfig en0 | grep ether - MAC address 82:7a:be:2e:4f:70
#uvicorn cam_apiV2:app --reload --host 0.0.0.0 --port 8000