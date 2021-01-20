/**
 * Bosch BMP180 Digital Pressure Sensor
 */

#include <BMP180Wrapper.h>

BMP180Wrapper::BMP180Wrapper( DevI2C* i2c ) : bmp180(i2c)
{
}

int BMP180Wrapper::init( void* init )
{
    while ( 1 )
    {
        if ( bmp180.init() != 0 )
        {
            printf( "Error communicating with BMP180\n" );
        }
        else
        {
            printf( "Initialized BMP180\n" );
            break;
        }
        wait( 1 );
    }
    return  ( 0 );
}

int BMP180Wrapper::read_id( uint8_t* id )
{
    *id = 0x55;
    return  ( 0 );
}

int BMP180Wrapper::get_humidity( float* pfData )
{
    *pfData = 0.0f;
    bmp180.startPressure(BMP180::ULTRA_LOW_POWER);
    wait_ms(10);    // Wait for conversion to complete
    int pressure;
    if(bmp180.getPressure(&pressure) == 0)
    {
        *pfData = pressure / 1000.0f;
    }
    return  ( 0 );
}

int BMP180Wrapper::get_temperature( float* pfData )
{
    bmp180.startTemperature();
    wait_ms( 5 ); // Wait for conversion to complete
    return ( bmp180.getTemperature( pfData ) );
}

int BMP180Wrapper::enable( void )
{
    return  ( 0 );
}
