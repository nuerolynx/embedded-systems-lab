#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoBLE.h>
#include "access_config.h"

// ---------------- RFID ----------------
#define SS_PIN   10
#define RST_PIN  9
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------------- IO ----------------
#define RELAY_PIN      7   // your relay IN1
#define RED_LED_PIN    2
#define GREEN_LED_PIN  3
#define BUZZER_PIN     6

// Timing
const unsigned long ACTION_MS   = 5000;
const unsigned long COOLDOWN_MS = 800;

// Your valid UID
// The authorized card UID is loaded from the ignored access_config.h file.

// State
unsigned long offAt = 0;
unsigned long lastEventMs = 0;
String lastUID = "";

// ---------------- RELAY LOGIC (the one that worked for you) ----------------
// ACTIVE-HIGH: relay ON = HIGH, OFF = LOW
inline void relayOn()  { digitalWrite(RELAY_PIN, HIGH); }
inline void relayOff() { digitalWrite(RELAY_PIN, LOW);  }

void allOff() {
  relayOff();
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

// ---------------- BEEP PATTERNS (tone version) ----------------
void soundGranted() {
  tone(BUZZER_PIN, 1800, 120); delay(160);
  tone(BUZZER_PIN, 2200, 140); delay(160);
  noTone(BUZZER_PIN);
}

void soundDenied() {
  tone(BUZZER_PIN, 400, 180); delay(240);
  tone(BUZZER_PIN, 400, 180); delay(240);
  tone(BUZZER_PIN, 300, 220); delay(260);
  noTone(BUZZER_PIN);
}

// ---------------- UID helper ----------------
String uidToString(MFRC522::Uid *uid) {
  String s; s.reserve(3 * uid->size);
  for (byte i = 0; i < uid->size; i++) {
    if (uid->uidByte[i] < 0x10) s += "0";
    s += String(uid->uidByte[i], HEX);
    if (i < uid->size - 1) s += ":";
  }
  s.toUpperCase();
  return s;
}

// ---------------- ACCESS ACTIONS ----------------
void grantAccess(const char* source);
void denyAccess(const char* source);

// ---------------- BLE “GUI” ----------------
// Custom UUIDs (you can change later)
BLEService testDoorService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");

// Write command string here (e.g. "open test door")
BLEStringCharacteristic cmdChar(
  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E",
  BLEWrite,
  40
);

// Notify status back to phone
BLEStringCharacteristic statusChar(
  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E",
  BLENotify,
  40
);

void setStatus(const String& s) {
  Serial.print("STATUS: ");
  Serial.println(s);
  statusChar.writeValue(s);
}

void grantAccess(const char* source) {
  allOff();
  Serial.print("ACCESS GRANTED via ");
  Serial.println(source);

  digitalWrite(GREEN_LED_PIN, HIGH);
  relayOn();
  soundGranted();

  offAt = millis() + ACTION_MS;
  setStatus(String("GRANTED (") + source + ")");
}

void denyAccess(const char* source) {
  allOff();
  Serial.print("ACCESS DENIED via ");
  Serial.println(source);

  digitalWrite(RED_LED_PIN, HIGH);
  soundDenied();

  offAt = millis() + ACTION_MS;
  setStatus(String("DENIED (") + source + ")");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  allOff();

  // RFID init
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  // BLE init
  if (!BLE.begin()) {
    Serial.println("BLE failed to start. Check UNO R4 WiFi radio firmware v0.2.0+.");
    while (1) delay(100);
  }

  BLE.setLocalName("Access Control Board");
  BLE.setDeviceName("Access Control Board");

  BLE.setAdvertisedService(testDoorService);
  testDoorService.addCharacteristic(cmdChar);
  testDoorService.addCharacteristic(statusChar);
  BLE.addService(testDoorService);

  cmdChar.writeValue("");               // clear
  statusChar.writeValue("READY");       // initial status

  BLE.advertise();

  Serial.println("BLE Advertising as: Access Control Board");
  Serial.println("Write command: open test door");
  Serial.println("RFID + BLE control ready.");
}

void loop() {
  unsigned long now = millis();

  // Turn everything off after 5 seconds
  if (offAt && now > offAt) {
    allOff();
    offAt = 0;
    setStatus("READY");
  }

  // Keep BLE alive
  BLE.poll();

  // Handle BLE command writes
  if (cmdChar.written()) {
    String cmd = cmdChar.value();
    cmd.trim();
    cmd.toLowerCase();

    Serial.print("BLE CMD: ");
    Serial.println(cmd);

    if (cmd == "open test door") {
      grantAccess("BLE");
    } else {
      setStatus("UNKNOWN CMD");
    }
  }

  // Handle RFID scans
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = uidToString(&rfid.uid);

  if ((now - lastEventMs) < COOLDOWN_MS && uid == lastUID) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  lastEventMs = now;
  lastUID = uid;

  Serial.print("RFID TAG: ");
  Serial.println(uid);

  if (uid == VALID_UID) grantAccess("RFID");
  else                 denyAccess("RFID");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(50);
}
