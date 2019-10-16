// These need to be included when using standard Ethernet
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
//#include <ESP8266WebServer.h>
#include <AppleMidi.h>
#include <DNSServer.h>
#include <WiFiManager.h>

//void OnAppleMidiConnected(uint32_t, char*);
//void OnAppleMidiDisconnected(uint32_t);
//void OnAppleMidiNoteOn(byte, byte, byte);
//void OnAppleMidiNoteOff(byte, byte, byte);

WiFiUDP udp;
char packetBuffer[255]; //buffer to hold incoming packet
char ReplyBuffer[] = "acknowledged";       // a string to send back

bool isConnected = false;
char ssid[] = "VirusAlert"; //  your network SSID (name)
char pass[] = "RCQJXM5FJPDPQGC7";    // your network password (use for WPA, or use as key for WEP)

APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h

void setup()
{
  // Serial communications and wait for port to open:
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  Serial.begin(115200);
  WiFiManager wifiManager;
//  Serial.print("Getting IP address...");
//  WiFi.hostname("Midi Pedal");
//  WiFi.mode(WIFI_STA);
//  WiFi.begin(ssid, pass); 
  wifiManager.setDebugOutput(false);
  wifiManager.autoConnect("WiFi Pedal Temp AP");
  
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
  //Serial.println("");
  //delay(3000);
  udp.begin(2390);
  //Serial.println("WiFi connected, IP address ");
  //Serial.print(WiFi.localIP());
  digitalWrite(2, LOW);
  
  if (!MDNS.begin("Midi Foot Pedal")){
    Serial.println("mDNS responder error");
    while(1) { 
      delay(1000);
    }
  }
  //Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("apple-midi", "udp", 5004);

  // Create a session and wait for a remote host to connect to us
  AppleMIDI.begin("Midi Foot Pedal");

  AppleMIDI.OnConnected(OnAppleMidiConnected);
  AppleMIDI.OnDisconnected(OnAppleMidiDisconnected);

  AppleMIDI.OnReceiveNoteOn(OnAppleMidiNoteOn);
  AppleMIDI.OnReceiveNoteOff(OnAppleMidiNoteOff);
  
  
  
  //Serial.println("Sending NoteOn/Off of note 45, every second");
}


void loop()
{
  // Listen to incoming notes
  AppleMIDI.run();
  MDNS.update();
  
  if (isConnected) {
    //check UART for data
    if (Serial.available()) {
      //Serial.println("data incoming");
      //check serial port for info from Teensy
      //Teensy sends int value of button pressed
      int note = Serial.read();
      int velocity = 127;
      int channel = 11;

      //Serial.println(note);

      //send note over network
      AppleMIDI.noteOn(note, velocity, channel);
      delay(20);
      AppleMIDI.noteOff(note, velocity, channel);
    }
  }

  // if there's data available from Ableton via UDP broadcast,
  //read in the packet and send the text to the Teensy for display
  int packetSize = udp.parsePacket();
  if (packetSize) {
//    Serial.print("Received packet of size ");
//    Serial.println(packetSize);
//    Serial.print("From ");
//    IPAddress remoteIp = udp.remoteIP();
//    Serial.print(remoteIp);
//    Serial.print(", port ");
//    Serial.println(udp.remotePort());

    // read the packet into packetBufffer
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
//    Serial.println("Contents:");
    Serial.println(packetBuffer);
//    Serial.write("Contents:");
//    Serial.write(packetBuffer);

    // send a reply, to the IP address and port that sent us the packet we received
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(ReplyBuffer);
    udp.endPacket();
  }
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnAppleMidiConnected(uint32_t ssrc, char* name) {
  isConnected  = true;
  Serial.print("Connected to session ");
  Serial.println(name);
  Serial.flush();
}

// -----------------------------------------------------------------------------
// rtpMIDI session. Device disconnected
// -----------------------------------------------------------------------------
void OnAppleMidiDisconnected(uint32_t ssrc) {
  isConnected  = false;
  Serial.println("Disconnected");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOn(byte channel, byte note, byte velocity) {
  Serial.print("Incoming NoteOn from channel:");
  Serial.print(channel);
  Serial.print(" note:");
  Serial.print(note);
  Serial.print(" velocity:");
  Serial.print(velocity);
  Serial.println();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOff(byte channel, byte note, byte velocity) {
  Serial.print("Incoming NoteOff from channel:");
  Serial.print(channel);
  Serial.print(" note:");
  Serial.print(note);
  Serial.print(" velocity:");
  Serial.print(velocity);
  Serial.println();
}
