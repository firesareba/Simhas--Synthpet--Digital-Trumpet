#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/dac.h>

#define VALVE1_PIN 19    // SW1 - First valve
#define VALVE2_PIN 21    // SW2 - Second valve
#define VALVE3_PIN 22    // SW3 - Third valve

#define OCTAVE1_PIN 18   // SW4
#define OCTAVE2_PIN 5    // SW5
#define OCTAVE3_PIN 17   // SW6
#define OCTAVE4_PIN 16   // SW7
#define OCTAVE5_PIN 4    // SW8

#define SOUND_EN_PIN 15  // SW9 - Sound enable
#define DAC_PIN 25       // GPIO25 - DAC1 output

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SAMPLE_RATE 44100
hw_timer_t *timer = NULL;
volatile float currentFrequency = 0.0;
volatile float phase = 0.0;
volatile bool soundEnabled = false;

struct TrumpetNote {
  const char* noteName;
  int fingering;      
  const char* octaveButton;
  float frequency;
};

const TrumpetNote FINGERING_CHART[] = {
  {"F#3", 0, "None", 185.0},
  {"G3", 23, "None", 196.0},
  {"G#3", 13, "None", 207.7},
  {"A3", 12, "None", 220.0},
  {"A#3", 1, "None", 233.1},
  {"B3", 2, "None", 246.9},
  {"C4", 0, "None", 261.6},
  

  {"C#4", 0, "SW4", 277.2},
  {"D4", 23, "SW4", 293.7},
  {"D#4", 13, "SW4", 311.1},
  {"E4", 12, "SW4", 329.6},
  {"F4", 1, "SW4", 349.2},
  {"F#4", 2, "SW4", 370.0},
  {"G4", 0, "SW4", 392.0},
  

  {"G#4", 23, "SW5", 415.3},
  {"A4", 13, "SW5", 440.0},
  {"A#4", 12, "SW5", 466.2},
  {"B4", 1, "SW5", 493.9},
  {"C5", 2, "SW5", 523.3},
  

  {"C#5", 0, "SW6", 554.4},
  {"D5", 13, "SW6", 587.3},
  {"D#5", 12, "SW6", 622.3},
  {"E5", 1, "SW6", 659.3},
  

  {"F5", 0, "SW7", 698.5},
  {"F#5", 2, "SW7", 740.0},
  {"G5", 0, "SW7", 784.0},
  
  
  {"G#5", 23, "SW8", 830.6},
  {"A5", 13, "SW8", 880.0},
  {"A#5", 12, "SW8", 932.3},
  {"B5", 1, "SW8", 987.8},
  {"C6", 2, "SW8", 1047.0}
};

const int CHART_SIZE = sizeof(FINGERING_CHART) / sizeof(TrumpetNote);


int getFingeringCode(bool v1, bool v2, bool v3) {
  if (!v1 && !v2 && !v3) return 0;    
  if (!v1 && !v2 && v3) return 3;      
  if (!v1 && v2 && !v3) return 2;   
  if (v1 && !v2 && !v3) return 1;   
  if (!v1 && v2 && v3) return 23;    
  if (v1 && !v2 && v3) return 13;     
  if (v1 && v2 && !v3) return 12;    
  if (v1 && v2 && v3) return 123;    
  return 0;
}


const char* getOctaveButtonName(int octave) {
  switch(octave) {
    case 0: return "None"; 
    case 1: return "SW4";   
    case 2: return "SW5";   
    case 3: return "SW6";   
    case 4: return "SW7";   
    case 5: return "SW8";   
    default: return "None";
  }
}


const TrumpetNote* findNote(int fingeringCode, const char* octaveButton) {
  for (int i = 0; i < CHART_SIZE; i++) {
    if (FINGERING_CHART[i].fingering == fingeringCode && 
        strcmp(FINGERING_CHART[i].octaveButton, octaveButton) == 0) {
      return &FINGERING_CHART[i];
    }
  }
  return nullptr;
}


void IRAM_ATTR onTimer() {
  if (soundEnabled && currentFrequency > 0) {
   
    float phaseIncrement = (2.0 * PI * currentFrequency) / SAMPLE_RATE;
    phase += phaseIncrement;
    if (phase >= 2.0 * PI) {
      phase -= 2.0 * PI;
    }
    
    
    float sineValue = sin(phase);
    uint8_t dacValue = (uint8_t)((sineValue + 1.0) * 127.5);
    
 
    dac_output_voltage(DAC_CHANNEL_1, dacValue);
  } else {

    dac_output_voltage(DAC_CHANNEL_1, 128);
    phase = 0.0;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Digital Trumpet Starting...");
  Serial.println("Using accurate fingering chart from CSV");
  
 
  pinMode(VALVE1_PIN, INPUT_PULLUP);
  pinMode(VALVE2_PIN, INPUT_PULLUP);
  pinMode(VALVE3_PIN, INPUT_PULLUP);
  

  pinMode(OCTAVE1_PIN, INPUT_PULLUP);
  pinMode(OCTAVE2_PIN, INPUT_PULLUP);
  pinMode(OCTAVE3_PIN, INPUT_PULLUP);
  pinMode(OCTAVE4_PIN, INPUT_PULLUP);
  pinMode(OCTAVE5_PIN, INPUT_PULLUP);
  

  pinMode(SOUND_EN_PIN, INPUT_PULLUP);
  

  Wire.begin(23, 32);
  

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1315 allocation failed"));
  } else {
    Serial.println("SSD1315 OLED initialized");
  }
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Digital");
  display.println(" Trumpet");
  display.setTextSize(1);
  display.setCursor(10, 50);
  display.println("CSV Chart");
  display.display();
  delay(2000);
  

  dac_output_enable(DAC_CHANNEL_1);
  dac_output_voltage(DAC_CHANNEL_1, 128); 
  
  
  timer = timerBegin(0, 80, true); 
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true);
  timerAlarmEnable(timer);
  
  Serial.println("Setup complete!");
  Serial.println("Loaded " + String(CHART_SIZE) + " notes from fingering chart");
  Serial.println("Octave Ranges:");
  Serial.println("No button: F#3-C4 (LOWEST)");
  Serial.println("SW4: C#4-G4");
  Serial.println("SW5: G#4-C5");
  Serial.println("SW6: C#5-E5");
  Serial.println("SW7: F5-G5");
  Serial.println("SW8: G#5-C6 (HIGHEST)");
}

void loop() {

  bool valve1 = !digitalRead(VALVE1_PIN);
  bool valve2 = !digitalRead(VALVE2_PIN);
  bool valve3 = !digitalRead(VALVE3_PIN);
  

  int octave = 0; 
  if (!digitalRead(OCTAVE1_PIN)) octave = 1;
  else if (!digitalRead(OCTAVE2_PIN)) octave = 2; 
  else if (!digitalRead(OCTAVE3_PIN)) octave = 3; 
  else if (!digitalRead(OCTAVE4_PIN)) octave = 4;
  else if (!digitalRead(OCTAVE5_PIN)) octave = 5;
  

  soundEnabled = !digitalRead(SOUND_EN_PIN);
  

  int fingeringCode = getFingeringCode(valve1, valve2, valve3);
  

  const char* octaveButton = getOctaveButtonName(octave);
  

  const TrumpetNote* note = findNote(fingeringCode, octaveButton);
  
  if (note != nullptr) {
  
    currentFrequency = note->frequency;
    
   
    display.clearDisplay();
    
  
    display.setTextSize(3);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 5);
    display.println(note->noteName);
    
  
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Freq: ");
    display.print(note->frequency, 1);
    display.println(" Hz");
    
   
    display.setCursor(0, 46);
    display.print("Octave: ");
    display.println(note->octaveButton);
    
    
    display.setCursor(0, 56);
    display.print("V:");
    display.print(valve1 ? "1" : "-");
    display.print(valve2 ? "2" : "-");
    display.print(valve3 ? "3" : "-");
    display.print(" [");
    display.print(fingeringCode);
    display.print("]");
    
 
    display.setCursor(90, 56);
    display.print(soundEnabled ? "ON" : "OFF");
    
    display.display();
    
   
    Serial.print("Note: ");
    Serial.print(note->noteName);
    Serial.print(" | Freq: ");
    Serial.print(note->frequency, 2);
    Serial.print(" Hz | Fingering: ");
    Serial.print(fingeringCode);
    Serial.print(" | Octave: ");
    Serial.print(octaveButton);
    Serial.print(" | Sound: ");
    Serial.println(soundEnabled ? "ON" : "OFF");
    
  } else {
   
    currentFrequency = 0.0;
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.println("Invalid");
    display.println("Combo");
    
    display.setTextSize(1);
    display.setCursor(0, 46);
    display.print("F:");
    display.print(fingeringCode);
    display.print(" O:");
    display.println(octaveButton);
    display.display();
    
    Serial.print("ERROR: No note for fingering ");
    Serial.print(fingeringCode);
    Serial.print(" with octave ");
    Serial.println(octaveButton);
  }
  
  delay(50);
}
