#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- TINY CUSTOM ICONS (8x8 pixels) ---
const unsigned char icon_bt[] PROGMEM = {
  0x08, 0x14, 0x2A, 0x1C, 0x2A, 0x14, 0x08, 0x00
};
const unsigned char icon_play[] PROGMEM = {
  0x00, 0x08, 0x0C, 0x0E, 0x0F, 0x0E, 0x0C, 0x08
};
const unsigned char icon_pause[] PROGMEM = {
  0x00, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x00
};

// --- SYSTEM STATES ---
enum ScreenState { HOME_SCREEN, BLUETOOTH_MENU, SONG_MENU };
ScreenState currentScreen = HOME_SCREEN;

// --- VARIABLES ---
int homeSelection = 0;       // 0 = Bluetooth Icon, 1 = "Select Song"
int batteryLevel = 85;       // Fake battery percentage for now
bool isPlaying = false; 
String currentSong = "Not Playing";
int currentMin = 0, currentSec = 30; // Fake time elapsed
int totalMin = 3, totalSec = 43;     // Fake total time

// Marquee (Scrolling Text) Variables
int textScrollX = 0;
unsigned long lastScrollTime = 0;

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed"));
    for(;;);
  }

  Serial.println(F("\n=== DASHBOARD OS READY ==="));
  Serial.println(F("Use 'w'/'s' to select, 'e' to enter, 'b' to go back, 'p' to play"));
  
  drawUI();
}

void loop() {
  checkSerialInput();
  
  // This handles the smooth scrolling of the song name if it's too long
  if (currentScreen == HOME_SCREEN && currentSong.length() > 16) {
    if (millis() - lastScrollTime > 150) { // Speed of scroll
      textScrollX--;
      if (textScrollX < (int)(currentSong.length() * -6)) {
        textScrollX = 128; // Reset to right side
      }
      lastScrollTime = millis();
      drawUI();
    }
  }
}

// --- THE NAVIGATION LOGIC ---
void checkSerialInput() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    bool uiChanged = false;

    if (currentScreen == HOME_SCREEN) {
      switch (command) {
        case 'w': // Scroll Left/Up
        case 's': // Scroll Right/Down
          homeSelection = (homeSelection == 0) ? 1 : 0; // Toggle between BT and Song
          uiChanged = true;
          break;
        case 'e': // Click Select
          if (homeSelection == 0) currentScreen = BLUETOOTH_MENU;
          if (homeSelection == 1) currentScreen = SONG_MENU;
          uiChanged = true;
          break;
        case 'p': // Play/Pause Media Button
          isPlaying = !isPlaying;
          if(isPlaying && currentSong == "Not Playing") currentSong = "Bohemian Rhapsody";
          uiChanged = true;
          break;
      }
    } 
    else if (currentScreen == BLUETOOTH_MENU || currentScreen == SONG_MENU) {
      if (command == 'b') { // Go Back
        currentScreen = HOME_SCREEN;
        uiChanged = true;
      }
    }

    if (uiChanged) drawUI();
  }
}

// --- THE MASTER DRAWING FUNCTION ---
void drawUI() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (currentScreen == HOME_SCREEN) {
    drawHomeScreen();
  } else if (currentScreen == BLUETOOTH_MENU) {
    display.setCursor(10, 20);
    display.print("--- BT MENU ---");
    display.setCursor(10, 40);
    display.print("(Press 'b' to return)");
  } else if (currentScreen == SONG_MENU) {
    display.setCursor(10, 20);
    display.print("--- SONG MENU ---");
    display.setCursor(10, 40);
    display.print("(Press 'b' to return)");
  }

  display.display();
}

// --- HOME SCREEN LAYOUT ---
void drawHomeScreen() {
  // 1. TOP ROW: Bluetooth and Battery
  // Highlight BT Icon if selected
  if (homeSelection == 0) {
    display.fillRect(0, 0, 16, 12, SSD1306_WHITE);
    display.drawBitmap(4, 2, icon_bt, 8, 8, SSD1306_BLACK);
  } else {
    display.drawBitmap(4, 2, icon_bt, 8, 8, SSD1306_WHITE);
  }
  
  // Battery Indicator (Top Right)
  display.setCursor(95, 2);
  display.print(batteryLevel);
  display.print("%");
  display.drawRect(115, 2, 10, 6, SSD1306_WHITE); // Battery outline
  display.fillRect(125, 3, 2, 4, SSD1306_WHITE);  // Battery tip

  // 2. SECOND ROW: "Select Song" Button
  display.setCursor(2, 18);
  if (homeSelection == 1) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Invert for highlight
    display.print(" > Select Song      ");
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.print("   Select Song      ");
  }
  display.setTextColor(SSD1306_WHITE); // Reset text color

  // 3. THE DIVIDER LINE
  display.drawLine(0, 32, 128, 32, SSD1306_WHITE);

  // 4. THE STATUS MENU (Bottom Section)
  // Song Name (with clipping so it doesn't bleed)
  display.setCursor(textScrollX > 0 ? textScrollX : 2, 38);
  display.print(currentSong);
  
  // Clear the edges so scrolling text doesn't wrap weirdly
  display.fillRect(0, 38, 2, 10, SSD1306_BLACK);
  
  // Play/Pause Icon
  if (isPlaying) {
    display.drawBitmap(2, 52, icon_play, 8, 8, SSD1306_WHITE);
  } else {
    display.drawBitmap(2, 52, icon_pause, 8, 8, SSD1306_WHITE);
  }

  // Time Tracker
  display.setCursor(16, 52);
  display.print(currentMin);
  display.print(":");
  if(currentSec < 10) display.print("0"); // Leading zero
  display.print(currentSec);
  display.print(" / ");
  display.print(totalMin);
  display.print(":");
  display.print(totalSec);
}
