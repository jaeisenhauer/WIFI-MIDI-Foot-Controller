/* USB MIDI Foot Controller
Controls 20 banks of 6 MIDI notes
Includes 2 switches for bank selection (Up, Down)
Includes 1 additional assignable button (Typically used for ALL STOP)
Displays Bank number and switch on 4 digit LED Display
You must select MIDI from the "Tools > USB Type" menu
*/
#include <i2c_t3.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include <Bounce.h>

Adafruit_7segment matrix = Adafruit_7segment();

// the MIDI channel number to send messages
const int channel = 1;

//dedicated buttons for bank switch up and down
const int buttonPinDown = 10;
const int buttonPinUP = 9;
const int buttonStop = 8;

//Control Change 11 is volume control (for added volume pedal)
const int controllerA1 = 11; 

// Create Bounce objects for each button.  The Bounce object
// automatically deals with contact chatter or "bounce", and
// it makes detecting changes very simple.

Bounce bouncer[] = {
  Bounce(2,10),
  Bounce(3,10),
  Bounce(4,10),
  Bounce(5,10),
  Bounce(6,10),
  Bounce(7,10)};

Bounce pushbuttonDown = Bounce(buttonPinDown, 10);  // 5 ms debounce
Bounce pushbuttonUp = Bounce(buttonPinUP, 10);  // 5 ms debounce
Bounce pushbuttonStop = Bounce(buttonStop, 10);

int analogValue = 0;
int previousMIDI = 0;
int newMIDI = 0;
int count = 0;  //count = bank number

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
  
  for (int i=2; i<11; i++)
  {
    pinMode(i, INPUT_PULLUP);
  }
  matrix.begin(0x70);
  matrix.setBrightness(5);
  delay(200);

  for (int i=0; i<7; i++)
  {
    if (bouncer[i].update())
    {
      matrix.clear();
    }
  }
  if (pushbuttonStop.update())
  {
    matrix.clear();
  }
  bankDisplay(0);
}

void loop()
{
  // Update all the buttons.  There should not be any long
  // delays in loop(), so this runs repetitively at a rate
  // faster than the buttons could be pressed and released.
  bankSelect();
  buttonPressed();
  volPedal();
  delay(20);
  while (usbMIDI.read())
  {
    // ignore incoming messages
  }
}

void bankSelect()
{
  if (pushbuttonDown.update())
  {  
    if (pushbuttonDown.fallingEdge())
    {
      count--;
      if (count < 0) {count = 19;}
      bankDisplay(count);
    }
  }
 
  if (pushbuttonUp.update())
  {  
    if (pushbuttonUp.fallingEdge())
    {
      count++;
      if (count > 19) {count = 0;}
      bankDisplay(count);
    }
  }
}

void buttonPressed()
{
  //read in the buttons data, then process it ..
  //start by checking for stop button

  if (pushbuttonStop.update())
  {
    if (pushbuttonStop.fallingEdge())
    {
      usbMIDI.sendNoteOn(127,127,channel);
    }

    delay(20);
    if (pushbuttonStop.risingEdge())
    {
      usbMIDI.sendNoteOff(127,0,channel);
      buttonDisplay(127);
    }
  }

  //next check the 6 note switches
  for (int i=0; i<7; i++)
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
        usbMIDI.sendNoteOn(i + (count * 6), 127, channel);
      }

      delay(50);

      // Check each button for "rising" edge
      // Send a MIDI Note Off message when each button releases
      // For many types of projects, you only care when the button
      // is pressed and the release isn't needed.
      // rising = low (pressed - button connects pin to ground)
      //          to high (not pressed - voltage from pullup resistor)

      if (bouncer[i].risingEdge())
      {
        usbMIDI.sendNoteOff(i + (count * 6), 0, channel); 
        //update the display to show which switch was pressed
        bankDisplay(count);
        buttonDisplay(i);
      }
    }
  }
}

void buttonDisplay(int buttonNumber)
{
  //update the display to show the bank number and which switch was pressed
  //if no switch was pressed, just show bank number

  int switchLetter;

  //take number of switch that was pressed and convert to show a-f
  switch (buttonNumber)
  {
    case 0:
      switchLetter = 10;
      break;
    case 1:
      switchLetter = 11;
      break;
    case 2:
      switchLetter = 12;
      break;
    case 3:
      switchLetter = 13;
      break;
    case 4:
      switchLetter = 14;
      break;
    case 5:
      switchLetter = 15;
      break;
    case 127:
      switchLetter = 127;
      break;
    default:
      switchLetter = 0;
      break;
  }
  
  if (switchLetter == 0){
    matrix.writeDigitRaw(3,0b00000000);
  }
  else if (switchLetter == 127){
    matrix.writeDigitRaw(0,0b01101101);
    matrix.writeDigitRaw(1,0b01111000);
    matrix.writeDigitRaw(3,0b01011100);
    matrix.writeDigitRaw(4,0b01110011);
  }
  else {
    matrix.writeDigitNum(3,switchLetter);
  }
  matrix.writeDisplay();
}

void bankDisplay(int bankNumber)
{
  matrix.clear();
  matrix.writeDisplay();
  bankNumber = bankNumber + 1;
  if (bankNumber < 10){
    matrix.writeDigitNum(0,0);
    matrix.writeDigitNum(1,bankNumber);
    matrix.writeDisplay();
  }
  else{
    if (bankNumber == 20){
      matrix.writeDigitNum(0,2);
      matrix.writeDigitNum(1,0);
    }
    else{
      matrix.writeDigitNum(0,1);
      matrix.writeDigitNum(1,(bankNumber) % 10);
    }
    matrix.writeDisplay();
  }
}

void volPedal()
{
  //check the volume pedal for changes 
  analogValue = analogRead(A9);

  if (analogValue % 8 == 0 && analogValue / 8 != previousMIDI)
  {
    newMIDI = analogValue / 8;
    usbMIDI.sendControlChange(controllerA1, newMIDI, channel);
    previousMIDI = newMIDI;
    Serial.println(newMIDI);
  }
}
