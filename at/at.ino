/*
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient client;
PubSubClient mqtt(client);

#define PINO_LED 2

//constantes p/ broker
const String URL = "test.mosquitto.org";
const int PORT = 1883;
const String USR = "";
const String broker_PASS = "";
const String MyTopic = "Amanda";
const String OtherTopic = "Kamila";


const String SSID = "FIESC_IOT_EDU";
const String PASS = "8120gv08";

void setup() {
  Serial.begin(115200);
  Serial.print("Conectado ao Wi-Fi");
  WiFi.begin(SSID,PASS);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.print("\nConectado com sucesso!");
  Serial.print("Conectando ao broker");
  mqtt.setServer(URL.c_str(),PORT);
  while(!mqtt.connected()){
    String ID = "at";
    ID += String(random(0xffff),HEX);
    mqtt.connect(ID.c_str(),USR.c_str(),broker_PASS.c_str());
    Serial.print(".");
    delay(200);
  }
  pinMode(PINO_LED, OUTPUT);
  mqtt.subscribe(MyTopic.c_str());
  mqtt.setCallback(callback);
  Serial.println("\nConecxao com sucesso ao broker!");
}

void loop() {
  String mensagem ="Amanda: ";
  if(Serial.available()>0){
    mensagem += Serial.readStringUntil('\n');
    mqtt.publish(OtherTopic.c_str(),mensagem.c_str());
  }
  mqtt.loop();
  delay(1000);

}

void callback(char* topic, byte* payload, unsigned int length){
  String mensagem = "";
  for(int i = 0; i < length; i++){
    mensagem += (char)payload[i];//acender led
  }
  Serial.print("Recebido: ");
  Serial.println(mensagem);
  if(mensagem == "Kamila: Acender"){
    digitalWrite(PINO_LED, HIGH);
  }else if(mensagem == "Kamila: Apagar"){
    digitalWrite(PINO_LED, LOW);
  }else{
    Serial.println(mensagem);
  }
}*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

WiFiClientSecure client;
PubSubClient mqtt(client);

#define LED_VERMELHO 23
#define LED_VERDE 22


const String URL = "824ad44343204d1b8cc1b3dc4f105a31.s1.eu.hivemq.cloud";
const int PORT = 8883;
const String broker_user = "Placa_4_Amanda";
const String broker_PASS = "Amanda12345678";
const String MyTopic = "projeto/trem/velocidade";


const String SSID = "FIESC_IOT_EDU";
const String PASS = "8120gv08";

void setup() {
  Serial.begin(115200);
  Serial.print("Conectado ao Wi-Fi");
  client.setInsecure();
  WiFi.begin(SSID,PASS);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.print("\nConectado com sucesso!");
  Serial.print("Conectando ao broker");
  mqtt.setServer(URL.c_str(),PORT);
  while(!mqtt.connected()){
    String ID = "Trem";
    ID += String(random(0xffff),HEX);
    mqtt.connect(ID.c_str(),broker_user.c_str(),broker_PASS.c_str());
    Serial.print(".");
    delay(200);
  }
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  mqtt.subscribe(MyTopic.c_str());
  mqtt.setCallback(callback);
  Serial.println("\nConecxao com sucesso ao broker!");
}

void loop() {
  if(Serial.available()>0){
    String mensagem = Serial.readStringUntil('\n');
    mqtt.publish(MyTopic.c_str(),mensagem.c_str());
  }
  mqtt.loop();
  delay(50);

}

void callback(char* topic, byte* payload, unsigned int length){
  String mensagem = "";
  for(int i = 0; i < length; i++){
    mensagem += (char)payload[i];
  }
  Serial.print("Recebido: ");
  Serial.println(mensagem);
  int val = mensagem.toInt();
  if(val0 > 0){
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
  }else if(val < 0){
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
  }else{
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, LOW);
  }
}:
