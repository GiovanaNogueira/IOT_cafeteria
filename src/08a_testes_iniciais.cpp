#include <Matter.h>

MatterColorLight luzRGBMatter;
MatterOccupancySensor movimentoMatter;
MatterHumiditySensor umidadeMatter;
MatterTemperatureSensor temperaturaMatter;
MatterThermostat termostatoMatter;

void recomissionarMatter() {
  if (!Matter.isDeviceCommissioned())
  {
    Serial.println("Procurando ambiente Matter");
    Serial.printf("Código de pareamento: %s\r\n", Matter.getManualPairingCode().c_str());
    Serial.printf("Site com QR code de pareamento: %s\r\n", Matter.getOnboardingQRCodeUrl().c_str());
    Serial.print("Procurando ambiente Matter...");
    while (!Matter.isDeviceCommissioned())
    {
      Serial.print(".");
      delay(1000);
    }
    Serial.println("\nConectado no ambiente Matter!");
  }
}

void setup() {
  Serial.begin(115200); delay(500);
  
  movimentoMatter.begin();
  temperaturaMatter.begin();
  luzRGBMatter.begin();
  umidadeMatter.begin();
  termostatoMatter.begin();

  Matter.begin();
  recomissionarMatter();
}

void loop() {
  recomissionarMatter();
}