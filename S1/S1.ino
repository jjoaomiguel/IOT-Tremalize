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
const String MyTopic = "Joao";
const String OtherTopic = "Daniel";


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
    String ID = "S1_";
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
  String mensagem ="Joao: ";
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
  if(mensagem == "Daniel: Acender"){
    digitalWrite(PINO_LED, HIGH);
  }else if(mensagem == "Daniel: Apagar"){
    digitalWrite(PINO_LED, LOW);
  }else{
    Serial.println(mensagem);
  }
}