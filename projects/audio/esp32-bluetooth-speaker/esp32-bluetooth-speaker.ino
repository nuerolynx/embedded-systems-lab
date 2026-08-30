/*
  ESP32 Bluetooth Speaker (A2DP Sink) -> I2S Amp/DAC
  Loud, stable version (keeps it simple so it doesn't WDT reset)

  Requires:
    - Espressif ESP32 Arduino core 3.x
    - ESP32-A2DP library installed

  I2S pins configured by this sketch:
    BCLK  = GPIO 26
    LRCLK = GPIO 25
    DATA  = GPIO 22

  Amp terminals:
    G -> GND
    V -> 5V/VIN
    BCLK -> GPIO26
    LRCLK -> GPIO25
    DIN -> GPIO22
*/

#include "ESP_I2S.h"
#include "BluetoothA2DPSink.h"

I2SClass i2s;
BluetoothA2DPSink a2dp_sink(i2s);

static const char* BT_NAME = "ESP32 Speaker";

// 0..127 (127 = max). Start high but not always max to reduce clipping risk.
// You can tune it live via Serial commands below.
static uint8_t volume_level = 115;

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  v<0-127>   Set volume (example: v127)");
  Serial.println("  +          Volume up 5");
  Serial.println("  -          Volume down 5");
  Serial.println("  ?          Show this help");
  Serial.println();
}

void applyVolume() {
  a2dp_sink.set_volume(volume_level);
  Serial.print("Volume set to: ");
  Serial.println(volume_level);
}

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println("====================================");
  Serial.println("ESP32 Bluetooth Speaker (LOUD build)");
  Serial.println("Pair and play from your phone.");
  Serial.println("Tip: Set phone MEDIA volume to 100%.");
  Serial.println("Default I2S: BCLK=26 LRCLK=25 DATA=22");
  Serial.println("====================================");

  i2s.setPins(26, 25, 22);
  if (!i2s.begin(I2S_MODE_STD, 44100, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("ERROR: failed to initialize I2S output");
    while (true) delay(1000);
  }

  // Start A2DP sink
  a2dp_sink.start(BT_NAME);

  // Apply high volume after BT stack is up
  delay(300);
  applyVolume();

  printHelp();
}

void loop() {
  // Simple Serial control so you can find the loudest CLEAN level
  if (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '+') {
      volume_level = (volume_level > 122) ? 127 : (volume_level + 5);
      applyVolume();
    } else if (c == '-') {
      volume_level = (volume_level < 5) ? 0 : (volume_level - 5);
      applyVolume();
    } else if (c == '?') {
      printHelp();
    } else if (c == 'v' || c == 'V') {
      // Read number that follows v
      int val = Serial.parseInt();
      if (val < 0) val = 0;
      if (val > 127) val = 127;
      volume_level = (uint8_t)val;
      applyVolume();
    }
  }
}
