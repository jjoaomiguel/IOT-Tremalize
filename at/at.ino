
//variáveis
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

WiFiClientSecure client;
PubSubClient mqtt(client);

#define LED_VERMELHO 19
#define LED_VERDE 18


const String URL = "824ad44343204d1b8cc1b3dc4f105a31.s1.eu.hivemq.cloud";
const int PORT = 8883;
const String broker_user = "Placa_4_Amanda";
const String broker_PASS = "Amanda12345678";
const String MyTopic = "projeto/trem/velocidade";


const String SSID = "FIESC_IOT_EDU";
const String PASS = "8120gv08";

//conectar ao wifi
void setup() {
  Serial.begin(115200);
  Serial.print("Conectado ao Wi-Fi");
  client.setInsecure();
  WiFi.begin(SSID,PASS);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  //conectar ao servidor
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
  //saida
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  //topico do trem
  mqtt.subscribe(MyTopic.c_str());
  mqtt.setCallback(callback);
  Serial.println("\nConecxao com sucesso ao broker!");
}
//tópico do trem "le a variável identifica sinais"
void loop() {
  if(Serial.available()>0){
    String mensagem = Serial.readStringUntil('\n');
    mqtt.publish(MyTopic.c_str(),mensagem.c_str());
  }
  mqtt.loop();
  delay(50);

}
//função 

void callback(char* topic, byte* payload, unsigned int length){
  String mensagem = "";
  for(int i = 0; i < length; i++){
    mensagem += (char)payload[i];
  }
  Serial.print("Recebido: ");
  Serial.println(mensagem);
  int val = mensagem.toInt();
  //se for maior que zero acende
  if(val > 0){
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
    //se for menor que zero acende vermelho
  }else if(val < 0){
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
    //se for igual a zero apaga
  }else{
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, LOW);
  }
}:


