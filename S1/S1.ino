// S1 - Placa 1 (DHT, LDR, Ultrassônico, RGB)
// Versão corrigida e comentada para iniciantes

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>

// --- MQTT ---
// Aqui é onde a gente coloca o servidor da HiveMQ
const char* MQTT_SERVER = "824ad44343204d1b8cc1b3dc4f105a31.s1.eu.hivemq.cloud";
const int   MQTT_PORT   = 8883;
const char* MQTT_USER   = "Placa_1_Joao";        // Usuário da placa
const char* MQTT_PASS   = "Joao12345678";        // Senha da placa

// --- WiFi ---
// Internet que a placa vai usar para se conectar
const char* SSID      = "FIESC_IOT_EDU";
const char* WIFI_PASS = "8120gv08";

// --- PINAGEM ---
// Aqui eu falo onde cada sensor e LED está ligado no ESP32
#define LDR_PIN 34        // O LDR (sensor de luz) vai no pino 34
#define LED_PIN 19        // Um LED normal vai no pino 19

#define DHTPIN 4          // O sensor de temperatura DHT11 tá no pino 4
#define DHTTYPE DHT11     
DHT dht(DHTPIN, DHTTYPE); // Aqui eu "instalo" o DHT para funcionar

// Ultrassônico
#define TRIG_PIN 22       // O pino que manda o sinal do ultrassônico
#define ECHO_PIN 23       // O pino que recebe o sinal de volta

// LED RGB (três cores)
#define LED_R 14          // LED vermelho no pino 14
#define LED_G 26          // LED verde no pino 26
#define LED_B 25          // LED azul no pino 25

WiFiClientSecure client;  // Conexão segura para MQTT
PubSubClient mqtt(client); // Cliente MQTT

// --- TOPICS ---
// Esses são os “caminhos” onde os dados são enviados e recebidos
const char* TOPICO_UMIDADE     = "S1/umidade";
const char* TOPICO_TEMPERATURA = "S1/temperatura";
const char* TOPICO_LED         = "S1/iluminacao";
const char* TOPICO_DISTANCIA   = "S1/distancia";
const char* TOPICO_RGB         = "S1/rgb";

// --- CALLBACK ---
// Toda vez que alguém manda um comando MQTT, essa função recebe
void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem;
  for (unsigned int i = 0; i < length; i++) mensagem += (char)payload[i];

  Serial.print("Recebido em ["); Serial.print(topic); Serial.print("]: ");
  Serial.println(mensagem);

  // Controle do LED normal
  if (String(topic) == TOPICO_LED) {
    if (mensagem == "acender") digitalWrite(LED_PIN, HIGH);  // liga
    else digitalWrite(LED_PIN, LOW);                         // desliga
  }

  // Controle do LED RGB
  if (String(topic) == TOPICO_RGB) {
    if (mensagem == "vermelho") {
      digitalWrite(LED_R, HIGH); digitalWrite(LED_G, LOW); digitalWrite(LED_B, LOW);
    }
    else if (mensagem == "verde") {
      digitalWrite(LED_R, LOW); digitalWrite(LED_G, HIGH); digitalWrite(LED_B, LOW);
    }
    else if (mensagem == "azul") {
      digitalWrite(LED_R, LOW); digitalWrite(LED_G, LOW); digitalWrite(LED_B, HIGH);
    }
    else if (mensagem == "off") {
      digitalWrite(LED_R, LOW); digitalWrite(LED_G, LOW); digitalWrite(LED_B, LOW);
    }
  }
}

// --- Conectar ao servidor MQTT ---
void conectaMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando ao HiveMQ... ");
    String clientId = "ESP32-S1-";
    clientId += String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("Conectado!");
      mqtt.subscribe(TOPICO_LED);  // ouvindo os comandos do LED
      mqtt.subscribe(TOPICO_RGB);  // ouvindo os comandos do RGB
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(mqtt.state());
      Serial.println(" - tentando novamente em 1s");
      delay(1000);
    }
  }
}

// --- Função para medir distância com ultrassônico ---
long distanciaCM(){
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);  // manda pulso
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duracao == 0) return -1; // se deu erro
  return (duracao * 0.034) / 2; // fórmula pra transformar em cm
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  // deixando tudo preparado para funcionar
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();

  // Conectando no Wi-Fi
  Serial.print("Conectando ao Wi-Fi ");
  WiFi.begin(SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Conectado!");

  client.setInsecure(); // pula verificação de segurança (só pra teste)
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
  conectaMQTT();
}

// --- LOOP PRINCIPAL ---
void loop() {
  if (!mqtt.connected()) conectaMQTT();
  mqtt.loop();

  // ---- LDR ----
  int leituraLDR = analogRead(LDR_PIN);  // lê a luz do ambiente
  if (leituraLDR > 3500){
    digitalWrite(LED_PIN, HIGH);
    mqtt.publish(TOPICO_LED, "acender");
  } else {
    digitalWrite(LED_PIN, LOW);
    mqtt.publish(TOPICO_LED, "apagar");
  }

  // ---- DHT ----
  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();

  char buf[16];
  if (!isnan(temperatura)) {
    dtostrf(temperatura, 6, 2, buf);
    mqtt.publish(TOPICO_TEMPERATURA, buf);
    Serial.print("Temp: "); Serial.println(buf);
  } else {
    Serial.println("Erro leitura temperatura");
  }

  if (!isnan(umidade)) {
    dtostrf(umidade, 6, 2, buf);
    mqtt.publish(TOPICO_UMIDADE, buf);
    Serial.print("Umidade: "); Serial.println(buf);
  } else {
    Serial.println("Erro leitura umidade");
  }

  // ---- ULTRASSÔNICO ----
  long distancia = distanciaCM();
  if (distancia >= 0) {
    char buf2[12];
    sprintf(buf2, "%ld", distancia);
    mqtt.publish(TOPICO_DISTANCIA, buf2);
    Serial.print("Distância: "); Serial.println(distancia);
  } else {
    Serial.println("Ultrassônico: sem leitura (timeout)");
  }

  Serial.print("LDR: "); Serial.println(leituraLDR);
  Serial.println("----------------------");

  delay(1500); // só pra não mandar mil dados por segundo
}