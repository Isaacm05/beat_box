#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Protomatter.h>

#define BTN_UP     2
#define BTN_DOWN   3
#define BTN_LEFT   4
#define BTN_RIGHT  5

enum WaveShape { SINE, SQUARE, TRIANGLE, SAW };
const char* waveNames[] = {"Sine", "Square", "Triangle", "Saw"};

struct Track {
  float freq; // min 0 Hz
  float amp; // from 0.0 to 1.0
  WaveShape shape; // 4 options from waveNames
};

const int MAX_TRACKS = 8; // edit for max number of tracks 
Track tracks[MAX_TRACKS];
int trackCount = 0; // current number of tracks
int selectedTrack = 0; // current selected track

bool inTrackView = false; // track view vs main menu
int detailField = 0; // when in track view, 0=freq, 1=amp, 2=shape

unsigned long holdStartLeft = 0;
unsigned long holdStartRight = 0;
unsigned long holdStartBoth = 0;
const unsigned long holdTime = 600; // hold time threshold

void setup() {
  pinMode(BTN_UP, INPUT_PULLUP); // buttons need pull up resistor
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  // Create 2 default tracks
  tracks[0] = {440.0, 0.8, SINE}; // can change these defaults or get rid of them
  tracks[1] = {880.0, 0.5, SQUARE};
  trackCount = 2;

  // display.begin();
}

void loop() {
  // reads button values
  bool up = !digitalRead(BTN_UP);
  bool down = !digitalRead(BTN_DOWN);
  bool left = !digitalRead(BTN_LEFT);
  bool right = !digitalRead(BTN_RIGHT);

  if (!inTrackView) // switch between track view controls to main menu controls
    handleMainMenu(up, down, left, right);
  else
    handleTrackView(up, down, left, right);

  // draw display
  drawUI();
}

void handleMainMenu(bool up, bool down, bool left, bool right) {
  // Scroll through tracks based on UP DOWN keys
  if (up && !down) {
    selectedTrack = (selectedTrack + 1) % trackCount;
    delay(200);
  } else if (down && !up) {
    selectedTrack = (selectedTrack - 1 + trackCount) % trackCount;
    delay(200);
  }

  // Hold both UP+DOWN to enter track view
  if (up && down) {
    if (holdStartBoth == 0) holdStartBoth = millis();
    if (millis() - holdStartBoth > holdTime) {
      inTrackView = true;
      detailField = 0;
      holdStartBoth = 0;
    }
  } else holdStartBoth = 0;

  // Hold RIGHT to add track
  if (right) {
    if (holdStartRight == 0) holdStartRight = millis();
    if (millis() - holdStartRight > holdTime) {
      addTrack();
      holdStartRight = 0;
    }
  } else holdStartRight = 0;

  // Hold LEFT to delete track, deletes the currently selected track
  if (left) {
    if (holdStartLeft == 0) holdStartLeft = millis();
    if (millis() - holdStartLeft > holdTime) {
      deleteTrack();
      holdStartLeft = 0;
    }
  } else holdStartLeft = 0;
}

void handleTrackView(bool up, bool down, bool left, bool right) {
  // Hold LEFT to return to main menu
  if (left) {
    if (holdStartLeft == 0) holdStartLeft = millis();
    if (millis() - holdStartLeft > holdTime) {
      inTrackView = false;
      holdStartLeft = 0;
      return;
    }
  } else holdStartLeft = 0;

  // Navigate fields
  if (up && !down) {
    detailField = (detailField + 1) % 3;
    delay(200);
  } else if (down && !up) {
    detailField = (detailField - 1 + 3) % 3;
    delay(200);
  }

  // Adjust parameters
  Track &t = tracks[selectedTrack];
  if (right && !left) {
    if (detailField == 0) t.freq += 10;
    else if (detailField == 1) t.amp = min(1.0f, t.amp + 0.1f);
    else if (detailField == 2) t.shape = (WaveShape)((t.shape + 1) % 4);
    delay(200);
  } else if (left && !right) {
    if (detailField == 0) t.freq = max(0.0f, t.freq - 10);
    else if (detailField == 1) t.amp = max(0.0f, t.amp - 0.1f);
    else if (detailField == 2) t.shape = (WaveShape)((t.shape - 1 + 4) % 4);
    delay(200);
  }
}

void addTrack() {
  if (trackCount < MAX_TRACKS) {
    tracks[trackCount] = {440.0, 0.8, SINE};
    trackCount++;
    selectedTrack = trackCount - 1;
  }
}

void deleteTrack() {
  if (trackCount > 1) {
    for (int i = selectedTrack; i < trackCount - 1; i++) {
      tracks[i] = tracks[i + 1];
    }
    trackCount--;
    if (selectedTrack >= trackCount)
      selectedTrack = trackCount - 1;
  }
}

void drawUI() {
  // display.fillScreen(BLACK);
  if (!inTrackView) {
    // display.println("Tracks:");
    for (int i = 0; i < trackCount; i++) {
      // display.setTextColor(i == selectedTrack ? RED : WHITE);
      // display.println(String("> Track ") + String(i + 1));
    }
  } else {
    Track &t = tracks[selectedTrack];
    // display.println("Track " + String(selectedTrack + 1));
    // display.println("----------------");
    // display.setTextColor(detailField == 0 ? RED : WHITE);
    // display.println("Freq: " + String(t.freq) + " Hz");
    // display.setTextColor(detailField == 1 ? RED : WHITE);
    // display.println("Amp: " + String(t.amp, 1));
    // display.setTextColor(detailField == 2 ? RED : WHITE);
    // display.println("Shape: " + String(waveNames[t.shape]));
  }
  // display.display();
}
