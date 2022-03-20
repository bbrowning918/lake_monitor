#include <SoftwareSerial.h>
#include <Adafruit_FONA.h>
#include <DHT.h>
#include "conf.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

char *allowList[] = ALLOW_LIST;

void setup() {
  Serial.begin(115200);

  dht.begin();
  fonaSerial->begin(4800);

  if ( !fona.begin(*fonaSerial)) {
    while (1);
  }

  while (fona.getNetworkStatus() != 1) {
    delay(5000);
  }
}

void loop() {
  float t = dht.readTemperature();
  Serial.println(t);
  if(isnan(t)) {
    fona.sendSMS(ALERT_PHONE,"Failed to read sensor");
    delay(86400000); //wait 24 hrs
    return;
  } else if(t <= 5.0) {
    Serial.println(F("Low temp alarm"));
    fona.sendSMS(ALERT_PHONE,"Low temp alarm; <5C detected");
    delay(86400000); //wait 24 hrs
    return;
  } else if (fona.getNumSMS() <= 0) {
    delay(1000);
    return;
  } else {
    uint16_t msgLen;
    char msg[50];
    char sender[13];

    uint8_t len = fona.readSMS(1, msg, 50, &msgLen);
    if (len == 0) {
      fona.deleteSMS(1);
      return;
    }

    if (!fona.getSMSSender(1, sender, sizeof(sender))) {
      // failed to get the sender?
      sender[0] = 0;
    }

    if (onAllowList(sender)) {
      if (strcasecmp(msg, "report") == 0) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        if (isnan(h) || isnan(t)) {
          fona.sendSMS(sender, "Failed to read from sensor!" );
        } else {
          char reply[50];
          generateReport(t, h, reply);
          fona.sendSMS(sender, reply);
        }
      }
    }
    fona.deleteSMS(1);
  }
  delay(1000);
}

bool onAllowList(char sender[]) {
  for (int i = 0; i < sizeof(allowList) / sizeof(char); i++) {
    if (strcasecmp(sender, allowList[i]) == 0 ) return true;
  }
  return false;
}

void generateReport(float temp, float humid, char* report) {
  String tempStr = "Temp: ";
  String humidStr = "Humidity: ";
  String reportStr = tempStr + temp + "C\n" + humidStr + humid + "%";

  reportStr.toCharArray(report, 50);
}
