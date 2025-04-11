#define BLYNK_TEMPLATE_ID "TMPL26d6IlP2q"
#define BLYNK_TEMPLATE_NAME "State Switching"
#define BLYNK_AUTH_TOKEN "_aAPe-_6hUGCvPYNFXiFMn8O9GUd7_X-"
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

/*char ssid[] = "Seth iPhone";
char pass[] = "sethavery";*/

char ssid[] = "MyResNet-2G";
char pass[] = "Sethavery1012";


#include "tpms.h"

TaskHandle_t Task_TPMS;


// BLE service
BLEScan* pBLEScan;
BLEClient*  pClient;
static BLEAddress *pServerAddress;
String knownAddresses[] = { "80:ea:ca:13:09:d6" , "81:ea:ca:23:0a:e7"};   //address of my TPMS sensors
String tirePositions[] = { "Front Tire", "Rear Tire" };
int desiredPressure = 20; // -> V12
// Front Tire -> V2, Rear Tire -> V7

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {}


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  public:
    void onResult(BLEAdvertisedDevice Device) override {
        pServerAddress = new BLEAddress(Device.getAddress());
        String ManufData = Device.toString().c_str();
        
        for (int i = 0; i < 2; i++) {
            if (strcmp(pServerAddress->toString().c_str(), knownAddresses[i].c_str()) == 0){
                Serial.println("Device found: " + tirePositions[i]);
                //Serial.print("RSSI: ");
                //Serial.println(Device.getRSSI());

                String inString = retmanData(ManufData, 0);
                
                // Pressure conversion
                long int kPa = returnData(inString, 8) / 1000.0;
                long int psi = (kPa * 1000) / 6895;
                Serial.print("Pressure: ");
                Serial.print(psi);
                Serial.println(" PSI");

                // Send pressure data to Blynk
                int whichPin = (i == 0) ? V2 : V7;
                Blynk.virtualWrite(whichPin, psi);

                if(psi < desiredPressure){
                  Blynk.logEvent("tire_pressure");
                  Serial.print("ALARM, your ");
                  if(i==0){
                    Serial.println("FRONT tire is low");
                  }
                  else{
                    Serial.println("BACK tire is low");
                  }
                }

                // Battery percentage
                Serial.print("Battery: ");
                Serial.print(returnBatt(inString));
                Serial.println("%");

                Serial.println("");
            }
        }
    }
};


// Task running on Core 1
void TaskTPMS(void *pvParameters) {
  while (1) {
    // Start the scan and retrieve results
    pBLEScan->start(3);  // Perform a scan for 3 seconds
    pBLEScan->clearResults();  // Optionally clear results to free memory
    Serial.println(uxTaskGetStackHighWaterMark(NULL));

    // Delay to prevent overwhelming the core
    vTaskDelay(pdMS_TO_TICKS(15000)); //18 seconds
  }
}




BLYNK_WRITE(V12){
  int p1 = param.asInt(); //read desired Tire pressure
  desiredPressure = p1;
  Serial.print("pressure set to: ");
  Serial.println(desiredPressure);
}


BLYNK_CONNECTED()
{
  Serial.println("Connected to Blynk. Syncing...");
  Blynk.syncVirtual(V12);  // will cause BLYNK_WRITE(V0) to be executed
}



void setup() {
  // Opening serial port
  Serial.begin(115200);
  delay(100);


  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // BLE Init
  BLEDevice::init("");
  pClient  = BLEDevice::createClient();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);

  xTaskCreatePinnedToCore(TaskTPMS, "Task2", 2048, NULL, 2, &Task_TPMS, 1);

}

void loop() {
  Blynk.run();
}






