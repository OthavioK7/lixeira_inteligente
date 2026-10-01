#include <WiFi.h>
#include "ThingSpeak.h"

// ========================================
// CONFIGURAÇÕES WI-FI E THINGSPEAK
// ========================================
const char* WIFI_NAME = "Wokwi-GUEST"; // Wi-Fi padrão do Wokwi
const char* WIFI_PASSWORD = "";        // Sem senha

unsigned long myChannelNumber = 3518124; // Ex: 1234567 (sem aspas)
const char* myApiKey = "Z9C08298HEAKP2UI";        // Ex: "ABC123XYZ" (com aspas)

WiFiClient client;

// ========================================
// LIXEIRA INTELIGENTE - ESP32
// ========================================

// Sensor ultrassônico
#define TRIG_PIN 5
#define ECHO_PIN 18

// LEDs
#define LED_VERDE 2
#define LED_AMARELO 4
#define LED_VERMELHO 15

// ========================================
// CALIBRACAO DA LIXEIRA
// ========================================

// Distância do sensor até o lixo quando a lixeira está vazia
#define DISTANCIA_VAZIA 30.0

// Distância do sensor até o lixo quando a lixeira está cheia
#define DISTANCIA_CHEIA 3.0

// Controle de tempo de envio para o ThingSpeak (mínimo de 15 segundos na conta gratuita)
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 15000; 

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  // Conexão Wi-Fi
  Serial.print("Conectando ao WiFi");
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  // Inicializa o ThingSpeak
  ThingSpeak.begin(client);

  Serial.println("Lixeira Inteligente iniciada!");
}

void loop() {

  // ========================================
  // MEDIR DISTANCIA
  // ========================================

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);

  // Sensor sem resposta
  if (duracao == 0) {
    Serial.println("ERRO: Sensor sem resposta!");

    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARELO, LOW);
    digitalWrite(LED_VERMELHO, LOW);

    delay(1000);
    return;
  }

  // ========================================
  // CALCULAR DISTANCIA E PORCENTAGEM
  // ========================================

  float distancia = duracao * 0.0343 / 2;

  float nivel = ((DISTANCIA_VAZIA - distancia) /
                 (DISTANCIA_VAZIA - DISTANCIA_CHEIA)) * 100;

  // Limitar entre 0 e 100
  if (nivel < 0) {
    nivel = 0;
  }

  if (nivel > 100) {
    nivel = 100;
  }

  // ========================================
  // DESLIGAR TODOS OS LEDs
  // ========================================

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  // ========================================
  // DEFINIR NIVEL E LEDS
  // ========================================

  if (nivel <= 50) {
    digitalWrite(LED_VERDE, HIGH);
    Serial.println("STATUS: NORMAL");
  }
  else if (nivel <= 80) {
    digitalWrite(LED_AMARELO, HIGH);
    Serial.println("STATUS: ATENCAO");
  }
  else {
    digitalWrite(LED_VERMELHO, HIGH);
    Serial.println("STATUS: LIXEIRA CHEIA!");
    Serial.println("ALERTA: COLETA NECESSARIA!");
  }

  // ========================================
  // MOSTRAR INFORMACOES
  // ========================================

  Serial.print("Distancia medida: ");
  Serial.print(distancia);
  Serial.println(" cm");

  Serial.print("Preenchimento: ");
  Serial.print(nivel);
  Serial.println("%");

  // ========================================
  // ENVIO DE DADOS PARA O THINGSPEAK
  // ========================================

  // Garante que o envio ocorra a cada 15 segundos (limite da conta gratuita do ThingSpeak)
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ThingSpeak.setField(1, nivel);       // Field 1 do ThingSpeak recebe o nível em %
    ThingSpeak.setField(2, distancia);   // Field 2 recebe a distância em cm

    int response = ThingSpeak.writeFields(myChannelNumber, myApiKey);

    if (response == 200) {
      Serial.println(">> Dados enviados para o ThingSpeak com sucesso! <<");
    } else {
      Serial.print(">> Erro ao enviar dados. Codigo de erro HTTP: ");
      Serial.println(response);
    }

    ultimoEnvio = millis();
  }

  Serial.println("-------------------------");

  delay(1000);
}