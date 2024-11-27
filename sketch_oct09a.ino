#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <time.h>

// Configurações de rede WiFi
const char* ssid = " ZEDAO";
const char* password = "44444444";

// Firebase configuration
const char* api_key = "AIzaSyDqNbZqAlRJQH0SG83ZxboA94ZCxogdMdM";
const char* db_url = "https://internet-das-coisas-ece5e-default-rtdb.firebaseio.com/";

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pinos e variáveis para medição de distância
const int redLedPin = D7; // Pino do LED
const int trigPin = D5;
const int echoPin = D6;
long duration;
int distance;
const int distanceLimit = 20; // Limite para acender o LED
unsigned long distancePrevMillis = 0;

// Configurações de NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600; // GMT -3 para horário de Brasília
const int daylightOffset_sec = 0;

void setup() {
  pinMode(redLedPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial.begin(115200);

  // Conectar ao WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Inicializar Firebase
  config.api_key = api_key;
  config.database_url = db_url;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Cadastro no Firebase OK");
  } else {
    Serial.println(config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;  // Fornece feedback do token
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Inicializar NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

int measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;  // Converter para cm
  return distance;
}

void logMeasurement(int distanceMeasured) {
  String path = "/Relatorio/medicoes";  // Caminho do Firebase para relatar medições
  
  // Criar o dado a ser enviado
  String value = String(distanceMeasured) + " cm";
  
  // Gerar timestamp
  String timestamp = convertTime();  // Usar a função para formatar o tempo

  // Criar um novo registro no Firebase
  String measurementPath = path + "/medicao_" + String(millis()); // Usando millis() como identificador único

  if (Firebase.RTDB.setString(&fbdo, measurementPath + "/valor", value)) {
    Serial.println("Valor enviado com sucesso: " + value);
  } else {
    Serial.println("Erro ao enviar valor: " + fbdo.errorReason());
  }

  if (Firebase.RTDB.setString(&fbdo, measurementPath + "/tempo", timestamp)) {
    Serial.println("Timestamp enviado com sucesso: " + timestamp);
  } else {
    Serial.println("Erro ao enviar timestamp: " + fbdo.errorReason());
  }
}

// Função para converter o horário atual para formato legível
String convertTime() {
  time_t now = time(nullptr); // Obter a hora atual
  struct tm *ti = localtime(&now);

  // Formatando o tempo como "dd/MM/yyyy HH:mm:ss"
  char buffer[20];
  sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d",
          ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900,
          ti->tm_hour, ti->tm_min, ti->tm_sec);
  
  return String(buffer);
}

void loop() {
  if (millis() - distancePrevMillis > 1000) {  // A cada segundo
    distancePrevMillis = millis();

    // Medir distância
    int distanceMeasured = measureDistance();
    Serial.println("Distância medida: " + String(distanceMeasured) + " cm");

    // Ligar ou desligar o LED com base na distância
    if (distanceMeasured < distanceLimit) {
      digitalWrite(redLedPin, HIGH);  // Ligar LED
      logMeasurement(distanceMeasured);
    } else {
      digitalWrite(redLedPin, LOW);  // Desligar LED
    }

    // Registrar distância e timestamp no Firebase
    
  }
}
