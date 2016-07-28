// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// MPL3115A2
// This code is designed to work with the MPL3115A2_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/products

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>

void main() 
{
	// Create I2C bus
	int file;
	char *bus = "/dev/i2c-1";
	if((file = open(bus, O_RDWR)) < 0) 
	{
		printf("Failed to open the bus. \n");
		exit(1);
	}
	// Get I2C device, TSL2561 I2C address is 0x60(96)
	ioctl(file, I2C_SLAVE, 0x60);

	// Select control register(0x26)
	// Active mode, OSR = 128, altimeter mode(0xB9)
	char config[2] = {0};
	config[0] = 0x26;
	config[1] = 0xB9;
	write(file, config, 2);
	// Select data configuration register(0x13)
	// Data ready event enabled for altitude, pressure, temperature(0x07)
	config[0] = 0x13;
	config[1] = 0x07;
	write(file, config, 2);
	// Select control register(0x26)
	// Active mode, OSR = 128, altimeter mode(0xB9)
	config[0] = 0x26;
	config[1] = 0xB9;
	write(file, config, 2);
	sleep(1);

	// Read 6 bytes of data from address 0x00(00)
	// status, tHeight msb1, tHeight msb, tHeight lsb, temp msb, temp lsb
	char reg[1] = {0x00};
	write(file, reg, 1);
	char data[6] = {0};
	if(read(file, data, 6) != 6)
	{
		printf("Error : Input/Output error \n");
		exit(1);
	}

	// Convert the data
	int tHeight = ((data[1] * 65536) + (data[2] * 256 + (data[3] & 0xF0)) / 16);
	int temp = ((data[4] * 256) + (data[5] & 0xF0)) / 16;
	float altitude = tHeight / 16.0;
	float cTemp = (temp / 16.0);
	float fTemp = cTemp * 1.8 + 32;

	// Select control register(0x26)
	// Active mode, OSR = 128, barometer mode(0x39)
	config[0] = 0x26;
	config[1] = 0x39;
	write(file, config, 2);
	sleep(1);

	// Read 4 bytes of data from register(0x00)
	// status, pres msb1, pres msb, pres lsb
	reg[0] = 0x00;
	write(file, reg, 1);
	read(file, data, 4);

	// Convert the data to 20-bits
	int pres = ((data[1] * 65536) + (data[2] * 256 + (data[3] & 0xF0))) / 16;
	float pressure = (pres / 4.0) / 1000.0;
	
	// Output data to screen
	printf("Pressure : %.2f kPa \n", pressure);
	printf("Altitude : %.2f m \n", altitude);
	printf("Temperature in Celsius : %.2f C \n", cTemp);
	printf("Temperature in Fahrenheit : %.2f F \n", fTemp);
}
