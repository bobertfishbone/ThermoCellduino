#include <EEPROM.h>

#include <Adafruit_FONA.h>

#include <SoftwareSerial.h>

#include <math.h>
const int RX = 8;
const int TX = 9;
const int RST = 10;
SoftwareSerial fonaSS = SoftwareSerial(TX, RX);
Adafruit_FONA fona = Adafruit_FONA(&fonaSS, RST);

unsigned int setTemp = EEPROM.read(0);
unsigned int aTemp = 50;
unsigned int bTemp = 50;
unsigned int cTemp = 50;



char msg[60];
char readMsg[50];
char MY_PHONE_NUMBER[13] = PHONE_NUMBER;
boolean sent = false;

volatile boolean ringing = false;
String bufferString;
String A = "Bottom: ";
String B = " Middle: ";
String C = " Top: ";
String set = " Setpoint: ";
void ringInterrupt () {
  ringing = true;
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 13; i++) {
    MY_PHONE_NUMBER[i] = EEPROM.read(i + 2);
    delay(100);
  }

  while (1) {
    Serial.println(F("Waiting for Fona to start"));
    boolean fonaStarted = fona.begin(4800);
    if (fonaStarted) break;
    delay (1000);
  }
  Serial.println(F("Fona Started!"));
  while (1) {
    Serial.println(F("Waiting for network status"));
    uint8_t network_status = fona.getNetworkStatus();
    if (network_status == 1 || network_status == 5) break;
    delay(250);
  }
  Serial.println(F("Network found!"));
  aTemp = int(Thermistor(analogRead(0)));
  bTemp = int(Thermistor(analogRead(1)));
  cTemp = int(Thermistor(analogRead(2)));


  attachInterrupt(1, ringInterrupt, FALLING);
  deleteSMS();
  Serial.println("I'm here!");
  Serial.println(MY_PHONE_NUMBER);
}

void loop() {
  Serial.println(sent);
  aTemp = int(Thermistor(analogRead(0)));
  bTemp = int(Thermistor(analogRead(1)));
  cTemp = int(Thermistor(analogRead(2)));

  String msgString = A + aTemp + B + bTemp + C + cTemp + set + setTemp;
  msgString.toCharArray(msg, 60);
        Serial.println(msgString);
  // If I got a text, figure out wtf it means and respond accordingly
  if (ringing) handleRing();

  // If the temperature of A, B, and C is lower than one degree less than the setpoint:
  if ((aTemp < (setTemp - 2)) && (bTemp < (setTemp - 2) ) && (cTemp < (setTemp - 2))) {
    // If sent is true, then set sent = false
    if (sent) {
      Serial.println(F("Setting sent to false!"));
      sent = false;
    }
    // Otherwise if sent is already false, do nothing.
    else {
      Serial.println(F("Sent is already false!"));
    }

  }
  // If the temperature of A, B, and C is NOT less than one degree less than the setpoint, check
  // to see if A, B, or C is greater than the setpoint.
  else if ((aTemp > setTemp) || (bTemp > setTemp) || (cTemp > setTemp)) {

    //If so, and sent is false, then send SMS
    if (!sent) {
            Serial.println(F("A temp is high, and sent is false!"));
      sent = true;
      Serial.println(msgString);

      if (fonaSendTempSMS(MY_PHONE_NUMBER)) {
        Serial.println(F(" sent."));
      }
      else {
        Serial.println(F(" failed!"));
      }

    }
    //Otherwise sent is true; don't send anything
    else {
      Serial.println(F("Sent is already true."));
    }

  }
  // If A, B, C is NOT less than one degree less than the setpoint, and NOT greater than the setpoint,
  // do nothing.
  else {
      Serial.println(F("Not too high, not too low. Just chill out."));
  }


}




double Thermistor(int RawADC) {
    double Temp;
    Temp =  log(10000.0 / (1024.0 / RawADC - 1));
    //         log(10000.0*((1024.0/RawADC-1))); // for pull-down configuration
    Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp )) * Temp );
    Temp = Temp - 273.15;            // Convert Kelvin to Celsius
    Temp = (Temp * 9.0) / 5.0 + 32.0; // Convert Celsius to Fahrenheit
  return Temp;
}

void handleRing () {
  Serial.println(F("Ring ring."));

  fonaSS.listen();
  int8_t sms_num = fona.getNumSMS();

  if (sms_num > -1) {
    Serial.print(sms_num); Serial.println(" messages waiting.");

    char sms_buffer[20];
    char senderbuffer[13];
    uint16_t smslen;

    // Read any SMS message we may have...
    for (int8_t sms_index = 1; sms_index <= sms_num; sms_index++) {
      Serial.print(F("  SMS #")); Serial.print(sms_index); Serial.print(F(": "));
      if (! fona.getSMSSender(sms_index, senderbuffer, 250)) {
        Serial.println("Failed!");
        break;
      }
      Serial.print(F("FROM: ")); Serial.println(senderbuffer);
      for (int i = 0; i < sizeof(senderbuffer) - 1; i++) {
        MY_PHONE_NUMBER[i] = senderbuffer[i];
      }
      Serial.println(MY_PHONE_NUMBER);
      if (fona.readSMS(sms_index, sms_buffer, 8, &smslen)) {
        // if the length is zero, its a special case where the index number is higher
        // so increase the max we'll look at!
        if (smslen == 0) {
          Serial.println(F("[empty slot]"));
          sms_num++;
          continue;
        }

        Serial.println(sms_buffer);
        Serial.print("atoi = ");
        Serial.println(atoi(sms_buffer));

        // If it matches our pre-defined command string...
        if (strcmp(sms_buffer, "Temp") == 0) {
          Serial.print(F("  Responding with temperature..."));
          String msgString = A + aTemp + B + bTemp + C + cTemp + set + setTemp;
          msgString.toCharArray(msg, 60);
          if (fonaSendTempSMS(MY_PHONE_NUMBER)) {
            Serial.println(F(" sent."));
          } else {
            Serial.println(F(" failed!"));
          }

          // delete this SMS
          fona.deleteSMS(sms_index);
        } else if (strcmp(sms_buffer, "Status") == 0) {
          Serial.print(F("  Responding with status..."));

          if (fonaSendStatusSMS(MY_PHONE_NUMBER)) {
            Serial.println(F(" sent."));
          } else {
            Serial.println(F(" failed!"));
          }

          // delete this SMS
          fona.deleteSMS(sms_index);
        }
        else if (atoi(sms_buffer) > 0 ) {
          Serial.print(F("  Setting threshold..."));
          setTemp = atoi(sms_buffer);
          Serial.println(setTemp);
          if (fonaSendConfirmSMS(MY_PHONE_NUMBER, setTemp)) {
            Serial.println(F(" sent."));
          } else {
            Serial.println(F(" failed!"));
          }

          // delete this SMS
          fona.deleteSMS(sms_index);
        }
        else {
          Serial.println(F("Failed to read SMS messages!"));
        }
      }


      ringing = false;
    }
  }
}
boolean fonaSendStatusSMS (char *recipient) {
  uint8_t rssi = fona.getRSSI();
  uint16_t vbat;
  fona.getBattVoltage(&vbat);

  char sms_response[16];
  sprintf (sms_response, "%d bars, %d mV", barsFromRSSI(rssi), vbat);

  return fona.sendSMS(recipient, sms_response);
}

uint8_t barsFromRSSI (uint8_t rssi) {
  // https://en.wikipedia.org/wiki/Mobile_phone_signal#ASU
  //
  // In GSM networks, ASU maps to RSSI (received signal strength indicator, see TS 27.007[1] sub clause 8.5).
  //   dBm = 2 × ASU - 113, ASU in the range of 0..31 and 99 (for not known or not detectable).

  int8_t dbm = 2 * rssi - 113;

  if (rssi == 99 || rssi == 0) {
    return 0;
  } else if (dbm < -107) {
    return 1;
  } else if (dbm < -98) {
    return 2;
  } else if (dbm < -87) {
    return 3;
  } else if (dbm < -76) {
    return 4;
  }

  return 5;
}

boolean fonaSendTempSMS (char *recipient) {
  char sms_response[52];

  sprintf (sms_response, msg);

  return fona.sendSMS(recipient, sms_response);
}

boolean fonaSendConfirmSMS (char *recipient, int setTemp) {
  char sms_response[52];
  int addr = 0;
  EEPROM.write(0, setTemp);
    aTemp = int(Thermistor(analogRead(0)));
  bTemp = int(Thermistor(analogRead(1)));
  cTemp = int(Thermistor(analogRead(2)));

  String msgString = A + aTemp + B + bTemp + C + cTemp + set + setTemp;
  msgString.toCharArray(msg, 60);
  sprintf (sms_response, msg);

  return fona.sendSMS(recipient, sms_response);
}

void deleteSMS() {
  // read all SMS
  int8_t smsnum = fona.getNumSMS();
  uint16_t smslen;
  for (int8_t smsn = 1; smsn <= smsnum; smsn++) {
    Serial.print(F("\n\rDeleting SMS #")); Serial.println(smsn);
    if (!fona.deleteSMS(smsn)) {
      Serial.println(F("Failed!"));
      break;
    }
  }
}
