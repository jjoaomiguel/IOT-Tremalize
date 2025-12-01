// S1 - Placa 1 (DHT, LDR, Ultrassônico, RGB)
// Este código controla sensores (LDR, DHT11, Ultrassom) e LEDs (simples e RGB)
// e envia/recebe dados via MQTT usando HiveMQ.

// ---- BIBLIOTECAS ----
#include <WiFi.h>               // Usada para conectar o ESP32 ao Wi-Fi
#include <WiFiClientSecure.h>   // Necessária para conexões seguras (SSL)
#include <PubSubClient.h>       // Biblioteca para comunicação MQTT
#include <DHT.h>                // Biblioteca do sensor de temperatura/umidade DHT11

// ---- CONFIGURAÇÃO MQTT ----
// Aqui defino o servidor HiveMQ e os dados de login desta placa
const char* MQTT_SERVER = "824ad44343204d1b8cc1b3dc4f105a31.s1.eu.hivemq.cloud";
const int   MQTT_PORT   = 8883;                 // Porta SSL do HiveMQ
const char* MQTT_USER   = "Placa_1_Joao";       // Usuário da placa
const char* MQTT_PASS   = "Joao12345678";       // Senha da placa

// ---- CONFIGURAÇÃO DO Wi-Fi ----
// A placa precisa de internet para se conectar ao HiveMQ
const char* SSID      = "FIESC_IOT_EDU";
const char* WIFI_PASS = "8120gv08";

// ---- PINAGEM ----
// Aqui defino em quais pinos cada sensor está ligado no ESP32

#define LDR_PIN 34        // Sensor de luminosidade (LDR)
#define LED_PIN 19        // LED comum que acende/apaga

#define DHTPIN 4          // Sensor DHT11 (temp/umidade) no pino 4
#define DHTTYPE DHT11     // Defino o modelo do sensor
DHT dht(DHTPIN, DHTTYPE); // Aqui inicializo o sensor DHT

// Sensor ultrassônico
#define TRIG_PIN 22       // Pino que envia o pulso do ultrassônico
#define ECHO_PIN 23       // Pino que recebe o retorno do pulso

// LED RGB (um pino pra cada cor)
#define LED_R 14          // LED vermelho
#define LED_G 26          // LED verde
#define LED_B 25          // LED azul

// ---- OBJETOS DO MQTT ----
WiFiClientSecure client;     // Cliente com conexão segura (SSL)
PubSubClient mqtt(client);   // Cliente MQTT que usa o cliente SSL

// ---- TÓPICOS MQTT ----
// São os "endereços" onde a placa envia e recebe mensagens
const char* TOPICO_UMIDADE     = "S1/umidade";
const char* TOPICO_TEMPERATURA = "S1/temperatura";
const char* TOPICO_LED         = "S1/iluminacao";
const char* TOPICO_DISTANCIA   = "S1/distancia";
const char* TOPICO_RGB         = "S1/rgb";

// ---- CALLBACK MQTT ----
// Esta função roda automaticamente quando o HiveMQ envia algo para a placa
void callback(char* topic, byte* payload, unsigned int length) {

  // Aqui eu converto o payload (bytes) em String
  String mensagem;
  for (unsigned int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print("Recebido em [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(mensagem);

  // ---- CONTROLE DO LED NORMAL ----
  if (String(topic) == TOPICO_LED) {
    if (mensagem == "acender") digitalWrite(LED_PIN, HIGH);
    else digitalWrite(LED_PIN, LOW);
  }

  // ---- CONTROLE DO LED RGB ----
  if (String(topic) == TOPICO_RGB) {

    if (mensagem == "vermelho") {
      digitalWrite(LED_R, HIGH);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
    }
    else if (mensagem == "verde") {
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, HIGH);
      digitalWrite(LED_B, LOW);
    }
    else if (mensagem == "azul") {
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, HIGH);
    }
    else if (mensagem == "off") {
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
    }
  }
}

// ---- FUNÇÃO PARA CONECTAR AO MQTT ----
// Fica tentando conectar até conseguir
void conectaMQTT() {
  while (!mqtt.connected()) {

    Serial.print("Conectando ao HiveMQ... ");

    // Gero um ID aleatório pra evitar conflito entre dispositivos
    String clientId = "ESP32-S1-";
    clientId += String(random(0xffff), HEX);

    // Tenta conectar com usuário e senha configurados
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("Conectado!");

      // Assim que conecta, começa a "escutar" os tópicos
      mqtt.subscribe(TOPICO_LED);
      mqtt.subscribe(TOPICO_RGB);

    } else {
      Serial.print("Falhou, rc=");
      Serial.print(mqtt.state());
      Serial.println(" - tentando novamente em 1s");
      delay(1000);
    }
  }
}

// ---- FUNÇÃO DO ULTRASSÔNICO (RETORNA DISTÂNCIA EM CM) ----
long distanciaCM() {

  // Mando um pulso curto para iniciar a medição
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Aqui espero o pulso voltar e meço quanto tempo levou
  long duracao = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout 30ms

  if (duracao == 0) return -1; // se nada voltou, deu erro

  // Fórmula para converter tempo → distância em centímetros
  return (duracao * 0.034) / 2;
}

// ---- SETUP ----
// Roda apenas uma vez quando a placa liga
void setup() {

  Serial.begin(115200); // Abro comunicação no monitor serial

  // Configuro os pinos como entrada ou saída
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();  // Inicializo o DHT11

  // ---- Conectar ao Wi-Fi ----
  Serial.print("Conectando ao Wi-Fi ");
  WiFi.begin(SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Conectado!");

  client.setInsecure();              // Pula verificação SSL (mais simples)
  mqtt.setServer(MQTT_SERVER, MQTT_PORT); // Configuro o servidor MQTT
  mqtt.setCallback(callback);        // Defino a função que recebe mensagens
  conectaMQTT();                     // Tenta conectar ao HiveMQ
}

// ---- LOOP PRINCIPAL ----
// Fica rodando infinitamente
void loop() {

  // Se perder a conexão, reconecta automaticamente
  if (!mqtt.connected()) conectaMQTT();
  mqtt.loop();

  // ---- LEITURA DO LDR ----
  int leituraLDR = analogRead(LDR_PIN);

  // Regra simples: se estiver muito claro → acende LED
  if (leituraLDR > 3500){
    digitalWrite(LED_PIN, HIGH);
    mqtt.publish(TOPICO_LED, "acender");
  } else {
    digitalWrite(LED_PIN, LOW);
    mqtt.publish(TOPICO_LED, "apagar");
  }

  // ---- LEITURA DO DHT ----
  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();

  char buf[16];   // Buffer para enviar números como texto

  if (!isnan(temperatura)) {
    dtostrf(temperatura, 6, 2, buf);
    mqtt.publish(TOPICO_TEMPERATURA, buf);
    Serial.print("Temp: ");
    Serial.println(buf);
  }

  if (!isnan(umidade)) {
    dtostrf(umidade, 6, 2, buf);
    mqtt.publish(TOPICO_UMIDADE, buf);
    Serial.print("Umidade: ");
    Serial.println(buf);
  }

  // ---- LEITURA DO ULTRASSÔNICO ----
  long distancia = distanciaCM();
  if (distancia >= 0) {
    char buf2[12];
    sprintf(buf2, "%ld", distancia);
    mqtt.publish(TOPICO_DISTANCIA, buf2);
    Serial.print("Distância: ");
    Serial.println(distancia);
  } else {
    Serial.println("Ultrassônico: sem leitura (timeout)");
  }

  // Apenas para acompanhar leituras do LDR
  Serial.print("LDR: ");
  Serial.println(leituraLDR);
  Serial.println("----------------------");

  delay(1500); // Intervalo entre leituras para não inundar o servidor
}
