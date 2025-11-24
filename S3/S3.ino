#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// ===== PINOS =====
#define PINO_LED 2
#define TRIG 26
#define ECHO 25
#define PINO_SERVO_3 19
#define PINO_SERVO_4 18

// ===== WIFI E MQTT =====
WiFiClientSecure client;
PubSubClient mqtt(client);
Servo servo3, servo4;

const char* SSID = "FIESC_IOT_EDU";
const char* PASS = "8120gv08";

const char* BROKER_URL  = "824ad44343204d1b8cc1b3dc4f105a31.s1.eu.hivemq.cloud";
const int   BROKER_PORT = 8883;
const char* BROKER_USER = "Placa_3_Daniel";
const char* BROKER_PASS = "Daniel12345678";

// ===== TOPICOS =====
const char* TOPICO_SUBSCRIBE_S1      = "S1/iluminacao";
const char* TOPIC_SUBSCRIBE_S2_1     = "Projeto/S2/Distancia1";
const char* TOPIC_SUBSCRIBE_S2_2     = "Projeto/S2/Distancia2";
const char* TOPIC_RECEIVE_S2_CONTROL = "Projeto/S3/Controle";

const char* TOPIC_PUBLISH_OBJETO     = "Projeto/S3/Ultrassom3";


// ===== Função Ultrassom =====
long medirDistancia() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(3);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(12);
  digitalWrite(TRIG, LOW);

  long duracao = pulseIn(ECHO, HIGH, 30000);
  if (duracao == 0) return -1;

  return duracao * 0.034 / 2;
}


// ===== Callback MQTT =====
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;
  for (unsigned int i = 0; i < length; i++) mensagem += (char)payload[i];

  Serial.print("Recebido em "); Serial.println(topic);
  Serial.print("Mensagem: "); Serial.println(mensagem);

  String t = String(topic);

  // --- Controle de LED vindo da S1 ---
  if (t == TOPICO_SUBSCRIBE_S1) {
    digitalWrite(PINO_LED, mensagem == "acender" ? HIGH : LOW);
    return;
  }

  // --- Controle do Servo 3 (distância 1) ---
  if (t == TOPIC_SUBSCRIBE_S2_1) {
    if (mensagem == "perto") servo3.write(90);
    else if (mensagem == "longe") servo3.write(45);
    else {
      int v = mensagem.toInt();
      int angle = map(constrain(v, 0, 100), 0, 100, 90, 45);
      servo3.write(angle);
    }
    return;
  }

  // --- Controle do Servo 4 (distância 2) ---
  if (t == TOPIC_SUBSCRIBE_S2_2) {
    if (mensagem == "perto") servo4.write(90);
    else if (mensagem == "longe") servo4.write(45);
    else {
      int v = mensagem.toInt();
      int angle = map(constrain(v, 0, 100), 0, 100, 90, 45);
      servo4.write(angle);
    }
    return;
  }

  // --- Controle direto S2 -> S3 ---
  if (t == TOPIC_RECEIVE_S2_CONTROL) {
    Serial.println("Controle recebido: " + mensagem);
  }
}


// ===== WIFI =====
void conectarWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi conectado!");
}


// ===== MQTT =====
void conectarMQTT() {
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  client.setInsecure();
  mqtt.setCallback(callback);

  while (!mqtt.connected()) {
    Serial.print("Conectando ao broker...");

    String clientId = "S3_" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), BROKER_USER, BROKER_PASS)) {
      Serial.println("Conectado!");

      mqtt.subscribe(TOPICO_SUBSCRIBE_S1);
      mqtt.subscribe(TOPIC_SUBSCRIBE_S2_1);
      mqtt.subscribe(TOPIC_SUBSCRIBE_S2_2);
      mqtt.subscribe(TOPIC_RECEIVE_S2_CONTROL);
    } else {
      Serial.print("Falha. RC=");
      Serial.println(mqtt.state());
      delay(1500);
    }
  }
}


// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(PINO_LED, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  servo3.attach(PINO_SERVO_3);
  servo4.attach(PINO_SERVO_4);

  servo3.write(60);  // posição inicial segura
  servo4.write(60);

  conectarWiFi();
  conectarMQTT();
}


// ===== LOOP =====
void loop() {
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop();

  // --- Ultrassom ---
  long dist = medirDistancia();
  if (dist > 0) {
    char buf[8];
    sprintf(buf, "%ld", dist);
    mqtt.publish(TOPIC_PUBLISH_OBJETO, buf);

    if (dist < 10) mqtt.publish(TOPIC_PUBLISH_OBJETO, "perto");
    else mqtt.publish(TOPIC_PUBLISH_OBJETO, "longe");
  }

  delay(150);
}
