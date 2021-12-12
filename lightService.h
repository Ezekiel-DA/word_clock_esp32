#pragma once

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "config.h"
#include "LightDataBlock.h"

struct MWNEXTDeviceInfo {
  MWNEXT_DEVICE_TYPE type;
  BLEUUID uuid;
  std::string name;
};

class LightService : public BLECharacteristicCallbacks {
private:
  MWNEXTDeviceInfo _deviceInfo;
  BLEService* _service = nullptr;

public:
  LightDataBlock* _lightData;

  LightService(BLEServer* iServer, MWNEXTDeviceInfo& iDeviceInfo, LightDataBlock* iLightData);
  
  void onWrite(BLECharacteristic* characteristic);

  void setHue(uint8_t iHue);
  void setSaturation(uint8_t iSaturation);
  void setPatternID(uint8_t iPatternID);
  void setCycleColor(bool iCycleColor);

  // used after unstreaming a tag, to force notification of new values, since unstream is done via memcpy and not the above setters!
  void forceBLEUpdate();

  void debugDump();
};
