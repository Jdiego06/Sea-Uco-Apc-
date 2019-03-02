#include <Arduino.h>
#include <UbidotsMicroESP8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <WiFiClient.h>
#include <time.h>
#include <TimeLib.h>


//--------------------------------------------------------------------------------------------------------------------


#define WIFISSID "xxxxxx"                           //credenciales de la red wifi
#define PASSWORD "xxxxxx"


//--------------------------------------------------TANQUE X-----------------------------------------------------------

#define TOKEN  "xxxxxx"        //Token de la cuenta Ubidots
char* Time_on_valve = "xxxxxx";                             //Ids de variables Ubidots
char* lcdb = "xxxxxx";
char* HourOn = "xxxxxx";
char* MinuteOn = "xxxxxx";
//---------------------------------------------------------------------------------------------------------------------





#define ONE_WIRE_BUS D3                              //Pin del bus One Wire
#define GPIO_ADDR   0x27                             //direccion i2c del lcd
#define iteracciones 10
#define dst  0
#define timezone -5


int valvula = 12;                                    //pin de control de la válvula
int Hour=0;
int Minute=0;
int Humedad=0;                                       //inicialización de variables
int Lectura_Analogica=0;
int reconection_time=0;
float temp_value=0;
float On_Time=0;
float valve_state=0;
float last_valve_state=0;
float temperatura=0;
char* Wifistatus="D";
float lcd_state=0;
int it=0;



LiquidCrystal_I2C lcd(GPIO_ADDR, 20, 4);             //Se inicializa el lcd
Ubidots client(TOKEN);                               //Se inicializa el cliente de Ubidots
OneWire oneWire(ONE_WIRE_BUS);                       //Se inicializa el sensor de temperatura
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);                              //inicializa el puerto serial
  Serial.setDebugOutput(true);
  delay(10);
  client.wifiConnection(WIFISSID, PASSWORD);         //se conecta a la red wifi
  lcd.begin();                                       //se inicializan sensores, y lcd
  lcd.backlight();
  sensors.begin();


  configTime(timezone * 3600, dst, "pool.ntp.org", "time.nist.gov");   //sincroniza la hora interna con el servidor ntp

    Serial.println("\nWaiting for time");

    while (!time(nullptr)) {
      Serial.print(".");
      delay(1000);
    }

  pinMode(valvula, OUTPUT);                          //se definen pines de entrada y salida

  lcd.setCursor(0,0);                                //imprime el texto estatico del lcd
  lcd.print("Temperatura:");
  lcd.setCursor(0,1);
  lcd.print("Humedad:");
 }

 void loop() {

   Lectura_Analogica = analogRead(A0);              //Obtiene la humedad
   Humedad = map(Lectura_Analogica,0, 1024, 0, 100);
   Humedad=abs(100-Humedad);

   sensors.requestTemperatures();                    //Obtiene la temperatura
   temperatura = sensors.getTempCByIndex(0);

   if(temperatura<0){
     temperatura=0;
   }

   if((Humedad>100)||(Humedad<=0)){
     Humedad=0;
   }


   time_t now = time(nullptr);                        //Obtiene la hora interna

   // Serial.println(ctime(&now));
   // Serial.print("Temperatura: ");                    //Imprime temperatura y humedad puerto serial
   // Serial.println(temperatura);
   // Serial.print("Lectura analógica: ");
   // Serial.println(Lectura_Analogica);
   // Serial.print("Humedad: ");
   // Serial.print(Humedad);
   // Serial.println('%');
   // Serial.print("Hora: ");
   // Serial.print(hour(now));
   // Serial.print(":");
   // Serial.println(minute(now));

   lcd.setCursor(13,0);                              //Imprime temperatura y humedad en el lcd
   lcd.print(round(temperatura));
   lcd.print('c');
   lcd.setCursor(8,1);
   lcd.print("       ");
   lcd.setCursor(9,1);
   lcd.print(Humedad);
   lcd.print('%');


   if(WiFi.status() == 3){

      //Intenta descargar los valores de la nube; de no ser capaz, deja los valores anteriores como estaban

      int value;
      it=0;
      while(it <= iteracciones){
          value =  client.getValue(Time_on_valve);
         if (value != ERROR_VALUE){
            lcd.setCursor(15,1);
            lcd.print("C");
            Wifistatus="C";
            On_Time=value;
            On_Time=On_Time*1000;
            break;
         }else if(it==iteracciones || WiFi.status() != 3 ){
            lcd.setCursor(15,1);
            lcd.print("D");
            Wifistatus="D";
            goto var2;
         }
      it++;
      }

   var2:

      it=0;
      while(it <= iteracciones){
         value =  client.getValue(lcdb);
          if (value != ERROR_VALUE){
            lcd_state=value;
            break;
         }else if(WiFi.status() != 3 ){
            goto var3;
         }
         it++;
       }

       var3:

       it=0;
       while(it <= iteracciones){
          value =  client.getValue(HourOn);
          if (value != ERROR_VALUE){
            Hour=value;
            break;
         }else if(WiFi.status() != 3 ){
            goto var4;
         }
         it++;
       }

       var4:

       it=0;
       while(it <= iteracciones){
          value =  client.getValue(MinuteOn);
         if (value != ERROR_VALUE){
            Minute=value;
            break;
         }else if(WiFi.status() != 3 ){
            goto main;
         }
         it++;
      }

   }

main:

   //Controla la válvula, y el lcd

   // On_Time = client.getValue(Time_on_valve);            //Prueba la conectividad con el servidor de Ubidots
   //
   //  if(On_Time!=999){                             //Si HAY conectividad
   //   Serial.println("Esta conectado a internet");
   //   lcd.setCursor(15,1);
   //   lcd.print("C");
   //   On_Time = client.getValue(Time_on_valve);       //Establece los valores de INTERNET para tiempo de encendido,
   //   lcd_state = client.getValue(lcdb);
   //   On_Time=On_Time*1000;
   //   Hour = client.getValue(HourOn);
   //   Minute = client.getValue(MinuteOn);
   //   Wifistatus="C";     // Serial.println("Se establecieron los valores de la nube");
   //
   //  }else{                                  //Si NO hay conectividad
   //   Serial.println("Error al descargar variables, no esta conectado a internet. ");
   //   lcd.setCursor(15,1);
   //   lcd.print("D");
   //  }



   if(lcd_state==1){
     lcd.backlight();                                //Enciende o apaga la luz del lcd
   }
   else if(lcd_state==0){
     lcd.noBacklight();
   }


   if((minute(now)>=Minute)&&(minute(now)<(Minute+(On_Time/60000)))){      // verifica es hora de encender la válvula

       if(hour(now)==Hour){
         digitalWrite(valvula, HIGH);
         valve_state=1;
       }

        if(hour(now)==(Hour+6)){
         digitalWrite(valvula, HIGH);
         valve_state=1;
       }

        if(hour(now)==(Hour+12)){
         digitalWrite(valvula, HIGH);
         valve_state=1;
       }

    }else{
       digitalWrite(valvula, LOW);
       valve_state=0;
    }


   // if(counter>=(On_Time/Sample)){                 //Controla la electrovalvula, si ya ha pasado el tiempo de
   //   counter=0;                                   // encendido o apagado.
   //  if((temperatura>=temp_value) && (last_state==0)){
   //    digitalWrite(valvula, HIGH);
   //    valve_state=1;
   //    last_state=1;
   //    Serial.println("Valvula Encendida");
   //  }
   //  else if(((temperatura>=temp_value) && (last_state==1))||(temperatura<temp_value)){
   //    digitalWrite(valvula,LOW);
   //    valve_state=0;
   //    last_state=0;
   //    Serial.println("Valvula Apagada");
   //  }
   // }


   client.add("Humedad", Humedad);                 //Empaqueta los valores de temperatura, humedad, tiempo
   client.add("Temperatura", temperatura);         //y estado de la válvula
   client.add("Hora del dispositivo", hour(now));
   client.add("Minutos del dispositivo", minute(now));
   client.add("Estado de la valvula", valve_state);
   delay(1000);
   client.sendAll(true);                           //Envia el paquete
   Serial.println("");



   // Serial.print("Wifi State: ");                    //Imprime configuraciones, y lecturas por el puerto     serial.
   // Serial.println(Wifistatus);
   // Serial.print("La valvula permanece encendida por:  ");
   // Serial.print(round(On_Time/1000));
   // Serial.println(" Seg");
   // Serial.print("Estado de la valvula: ");
   // Serial.println(round(valve_state));
   // Serial.print("Estado del lcd: ");
   // Serial.println(lcd_state);
   // Serial.print("La hora de encendido es: ");
   // Serial.print(Hour);
   // Serial.print(":");
   // Serial.println(Minute);
   // Serial.println("______________________________________________________________________________________________");


     delay(20000);                                  //Espera un tiempo antes el iniciar siguiente ciclo


 }
