/* USB MIDI and WIFI MIDI Foot Controller
Controls 4 MIDI notes
Includes 4 switches, one for each note
Displays song data from Ableton Live Scene
*/

#define sclk 13  // SCLK can also use pin 14
#define mosi 11  // MOSI can also use pin 7
#define cs   10  // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
#define dc   8   //  but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define rst  9   // RST can use any pin
#define sdcs 4   // CS for SD card, can use any pin

//#include <Adafruit_ST7735.h> // Hardware-specific library
#include <Adafruit_ILI9341.h>
#include <SPI.h>
//#include <i2c_t3.h>
//#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include <Bounce.h>

#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

//Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);
Adafruit_ILI9341 tft = Adafruit_ILI9341(cs, dc);
bool getStarted = false;
// the MIDI channel number to send messages
const int channel = 1;
//Control Change 11 is volume control (for added volume pedal)
//const int controllerA1 = 11; 

// Create Bounce objects for each button.  The Bounce object
// automatically deals with contact chatter or "bounce", and
// it makes detecting changes very simple.
Bounce bouncer[] = {
  Bounce(2,10),
  Bounce(3,10),
  Bounce(4,10),
  Bounce(5,10),
  //Bounce(6,10),
  //Bounce(7,10)
  };

int analogValue = 0;
int previousMIDI = 0;
int newMIDI = 0;
//int count = 0;  //count = bank number
const byte numChars = 96;
char receivedChars[numChars];
boolean newData = false;
int charRemaining = 13;

// handle incoming sysex messages
void handleSysEx(const byte* sysExData, uint16_t sysExSize, bool complete)
{
  static byte ndx = 0;
  byte endMarker = 0xF7;
  char rc;
  if (newData == false) {
    for(int i = 1; i < sysExSize; i++){
      rc = sysExData[i];
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= sysExSize) {
          ndx = sysExSize - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        ndx = 0;
        newData = true;
      }
    }
  }
  showNewData();
  //workOutLines();
  //tft.print(receivedChars);
}

void setup() 
{
  // Configure the pins for input mode with pullup resistors.
  // The pushbuttons connect from each pin to ground.  When
  // the button is pressed, the pin reads LOW because the button
  // shorts it to ground.  When released, the pin reads HIGH
  // because the pullup resistor connects to +5 volts inside
  // the chip.  LOW for "on", and HIGH for "off" may seem
  // backwards, but using the on-chip pullup resistors is very
  // convenient.  The scheme is called "active low", and it's
  // very commonly used in electronics... so much that the chip
  // has built-in pullup resistors
  //Serial.begin(9600);
  //Serial1.begin(115200);
  usbMIDI.setHandleSysEx(handleSysEx);
  
  for (int i=2; i<6; i++)
  {
    pinMode(i, INPUT_PULLUP);
  }
  pinMode(17, INPUT_PULLUP);
  pinMode(A4, INPUT);
  //tft.initR(INITR_GREENTAB); // initialize a ST7735R chip, green tab
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setRotation(3);
  tft.setTextWrap(true);
  tft.println("Welcome");
  tft.println("Waiting for  WiFi...");
  delay(5000);
  int val = digitalRead(17);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0,0);
  if (val){
    tft.println("No known WiFi networks.");
    tft.println("To connect to new network:");
    tft.println("  1) Connect via phone or");
    tft.println("     computer to");
    tft.println("     \"WiFi Pedal Temp AP\"");
    tft.println("  2) Enter new network");
    tft.println("     credentials.");
    tft.println("  3) System will reset");
    tft.println("     and connect WiFi.");
    tft.println();
    tft.println("To use without WiFi,");
    tft.println("connect via USB.");
    tft.println();
    tft.println("Press any button");
    tft.println("to continue.");

    while(!getStarted){
      for (int i=0; i<4; i++)
      {
        if (bouncer[i].update())
        {
          if (bouncer[i].fallingEdge())
          {
            getStarted = true;
          }
          delay(10);
        }
      }
    }
  }
  else{
    //tft.setTextSize(2);
    Serial1.begin(115200);
    tft.println("WiFi Connected.");
    tft.println("Press any button");
    tft.println("to continue.");
    while(!getStarted){
      for (int i=0; i<4; i++)
      {
        if (bouncer[i].update())
        {
          if (bouncer[i].fallingEdge())
          {
            getStarted = true;
            tft.fillScreen(ILI9341_BLACK);
            tft.setCursor(0,0);
          }
          delay(10);
        }
      }
    }
  }
  tft.setTextSize(4);
}

void loop()
{
  usbMIDI.read();
  delay(20);
  buttonPressed();
  //delay(50);
  recvWithEndMarker();
  //showNewData(); 
  //updateDisplay();
  //volPedal();
  delay(20);
}

void buttonPressed()
{
  //read in the buttons data, then process it ..
  for (int i=0; i<4; i++)
  {
    if (bouncer[i].update())
    {
      // Check each button for "falling" edge.
      // Send a MIDI Note On message when each button presses
      // Update the buttons only upon changes.
      // falling = high (not pressed - voltage from pullup resistor)
      //           to low (pressed - button connects pin to ground)

      if (bouncer[i].fallingEdge())
      {
        Serial1.write(i);
        usbMIDI.sendNoteOn(i, 127, channel);
      }

      if (bouncer[i].risingEdge())
      {
        usbMIDI.sendNoteOff(i, 127, channel);
      }
      delay(10);
    }
  }
}

void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  while (Serial1.available() > 0 && newData == false) {
    rc = Serial1.read();
    
    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
  //workOutLines();
  showNewData();
}

void showNewData() {
  if (newData == true) {
    //Serial.print("This just in ... ");
    //Serial.println(receivedChars);
    //tft.fillScreen(ST7735_BLACK);
    //tft.setCursor(0, 2);
    //tft.print(receivedChars);
    //testdrawtext(receivedChars, ST7735_WHITE);
    workOutLines();
    newData = false;
  }
}

void workOutLines(){
  String receivedWord;
  int wordSize;
  char * token;
  
  if (newData == true){
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0,2);
    charRemaining = 13;
    token = strtok(receivedChars," ");
    if(token){
      while (token){
        receivedWord = token;
        token = strtok(NULL, " ");
        wordSize = (receivedWord.length());
        
        if (wordSize <= charRemaining){
          newData = false;
          Serial.print(receivedWord);
          tft.print(receivedWord);
          if (wordSize < charRemaining){
            Serial.print(' ');
            tft.print(' ');
          }
          charRemaining = charRemaining - wordSize;
          charRemaining--;
        }
        else if(wordSize > charRemaining){
          charRemaining = 13;
          if (newData == false){
            Serial.println();
            tft.println();
          }
          Serial.print(receivedWord);
          tft.print(receivedWord);
          int temp = wordSize % 13;
          charRemaining = charRemaining - temp;
          if (charRemaining < 13){
            Serial.print(' ');
            tft.print(' ');
          }
          charRemaining--;
          newData = false;
        }
        else{
          newData = false;
          charRemaining = 13;
          Serial.println();
          Serial.print(receivedWord);
          tft.println();
          tft.print(receivedWord);
          if (wordSize < charRemaining){
            Serial.print(' ');
            tft.print(' ');
          }
          charRemaining = charRemaining - wordSize;
          charRemaining--;
        }
      }
    }
  }
}

/*void testdrawtext(const char* text, uint16_t color) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 2);
  tft.print(text);
}
void volPedal()
{
  //check the volume pedal for changes 
  analogValue = analogRead(A9);

  if (analogValue % 8 == 0 && analogValue / 8 != previousMIDI)
  {
    newMIDI = analogValue / 8;
    //usbMIDI.sendControlChange(controllerA1, newMIDI, channel);
    previousMIDI = newMIDI;
    Serial.println(newMIDI);
  }
}*/
