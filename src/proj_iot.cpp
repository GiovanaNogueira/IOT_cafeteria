#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h> 
#include <WiFiClientSecure.h> 
#include "certificados.h" 
#include <MQTT.h> 
#include <GFButton.h> 
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include "tela.h"
#include <HX711.h>
#include <Ultrasonic.h>
//#include <ESP32Encoder.h>

#define PINO_BOTAO_RIGHT 5
#define PINO_BOTAO_LEFT 4
// #define CONTATO 8
#define VALV_PINO 21
#define SOL_PIN1 39
#define SOL_PIN2 40
#define PINO_TRIGGER1 42
#define PINO_ECHO1 41
#define PINO_TRIGGER2 1 
#define PINO_ECHO2 2

// unsigned long anterior = 0;
bool esperandoJanelaAbrir = false;
bool janelaFechada = true;
bool janelaFechadaAnterior = false;

Servo servo_amendoim;
int pos_amendoim = 0;

Servo servo_mm;
int pos_mm = 0;

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);

MFRC522 rfid(46, 17);
MFRC522::MIFARE_Key chaveA = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

Ultrasonic ultrasonicAmendoim(PINO_TRIGGER1, PINO_ECHO1);
// Ultrasonic ultrasonicMM(PINO_TRIGGER2, PINO_ECHO2);

GFButton left(PINO_BOTAO_LEFT); //azul
// GFButton right(PINO_BOTAO_RIGHT); // amarelo
//ESP32Encoder encoder;

JsonDocument produto;
JsonDocument user; 
JsonDocument produtos;

HX711 balanca;

bool usuarioValido = false;
bool fim = false;
bool comecaPesagem = false;

bool modoCadastroRFID = false;

bool travar = true;

unsigned long instanteAnterior = 0;

void verificaUser(String rfid){
  JsonDocument msg;
  msg["rfid"] = rfid;
  String textoJson;
  serializeJson(msg, textoJson);
  mqtt.publish("verifica_usuario_cafe", textoJson);
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
      mqtt.connect("cafeteria_iot22", "aula", "zowmad-tavQez"); 
      Serial.print(".m"); 
      delay(1000); 
    } 
    Serial.println(" conectado!"); 
    
    mqtt.subscribe("cafeteria_iot");  
    mqtt.subscribe("verifica_usuario_cafe");  
    mqtt.subscribe("retorna_usuario_cafe");  
    mqtt.subscribe("retorna_produtos_cafe");
  } 
}

void pegaProdutos(){
  mqtt.publish("pega_produtos_cafe", "");
}

void enviarNovoRFID(String id) {
  JsonDocument msg;
  msg["rfid"] = id;
  String txt;
  serializeJson(msg, txt);
  mqtt.publish("novo_rfid", txt);
}

float lerDistanciaMedia(Ultrasonic &sensor, int nAmostras = 5) {
  long soma = 0;

  for (int i = 0; i < nAmostras; i++) {
    unsigned int d = sensor.read();
    soma += d;
    delay(30);
  }

  float media = soma / (float)nAmostras;
  Serial.print("Distância média estoque: ");
  Serial.print(media);
  Serial.println(" cm");

  return media;
}

bool estoqueBaixoSensor(Ultrasonic &sensor, float limiarCm, const char *nome) {
  float media = lerDistanciaMedia(sensor);

  if (media > limiarCm) {
    Serial.print("Estoque baixo de ");
    Serial.println(nome);
    return true;
  }

  Serial.print("Estoque OK de ");
  Serial.println(nome);
  return false;
}

// produto = 1 -> Amendoim
// produto = 2 -> M&M
// produto = 0 -> verificar os dois
bool verificaEstoque(int produtoId) {
  bool baixo = false;
  bool baixoAmendoim = false;
  bool baixoMM = false;

  float limite = 8.0;

  if (produtoId == 1) {
    baixo = estoqueBaixoSensor(ultrasonicAmendoim, limite, "Amendoim");
  } 
  else if (produtoId == 2) {
    // baixo = estoqueBaixoSensor(ultrasonicMM, 15.0, "M&M");
    baixo = true;
  } 
  else {
    // verifica os dois
    baixoAmendoim = estoqueBaixoSensor(ultrasonicAmendoim, limite, "Amendoim");
    // baixoMM = estoqueBaixoSensor(ultrasonicMM, 15.0, "M&M");
    baixoMM = false;
    // baixo = baixoAmendoim || baixoMM;
  }

  if (baixo && produtoId != 0) {
    telaSemEstoque();
    String produtoStr = "{\"produto\":" + String(produtoId) + "}";
    mqtt.publish("estoque_baixo_cafe", produtoStr);
    fim = true;
    instanteAnterior = millis();
  }

  if (produtoId == 0) {
    if (baixoAmendoim) {
      mqtt.publish("estoque_baixo_cafe", "{\"produto\":1}");
      fim = true;
      instanteAnterior = millis();
    }
    if (baixoMM) {
      mqtt.publish("estoque_baixo_cafe", "{\"produto\":2}");
      fim = true;
      instanteAnterior = millis();
    }
  }
  return baixo;
}

void destravarJanela() { 
  digitalWrite(SOL_PIN1, LOW); 
  digitalWrite(SOL_PIN2, HIGH); 

  esperandoJanelaAbrir = true;
}

void travarJanela() { 
  delay(50);
  digitalWrite(SOL_PIN1, HIGH); 
  digitalWrite(SOL_PIN2, LOW); 

  esperandoJanelaAbrir = false;
}

void recebeuMensagem(String topico, String conteudo) { 
  Serial.println(topico + ": " + conteudo);
  if (topico == "retorna_usuario_cafe"){
    deserializeJson(user, conteudo);
    int id = user["id"];
    usuarioValido = id != 0; 
    // fim = !usuarioValido;

    if (usuarioValido){
      float balance = user["balance"].as<float>();
      if(balance < 0){
        telaSaldoNegativo(balance);
        usuarioValido = false;
        fim = true;
        instanteAnterior = millis();
        return;
      }
      telaProdutos(user["name"], user["balance"]);
    }
    else usuarioInvalido();
  }
  else if (topico == "cafeteria_iot"){
    destravarJanela();
    usuarioValido = false;
  }
  else if(topico == "retorna_produtos_cafe"){
    deserializeJson(produtos, conteudo);
    Serial.println("Produtos recebidos:");
    serializeJson(produtos, Serial); 
    // tratar produtos
  }
}

void fecharSnack (GFButton& botaoDoEvento) { 
  Serial.println("Parou!!!!");
  if (usuarioValido){
    for(; pos_amendoim > 8; pos_amendoim--){
      servo_amendoim.write(pos_amendoim);
      delay(5);
    }
    delay(50);
    fim = true;
    instanteAnterior = millis();
  }
}

void abrirSnack (GFButton& botaoDoEvento) { 
  Serial.println("Liberando...");
  if (usuarioValido){
    for(; pos_amendoim <= 45; pos_amendoim++){
      Serial.println("Posição amendoim: " + String(pos_amendoim));
      servo_amendoim.write(pos_amendoim);
      delay(5);
    }
    comecaPesagem = true;
    botaoDoEvento.setReleaseHandler(fecharSnack);
  }

}



// void transicao (GFButton& botaoDoEvento){
//   botaoDoEvento.setReleaseHandler(fecharSnack);
// }

void do_nothing(GFButton& botaoDoEvento){
  return;
}

void selecionaProduto (GFButton& botaoDoEvento) {
  left.setPressHandler(do_nothing);
  // right.setPressHandler(do_nothing);

  bool isLeft = (&botaoDoEvento == &left);
  int  id_produto_int = isLeft ? 1 : 2;
  String id_produto = String(id_produto_int);

  if (verificaEstoque(id_produto_int)) {
    Serial.println("Tentou selecionar produto sem estoque.");
    // volta os botões pro estado normal de seleção
    left.setPressHandler(selecionaProduto);
    // right.setPressHandler(selecionaProduto);
    return;  // não segue pro abrirSnack
  }
  
  JsonArray arr = produtos.as<JsonArray>();

  JsonObject escolhido;

  for (JsonObject p : arr) {
    if (p["id"].as<String>() == id_produto) {
      escolhido = p;
      break;
    }
  }

  produto.clear();
  if (!escolhido.isNull()) {
    produto.set(escolhido);
  } else {
    Serial.println("Produto não encontrado!");
    return;
  }

  String nome_produto = produto["nome"];

  botaoDoEvento.setPressHandler(abrirSnack);

  String preco = produto["preco"];
  Serial.println("Produto selecionado: " + nome_produto + " - R$" + preco);
  telaSegure();
}

void finalizarCompra(float pesoMedido) {
  if (pesoMedido < 0) pesoMedido = 0;
  Serial.println("Finalizando compra...");
  Serial.println("Peso medido: " + String(pesoMedido) + " g");
  Serial.println("preco unitario: " + String(produto["preco_100g"].as<float>()) + " R$/100g");
  produto["total"] = produto["preco_100g"].as<float>() * (pesoMedido / 100.0);
  produto["peso"] = pesoMedido;
  float total = produto["total"];
  float saldoUsuario = user["balance"];

  JsonDocument msg;
  msg["id_user"] = user["id"];
  msg["id_produto"] = produto["id"];
  msg["peso"] = pesoMedido;
  msg["total"] = total;
  msg["saldoAtual"] = saldoUsuario - total;

  telaSaldoFinal(total, saldoUsuario);
  String textoJson;
  serializeJson(msg, textoJson);
  mqtt.publish("cafeteria_iot", textoJson);
  produto.clear();
  usuarioValido = false;

  travar = true;
  fim = true;
  instanteAnterior = millis();

  left.setPressHandler(selecionaProduto);
  left.setReleaseHandler(do_nothing);
  // right.setPressHandler(selecionaProduto);
  // right.setReleaseHandler(do_nothing); 
}

/*
void navegacao(long pos){
  if (pos <= 50){
    atualiza_tela(pos)
  }
}
*/


void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("Projeto Final – Controle da Tranca");

  reconectarWiFi(); 

  conexaoSegura.setCACert(certificado1); 

  mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura); 
  mqtt.onMessage(recebeuMensagem); 
  mqtt.setKeepAlive(10); 
  mqtt.setTimeout(10000);
  mqtt.setWill("tópico da desconexão", "conteúdo"); 
  reconectarMQTT(); 

  pegaProdutos();
  Serial.println("Setup completo.");
  /*
  ESP32Encoder::useInternalWeakPullResistors = UP;  
  encoder.attachFullQuad(ENCODER_A, ENCODER_B);
  encoder.clearCount();
  */

  telaSetup();



  SPI.begin();
  rfid.PCD_Init();

  balanca.begin(6, 7);
  balanca.set_scale(478);
  balanca.tare(5);


  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	servo_amendoim.setPeriodHertz(50);    
	servo_amendoim.attach(VALV_PINO, 500, 2400);



  delay(100);
  servo_amendoim.write(9);



  left.setPressHandler(selecionaProduto);
  // right.setPressHandler(selecionaProduto);



  pinMode(SOL_PIN1, OUTPUT);
  pinMode(SOL_PIN2, OUTPUT);


  // pinMode(CONTATO, INPUT_PULLUP);
  // janelaFechada = digitalRead(CONTATO) == HIGH;
  janelaFechadaAnterior = janelaFechada;



  verificaEstoque(0);
  
  telaInicial();

  Serial.println("Setup completo 123");
}

void loop() {
  // Serial.println("Looping...");
  reconectarWiFi(); 
  reconectarMQTT();
  mqtt.loop();

  // Serial.println("Fim: " + String(fim));

  // Serial.println("Looping 2");
  if(fim){
    // delay(5000);
    unsigned long instanteAtual = millis(); 
    if (instanteAtual > instanteAnterior + 5000) { 
      Serial.println("+5 segundo"); 
      instanteAnterior = instanteAtual; 
      travar = true;
      fim = false;
      usuarioValido = false;
      telaInicial();
    }
  }

  // Serial.println("Looping 3");
  //remover depois (apenas debug)
  lerDistanciaMedia(ultrasonicAmendoim);

  // Serial.println("Looping 4");
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){ 
    String id = lerRFID(); 
    Serial.println("UID: " + id); 

    // // 1) Se AINDA não estamos em modo cadastro
    // if (!modoCadastroRFID) {
    //   // Esse é o cartão ADMIN
    //   if (id == "17 28 7C 40") {
    //     Serial.println("Cartão administrador lido. Entre com o cartão a ser cadastrado...");
    //     modoCadastroRFID = true;
    //     telaCadastraRFID();
    //   } else {
    //     // Cartão normal: segue fluxo de usuário
    //     String texto = lerTextoDoBloco(6); 
    //     Serial.println("Bloco 6: " + texto); 
    //     verificaUser(id);
    //   }
    // } 
    // // 2) Já estamos em modo cadastro → esse cartão será cadastrado
    // else {
    //   Serial.println("Registrando novo RFID: " + id);
    //   enviarNovoRFID(id);      // manda pro MQTT → Node-RED → formulário
    //   modoCadastroRFID = false;
    //   telaCadastroRealizado();
    // }

    rfid.PICC_HaltA(); 
    rfid.PCD_StopCrypto1(); 
  }

  // Serial.println("Looping 5");

  if (usuarioValido){
    left.process();
    // right.process();

    // Serial.println("Looping 6");
    if(comecaPesagem){
      float peso = balanca.get_units(5);
      Serial.println("peso sendo medido: " + String(peso));
      if (peso < 0) peso = 0;
      
      float preco100g = produto["preco_100g"];
      float preco = preco100g * (peso / 100.0);

      // String textoJson; 
      // Serial.println("Produto em pesagem:");
      // serializeJson(produto, Serial); 

      // Serial.println("preco: " + String(preco));

      int termino = telaPesagem(peso, preco);

      // Serial.println("termino: " + String(termino));
      // Serial.println("fim: " + String(fim));

      if (fim || termino == 1) {
          finalizarCompra(peso);
          // fim = false;
          comecaPesagem = false;
      }
    }
    
    
  }
  // Serial.println("Looping 7");

  // janelaFechada = digitalRead(CONTATO) == LOW;
  travar = esperandoJanelaAbrir && janelaFechada && !janelaFechadaAnterior; // travar janela se ela foi destravada (esperandoJanelaAbrir) e ela fechou depois de abrir (janelaFechada && !janelaFechadaAnterior)
    
  if (travar) {
    delay(200);
    travarJanela();
    travar = false;
    telaInicial();
  }
  // Serial.println("Looping 8");
  
  janelaFechadaAnterior = janelaFechada;

  // Serial.println("Looping 9");
  
}