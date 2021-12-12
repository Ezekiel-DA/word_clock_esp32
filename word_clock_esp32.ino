#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <FastLED.h>

#include "config.h"
#include "lights.h"
#include "BLE.h"
#include "CostumeControllerService.h"
#include "lightService.h"

PatternLightLEDStrip<21> wordLEDs(130);
PatternLightPWMPort topLeftLED(19);
PatternLightPWMPort bottomRightLED(13);
PatternLightPWMPort bottomLeftLED(14);
PatternLightPWMPort topRightLED(15);
ILight *lights[] = {&wordLEDs, &topLeftLED, &bottomRightLED, &bottomLeftLED, &topRightLED};
extern const byte NUM_LIGHTOBJECTS = sizeof(lights) / sizeof(void *);

byte whichObject = 0;

MWNEXTDeviceInfo MWNEXTDevices[] = {
  {.type=MWNEXT_DEVICE_TYPE::RGB_LED,     .uuid=(BLEUUID)MWNEXT_BLE_CLOUDS_SERVICE_UUID,   .name="Words"},
  {.type=MWNEXT_DEVICE_TYPE::ONOFF_PORT,  .uuid=(BLEUUID)MWNEXT_BLE_WINDOWS_SERVICE_UUID,  .name="TopLeft"},
  {.type=MWNEXT_DEVICE_TYPE::ONOFF_PORT,  .uuid=(BLEUUID)MWNEXT_BLE_WALLS_SERVICE_UUID,    .name="BottomRight"},
  {.type=MWNEXT_DEVICE_TYPE::ONOFF_PORT,  .uuid=(BLEUUID)MWNEXT_BLE_MOAT_SERVICE_UUID,     .name="BottomLeft"},
  {.type=MWNEXT_DEVICE_TYPE::ONOFF_PORT,  .uuid=(BLEUUID)MWNEXT_BLE_STARS_SERVICE_UUID,    .name="topRight"}
};
const uint8_t NUM_MWNEXT_BLE_SERVICES = sizeof(MWNEXTDevices) / sizeof(MWNEXTDeviceInfo);

CostumeControlService* costumeController = nullptr;

static LightService* MWNEXTServices [NUM_MWNEXT_BLE_SERVICES];

void setup() {
  Serial.begin(115200);

  BLEDevice::init("WordClock2");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  
  // disable this for now, to stay under what appears to be a 6 user services limit (https://github.com/espressif/esp-idf/issues/5495)
  // BLEService* devInfoService = server->createService((uint16_t)ESP_GATT_UUID_DEVICE_INFO_SVC);
  // BLECharacteristic* manufacturerID = devInfoService->createCharacteristic((uint16_t)ESP_GATT_UUID_MANU_NAME, BLECharacteristic::PROPERTY_READ);
  // manufacturerID->setValue("Team Freyja");
  // devInfoService->start();

  costumeController = new CostumeControlService(server);

  Serial.print("Creating "); Serial.print(NUM_MWNEXT_BLE_SERVICES); Serial.println(" services...");
  for (byte i = 0; i < NUM_MWNEXT_BLE_SERVICES; ++i) {
    MWNEXTServices[i] = new LightService(server, MWNEXTDevices[i], &(lights[i]->_lightDataBlock));
  }
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAppearance(0x0CC1); // powered wheelchair appearance

  // advertising: we wanna advertise a service with a fixed "name" (UUID), so that clients can look for this, decide to connect based on that,
  // and then enumerate the other services (one per logical "device" on the costume), which do not need to be advertised.
  // pAdvertising->addServiceUUID((uint16_t)ESP_GATT_UUID_DEVICE_INFO_SVC); // maybe don't advertise a service we disabled. No idea how that worked for a while!
  pAdvertising->addServiceUUID(MWNEXT_BLE_COSTUME_CONTROL_SERVICE_UUID);
  
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue // NLV: what the heck is this? Taken from the ESP32 samples, should probably test what "iPhone issue" it's talking about...
  pAdvertising->setMinPreferred(0x12);
  
  BLEDevice::startAdvertising();
  Serial.println("BLE init complete.");

  for (byte i = 0; i < NUM_LIGHTOBJECTS; ++i)
  {
    lights[i]->setup();
  }

  FastLED.setMaxRefreshRate(60); // 60 FPS cap
  FastLED.clear();

  Serial.print("Word Clock 2 ready; ");
  Serial.print(NUM_LIGHTOBJECTS);
  Serial.println(" lights available.");

  FastLED.show();

  // pulse the selected light, also serves as a boot up complete indicator
  // lights[whichObject]->pulse();
}

void loop()
{

  for (byte i = 0; i < NUM_LIGHTOBJECTS; ++i)
  {
    lights[i]->update();
  }

  FastLED.show();
};
