#include <SPI.h>
#include <TFT_eSPI.h> // Library for TFT display

// Define Constants
const byte PulsesPerRevolution = 2;  
const unsigned long ZeroTimeout = 100000;
const byte numReadings = 2;  // For smoothing speed

// Define wheel radius in meters (adjust as necessary)
const float wheelRadius = 0.3; // Example radius of 0.3 meters

// Pin Definitions
const int voltagePin = 27; // Analog pin for voltage reading (GPIO27)
const int switchPin1 = 14; // Switch 1 for D/R (GPIO12)
const int switchPin2 = 26; // Switch 2 for N/S (GPIO14)

// Display and TFT Setup
TFT_eSPI tft = TFT_eSPI();            // TFT display
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite for off-screen drawing

// Speed Measurement Variables
volatile unsigned long LastTimeWeMeasured; 
volatile unsigned long PeriodBetweenPulses = ZeroTimeout + 1000; 
volatile unsigned long PeriodAverage = ZeroTimeout + 1000; 
unsigned long FrequencyRaw;  
unsigned long SpeedKmh;  
unsigned long PeriodSum; 
unsigned long readings[numReadings];
unsigned long total;  
unsigned long average;  
unsigned long readIndex = 0;

// Voltage Reading Variables
float voltage = 0.0;
float maxVoltage = 5.0;  // Maximum expected voltage for scaling

void setup() {
  Serial.begin(115200);   // Begin serial communication.
  attachInterrupt(digitalPinToInterrupt(13), Pulse_Event, RISING);  // Enable interrupt on pin 13 for speed reading
  pinMode(switchPin1, INPUT_PULLUP);  // Switch 1 D/R with pull-up
  pinMode(switchPin2, INPUT_PULLUP);  // Switch 2 N/S with pull-up
  tft.begin();
  tft.setRotation(1);

  // Show loading screen only once at startup
  showLoadingPage();
  delay(1000);
  tft.fillScreen(TFT_BLACK);
}

void showLoadingPage() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Brawijaya Formula Team", tft.width() / 2, tft.height() / 2 - 30);
  tft.setTextSize(2);
  tft.drawString("#WaktunyaMainGila", tft.width() / 2, tft.height() / 2 + 30);
  delay(3000);
}

void loop() {
  // Update Speed and display
  unsigned long LastTimeCycleMeasure = LastTimeWeMeasured;
  unsigned long CurrentMicros = micros();

  if (CurrentMicros < LastTimeCycleMeasure) {
    LastTimeCycleMeasure = CurrentMicros;
  }

  FrequencyRaw = 10000000000 / PeriodAverage; // Calculate frequency
  if (PeriodBetweenPulses > ZeroTimeout || CurrentMicros - LastTimeCycleMeasure > ZeroTimeout) {
    FrequencyRaw = 0;
  }

  // Calculate Speed in Km/h
  SpeedKmh = (FrequencyRaw / PulsesPerRevolution) * (2 * 3.14159 * wheelRadius) * 3600 / 1000; // Convert to Km/h

  // Smoothing Speed readings
  total = total - readings[readIndex];
  readings[readIndex] = SpeedKmh;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  average = total / numReadings;

  Serial.print("\tSpeed (Km/h): ");
  Serial.println(average);

  displaySpeed(average);
  displaySwitchStatus();
  delay(100); // Optional delay for stability
}

void displaySpeed(unsigned long speed) {
  // Clear previous display
  tft.fillRect(0, 110, tft.width(), 100, TFT_BLACK);

  // Set text attributes
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  // Display Speed in the center of the screen
  tft.drawString("BRAWIJAYA FORMULA TEAM", tft.width() / 2, 10);
  tft.drawString("NOGO CAKRAWALA", tft.width() / 2, 40);
  tft.drawString("Km/h", tft.width() / 2, 120);
  
  // Using buffer to convert speed to string for display
  char buffer[10]; // Buffer for storing the string
  itoa(speed, buffer, 10); // Convert unsigned long to string
  tft.setTextSize(10);
  tft.drawString(buffer, tft.width() / 2, 180); // Displaying speed
}

void displaySwitchStatus() {
  // Read switch states
  int switch1State = digitalRead(switchPin1);
  int switch2State = digitalRead(switchPin2);

  // Determine and display switch labels
  String drStatus = (switch1State == LOW) ? "Drive" : "Reverse";  // Switch 1 (D/R)
  String nsStatus = (switch2State == LOW) ? "Netral" : "Sport";  // Switch 2 (N/S)

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.fillRect(tft.width() - 60, tft.height() / 2 - 30, 60, 60, TFT_BLACK);  // Clear previous status display

  tft.drawString("" + drStatus, tft.width() - 120, tft.height() / 2 + 120);
  tft.drawString("" + nsStatus, tft.width() - 380, tft.height() / 2 + 120);
}

void Pulse_Event() {
  unsigned long currentMicros = micros();
  PeriodBetweenPulses = currentMicros - LastTimeWeMeasured;

  // Debouncing logic: ignore readings that are too quick
  if (PeriodBetweenPulses < 1000) { // 1ms debounce time
    return;
  }

  LastTimeWeMeasured = currentMicros;
  PeriodAverage += (PeriodBetweenPulses - PeriodAverage) / 10; // Exponential moving average for period
  
  // Calculate the number of readings based on the period
  int RemapedAmountOfReadings = map(PeriodBetweenPulses, 40000, 5000, 1, 10);
  RemapedAmountOfReadings = constrain(RemapedAmountOfReadings, 1, 10);
  PeriodSum += PeriodBetweenPulses; // Update the period sum
}
