#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include "DHT.h"
#include "DHTesp.h"
#include <Servo.h>

Servo servoMotor;

//Maquina 
String trama; // String para acumular la trama
int estado = 1;
bool standbyMode = true; // Estado de stand-by inicial
char c;
Servo servoHelice;
int angle=1;
int n_servo=0;
DHTesp dhtSensor;

//semaforos
SemaphoreHandle_t semaforo_sensores;
SemaphoreHandle_t semaforo_actuadores;
// Cola
QueueHandle_t cola;



void setup() {

  Serial.begin(9600);  
  dhtSensor.setup(7,DHTesp::DHT22);
  servoMotor.attach(9);
  pinMode(10 , OUTPUT);
  pinMode(11 , OUTPUT);
  semaforo_actuadores = xSemaphoreCreateBinary();
  semaforo_sensores = xSemaphoreCreateBinary();
  //Creamos las tareas
  //Lectura
  xTaskCreate(ReadSerial, "ReadSerial", 100, NULL, 3, NULL);
  // Movimiento
  xTaskCreate(MoveServo, "Servo move", 100, NULL, 2, NULL); 
  // Lectura
  xTaskCreate(ReadSensors, "Sensors info", 100, NULL, 1, NULL);
  // Enviamos
  xTaskCreate(SendSerial, "Serial", 100, NULL, 0, NULL);

  //cola = xQueueCreate(5, sizeof(float));
  cola = xQueueCreate(30, sizeof(char));

  if(semaforo_actuadores != NULL){
    xSemaphoreGive(semaforo_actuadores);
  }
  if(semaforo_sensores != NULL){
    xSemaphoreGive(semaforo_sensores);
  }  
  else{
    Serial.println("Error!! No se ha podido crear la cola");
  }

}

//vacio
void loop() {
}

//obtenemos el valor del ldr
void readLDR() {

  float value = analogRead(A1);

  char buffer[10]; // Ajustar el tamaño
  // Este metodo convierte el valor LDR a una cadena de caracteres (char)
  dtostrf(value, 6, 0, buffer); 

  for (int i = 0; i < strlen(buffer); i++) {
    // char c_char = buffer[i];

    if (buffer[i] != ' ')
      xQueueSend(cola,&buffer[i], portMAX_DELAY);
      delay(25);
    }

}


void CheckSum() {


  int ldrValue = analogRead(A1);
  float temperature = dhtSensor.getTemperature();
  float humidity = dhtSensor.getHumidity();

  char cs = (ldrValue >= 0 && temperature > 0 && humidity > 0) ? '1' : '0';

  xQueueSend(cola,&cs, portMAX_DELAY);
  delay(25);
  

}

//obtenemos los valores del DHT
void readDHTTemp() {
  int t = dhtSensor.getTemperature();

  char buffer[10]; // Ajustar el tamaño
  // Convierte el valor LDR a una cadena de caracteres (char)
  dtostrf(t, 6, 0, buffer); 
  for (int i = 0; i < strlen(buffer); i++) {
    if (buffer[i] != ' ')
      xQueueSend(cola,&buffer[i], portMAX_DELAY);
      delay(25);
    }
}

//obtenemos los valores del DHT
void readDHTHum() {
  int t = dhtSensor.getHumidity();

  char buffer[10]; //
  dtostrf(t, 6, 0, buffer); // Ajustar el tamaño
  for (int i = 0; i < strlen(buffer); i++) {

    if (buffer[i] != ' ')
      xQueueSend(cola,&buffer[i], portMAX_DELAY);
      delay(25);
    }

}



static void SendSerial(void* pvParameters){
  char value;
  for (;;){
    if (xQueueReceive(cola,&value, portMAX_DELAY) == pdPASS) {
      // Serial.print(value);
      Serial.write(value);
    }
  }
}

static void ReadSensors(void* pvParameters){

  while(1){
    xSemaphoreTake(semaforo_sensores, portMAX_DELAY);


    //Le pasamos los primeros valores
    xQueueSend(cola,"[", portMAX_DELAY);
    delay(50);
    xQueueSend(cola,"O", portMAX_DELAY);
    delay(50);
    readLDR();
    delay(50);
    xQueueSend(cola,",", portMAX_DELAY);
    delay(50);
    readDHTTemp();
    delay(50);
    xQueueSend(cola,",", portMAX_DELAY);
    delay(50);
    readDHTHum();
    xQueueSend(cola,",", portMAX_DELAY);
    delay(50);
    CheckSum();
    xQueueSend(cola,"]", portMAX_DELAY);
    delay(50);
    vTaskDelay(50/portTICK_PERIOD_MS);
  }
}

static void ReadSerial(void* pvParameters){  
  while(1){ 

    MaquinaDeEstados();
    vTaskDelay(50/portTICK_PERIOD_MS);

  }

}


static void MoveServo(void* pvParameters) {
  while (1) {
    xSemaphoreTake(semaforo_actuadores, portMAX_DELAY);

   // Para el servomotor
    if(n_servo == 0){
        int angulo_actual = 0; // Inicialmente en 0 grados

      if (angle == 0) {
        angulo_actual = 20; // Cambia a 90 grados
      } 
      else {
        // angulo_actual = 0; // Cambia a 0 grados
        angulo_actual = 20*angle; // Cambia a 0 grados
      }

      servoMotor.write(angulo_actual);

      vTaskDelay(50/portTICK_PERIOD_MS);

    }

   //Para la helice
  if (n_servo == 1) {
    int angulo_actual = 0; // Inicialmente en 0 grados

    if (angle == 0) {
      angulo_actual = 0; // Cambia a 0 grados

      angle=1;

      // analogWrite(10,LOW);
      digitalWrite(10,LOW);
    } else {
      angulo_actual = (1 + angle) * 10; // Cambia a 0 grados

      int velocidad = map(angle, 1, 9, 90, 254); // Mapeo de la velocidad
      analogWrite(10, velocidad); // Configurar la velocidad con el valor calculado
      angle=1;
      vTaskDelay(50/portTICK_PERIOD_MS);

      }


  }

  }
}


void MaquinaDeEstados(){
 if (standbyMode) {
    //stand-by
    if (Serial.available() > 0) {
      c = Serial.read();
      switch (estado) {
        case 1:
          if (c == '[') {
            estado = 2;
            trama = c; // Inicializar la trama
          }
          break;
        case 2:
          if (c == 'S') {
            estado = 8;
            trama += c; // Acumular el comando
          }else if (c == 'A') {
              estado = 3; // Saltar al estado 4 para procesar la trama de control de actuadores
              trama += c; // Acumular el comando 
          }
          else {
            // Error, comando incorrecto
            trama = "[E]";
            
            // Serial.println(trama);
            Serial.write("[E]");

            estado = 1;
          }
          break;

        case 3:
          if (c == ',') {

            estado = 4;

          } else {
            trama = "[E]";
            Serial.write("[E]");

            estado = 1;
          }
          break;
          
      case 4:
        if (c >= '0' && c <= '1') {

          n_servo = c - '0'; // Restar el valor ASCII '0' para obtener el valor numérico

          estado = 5;
        } else {
          // Error, número de actuador incorrecto
            Serial.write("[E]");
          estado = 1;
        }
        break;
          
        case 5:
          if (c == ',') {

            estado = 6;

          } else {
            // Error, falta el carácter de fin
            Serial.write("[E]");
            estado = 1;
          }
          break;

          
        case 6:
          if (c >= '0' && c <= '9') {

            estado = 7;
            // angle=int(c);
            angle = c - '0'; // Restar el valor ASCII '0' para obtener el valor numérico

          } else {
            Serial.write("[E]");
          estado = 1;
          }
          break;

        
        case 7:
          if (c == ']') {

            // MoveServo();
            xSemaphoreGive(semaforo_actuadores);

          } else {

            estado = 1;
          }
          break;
        case 8:
          if (c == ']') {

            xSemaphoreGive(semaforo_sensores);

          } 
          else {
            estado = 1;
          }
          default:
            break;
        
      }
    }
  }
  

}

