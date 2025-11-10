#include <Arduino.h>
#include <GxEPD2_BW.h> 
#include <U8g2_for_Adafruit_GFX.h>
#include <WiFi.h> 
#include <WiFiClientSecure.h>
#include "certificados.h" 
#include <MQTT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HX711.h>
#include <GFButton.h>
#include <ArduinoJson.h>


U8G2_FOR_ADAFRUIT_GFX fontes; 
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16); 
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);

MFRC522 rfid(46, 17);
MFRC522::MIFARE_Key chaveA =
{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

// GFButton up(1);
// GFButton down(2);
GFButton left(3);
GFButton right(4);
// GFButton press(5);

JsonDocument produto;


HX711 balanca;

unsigned long instanteAnterior = 0; 

bool usuarioValido = false;
bool roda = true;
bool comecaPesagem = false;
bool finalizou = false;

const float PESO_MAXIMO = 200.0f;

void telaInicial(){
  tela.fillScreen(GxEPD_WHITE); 
  fontes.setFont( u8g2_font_helvB24_te );
  fontes.setFontMode(1);
  fontes.setCursor(50, 50);
  fontes.print("Bem vindo!");

  fontes.setFont( u8g2_font_helvB12_te );
  fontes.setFontMode(1);
  fontes.setCursor(60, 80);
  fontes.print("Aproxime sua tag no");
  fontes.setCursor(90, 100);
  fontes.print("local indicado");

  tela.display(true);

}

void usuarioInvalido(){
  tela.fillScreen(GxEPD_WHITE); 
  fontes.setFont( u8g2_font_helvB24_te );
  fontes.setFontMode(1);
  fontes.setCursor(70, 50);
  fontes.print("Inválido!");

  fontes.setFont( u8g2_font_helvB12_te );
  fontes.setFontMode(1);
  fontes.setCursor(70, 90);
  fontes.print("Tente novamente");
  tela.display(true);
}

void telaProdutos(){
  tela.fillScreen(GxEPD_WHITE); 
  fontes.setFont( u8g2_font_helvR10_te );
  fontes.setFontMode(1);
  fontes.setCursor(160, 20);
  fontes.print("Giovana: R$10,00");


  fontes.setFont( u8g2_font_helvB14_te );
  fontes.setFontMode(1);
  fontes.setCursor(10, 50);
  fontes.print("Selecione o produto:");

  fontes.setFont( u8g2_font_helvB12_te );
  fontes.setFontMode(1);
  fontes.setCursor(10, 90);
  fontes.print("Amendoim");
  fontes.setFont( u8g2_font_helvR12_te );
  fontes.setFontMode(1);
  fontes.setCursor(10, 110);
  fontes.print("R$0,50");


  fontes.setFont( u8g2_font_helvB12_te );
  fontes.setFontMode(1);
  fontes.setCursor(200, 90);
  fontes.print("M&M");
  fontes.setFont( u8g2_font_helvR12_te );
  fontes.setFontMode(1);
  fontes.setCursor(200, 110);
  fontes.print("R$0,80");
  tela.display(true);
}

void telaSegure(){
  tela.fillScreen(GxEPD_WHITE); 
  fontes.setFont( u8g2_font_helvB14_te );
  fontes.setFontMode(1);
  fontes.setCursor(20, 50);
  fontes.print("Segure o botão do produto");

  fontes.setFont( u8g2_font_helvB14_te );
  fontes.setFontMode(1);
  fontes.setCursor(70, 90);
  fontes.print("para despejá-lo");
  tela.display(true);
}

void telaSaldoFinal(float preco){
  tela.fillScreen(GxEPD_WHITE); 
  fontes.setFont( u8g2_font_helvB14_te );
  fontes.setFontMode(1);
  fontes.setCursor(90, 50);
  fontes.print("Saldo atual:");

  float saldoAtual = 10.00 - preco;

  fontes.setFont( u8g2_font_helvR14_te );
  fontes.setFontMode(1);
  fontes.setCursor(120, 90);
  fontes.print("R$" + String(saldoAtual, 2));
  tela.display(true);
}

void telaSaldoNegativo(){
  tela.fillScreen(GxEPD_WHITE);
  fontes.setFont( u8g2_font_helvB14_te );
  fontes.setFontMode(1);
  fontes.setCursor(90, 30);
  fontes.print("Saldo negativo");

  fontes.setFont( u8g2_font_helvR14_te );
  fontes.setFontMode(1);
  fontes.setCursor(120, 70);
  fontes.print("R$-0,50");

  fontes.setFont( u8g2_font_helvR12_te );
  fontes.setFontMode(1);
  fontes.setCursor(100, 110);
  fontes.print("Faça a recarga!");
  tela.display(true);
}

void telaSemEstoque(){
  tela.fillScreen(GxEPD_WHITE);
  fontes.setFont( u8g2_font_helvB24_te );
  fontes.setFontMode(1);
  fontes.setCursor(45, 50);
  fontes.print("Sem estoque");

  fontes.setFont( u8g2_font_helvB12_te );
  fontes.setFontMode(1);
  fontes.setCursor(65, 90);
  fontes.print("Escolha outro produto");
  tela.display(true);
}

void finalizarCompra(float pesoMedido) {
  produto["total"] = produto["preco"].as<float>() * (pesoMedido / 100.0);
  produto["peso"] = pesoMedido;
  float total = produto["total"];
  telaSaldoFinal(total);
  String textoJson;
  serializeJson(produto, textoJson);
  mqtt.publish("cafeteria_iot", textoJson);
  produto.clear();
  comecaPesagem = false;
  finalizou = true;
}

void telaPesagem(){
  tela.fillScreen(GxEPD_WHITE); 
  tela.drawLine(50, 30, 50, 110, GxEPD_BLACK);
  tela.drawLine(50, 110, 150, 110, GxEPD_BLACK);
  tela.drawLine(150, 30, 150, 110, GxEPD_BLACK);

  // 🔹 Lê o peso atual
  float peso = balanca.get_units(1);
  if (comecaPesagem && (peso >= PESO_MAXIMO * 0.995f)) { // 0.5% de margem contra ruído
    finalizarCompra(peso);
    return; // já mudou para a tela final
  }

  float nivel = constrain(peso / PESO_MAXIMO, 0.0, 1.0);

  int altura = nivel * 80; 
  int yInicio = 110 - altura;

  tela.fillRect(50, yInicio, 100, altura, GxEPD_BLACK);

  float preco = produto["preco"].as<float>() * (peso / 100.0);

  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(200, 70);
  fontes.print("R$ " + String(preco, 2));

  tela.display(true);
}


String lerRFID() {
  String id = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) {
      id += " ";
    }
    if (rfid.uid.uidByte[i] < 0x10) {
      id += "0";
    }
    id += String(rfid.uid.uidByte[i], HEX);
  }
  id.toUpperCase();
  return id;
}

String lerTextoDoBloco(byte bloco) {
  byte tamanhoDados = 18;
  char dados[tamanhoDados];
  MFRC522::StatusCode status = rfid.PCD_Authenticate(
  MFRC522::PICC_CMD_MF_AUTH_KEY_A,
  bloco, &chaveA, &(rfid.uid)
  );
  if (status != MFRC522::STATUS_OK) { return ""; }
  status = rfid.MIFARE_Read(bloco,
  (byte*)dados, &tamanhoDados);
  if (status != MFRC522::STATUS_OK) { return ""; }
  dados[tamanhoDados - 2] = '\0';
  return String(dados);
}

void reconectarWiFi() { 
  if (WiFi.status() != WL_CONNECTED) { 
    WiFi.begin("Projeto", "2022-11-07"); 
    Serial.print("Conectando ao WiFi..."); 
    while (WiFi.status() != WL_CONNECTED) { 
      Serial.print("."); 
      delay(1000); 
    } 
    Serial.print("conectado!\nEndereço IP: "); 
    Serial.println(WiFi.localIP()); 
  } 
} 

void reconectarMQTT() { 
  if (!mqtt.connected()) { 
    Serial.print("Conectando MQTT..."); 
    while(!mqtt.connected()) { 
      mqtt.connect("Giovana", "aula", "zowmad-tavQez"); 
      Serial.print("."); 
      delay(1000); 
    } 
    Serial.println(" conectado!"); 
    
    mqtt.subscribe("cafeteria_iot");  
  } 
}

void recebeuMensagem(String topico, String conteudo) { 
  Serial.println(topico + ": " + conteudo); 
}


void botaoPressionadoLeft (GFButton& botaoDoEvento) {
  Serial.println("Botão esquerdo foi pressionado!");

  if(produto["id"].isNull()){
    produto["id"] = 1;
    produto["nome"] = "Amendoim";
    produto["preco"] = 0.50;
    String nome = produto["nome"]; 
    float preco = produto["preco"];
    Serial.println("Produto selecionado: " + nome + " - R$" + String(preco));
    telaSegure();
    return;
  }
  comecaPesagem = true;
}

void botaoSolto(GFButton& botaoDoEvento) {
  Serial.println("Botão esquerdo foi solto!");
  if(comecaPesagem){
    float pesoMedido = balanca.get_units(1);
    finalizarCompra(pesoMedido);
  }
}

void botaoPressionadoRight (GFButton& botaoDoEvento) {
  Serial.println("Botão direito foi pressionado!");
  if(produto["id"].isNull()){
    produto["id"] = 2;
    produto["nome"] = "M&M";
    produto["preco"] = 0.80;
    String nome = produto["nome"];
    float preco = produto["preco"];
    Serial.println("Produto selecionado: " + nome + " - R$" + String(preco));
    telaSegure();
    return;
  }
  comecaPesagem = true;
}


void setup() {
  Serial.begin(115200); delay(500);

  reconectarWiFi(); 
  conexaoSegura.setCACert(certificado1);

  mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura); 
  mqtt.onMessage(recebeuMensagem); 
  mqtt.setKeepAlive(10); 
  reconectarMQTT();

  tela.init(); 
  tela.setRotation(3); 
  tela.fillScreen(GxEPD_WHITE); 
  tela.display(true);

  fontes.begin(tela); 
  fontes.setForegroundColor(GxEPD_BLACK);

  SPI.begin();
  rfid.PCD_Init();

  balanca.begin(6, 7);
  balanca.set_scale(478);
  balanca.tare(5);

  left.setPressHandler(botaoPressionadoLeft);
  left.setReleaseHandler(botaoSolto);
  right.setPressHandler(botaoPressionadoRight);
  right.setReleaseHandler(botaoSolto);
}

void loop() {
  reconectarWiFi(); 
  reconectarMQTT(); 
  mqtt.loop();


  unsigned long instanteAtual = millis(); 
  if(roda){
    telaInicial();
  }

  if(finalizou){
    if (instanteAtual > instanteAnterior + 30000) { 
      finalizou = false;
      usuarioValido = false;
      comecaPesagem = false;
      produto.clear();
      roda = true;
      telaInicial();  
      instanteAnterior = instanteAtual; 
    }
  }
  

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
    String id = lerRFID();
    Serial.println("UID: " + id);

    if(id == "17 28 7C 34"){
      usuarioValido = true;
      telaProdutos();
    }else{
      usuarioValido = false;
      usuarioInvalido();
    }

    String texto = lerTextoDoBloco(6);
    Serial.println("Bloco 6: " + texto);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    roda = false;
  }

  if(usuarioValido){
    left.process();
    right.process();
    if(comecaPesagem){
      telaPesagem();
    }
  }
}