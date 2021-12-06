// Basic ESP8266 Packet Monitor Interface for the SH1106
// github.com/HakCat-Tech/ESP8266-Packet-Monitor

#include "./esppl_functions.h"

#include "SH1106Wire.h"
SH1106Wire display(0x3C, SDA, SCL); // use builtin i2C

#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels {1, D8, NEO_GRB + NEO_KHZ800 }; // NeoPixel pin

#define ltBtn D7 // left button
#define rtBtn D5 // right button
#define upBtn D6
#define dnBtn D3

// button states and previous states
int lState = 0; int plState = 1;
int rState = 0; int prState = 1;
int uState = 0; int puState = 1;
int dState = 0; int pdState = 1;

String packet[7];
String devices[100][3]; int devCnt = 0;
String srcMac, ssid, src, dest;
char srcOctet[2], destOctet[2];
int addr, fst, ft;
String pktType;

int filter = 0 ;
int pFilter = 0 ;

uint32_t flashColor = 0 ;

void cb(esppl_frame_info *info) { /*--- WiFi Scanner Function ---*/
  ssid = "";
    src = "";  // source

  Serial.print("\n");
  Serial.print("FT: ");
  Serial.print((int) info->frametype);

  Serial.print(" FST: ");
  Serial.print((int) info->framesubtype);

  Serial.print(" SRC: ");
  for (int i = 0; i < 6; i++) Serial.printf("%02x", info->sourceaddr[i]);
      for (int i= 0; i< 6; i++) {
      sprintf(srcOctet, "%02x", info->sourceaddr[i]);
      src+=srcOctet;
    }

  Serial.print(" DEST: ");
  for (int i = 0; i < 6; i++) Serial.printf("%02x", info->receiveraddr[i]);
    dest = "";   // dest MAC
    for (int i= 0; i< 6; i++) {
      sprintf(destOctet, "%02x", info->receiveraddr[i]); dest+=destOctet;
    }

  Serial.print(" RSSI: ");
  Serial.print(info->rssi);

  Serial.print(" SEQ: ");
  Serial.print(info->seq_num);

  Serial.print(" CHNL: ");
  Serial.print(info->channel);

  if (info->ssid_length > 0) {
    Serial.print(" SSID: ");
    for (int i = 0; i < info->ssid_length; i++) Serial.print((char) info->ssid[i]);
  }
  if (info->ssid_length > 0) {
     for (int i= 0; i< info->ssid_length; i++) { ssid+= (char) info->ssid[i]; }
    }

   // append packets metadata to packet list
    packet[0] = (String) info->frametype;
    packet[1] = (String) info->framesubtype;
    packet[2] = src;
    packet[3] = dest;
    packet[4] = (String) info->rssi;
    packet[5] = (String) info->channel;
    packet[6] = ssid;
    ft = packet[0].toInt(); fst = packet[1].toInt();
}

void printPacket() { // function to print wifi packets to the screen
  boolean show = false ;

  if (ft == 0 and (fst == 0 or fst == 1)) {
      pktType = "Association Req.";
      show = (filter == 0) || (filter == 1) ;
  } else if (ft == 0 and (fst == 2 or fst == 3)) {
      pktType = "Re-Assoc";
      show = (filter == 0) || (filter == 2) ;
  } else if (ft == 0 and fst == 4) {
      pktType = "Probe Request";
      show = (filter == 0) || (filter == 3) ;
  } else if (ft == 0 and fst == 8 ) {
      pktType = "Beacon";
      show = (filter == 0) || (filter == 4) ;
  } else if (ft == 0 and fst == 10) {
      pktType = "Disassociation";
      show = (filter == 0) || (filter == 5) ;
  } else if (ft == 0 and fst == 11) {
      pktType = "Authentication";
      show = (filter == 0) || (filter == 6) ;
  } else if (ft == 0 and fst == 12) {
      pktType = "De-Authentication";
      show = (filter == 0) || (filter == 7) ;
  } else if (ft == 0) {
      pktType = "Management";
      show = (filter == 0) || (filter == 8) ;
  } else if (ft == 1) {
      pktType = "Control";
      show = (filter == 0) || (filter == 9) ;
  } else if (ft == 2) {
      pktType = "Data";
      show = (filter == 0) || (filter == 10) ;
  } else {
      pktType = "Extension";
      show = (filter == 0) || (filter == 11) ;
  }

  if (show == true) {
      display.clear();
      srcMac = packet[2];
      display.drawString(0,0,"PKT: "); display.drawString(30,0,pktType);
      display.drawString(0,8,"SRC: "); display.drawString(30,8,srcMac);
      display.drawString(0,16,"DST: "); display.drawString(30,16, packet[3]);
      display.drawString(0,24,"RSS: "); display.drawString(30,24,packet[4]);
      display.drawString(0,32,"CH: "); display.drawString(30,32, packet[5]);
      display.drawString(0,40,"SSID: ");
      if (packet[6].length() < 18) { display.drawString(30,40,packet[6]); }
      else if (packet[6].length() > 1) { display.drawString(30,40,packet[6].substring(0, 17 ) + "..."); }

      pixels.setPixelColor(0, flashColor) ;
      pixels.show() ;

      updateMenu();
      display.display();

  }
}

// check if button is pressed
void checkForPress() {
  lState = digitalRead(ltBtn);
  rState = digitalRead(rtBtn);
  uState = digitalRead(upBtn);
  dState = digitalRead(dnBtn);

  if (lState == 0 && lState!=plState) {
      filter-- ;
      if (filter < 0) filter = 11 ;
  } else if (rState == 0 && rState!=prState) {
      filter++ ;
      if (filter > 11) filter = 0 ;
  } else if (dState == 0 && dState != pdState) {
      filter = 0 ;
  } else if (uState == 0 && uState != puState) {
      filter = 4 ;
  }

  plState = lState;
  prState = rState;
  puState = uState;
  pdState = dState;

  if (filter != pFilter) {
      pFilter = filter ;
      display.clear();
      updateMenu();
      display.display();
  }

}

void updateMenu() { // update scroll menu and packet type selection
  display.drawLine(0,54, 127,54);
  display.drawLine(20,54, 20,63);
  display.fillTriangle(8, 59, 11, 56, 11, 62);
  display.drawLine(107,54, 107,63);
  display.fillTriangle(119, 59, 116, 56, 116, 62);

  String label ;

  if (filter == 0) {
    label = "ALL" ;        flashColor = pixels.Color(120, 120, 120) ;
  } else if (filter == 1) {
    label = "ASSOC" ;      flashColor = pixels.Color(120, 0, 0) ;
  } else if (filter == 2) {
    label = "RE-ASSOC" ;   flashColor = pixels.Color(120, 0, 0) ;
  } else if (filter == 3) {
    label = "PROBE" ;      flashColor = pixels.Color( 0, 120, 120) ;
  } else if (filter == 4) {
    label = "BEACON" ;     flashColor = pixels.Color( 0, 120, 120) ;
  } else if (filter == 5) {
    label = "DISASSOC" ;   flashColor = pixels.Color(120, 0, 0) ;
  } else if (filter == 6) {
    label = "AUTH" ;       flashColor = pixels.Color( 0, 120, 0) ;
  } else if (filter == 7) {
    label = "DEAUTH" ;     flashColor = pixels.Color( 0, 120, 0) ;
  } else if (filter == 8) {
    label = "MANAGEMENT" ; flashColor = pixels.Color(120, 0, 120) ;
  } else if (filter == 9) {
    label = "CONTROL" ;    flashColor = pixels.Color(120, 0, 120) ;
  } else if (filter == 10) {
    label = "DATA" ;       flashColor = pixels.Color( 0, 0, 120) ;
  } else {
    label = "EXTENSION" ;  flashColor = pixels.Color(120, 120, 0) ;
  }

  uint16_t txtX = 64 - (display.getStringWidth(label) / 2) ;
  display.drawString(txtX, 54, label) ;
}

void setup() {
  pinMode(ltBtn, INPUT_PULLUP);
  pinMode(rtBtn, INPUT_PULLUP);
  pinMode(upBtn, INPUT_PULLUP);
  pinMode(dnBtn, INPUT_PULLUP);

  delay(500);
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  pixels.begin(); pixels.clear();
  esppl_init(cb);
}

void loop() {
  esppl_sniffing_start();
  while (true) {

    pixels.setPixelColor(0, pixels.Color(0, 0, 0)) ;
    pixels.show() ;
    for (int i = 1; i < 15; i++ ) {
      esppl_set_channel(i);
      while (esppl_process_frames()) {
        // allow for menu updates while waiting for frames to be processed
        checkForPress() ;
      }
    }

    checkForPress() ;
    printPacket();

    //if (filter>0) delay(600); //dumb delay to display packets longer
    delay(0);
  }
}
