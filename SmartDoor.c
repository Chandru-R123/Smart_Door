#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==== Wi-Fi credentials ====
const char* ssid = "ESP32-Door";
const char* password = "12345678";

// ==== Door access password ====
String doorPassword = "4321";  // Change this as you like

// ==== Objects ====
LiquidCrystal_I2C lcd(0x27, 16, 2);  // (0x27 or 0x3F)
Servo doorServo;
WiFiServer server(80);

// ==== Pins ====
int servoPin = 13;

// ==== Variables ====
int doorState = 0; // 0 = Closed, 1 = Open

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- LCD setup ---
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(1000);

  // --- Servo setup ---
  doorServo.attach(servoPin);
  doorServo.write(0);  // Start closed
 // --- Wi-Fi Access Point setup ---
  Serial.println();
  Serial.println("Starting Access Point...");
  WiFi.mode(WIFI_AP);
  bool result = WiFi.softAP(ssid, password);

  if (result) {
    Serial.println("✅ AP Started Successfully");
    Serial.print("Network Name: ");
    Serial.println(ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi: ESP32-Door");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.softAPIP().toString());
  } else {
    Serial.println("❌ AP Failed to Start");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
  }

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  // Extract password from URL
  String passwordParam = "";
  int pwIndex = request.indexOf("password=");
  if (pwIndex != -1) {
    passwordParam = request.substring(pwIndex + 9);
    int endIndex = passwordParam.indexOf(' ');
    if (endIndex != -1) {
      passwordParam = passwordParam.substring(0, endIndex);
    }
  }

  // --- Validate Password and Control Door ---
  String message = "";
  if (request.indexOf("/open") != -1) {
    if (passwordParam == doorPassword) {
      doorServo.write(180);  // fully open
      doorState = 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Door: OPEN");
      message = "Door Opened Successfully!";
      Serial.println("Door Opened");
    } else {
      message = "Invalid Password!";
      Serial.println("⚠ Wrong password for OPEN");
    }
  }
  else if (request.indexOf("/close") != -1) {
    if (passwordParam == doorPassword) {
      doorServo.write(0);  // fully closed
      doorState = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Door: CLOSED");
      message = "Door Closed Successfully!";
      Serial.println("Door Closed");
    } else {
      message = "Invalid Password!";
      Serial.println("⚠ Wrong password for CLOSE");
    }
  }

  // --- Send HTML webpage ---
 client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html>");
  client.println("<head><title>ESP32 Door</title>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<style>");
  client.println("body{font-family:Arial;text-align:center;background:#f4f4f4;}");
  client.println("button{padding:15px 30px;font-size:18px;margin:10px;border:none;border-radius:8px;}");
  client.println(".open{background:green;color:white;}");
  client.println(".close{background:red;color:white;}");
  client.println("input{padding:10px;font-size:16px;margin:10px;border-radius:5px;border:1px solid #ccc;}");
  client.println("</style></head><body>");
  client.println("<h2>ESP32 Smart Door</h2>");
  client.println("<p>Status: " + String(doorState ? "OPEN" : "CLOSED") + "</p>");

  client.println("<form action='/open' method='get'>");
  client.println("<input type='password' name='password' placeholder='Enter password'>");
  client.println("<button class='open' type='submit'>Open Door</button>");
  client.println("</form>");

  client.println("<form action='/close' method='get'>");
  client.println("<input type='password' name='password' placeholder='Enter password'>");
  client.println("<button class='close' type='submit'>Close Door</button>");
  client.println("</form>");

  if (message != "") {
    client.println("<p><b>" + message + "</b></p>");
  }

  client.println("</body></html>");
  client.println();
}
