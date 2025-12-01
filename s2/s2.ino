//s2 - (2x ultrasonico,Led) 

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* SSID = "FIESC_IOT_EDU";   
const char* PASS = "8120gv08";       

const char* BROKER = "824ad44343204d1b8cc1b3dc4f105a31.s1.eu.hivemq.cloud";
const int PORT = 8883;

const char* BROKER_USER = "Placa_2_Kamila";
const char* BROKER_PASS = "Kamila12345678";

#define TRIG1 12
#define ECHO1 13
#define TRIG2 25
#define ECHO2 26
#define PINO_LED 4

// CANAIS DE MENSAGEM
const char* TOPICO_PUBLISH_1 = "Projeto/S2/Distancia1";  // Envia distância do sensor 1
const char* TOPICO_PUBLISH_2 = "Projeto/S2/Distancia2";  // Envia distância do sensor 2
const char* TOPICO_SUBSCRIBE = "S1/iluminacao";          // Recebe comando para acender/apagar LED
const char* TOPICO_ENVIO_S3   = "Projeto/S3/Controle";   // Envia mensagens sobre objeto perto/longe

// CLIENTES WI-FI E MQTT
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);


// FUNÇÃO PARA MEDIR DISTÂNCIA DO SENSOR
long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);// Garante TRIG desligado
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);// Pulso de 10µs para disparar o sensor
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);// Garante TRIG desligado novamente

  // Lê o tempo que o sinal demora para voltar
  long duracao = pulseIn(echoPin, HIGH, 30000); // 30 ms de tempo limite

  if (duracao == 0) return -1; // Se não recebeu nada, retorna erro

  // Converte tempo em distância
  long distancia = (duracao * 0.034) / 2;

  return distancia;
}

// FUNÇÃO QUE É CHAMADA QUANDO UMA MENSAGEM É RECEBIDA DO MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;

  // Transforma o payload recebido em texto
  for (unsigned int i = 0; i < length; i++)
    mensagem += (char)payload[i];

  Serial.print("Recebido em [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(mensagem);

  // Se a mensagem veio do tópico de iluminação
  if (String(topic) == TOPICO_SUBSCRIBE) {

    if (mensagem == "acender")
      digitalWrite(PINO_LED, HIGH);  // Liga LED

    else if (mensagem == "apagar")
      digitalWrite(PINO_LED, LOW);   // Desliga LED
  }
}

// CONECTA NO WI-FI
void conectaWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(SSID, PASS);

  // Espera conectar
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Conectado ao WiFi!");
}

// CONECTA AO BROKER MQTT
void conectaMQTT() {
  mqtt.setServer(BROKER, PORT);   // Configura servidor
  mqtt.setCallback(callback);     // Define função que recebe mensagens
  espClient.setInsecure();        // Evita erro de certificado SSL

  while (!mqtt.connected()) {
    String clientId = "S2_" + String(random(0xffff), HEX); // ID aleatório

    Serial.print("Tentando MQTT...");

    // Tenta conectar com usuário e senha
    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado ao broker!");
      mqtt.subscribe(TOPICO_SUBSCRIBE);  // Inscreve no tópico de iluminação
    } 
    else {
      Serial.print("Falha, rc=");
      Serial.print(mqtt.state());
      Serial.println(" - tentando em 2s");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configura pinos dos sensores
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  // Configura LED
  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, LOW);

  conectaWiFi(); 
  conectaMQTT();  
}

void loop() {
  // Se desconectar, conecta de novo
  if (!mqtt.connected()) conectaMQTT();
  mqtt.loop();

  // Leitura dos dois sensores
  long dist1 = medirDistancia(TRIG1, ECHO1);
  long dist2 = medirDistancia(TRIG2, ECHO2);

  char buf[12];

  // PUBLICA RESULTADO DO SENSOR 1
  if (dist1 >= 0) {
    sprintf(buf, "%ld", dist1);
    mqtt.publish(TOPICO_PUBLISH_1, buf);

    // Mensagem de objeto próximo/longe
    if (dist1 < 10)
      mqtt.publish(TOPICO_ENVIO_S3, "perto");
    else
      mqtt.publish(TOPICO_ENVIO_S3, "longe");
  } 
  else {
    Serial.println("Dist1: sem leitura");
  }

  // PUBLICA RESULTADO DO SENSOR 2
  if (dist2 >= 0) {
    sprintf(buf, "%ld", dist2);
    mqtt.publish(TOPICO_PUBLISH_2, buf);

    // Mensagem extra
    if (dist2 < 10)
      mqtt.publish(TOPICO_ENVIO_S3, "perto");
    else
      mqtt.publish(TOPICO_ENVIO_S3, "longe");
  } 
  else {
    Serial.println("Dist2: sem leitura");
  }

  // Mostra no serial
  Serial.print("Dist1: ");
  Serial.print(dist1);
  Serial.print(" cm  | Dist2: ");
  Serial.println(dist2);

  delay(1000);  // Espera 1 segundo
}