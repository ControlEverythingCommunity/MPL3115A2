// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// MPL3115A2
// This code is designed to work with the MPL3115A2_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/content/Analog-Digital-Converters?sku=MPL3115A2_I2CS#tabs-0-product_tabset-2

#include <application.h>
#include <spark_wiring_i2c.h>

// MPL3115A2 I2C address is 0x60(96)
#define Addr 0x60

float cTemp = 0.0, fTemp = 0.0, pressure = 0.0, altitude = 0.0;
int temp = 0, tHeight = 0;
void setup() 
{
    // Set variable
    Particle.variable("i2cdevice", "MPL3115A2");
    Particle.variable("cTemp", cTemp);
    Particle.variable("pressure", pressure);
    Particle.variable("altitude", altitude);

    // Initialise I2C communication
    Wire.begin();
    // Initialise Serial Communication, set baud rate = 9600
    Serial.begin(9600);

    // Start I2C transmission
    Wire.beginTransmission(Addr);
    // Select control register
    Wire.write(0x26);
    // Active mode, OSR = 128, altimeter mode
    Wire.write(0xB9);
    // Stop I2C transmission
    Wire.endTransmission();

    // Start I2C transmission
    Wire.beginTransmission(Addr);
    // Select data configuration register
    Wire.write(0x13);
    // Data ready event enabled for altitude, pressure, temperature
    Wire.write(0x07);
    // Stop I2C transmission
    Wire.endTransmission();
    delay(300);
}

void loop()
{
    unsigned int data[6];

    // Start I2C transmission
    Wire.beginTransmission(Addr);
    // Select control register
    Wire.write(0x26);
    // Active mode, OSR = 128, altimeter mode
    Wire.write(0xB9);
    // Stop I2C transmission
    Wire.endTransmission();
    delay(1000);

    // Start I2C transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write(0x00);
    // Stop I2C transmission
    Wire.endTransmission();

    // Request 6 bytes of data
    Wire.requestFrom(Addr, 6);
    
    // Read 6 bytes of data from address 0x00(00)
    // status, tHeight msb1, tHeight msb, tHeight lsb, temp msb, temp lsb
    if(Wire.available() == 6)
    {
        data[0] = Wire.read();
        data[1] = Wire.read();
        data[2] = Wire.read();
        data[3] = Wire.read();
        data[4] = Wire.read();
        data[5] = Wire.read();
    }
   

    // Convert the data to 20-bits
    tHeight = ((((long)data[1] * (long)65536) + (data[2] * 256) + (data[3] & 0xF0)) / 16);
    temp = ((data[4] * 256) + (data[5] & 0xF0)) / 16;
    altitude = tHeight / 16.0;
    cTemp = (temp / 16.0);
    fTemp = cTemp * 1.8 + 32;

    // Start I2C transmission
    Wire.beginTransmission(Addr);
    // Select control register
    Wire.write(0x26);
    // Active mode, OSR = 128, barometer mode
    Wire.write(0x39);
    // Stop I2C transmission
    Wire.endTransmission();
    
    // Start I2C transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write(0x00);
    // Stop I2C transmission
    Wire.endTransmission();
    delay(1000);
    
    // Request 4 bytes of data
    Wire.requestFrom(Addr, 4);
    
    // Read 4 bytes of data
    // status, pres msb1, pres msb, pres lsb
    if(Wire.available() == 4)
    {
        data[0] = Wire.read();
        data[1] = Wire.read();
        data[2] = Wire.read();
        data[3] = Wire.read();
    }
   

    // Convert the data to 20-bits
    pres = (((long)data[1] * (long)65536) + (data[2] * 256) + (data[3] & 0xF0)) / 16;
    pressure = (pres / 4.0) / 1000.0;

    // Output data to dashboard
    Particle.publish("Altitude :", String(altitude));
    Particle.publish("Pressure :", String(pressure));
    Particle.publish("Temperature in Celsius :", String(cTemp));
    Particle.publish("Temperature in Fahrenheit :", String(fTemp));
    delay(1000);
}
