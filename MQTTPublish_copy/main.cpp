/** MQTT Publish von Sensordaten */
#include "mbed.h"
#include "HTS221Sensor.h"
#include "network-helper.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include "OLEDDisplay.h"
#include "Motor.h"

#ifdef TARGET_K64F
#include "QEI.h"
#include "MFRC522.h"

// NFC/RFID Reader (SPI)
MFRC522    rfidReader( MBED_CONF_IOTKIT_RFID_MOSI, MBED_CONF_IOTKIT_RFID_MISO, MBED_CONF_IOTKIT_RFID_SCLK, MBED_CONF_IOTKIT_RFID_SS, MBED_CONF_IOTKIT_RFID_RST ); 
//Use X2 encoding by default.
QEI wheel (MBED_CONF_IOTKIT_BUTTON2, MBED_CONF_IOTKIT_BUTTON3, NC, 624);

#endif

#ifdef TARGET_NUCLEO_F303RE
#include "BMP180Wrapper.h"
#endif

// Sensoren wo Daten fuer Topics produzieren
static DevI2C devI2c( MBED_CONF_IOTKIT_I2C_SDA, MBED_CONF_IOTKIT_I2C_SCL );
#ifdef TARGET_NUCLEO_F303RE
static BMP180Wrapper hum_temp( &devI2c );
#else
static HTS221Sensor hum_temp(&devI2c);
#endif
//AnalogIn hallSensor( MBED_CONF_IOTKIT_HALL_SENSOR );
DigitalIn button( MBED_CONF_IOTKIT_BUTTON1 );

// Topic's
char* topicTEMP =  "iotkit/sensor";
//char* topicALERT = "iotkit/alert";
char* topicBUTTON = "iotkit/button";
//char* topicENCODER = "iotkit/encoder";
//char* topicRFID = "iotkit/rfid";
// MQTT Brocker
char* hostname = "31.165.207.208";
int port = 1883;
// MQTT Message
MQTT::Message message;
// I/O Buffer
char buf[100];


// UI
OLEDDisplay oled( MBED_CONF_IOTKIT_OLED_RST, MBED_CONF_IOTKIT_OLED_SDA, MBED_CONF_IOTKIT_OLED_SCL );
DigitalOut led1( MBED_CONF_IOTKIT_LED1 );
DigitalOut led2( MBED_CONF_IOTKIT_LED2 );
DigitalOut led3( MBED_CONF_IOTKIT_LED3 );
DigitalOut led4( MBED_CONF_IOTKIT_LED4 );

int p;
char * t;

// Aktore(n)
Motor m1( MBED_CONF_IOTKIT_MOTOR2_PWM, MBED_CONF_IOTKIT_MOTOR2_FWD, MBED_CONF_IOTKIT_MOTOR2_REV ); // PWM, Vorwaerts, Rueckwarts
PwmOut speaker( MBED_CONF_IOTKIT_BUZZER );


void messageArrived(MQTT::MessageData& md)
{
    
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
    t = (char*)message.payload;
    p = atoi(t);

}


/** Hilfsfunktion zum Publizieren auf MQTT Broker */
void publish( MQTTNetwork &mqttNetwork, MQTT::Client<MQTTNetwork, Countdown> &client, char* topic, int rc )
{
    MQTT::Message message;
    oled.cursor( 2, 0 );
    oled.printf( "Topi: %s\n", topic );
    oled.cursor( 3, 0 );    
    oled.printf( "Push: %s\n", buf );
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buf;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish( topic, message);
    
}



/** Hauptprogramm */
int main()
{
    //uint8_t id;
    float temp, hum;
    //int encoder;
    //alert = 0;
    
    oled.clear();
    oled.printf( "MQTTPublish\r\n" );
    oled.printf( "host: %s:%s\r\n", hostname, port );

    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    oled.printf( "SSID: %s\r\n", MBED_CONF_APP_WIFI_SSID );
    // Connect to the network with the default networking interface
    // if you use WiFi: see mbed_app.json for the credentials
    NetworkInterface* wifi = connect_to_default_network_interface();
    if ( !wifi ) 
    {
        printf("Cannot connect to the network, see serial output\n");
        return 1;
    }

    // TCP/IP und MQTT initialisieren (muss in main erfolgen)
    MQTTNetwork mqttNetwork( wifi );
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);
    
    /* Init all sensors with default params */
    hum_temp.init(NULL);
    hum_temp.enable();  
    
    #ifdef TARGET_K64F        
        // RFID Reader initialisieren
        rfidReader.PCD_Init();  
    #endif

    printf("Connecting to %s:%d\r\n", hostname, port);

    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "mbed-sample";
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);
        

    rc = client.subscribe("iotkit/led", MQTT::QOS0, messageArrived);
    if (rc != 0)
        printf("rc from MQTT subscribe is %d\n", rc);


    while(true)
    {
        oled.clear();
        hum_temp.get_temperature(&temp);
        hum_temp.get_humidity(&hum);  
    
        oled.printf("%2.2f,%2.1f", temp, hum);
        
        oled.cursor(1,0);
        oled.printf("%d", p);
        
        sprintf(buf, "%2.2f,%2.1f", temp, hum);
        publish( mqttNetwork, client, topicTEMP, rc );

        led1 = p;
        led2 = p;
        led3 = p;
        led4 = p;
        
        wait( 1.0f );
    }
    
    // Verbindung beenden, ansonsten ist nach 4x Schluss
    if ((rc = client.disconnect()) != 0)
        printf("rc from disconnect was %d\r\n", rc);

    mqttNetwork.disconnect();

}
