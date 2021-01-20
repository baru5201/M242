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

// Klassifikation 
//char cls[3][10] = { "low", "middle", "high" };
//int type = 0;

// UI
OLEDDisplay oled( MBED_CONF_IOTKIT_OLED_RST, MBED_CONF_IOTKIT_OLED_SDA, MBED_CONF_IOTKIT_OLED_SCL );
DigitalOut led1( MBED_CONF_IOTKIT_LED1 );
DigitalOut led2( MBED_CONF_IOTKIT_LED2 );
DigitalOut led3( MBED_CONF_IOTKIT_LED3 );
DigitalOut led4( MBED_CONF_IOTKIT_LED4 );

int p;
char t;

// Aktore(n)
Motor m1( MBED_CONF_IOTKIT_MOTOR2_PWM, MBED_CONF_IOTKIT_MOTOR2_FWD, MBED_CONF_IOTKIT_MOTOR2_REV ); // PWM, Vorwaerts, Rueckwarts
PwmOut speaker( MBED_CONF_IOTKIT_BUZZER );


void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    
    
    p = message.payload;
    
}


/** Hilfsfunktion zum Publizieren auf MQTT Broker */
void publish( MQTTNetwork &mqttNetwork, MQTT::Client<MQTTNetwork, Countdown> &client, char* topic )
{
    printf("Connecting to %s:%d\r\n", hostname, port);
    
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "mbed-sample";
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);
        
    rc = client.subscribe("iotkit/led", MQTT::QOS2, messageArrived);

    MQTT::Message message;    
    
    oled.cursor( 2, 0 );
    oled.printf( "Topi: %s\n", topic );
    oled.cursor( 3, 0 );    
    oled.printf( "Push: %s\n", buf );
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buf;
    message.payloadlen = strlen(buf)+1;
    client.publish( topic, message);  
    
    // Verbindung beenden, ansonsten ist nach 4x Schluss
    if ((rc = client.disconnect()) != 0)
        printf("rc from disconnect was %d\r\n", rc);

    mqttNetwork.disconnect();
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

/*
    while   ( 1 ) 
    {
        // Temperator und Luftfeuchtigkeit
        hum_temp.read_id(&id);
        hum_temp.get_temperature(&temp);
        hum_temp.get_humidity(&hum);    
        if  ( type == 0 )
        {
            temp -= 5.0f;
            m1.speed( 0.0f );
        }
        else if  ( type == 2 )
        {
            temp += 5.0f;
            m1.speed( 1.0f );
        }
        else
        {
            m1.speed( 0.75f );
        }
        sprintf( buf, "0x%X,%2.2f,%2.1f,%s", id, temp, hum, cls[type] ); 
        type++;
        if  ( type > 2 )
            type = 0;       
        publish( mqttNetwork, client, topicTEMP );
        
        // alert Tuer offen 
        printf( "Hall %4.4f, alert %d\n", hallSensor.read(), alert.read() );
        if  ( hallSensor.read() > 0.6f )
        {
            // nur einmal Melden!, bis Reset
            if  ( alert.read() == 0 )
            {
                sprintf( buf, "alert: hall" );
                message.payload = (void*) buf;
                message.payloadlen = strlen(buf)+1;
                publish( mqttNetwork, client, topicALERT );
                alert = 1;
            }
            speaker.period( 1.0 / 3969.0 );      // 3969 = Tonfrequenz in Hz
            speaker = 0.5f;
            wait( 0.5f );
            speaker.period( 1.0 / 2800.0 );
            wait( 0.5f );
        }
        else
        {
            alert = 0;
            speaker = 0.0f;
        }

        // Button (nur wenn gedrueckt)
        if  ( button == 0 )
        {
            sprintf( buf, "ON" );
            publish( mqttNetwork, client, topicBUTTON );
        }

#ifdef TARGET_K64F        

        // Encoder
        encoder = wheel.getPulses();
        sprintf( buf, "%d", encoder );
        publish( mqttNetwork, client, topicENCODER );
        
        // RFID Reader
        if ( rfidReader.PICC_IsNewCardPresent())
            if ( rfidReader.PICC_ReadCardSerial()) 
            {
                // Print Card UID (2-stellig mit Vornullen, Hexadecimal)
                printf("Card UID: ");
                for ( int i = 0; i < rfidReader.uid.size; i++ )
                    printf("%02X:", rfidReader.uid.uidByte[i]);
                printf("\n");
                
                // Print Card type
                int piccType = rfidReader.PICC_GetType(rfidReader.uid.sak);
                printf("PICC Type: %s \n", rfidReader.PICC_GetTypeName(piccType) );
                
                sprintf( buf, "%02X:%02X:%02X:%02X:", rfidReader.uid.uidByte[0], rfidReader.uid.uidByte[1], rfidReader.uid.uidByte[2], rfidReader.uid.uidByte[3] );
                publish( mqttNetwork, client, topicRFID );                
                
            }        
#endif        

        wait    ( 2.0f );
    }*/
    
    while(true)
    {
        oled.clear();
        hum_temp.get_temperature(&temp);
        hum_temp.get_humidity(&hum);  
    
        oled.printf("%2.2f,%2.1f", temp, hum);
        
        oled.cursor(1,0);
        oled.printf("%d", p);
        
        sprintf(buf, "%2.2f,%2.1f", temp, hum);
        publish( mqttNetwork, client, topicTEMP );
        
        led1 = p;
        
        wait( 1.0f );
    }
    
}
