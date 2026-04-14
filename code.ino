#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <YX5300_ESP32.h>

// --- DISPLAY SETTINGS ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- HARDWARE PINS ---
#define BATTERY_PIN 34
#define MP3_RX 16 
#define MP3_TX 17

// --- TINY CUSTOM ICONS ---
const unsigned char icon_bt[] PROGMEM = {0x10, 0x18, 0x54, 0x38, 0x54, 0x18, 0x10, 0x00};
const unsigned char icon_play[] PROGMEM = {0x00, 0x08, 0x0C, 0x0E, 0x0F, 0x0E, 0x0C, 0x08};
const unsigned char icon_pause[] PROGMEM = {0x00, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x00};
const unsigned char icon_shuffle[] PROGMEM = {0x06, 0x4B, 0x24, 0x18, 0x18, 0x24, 0x4B, 0x06};

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
int menuScrollX = 14; 
unsigned long lastMenuScrollTime = 0;
bool isMenuPaused = true;
unsigned long menuPauseStartTime = 0;

// --- VIEWPORT VARIABLES ---
const int maxVisible = 4;   
int windowTop = 0;          
int currentIndex = 0;       
int activeArtist = 0; // Remembers which artist we clicked
int activeAlbum = 0;  // Remembers which album we clicked

// ==========================================
// --- THE DATABASE BLUEPRINTS ---
// ==========================================
struct Album {
  const char* title;
  int startIndex;  // The actual SD card file number where this album starts
  int trackCount;  // How many songs are in this album
};

struct Artist {
  const char* name;
  int folderNum;          // The SD Folder (01, 02...)
  int albumCount;         // How many albums to show
  Album albums[20];       // Max 20 albums
  const char* tracks[160]; // Max 160 flat tracks
};

// ==========================================
// --- THE MASTER DATA TABLE ---
// ==========================================
const int NUM_ARTISTS = 2;

const Artist database[NUM_ARTISTS] = {
  // --- ARTIST 1: EMINEM (Folder 01) ---
  {
    "Eminem", 1, 8, 
    { // ALBUMS
      {"Encore", 1, 20},
      {"Kamikaze", 21, 13},
      {"Music To Be Murdered By", 34, 20},
      {"Recovery", 54, 19},
      {"The Eminem Show", 73, 20},
      {"The Marshall Mathers LP", 93, 18},
      {"The Slim Shady LP", 111, 20},
      {"Singles", 131, 4}
    },
    { // FLAT TRACK LIST (Index 0 is blank, Track 1 is at Index 1)
      "", 
      // Encore (1-20)
      "Curtains up", "Evil Deeds", "Never Enough", "Yellow Brick Road", "Like Toy Soldiers", 
      "Mosh", "Puke", "My 1st Single", "Paul (Skit)", "Rain Man", "Big Weenie", "Em Calls Paul", 
      "Just Lose it", "Ass Like That", "Spend Some Time", "Mockingbird", "Crazy In Love", 
      "One Shot 2 Shot", "Final Thoughts", "Curtains Down",
      // Kamikaze (21-33)
      "The Ringer", "Greatest", "Lucky You", "Paul(Skit)", "Normal", "Em Calls Paul", 
      "Stepping Stones", "Not Alike", "Kamikaze", "Fall", "Nice Guy", "Good Guy", "Venom"
      // Music To Be Murdered By 
      "Premonition","Unaccommodating","You Gon' Learn","Alfred","Those Kinda Nights","In Too Deep","Godzilla","Darkness","Leaving Heaven",
      "Yah Yah","Stepdad (Intro)","Stepdad","Marsh","Never Love Again","Little Engine","Lock It Up","Farewell","No Regrets","I Will","Alfred",
      //Recovery
      "Cold Wind Blows", "Talkin' 2 Myself", "On Fire","Won't Back Down","W.T.P.","Going Through Changes","Not Afraid","Seduction","No Love",
      "Space Bound","Cinderella Man","To Life","So Bad","Almost Famous","Love The Way You Lie","You're Never Over","Untitled","Ridaz","Session One",
      //The Eminem Show
      "Curtains Up (Skit)","White America","Business","Cleanin' Out My Closet","Square Dance","The Kiss (Skit)","Soldier",
      "Say Goodbye Hollywood","Drips","Without Me","Paul Rosenberg (Skit)","Sing For The Moment","Superman","Hailie's Song","Steve Barman","When The Music Stops",
      "Say What You Say","Till I Collapse","My Dad's Gone Crazy","Curtains Close",
      //The Marshall Mathers LP
      "Public Service Announcement","Kill You","Stan","Paul (Skit)","Who Knew","Steve Berman","The Way I Am","The Real Slim Shady",
      "Remember Me","I'm Back","Marshall Mathers","Ken Kaniff (Skit)","Drug Ballad","Amityville","Bitch Please II","Kim","Under The Influence","Criminal",
      //The Slim Shady LP
      "Public Service Anouncement","My Name Is","Guilty Conscience","Brain Damage","Paul","If I Had","97 Bonnie & Clyde","Bitch","Role Model","Lounge",
      "My Fault","Ken Kaniff","Come On Everybody","Rock Bottom","Just Don't Give a F","Soap","As The World Turns","I'm Shady","Bad Meets Evil","Still Don't Give",
      //Singles
      "Lose Yourself","When I'm Gone","Rap God","Shake That",
    }
  },

  // --- ARTIST 2: KRISHNA DAS (Folder 02) ---
  {
    "Krishna Das", 2, 15, 
    { // ALBUMS
      {"All One", 1, 4},
      {"Breath of the Heart", 5, 6},
      {"Door Of Faith", 11, 7},
      {"Flow Of Grace", 18, 9},
      {"Greatest Hits Of the Kali Yuga", 27, 10},
      {"Heart As Wide As The Wordld", 37, 7},
      {"Heart Full Of Soul", 44, 12},
      {"Home In The Heart", 56, 8},
      {"Kirtan Wallah", 64, 9},
      {"Live Ananda", 73, 5},
      {"Live On Earth", 78, 14},
      {"One Track Heart", 92, 11},
      {"Peace Of My Heart", 103, 5},
      {"Pilgrim Heart", 108, 12},
      {"Pilgrim Of The Heart", 120, 31},
      {"Trust In The Heart", 151, 6}
    },
    { // FLAT TRACK LIST
      "",
      // All One (1-4)
      "Calling From Afar", "Refuge In The Name", "Rock In A Heart Space", "Township Krishna",
      // Breath Of The Heart (5-10)
      "Baba Hanuman", "Kainchi Hare Krishna", "Ma Durga", "Kashi Vishwanath Gange", 
      "Om Namo Bhagavate Vasudevaya", "Brindavan Hare Ram",
      //Door Of Faith
      "Puja", "Sita's Prayer / Hey Mata Durga","Mere Gurudev","Rudrashtakam","Jai Jagadisha Hare",
      "Sri Hanuman Chalisa / Gate Of Sweet Nectar","God Is Real / Hare Ram",
      //Flow Of Grace
      "Sri Ram Chalisa","Hallelujah Chalisa","Good Ole Chalisa","Nina Chalisa","Mountain Chalisa",
      "Bernie's Chalisa","Ring Song / Jaya Siya Ram","Hanuman Chalisa (Slow)","Hanuman Chalisa (Phrase By Phrase)",
      //Greatest Hits Of The Kali Yuga
      "Bhajelo-ji Hanuman","Namah Shivaya","Ma Durga","Hara Hara Mahadev","Mountain Hare Krishna",
      "Hanuman Baba","Shri Guru Charanam","Devi Puja","Mere Guru Dev","Brindavan Hare Ram",
      //Heart As Wide As The World
      "My Foolish Heart / Bhaja Govinda", "Narayana / For Your Love","Hallelujah Shri Ram",
      "Shri Ram Jai Ram","Shiva Puja & Chant","Sitaram","By Your Grace _ Jai Gurudev",
      //Heart Full Of Soul
      "Hanuman Prayer","Shri Ram Jai Ram Jai Jai Ram","Om Namoh Bhagavate Vasudevaya","Govinda Hare Gopala Hare",
      "Devakinandan Gopala","Radhe Radhe Shyam","Goddess Prayer","Jaya Jagatambe Ma Durga","All One (Hare Krishna)","Om Namah Shivaya","Jaya Bhagavan","Jesus on the Main Line",
      //Home in The Heart
      "Om Sri Sita Ram","Hanuman Mantra","Amitabha Mantra","Yogi Hanuman Chalisa",
      "Mata Durga","Kalabinashini Kali","Lokah Samastah Sukhino Bhavantu","Goddess Come Down",
      //Kirtan Wallah
      "Radhe Govinda","Sri Argala Stotram / Show Me Love","Waltzing My Krishna","Saraswati",
      "4AM Hanuman Chalisa","Tara's Mantra","Sri Bajrang Baan","I Phoned Govinda","My Foolish Heart / Bhajagovindam",
      //Live Ananda
      "Samadhi Sitaram","Hare Krishna","Baba Hanuman","Namah Shivaya","Devi Puja",
      //Live On Earth
      "Radhe Shyam","Samadhi Sita Ram","Shri Guru Charanam","Three Rivers Hare Krishna","Hanuman Puja",
      "Hanuman Chaleesa","Sita Ram","Jaya Bhagavan","Devi Puja","Jaya Jagatambe","Mountain Hare Krishna","Namah Shivaya","Rama Bolo","Shri Krishna Govinda Gopala",
      //One Track Heart
      "Hara Hara Mahaadeva","Devi Puja","Kali Durge","The Krishna Waltz","Hanuman Chaleesa",
      "Forgiveness","Prayer To The Goddess For Forgiveness","Hare (Mc) Krishna","Prayer To Hanuman","Shri Ram Jai Ram","Brindavan Hare Ram",
      //Peace Of My Heart
      "Prema Hare Ram","The Blue Krishna Waltz","Jai Shiva Omkara","Hanuman Bhajan","Om Sri Matre Namaha",
      //Pilgrim Heart
      "Namah Shivaya","Govinda Hare","Mountain Hare Krishna","Mahamantra Meltdown","Hara Hara Mahader","Kalabinashini Kali",
      "The Goddess Suite - Mother Song","Devi Puja","Jaya Jayatambe","Yah Devi","Devi Rave","The Ring Song",
      //Pilgrim Of The Heart
     "Introduction","Chanting The Divine Names","God Is Running After Us","Falling In Love with Who We Are","The Natural Unfolding Of The Heart","Faith, The Guru, Surrender, and Grace",
     "Meeting Ram Dass","The Beginning of Faith","Chant Shri Ram","Waking Up","To Lose One's Self In Love","Your Life as Your Teacher",
     "Feeding our Own Hearts","Introduction 2","Drawn Toward Chant","Chant Jaya Jagatambe",
     "Hanuman The Monkey God","Chant Baba Hanuman","Searching For Neem Karoli Baba","Meeting Maharaj ji","Stories Of Maharaj ji","Too Much Compassion",
     "Introduction 3","Burning With Longing Fire","Leaving India","Chant Jaya Bhagavan","The Path Of The Heart","Finding Courage","Grace Is The Unseen Hand","The Absolute Power Of Love","Chant Om Namah Shivaya",
     //Trust in The Heart
     "Guru Puja","Sundhara Chalisa","Namoh","Devi Chant","River Of Mahamantra","Prema Chalisa",
    }
  }
};

//Defining Mp3 Module
YX5300_ESP32 mp3;

void setup() {
  mp3 = YX5300_ESP32(Serial2, 16, 17); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed")); for(;;);
  }
  
  Serial.begin(115200);
  mp3.enableDebugging();

  // Hardware Bass Boost
  //delay(500); 
  //uint8_t bassCommand[] = {0x7E, 0xFF, 0x06, 0x07, 0x00, 0x00, 0x05, 0xEF};
  //Serial2.write(bassCommand, 8);
  
  drawUI();
}

void loop() {
  checkSerialInput();
  updateBattery();
  handleMarquee();
  handleMenuMarquee();
}

//Handle Menu Scrolling
void handleMenuMarquee() {
  // Only scroll if we are in the Track Menu and highlighting an actual song
  if (currentScreen == TRACK_MENU && currentIndex >= 2) {
    int startIndex = database[activeArtist].albums[activeAlbum].startIndex;
    const char* highlightedTrack = database[activeArtist].tracks[startIndex + (currentIndex - 2)];
    
    // Only scroll if the name is longer than 16 characters!
    if (strlen(highlightedTrack) > 16) { 
      if (isMenuPaused) {
        if (millis() - menuPauseStartTime > 1000) { // The 1-Second Delay!
          isMenuPaused = false; 
          lastMenuScrollTime = millis();
        }
      } else {
        if (millis() - lastMenuScrollTime > scrollSpeed) { 
          menuScrollX--;
          int textWidth = strlen(highlightedTrack) * 6;
          
          // If it scrolls completely out of view, snap it back to the start and pause again
          if (menuScrollX <= 14 - textWidth) {
            menuScrollX = 14;              
            isMenuPaused = true;              
            menuPauseStartTime = millis();    
          }
          lastMenuScrollTime = millis();
          drawUI();
        }
      }
    } else if (menuScrollX != 14) {
      // If we move to a short track, reset the variables
      menuScrollX = 14; isMenuPaused = true; drawUI();
    }
  }
}

void updateBattery() {
  int rawVoltage = analogRead(BATTERY_PIN);
  int calcPercent = map(rawVoltage, 1985, 2600, 0, 100);
  if (calcPercent > 100) calcPercent = 100;
  if (calcPercent < 0) calcPercent = 0;
  // batteryLevel = calcPercent; 
}

// Rotating Text
void handleMarquee() {
  if (currentScreen == HOME_SCREEN && currentSong.length() > 21 && isPlaying) { 
    if (isPaused) {
      if (millis() - pauseStartTime > scrollPauseTime) { isPaused = false; lastScrollTime = millis(); }
    } else {
      if (millis() - lastScrollTime > scrollSpeed) { 
        textScrollX--;
        int singleWidth = (currentSong.length() + 5) * 6; 
        if (textScrollX <= 2 - singleWidth) {
          textScrollX = 2; isPaused = true; pauseStartTime = millis();    
        }
        lastScrollTime = millis(); drawUI();
      }
    }
  } else if (currentScreen == HOME_SCREEN) {
    if (textScrollX != 2) { textScrollX = 2; isPaused = true; drawUI(); }
  }
}

// ==========================================
// --- NAVIGATION LOGIC ---
// ==========================================
void checkSerialInput() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    bool uiChanged = false;

    // --- HOME SCREEN ---
    if (currentScreen == HOME_SCREEN) {
      switch (command) {
        case 'w': case 's': homeSelection = (homeSelection == 0) ? 1 : 0; uiChanged = true; break;
        case 'e': 
          if (homeSelection == 0) currentScreen = BLUETOOTH_MENU;
          if (homeSelection == 1) { currentScreen = ARTIST_MENU; currentIndex = 0; windowTop = 0; }
          uiChanged = true; break;
        case 'p': 
          isPlaying = !isPlaying; uiChanged = true; break;
      }
    } 
    
    // --- ARTIST MENU ---
    else if (currentScreen == ARTIST_MENU) {
      if (command == 'b') { currentScreen = HOME_SCREEN; uiChanged = true; }
      if (command == 'w') { scrollViewportUp(NUM_ARTISTS); uiChanged = true; }
      if (command == 's') { scrollViewportDown(NUM_ARTISTS); uiChanged = true; }
      if (command == 'e') { 
        activeArtist = currentIndex; // Save Artist
        currentScreen = ALBUM_MENU; currentIndex = 0; windowTop = 0; uiChanged = true; 
      }
    }

    // --- ALBUM MENU ---
    else if (currentScreen == ALBUM_MENU) {
      if (command == 'b') { currentScreen = ARTIST_MENU; currentIndex = activeArtist; windowTop = 0; uiChanged = true; }
      if (command == 'w') { scrollViewportUp(database[activeArtist].albumCount); uiChanged = true; }
      if (command == 's') { scrollViewportDown(database[activeArtist].albumCount); uiChanged = true; }
      if (command == 'e') {
        activeAlbum = currentIndex; // Save Album
        currentScreen = TRACK_MENU; currentIndex = 0; windowTop = 0; uiChanged = true;
      }
    }

    // --- TRACK MENU ---
    else if (currentScreen == TRACK_MENU) {
      // Total items = track count + 2 (for the dynamic Play & Shuffle options)
      int totalItems = database[activeArtist].albums[activeAlbum].trackCount + 2;
      
      if (command == 'b') { currentScreen = ALBUM_MENU; currentIndex = activeAlbum; windowTop = 0; uiChanged = true; }
      if (command == 'w') { scrollViewportUp(totalItems); uiChanged = true; }
      if (command == 's') { scrollViewportDown(totalItems); uiChanged = true; }
      
      if (command == 'e') {
        int folder = database[activeArtist].folderNum;
        int startIndex = database[activeArtist].albums[activeAlbum].startIndex;
        int targetFile = 0;

        if (currentIndex == 0) {
          // "PLAY" CLICKED - Play the first song of the album
          targetFile = startIndex;
          currentSong = database[activeArtist].tracks[startIndex];
        } else if (currentIndex == 1) {
          // "SHUFFLE" CLICKED
          targetFile = startIndex; // Math for actual shuffle comes later!
          currentSong = String(database[activeArtist].tracks[startIndex]) + " (Shuffle)";
        } else {
          // SPECIFIC SONG CLICKED
          targetFile = startIndex + (currentIndex - 2); 
          currentSong = database[activeArtist].tracks[targetFile];
        }

        // ---> SEND TO MP3 PLAYER HERE! <---
        mp3.playTrackInFolder(targetFile, folder);

        currentScreen = HOME_SCREEN;
        isPlaying = true;
        textScrollX = 2; isPaused = true; pauseStartTime = millis();
        uiChanged = true;
      }
    }

    // --- BLUETOOTH MENU ---
    else if (currentScreen == BLUETOOTH_MENU) {
      if (command == 'b') { currentScreen = HOME_SCREEN; uiChanged = true; }
    }

    //Checks for the menu scroll
    if (uiChanged) {
      // Reset the menu scroll every time you move the cursor
      menuScrollX = 14;
      isMenuPaused = true;
      menuPauseStartTime = millis();
      
      drawUI(); 
    }
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

// ==========================================
// --- DRAWING FUNCTIONS ---
// ==========================================
void drawUI() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false); 

  if (currentScreen == HOME_SCREEN) {
    drawHomeScreen();
  } 
  else if (currentScreen == ARTIST_MENU) {
    // Generate temporary array of artist names
    String artistNames[NUM_ARTISTS];
    for(int i=0; i<NUM_ARTISTS; i++) artistNames[i] = database[i].name;
    drawViewportMenu("ARTISTS", artistNames, NUM_ARTISTS);
  } 
  else if (currentScreen == ALBUM_MENU) {
    // Generate temporary array of album titles for the chosen artist
    int numAlbs = database[activeArtist].albumCount;
    String albTitles[20];
    for(int i=0; i<numAlbs; i++) albTitles[i] = database[activeArtist].albums[i].title;
    drawViewportMenu(database[activeArtist].name, albTitles, numAlbs);
  } 
  else if (currentScreen == TRACK_MENU) {
    drawTrackMenu();
  } 
  else if (currentScreen == BLUETOOTH_MENU) {
    display.setCursor(10, 20); display.print("--- BT MENU ---");
    display.setCursor(10, 40); display.print("(Press 'b' back)");
  }
  display.display();
}



void drawViewportMenu(String title, String list[], int listSize) {
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(2, 2); display.print(title);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  
  int yPos = 16;
  for (int i = 0; i < maxVisible; i++) {
    int actualItemIndex = windowTop + i;
    if (actualItemIndex >= listSize) break; 

    display.setCursor(2, yPos);
    if (actualItemIndex == currentIndex) display.print("> "); else display.print("  "); 
    
    display.print(list[actualItemIndex]);
    yPos += 12; 
  }
  
  int barHeight = (64 - 16) / listSize;
  if (barHeight < 2) barHeight = 2;
  int barY = 16 + (windowTop * ((64 - 16) / listSize));
  display.fillRect(126, barY, 2, barHeight, SSD1306_WHITE);
}



void drawTrackMenu() {
  // Title Bar uses the Album name
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(2, 2);
  display.print(database[activeArtist].albums[activeAlbum].title); 
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  
  int trackCount = database[activeArtist].albums[activeAlbum].trackCount;
  int totalItems = trackCount + 2; // +2 for Play and Shuffle
  int startIndex = database[activeArtist].albums[activeAlbum].startIndex;

  int yPos = 16;
  for (int i = 0; i < maxVisible; i++) {
    int actualItemIndex = windowTop + i;
    if (actualItemIndex >= totalItems) break; 
    
    int textX = 14; 
    
    // 1. DRAW TEXT FIRST
    if (actualItemIndex == 0) { 
      display.drawBitmap(14, yPos, icon_play, 8, 8, SSD1306_WHITE);
      display.setCursor(26, yPos); display.print("Play");
    } else if (actualItemIndex == 1) { 
      display.drawBitmap(14, yPos, icon_shuffle, 8, 8, SSD1306_WHITE);
      display.setCursor(26, yPos); display.print("Shuffle");
    } else {
      const char* trackName = database[activeArtist].tracks[startIndex + (actualItemIndex - 2)];
      
      if (actualItemIndex == currentIndex && strlen(trackName) > 16) {
        // SCROLLING TEXT: Use the moving X coordinate
        display.setCursor(menuScrollX, yPos);
        display.print(trackName);
        
        // THE CLIPPING TRICK: Draw a black box over the left margin to erase the bleed!
        display.fillRect(0, yPos, 14, 10, SSD1306_BLACK); 
      } else {
        // STATIC TEXT
        display.setCursor(textX, yPos);
        display.print(trackName);
      }
    }

    // 2. DRAW CURSOR LAST (Safely on top of the black mask)
    display.setCursor(2, yPos);
    if (actualItemIndex == currentIndex) display.print("> "); else display.print("  "); 

    yPos += 12; 
  }
  
  // Scrollbar
  int barHeight = (64 - 16) / totalItems;
  if (barHeight < 2) barHeight = 2;
  int barY = 16 + (windowTop * ((64 - 16) / totalItems));
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
