#ifndef __NETWORK_HELPER_H_
#define _NETWORK_HELPER_H_

#include "mbed.h"

#ifdef TARGET_K64F
#include "ESP8266Interface.h"
ESP8266Interface wifi(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX);
#endif

#ifdef TARGET_DISCO_L475VG_IOT01A
#include "ISM43362Interface.h"
ISM43362Interface wifi( false );
#endif

#if defined( TARGET_NUCLEO_F303RE  ) || defined( TARGET_NUCLEO_F411RE ) || defined( TARGET_NUCLEO_F103RB )
#include "ESP8266Interface.h"
ESP8266Interface wifi( MBED_CONF_IOTKIT_ESP8266_TX, MBED_CONF_IOTKIT_ESP8266_RX );
#endif

/**
 * Connect to the network using the default networking interface,
 * you can also swap this out with a driver for a different networking interface
 * if you use WiFi: see mbed_app.json for the credentials
 */
NetworkInterface *connect_to_default_network_interface() 
{
    printf("[NWKH] Connecting to network...\n");

#ifdef TARGET_NUCLEO_F746ZG
    NetworkInterface* network = NetworkInterface::get_default_instance();
    nsapi_error_t connect_status = network->connect();
#else
    NetworkInterface* network = &wifi;
    nsapi_error_t connect_status = wifi.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
#endif 

    if (connect_status != NSAPI_ERROR_OK) 
    {
        printf("[NWKH] Failed to connect to network (%d)\n", connect_status);
        return NULL;
    }

    printf("[NWKH] Connected to the network\n");
    printf("[NWKH] IP address: %s\n", network->get_ip_address());
    return network;
}

#endif _NETWORK_HELPER_H_