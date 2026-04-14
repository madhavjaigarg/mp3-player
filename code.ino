#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- DISPLAY SETTINGS ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- HARDWARE PINS ---
#define BATTERY_PIN 34
#define MP3_RX 16 
#define MP3_TX 17

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
// NEW: A tiny crossing-arrows shuffle icon!
const unsigned char icon_shuffle[] PROGMEM = {
  0x06, // 00000110
  0x4B, // 01001011
  0x24, // 00100100
  0x18, // 00011000
  0x18, // 00011000
  0x24, // 00100100
  0x4B, // 01001011
  0x06  // 00000110
};

// --- SYSTEM STATES & VARIABLES ---
enum ScreenState { HOME_SCREEN, BLUETOOTH_MENU, ARTIST_MENU, ALBUM_MENU, TRACK_MENU };
ScreenState currentScreen = HOME_SCREEN;

int homeSelection = 0;       
int batteryLevel = 100;       
bool isPlaying = false; 
String currentSong = "Not Playing"; 

// --- MARQUEE VARIABLES ---
int textScrollX = 2; 
unsigned long lastScrollTime = 0;
int scrollSpeed = 40;       
int scrollPauseTime = 2000; 
bool isPaused = true;       
unsigned long pauseStartTime = 0; 

// --- VIEWPORT VARIABLES ---
const int maxVisible = 4;   
int windowTop = 0;          
int currentIndex = 0;       
int selectedAlbumIndex = 0; // Remembers which album title to show on the track screen

// --- MOCK DATABASE ---
const int NUM_ARTISTS = 3;
String artists[NUM_ARTISTS] = {"Krishna Das", "The Neighbourhood", "Eminem"};

const int NUM_ALBUMS = 7;
String albums[NUM_ALBUMS] = {"Infinite", "The Slim Shady LP", "The Marshall Mathers LP", "The Eminem Show", "Encore", "Relapse", "Recovery"};

const int NUM_TRACKS = 8;
String tracks[NUM_TRACKS] = {"Play", "Shuffle", "Placeholder 1", "Placeholder 2", "Placeholder 3", "Placeholder 4", "Placeholder 5", "Placeholder 6"};


void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, MP3_RX, MP3_TX); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed"));
    for(;;);
  }

  delay(500); 
  uint8_t bassCommand[] = {0x7E, 0xFF, 0x06, 0x07, 0x00, 0x00, 0x05, 0xEF};
  Serial2.write(bassCommand, 8);
  
  drawUI();
}

void loop() {
  checkSerialInput();
  updateBattery();
  handleMarquee();
}

void updateBattery() {
  int rawVoltage = analogRead(BATTERY_PIN);
  int calcPercent = map(rawVoltage, 1985, 2600, 0, 100);
  if (calcPercent > 100) calcPercent = 100;
  if (calcPercent < 0) calcPercent = 0;
  // batteryLevel = calcPercent; 
}

void handleMarquee() {
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
        int singleWidth = (currentSong.length() + 5) * 6; 
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
    if (textScrollX != 2) {
      textScrollX = 2;
      isPaused = true; 
      drawUI();
    }
  }
}

// --- NAVIGATION LOGIC ---
void checkSerialInput() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    bool uiChanged = false;

    // --- HOME SCREEN ---
    if (currentScreen == HOME_SCREEN) {
      switch (command) {
        case 'w': case 's': 
          homeSelection = (homeSelection == 0) ? 1 : 0; uiChanged = true; break;
        case 'e': 
          if (homeSelection == 0) currentScreen = BLUETOOTH_MENU;
          if (homeSelection == 1) { currentScreen = ARTIST_MENU; currentIndex = 0; windowTop = 0; }
          uiChanged = true; break;
        case 'p': 
          if (currentSong == "Not Playing") {
            currentSong = "Lose Yourself (Original Soundtrack)";
            textScrollX = 2; isPaused = true; pauseStartTime = millis();
          }
          isPlaying = !isPlaying; uiChanged = true; break;
      }
    } 
    
    // --- ARTIST MENU ---
    else if (currentScreen == ARTIST_MENU) {
      if (command == 'b') { currentScreen = HOME_SCREEN; uiChanged = true; }
      if (command == 'w') { scrollViewportUp(NUM_ARTISTS); uiChanged = true; }
      if (command == 's') { scrollViewportDown(NUM_ARTISTS); uiChanged = true; }
      if (command == 'e') { 
        currentScreen = ALBUM_MENU; currentIndex = 0; windowTop = 0; uiChanged = true; 
      }
    }

    // --- ALBUM MENU ---
    else if (currentScreen == ALBUM_MENU) {
      if (command == 'b') { currentScreen = ARTIST_MENU; currentIndex = 0; windowTop = 0; uiChanged = true; }
      if (command == 'w') { scrollViewportUp(NUM_ALBUMS); uiChanged = true; }
      if (command == 's') { scrollViewportDown(NUM_ALBUMS); uiChanged = true; }
      if (command == 'e') {
        selectedAlbumIndex = currentIndex; 
        currentScreen = TRACK_MENU;
        currentIndex = 0; windowTop = 0; 
        uiChanged = true;
      }
    }

    // --- TRACK MENU ---
    else if (currentScreen == TRACK_MENU) {
      if (command == 'b') { currentScreen = ALBUM_MENU; currentIndex = selectedAlbumIndex; windowTop = 0; uiChanged = true; }
      if (command == 'w') { scrollViewportUp(NUM_TRACKS); uiChanged = true; }
      if (command == 's') { scrollViewportDown(NUM_TRACKS); uiChanged = true; }
      if (command == 'e') {
        
        // THE FIX: Check if they clicked "Play" (0) or "Shuffle" (1)
        if (currentIndex == 0 || currentIndex == 1) {
          // If so, load the first actual song (Index 2)
          currentSong = tracks[2]; 
        } else {
          // Otherwise, load whatever track they actually clicked
          currentSong = tracks[currentIndex]; 
        }

        currentScreen = HOME_SCREEN;
        isPlaying = true;
        
        // Reset the scrolling text for the new song
        textScrollX = 2; 
        isPaused = true; 
        pauseStartTime = millis();
        
        uiChanged = true;
      }
    }

    // --- BLUETOOTH MENU ---
    else if (currentScreen == BLUETOOTH_MENU) {
      if (command == 'b') { currentScreen = HOME_SCREEN; uiChanged = true; }
    }

    if (uiChanged) drawUI();
  }
}

// --- VIEWPORT MATH ---
void scrollViewportDown(int listSize) {
  currentIndex++;
  if (currentIndex >= listSize) { currentIndex = 0; windowTop = 0; } 
  else if (currentIndex >= windowTop + maxVisible) { windowTop++; }
}

void scrollViewportUp(int listSize) {
  currentIndex--;
  if (currentIndex < 0) {
    currentIndex = listSize - 1;
    windowTop = listSize - maxVisible; 
    if (windowTop < 0) windowTop = 0; 
  } else if (currentIndex < windowTop) { windowTop--; }
}

// --- DRAWING FUNCTIONS ---
void drawUI() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false); 

  if (currentScreen == HOME_SCREEN) drawHomeScreen();
  else if (currentScreen == ARTIST_MENU) drawViewportMenu("ARTISTS", artists, NUM_ARTISTS);
  else if (currentScreen == ALBUM_MENU) drawViewportMenu("EMINEM ALBUMS", albums, NUM_ALBUMS);
  else if (currentScreen == TRACK_MENU) drawTrackMenu();
  else if (currentScreen == BLUETOOTH_MENU) {
    display.setCursor(10, 20); display.print("--- BT MENU ---");
    display.setCursor(10, 40); display.print("(Press 'b' back)");
  }
  display.display();
}

void drawViewportMenu(String title, String list[], int listSize) {
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(2, 2);
  display.print(title);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  
  int yPos = 16;
  for (int i = 0; i < maxVisible; i++) {
    int actualItemIndex = windowTop + i;
    if (actualItemIndex >= listSize) break; 

    display.setCursor(2, yPos);
    if (actualItemIndex == currentIndex) display.print("> ");
    else display.print("  "); 
    
    display.print(list[actualItemIndex]);
    yPos += 12; 
  }
  
  int barHeight = (64 - 16) / listSize;
  if (barHeight < 2) barHeight = 2;
  int barY = 16 + (windowTop * ((64 - 16) / listSize));
  display.fillRect(126, barY, 2, barHeight, SSD1306_WHITE);
}

// NEW: Special drawer just for the Track Menu to handle the icons!
void drawTrackMenu() {
  // Use the selected album name as the title bar
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(2, 2);
  
  // Truncate the album title if it's too long, or just print it
  display.print(albums[selectedAlbumIndex]); 
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  
  int yPos = 16;
  for (int i = 0; i < maxVisible; i++) {
    int actualItemIndex = windowTop + i;
    if (actualItemIndex >= NUM_TRACKS) break; 

    // Draw the selection cursor
    display.setCursor(2, yPos);
    if (actualItemIndex == currentIndex) display.print("> ");
    else display.print("  "); 
    
    int textX = 14; // Default starting pixel for text
    
    // Inject icons for the first two items
    if (actualItemIndex == 0) { 
      display.drawBitmap(14, yPos, icon_play, 8, 8, SSD1306_WHITE);
      textX = 26; // Push text further right to make room for the icon
    } else if (actualItemIndex == 1) { 
      display.drawBitmap(14, yPos, icon_shuffle, 8, 8, SSD1306_WHITE);
      textX = 26; 
    }
    
    display.setCursor(textX, yPos);
    display.print(tracks[actualItemIndex]);
    yPos += 12; 
  }
  
  // Scrollbar
  int barHeight = (64 - 16) / NUM_TRACKS;
  if (barHeight < 2) barHeight = 2;
  int barY = 16 + (windowTop * ((64 - 16) / NUM_TRACKS));
  display.fillRect(126, barY, 2, barHeight, SSD1306_WHITE);
}

void drawHomeScreen() {
  if (homeSelection == 0) {
    display.fillRect(0, 0, 16, 12, SSD1306_WHITE);
    display.drawBitmap(4, 2, icon_bt, 8, 8, SSD1306_BLACK);
  } else {
    display.drawBitmap(4, 2, icon_bt, 8, 8, SSD1306_WHITE);
  }
  
  display.setCursor(85, 2);
  display.print(batteryLevel); display.print("%");
  display.drawRect(113, 2, 10, 6, SSD1306_WHITE); 
  display.fillRect(123, 3, 2, 4, SSD1306_WHITE);  
  int fillWidth = map(batteryLevel, 0, 100, 0, 6);
  if (fillWidth > 0) display.fillRect(115, 4, fillWidth, 2, SSD1306_WHITE);

  display.setCursor(2, 18);
  if (homeSelection == 1) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
    display.print(" > Select Song      ");
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.print("   Select Song      ");
  }
  display.setTextColor(SSD1306_WHITE); 
  display.drawLine(0, 32, 128, 32, SSD1306_WHITE);

  display.setCursor(textScrollX, 38);
  if (currentSong.length() > 21 && isPlaying) display.print(currentSong + "     " + currentSong);
  else display.print(currentSong);
  display.fillRect(0, 38, 2, 10, SSD1306_BLACK); 
  
  if (isPlaying) display.drawBitmap(2, 52, icon_play, 8, 8, SSD1306_WHITE);
  else display.drawBitmap(2, 52, icon_pause, 8, 8, SSD1306_WHITE);
  
  display.setCursor(16, 52);
  display.print("0:30 / 5:55");
}
