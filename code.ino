#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- TINY CUSTOM ICONS ---
const unsigned char icon_bt[] PROGMEM = {
  0x10, 0x18, 0x54, 0x38, 0x54, 0x18, 0x10, 0x00
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
int homeSelection = 0;       
int batteryLevel = 50;       
bool isPlaying = false; 
String currentSong = "Not Playing"; 
int currentMin = 0, currentSec = 30; 
int totalMin = 5, totalSec = 55;     

// --- MARQUEE VARIABLES ---
int textScrollX = 2; 
unsigned long lastScrollTime = 0;
int scrollSpeed = 40;       
int scrollPauseTime = 2000; 
bool isPaused = true;       
unsigned long pauseStartTime = 0; 

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed"));
    for(;;);
  }
  
  drawUI();
}

void loop() {
  checkSerialInput();
  
  // --- THE NEW INFINITE LOOP & PAUSE LOGIC ---
  // Only scroll if the song is long AND the music is actively playing
  if (currentScreen == HOME_SCREEN && currentSong.length() > 21 && isPlaying) { 
    if (isPaused) {
      if (millis() - pauseStartTime > scrollPauseTime) {
        isPaused = false; 
        lastScrollTime = millis();
      }
    } 
    else {
      if (millis() - lastScrollTime > scrollSpeed) { 
        textScrollX--;
        
        // Calculate the exact pixel width of ONE copy of the song plus 5 spaces
        // (6 pixels per character)
        int singleWidth = (currentSong.length() + 5) * 6; 
        
        // When the first copy completely clears the starting point, teleport back!
        if (textScrollX <= 2 - singleWidth) {
          textScrollX = 2;              
          isPaused = true;              
          pauseStartTime = millis();    
        }
        
        lastScrollTime = millis();
        drawUI();
      }
    }
  } 
  else if (currentScreen == HOME_SCREEN) {
    // If the song is short OR the music is paused, lock it to the start position
    if (textScrollX != 2) {
      textScrollX = 2;
      isPaused = true; // Reset the pause timer so it waits 2s when you hit play again
      drawUI();
    }
  }
}

// --- NAVIGATION LOGIC ---
void checkSerialInput() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    bool uiChanged = false;

    if (currentScreen == HOME_SCREEN) {
      switch (command) {
        case 'w': 
        case 's': 
          homeSelection = (homeSelection == 0) ? 1 : 0; 
          uiChanged = true;
          break;
        case 'e': 
          if (homeSelection == 0) currentScreen = BLUETOOTH_MENU;
          if (homeSelection == 1) currentScreen = SONG_MENU;
          uiChanged = true;
          break;
        case 'p': 
          if (currentSong == "Not Playing") {
            currentSong = "Bohemian Rhapsody (Remastered 2011)";
            textScrollX = 2;
            isPaused = true;
            pauseStartTime = millis();
            isPlaying = true;
          } else {
            isPlaying = !isPlaying;
          }
          uiChanged = true;
          break;
      }
    } 
    else if (currentScreen == BLUETOOTH_MENU || currentScreen == SONG_MENU) {
      if (command == 'b') { 
        currentScreen = HOME_SCREEN;
        uiChanged = true;
      }
    }

    if (uiChanged) drawUI();
  }
}

// --- MASTER DRAWING FUNCTION ---
void drawUI() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false); 

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
  // 1. TOP ROW: Bluetooth
  if (homeSelection == 0) {
    display.fillRect(0, 0, 16, 12, SSD1306_WHITE);
    display.drawBitmap(4, 2, icon_bt, 8, 8, SSD1306_BLACK);
  } else {
    display.drawBitmap(4, 2, icon_bt, 8, 8, SSD1306_WHITE);
  }
  
  // Battery Indicator
  display.setCursor(85, 2);
  display.print(batteryLevel);
  display.print("%");
  
  display.drawRect(113, 2, 10, 6, SSD1306_WHITE); 
  display.fillRect(123, 3, 2, 4, SSD1306_WHITE);  

  int fillWidth = map(batteryLevel, 0, 100, 0, 6);
  if (fillWidth > 0) {
    display.fillRect(115, 4, fillWidth, 2, SSD1306_WHITE);
  }

  // 2. SECOND ROW: "Select Song" Button
  display.setCursor(2, 18);
  if (homeSelection == 1) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
    display.print(" > Select Song      ");
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.print("   Select Song      ");
  }
  display.setTextColor(SSD1306_WHITE); 

  // 3. THE DIVIDER LINE
  display.drawLine(0, 32, 128, 32, SSD1306_WHITE);

  // 4. THE STATUS MENU (Bottom Section)
  display.setCursor(textScrollX, 38);
  
  // THE FIX: If playing, print the string twice with a 5-space gap to create the loop illusion!
  if (currentSong.length() > 21 && isPlaying) {
    display.print(currentSong + "     " + currentSong);
  } else {
    display.print(currentSong);
  }
  
  // Clear the left border to prevent bleed
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
  if(currentSec < 10) display.print("0"); 
  display.print(currentSec);
  display.print(" / ");
  display.print(totalMin);
  display.print(":");
  display.print(totalSec);
}
