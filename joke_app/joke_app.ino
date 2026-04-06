#include "pins.h"
#include <SPI.h>
#include <Arduino.h>
#include <xpt2046.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- WiFi Credentials ---
// PLEASE UPDATE THESE
const char* ssid = "";
const char* password = "";

// --- Pins and Devices ---
XPT2046 touch = XPT2046(SPI, TOUCHSCREEN_CS_PIN, TOUCHSCREEN_IRQ_PIN);
TFT_eSPI tft = TFT_eSPI();

// --- Application State ---
unsigned long lastTouchTime = 0;
const unsigned long sleepTimeout = 10 * 60 * 1000; // 10 minutes
bool isDisplayOn = true;
String currentJoke = "Touch the screen to fetch a joke!";

// --- Function Prototypes ---
void connectWiFi();
String fetchJoke();
void displayJoke(String joke);
void goToSleep();
void wakeUp();
void wrapText(String text, int x, int y, int maxWidth, int lineHeight);

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initial Power configuration
    pinMode(PWR_ON_PIN, OUTPUT);
    digitalWrite(PWR_ON_PIN, HIGH);
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    pinMode(BK_LIGHT_PIN, OUTPUT);
    digitalWrite(BK_LIGHT_PIN, HIGH);

    // Initialise Touch screen
    SPI.begin(TOUCHSCREEN_SCLK_PIN, TOUCHSCREEN_MISO_PIN, TOUCHSCREEN_MOSI_PIN);
    touch.begin(320, 240);
    // Standard calibration for T-HMI
    touch.setCal(1788, 285, 1877, 311, 320, 240); 
    touch.setRotation(1);

    // Initialise TFT
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    
    // Welcome Screen
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(MC_DATUM); 
    tft.drawString("Joke of the Day", 160, 40, 4);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Connecting to WiFi...", 160, 120, 2);

    connectWiFi();

    lastTouchTime = millis();
    
    if (WiFi.status() == WL_CONNECTED) {
        currentJoke = fetchJoke();
    } else {
        currentJoke = "WiFi Failed. Touch to retry.";
    }
    
    displayJoke(currentJoke);
}

void loop() {
    if (touch.pressed()) {
        if (!isDisplayOn) {
            wakeUp();
        } else {
            // Check if it's a new touch (debounce/debounce-like check)
            if (millis() - lastTouchTime > 1000) {
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.setTextDatum(MC_DATUM);
                tft.drawString("Fetching new joke...", 160, 120, 2);
                currentJoke = fetchJoke();
                displayJoke(currentJoke);
            }
        }
        lastTouchTime = millis();
    }

    // Sleep timer
    if (isDisplayOn && (millis() - lastTouchTime > sleepTimeout)) {
        goToSleep();
    }
    
    delay(50);
}

void connectWiFi() {
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
    } else {
        Serial.println("\nWiFi connection failed");
    }
}

String fetchJoke() {
    if (WiFi.status() != WL_CONNECTED) {
        // Try to reconnect
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) return "Wait, I need WiFi for this! (Check credentials)";
    }

    HTTPClient http;
    // v2.jokeapi.dev: format=txt is easy, blacklist flags for safety
    http.begin("https://v2.jokeapi.dev/joke/Any?format=txt&blacklistFlags=nsfw,religious,political,racist,sexist,explicit");
    int httpCode = http.GET();

    String payload = "";
    if (httpCode > 0) {
        payload = http.getString();
        // Replace potential \r with \n for consistent behavior
        payload.replace("\r", "");
    } else {
        payload = "Oops! Something went wrong while fetching the joke.";
    }
    http.end();
    return payload;
}

void displayJoke(String joke) {
    tft.fillScreen(TFT_BLACK);
    
    // Background gradient/box Header
    tft.fillRect(0, 0, 320, 60, 0x18C3); // Dark Blueish
    tft.setTextColor(TFT_GOLD, 0x18C3);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("JOKE OF THE DAY", 160, 30, 4);
    
    // Separator line
    tft.drawFastHLine(0, 60, 320, TFT_WHITE);

    // Joke body with wrapping
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    wrapText(joke, 15, 80, 290, 20);

    // Footer prompt
    tft.fillRect(0, 210, 320, 30, 0x2104); // Dark Grey
    tft.setTextColor(TFT_LIGHTGREY, 0x2104);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Touch screen to refresh", 160, 225, 2);
}

void wrapText(String text, int x, int y, int maxWidth, int lineHeight) {
    int cursorX = x;
    int cursorY = y;
    String word = "";
    
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        
        if (c == ' ' || c == '\n') {
            int wordWidth = tft.textWidth(word + " ", 2);
            if (cursorX + wordWidth > x + maxWidth || c == '\n') {
                cursorX = x;
                cursorY += lineHeight;
            }
            tft.drawString(word + " ", cursorX, cursorY, 2);
            cursorX += tft.textWidth(word + " ", 2);
            word = "";
            if (c == '\n') {
                cursorX = x;
                cursorY += lineHeight;
            }
        } else {
            word += c;
        }
    }
    // Print last word
    if (word.length() > 0) {
        tft.drawString(word, cursorX, cursorY, 2);
    }
}

void goToSleep() {
    Serial.println("Entering power save mode...");
    // Just turn off backlight for this board as it's effective enough
    digitalWrite(BK_LIGHT_PIN, LOW);
    isDisplayOn = false;
}

void wakeUp() {
    Serial.println("Waking up...");
    digitalWrite(BK_LIGHT_PIN, HIGH);
    isDisplayOn = true;
    // Don't refresh joke immediately on wake, just show current one
}
