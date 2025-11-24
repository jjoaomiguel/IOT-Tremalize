
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

#define PINO_LED 2
#define TRIG 26
#define ECHO 25
#define PINO_SERVO_3 19
#define PINO_SERVO_4 18
#define PINO_PRESENCA 14

WiFiClientSecure client;
PubSubClient mqtt(client);
Servo servo3;
Servo servo4;

const char* SSID = "FIESC_IOT_EDU";
const char* PASS = "8120gv08";

const char* BROKER_URL  = "824ad44343204d1b8cc1b3dc4f105a31.s1.eu.hivemq.cloud";
const int   BROKER_PORT = 8883;
const char* BROKER_USER = "Placa_3_Daniel";
const char* BROKER_PASS = "Daniel12345678";

const char* TOPIC_PUBLISH_PRESENCA   = "Projeto/S3/Presenca3";
const char* TOPIC_PUBLISH_OBJETO     = "Projeto/S3/Ultrassom3";
const char* TOPICO_SUBSCRIBE_S1      = "S1/iluminacao";
const char* TOPIC_SUBSCRIBE_S2_1     = "Projeto/S2/Distancia1";
const char* TOPIC_SUBSCRIBE_S2_2     = "Projeto/S2/Distancia2";
const char* TOPIC_RECEIVE_S2_CONTROL = "Projeto/S3/Controle";

unsigned long lastPublish = 0;
int publishInterval = 3000;

long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracao = pulseIn(echoPin, HIGH, 30000);
  if (duracao == 0) return -1;
  long distancia = (duracao * 0.034) / 2;
  return distancia;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;
  for (unsigned int i = 0; i < length; i++) mensagem += (char)payload[i];

  Serial.print("Recebido em ["); Serial.print(topic); Serial.print("]: ");
  Serial.println(mensagem);

  String t = String(topic);

  // Comandos de iluminação vindos da S1
  if (t == TOPICO_SUBSCRIBE_S1) {
    if (mensagem == "acender") digitalWrite(PINO_LED, HIGH);
    else digitalWrite(PINO_LED, LOW);
    return;
  }

  // Mensagens de distância vindas da S2 -> controlar servos
  if (t == TOPIC_SUBSCRIBE_S2_1) {
    if (mensagem == "objeto_proximo") servo3.write(90);
    else if (mensagem == "objeto_longe") servo3.write(45);
    else {
      long v = mensagem.toInt();
      int angle = map(constrain(v, 0, 100), 0, 100, 90, 45);
      servo3.write(angle);
    }
    return;
  }

  if (t == TOPIC_SUBSCRIBE_S2_2) {
    if (mensagem == "objeto_proximo") servo4.write(90);
    else if (mensagem == "objeto_longe") servo4.write(45);
    else {
      long v = mensagem.toInt();
      int angle = map(constrain(v, 0, 100), 0, 100, 90, 45);
      servo4.write(angle);
    }
    return;
  }

  // Comandos diretos enviados para S3
  if (t == TOPIC_RECEIVE_S2_CONTROL) {
    Serial.println("Comando Controle S2->S3: " + mensagem);
  }
}

void conectarWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi conectado!");
}

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
      Serial.println("Inscrito em topicos S1 e S2");
    } else {
      Serial.print("Falha. rc=");
      Serial.println(mqtt.state());
      delay(1500);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_PRESENCA, INPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  servo3.attach(PINO_SERVO_3);
  servo4.attach(PINO_SERVO_4);
  servo3.write(0);
  servo4.write(0);

  conectarWiFi();
  conectarMQTT();
}

void loop() {
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop();

  long distancia = medirDistancia(TRIG, ECHO);
  if (distancia >= 0) {
    char buf[12];
    sprintf(buf, "%ld", distancia);
    mqtt.publish(TOPIC_PUBLISH_OBJETO, buf);
    if (distancia < 10) mqtt.publish(TOPIC_PUBLISH_OBJETO, "objeto_proximo");
    else mqtt.publish(TOPIC_PUBLISH_OBJETO, "objeto_longe");
  } else {
    Serial.println("Ultrassom S3: sem leitura");
  }

  unsigned long agora = millis();
  if (agora - lastPublish >= (unsigned long)publishInterval) {
    lastPublish = agora;
    int presenca = digitalRead(PINO_PRESENCA);
    char buf2[4];
    sprintf(buf2, "%d", presenca);
    mqtt.publish(TOPIC_PUBLISH_PRESENCA, buf2);
    Serial.print("Presença publicada: ");
    Serial.println(presenca);
  }

  delay(20);
}
