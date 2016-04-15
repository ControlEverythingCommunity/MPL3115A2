// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// MPL3115A2
// This code is designed to work with the MPL3115A2_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/products

import com.pi4j.io.i2c.I2CBus;
import com.pi4j.io.i2c.I2CDevice;
import com.pi4j.io.i2c.I2CFactory;
import java.io.IOException;

public class MPL3115A2
{
	public static void main(String args[]) throws Exception
	{
		// Create I2C bus
		I2CBus Bus = I2CFactory.getInstance(I2CBus.BUS_1);
		// Get I2C device, MPL3115A2 I2C address is 0x60(96)
		I2CDevice device = Bus.getDevice(0x60);
		
		// Select control register
		// Active mode, OSR = 128, altimeter mode
		device.write(0x26, (byte)0xB9);
		// Select data configuration register
		// Data ready event enabled for altitude, pressure, temperature
		device.write(0x13, (byte)0x07);
		
		// Select control register
		// Active mode, OSR = 128, altimeter mode
		device.write(0x26, (byte)0xB9);
		Thread.sleep(1000);

		// Read 6 bytes of data from address 0x00(00)
		// status, tHeight msb1, tHeight msb, tHeight lsb, temp msb, temp lsb
		byte[] data = new byte[6];
		device.read(0x00, data, 0, 6);

		// Convert the data to 20-bits
		int tHeight = ((((data[1] & 0xFF) * 65536) + ((data[2] & 0xFF) * 256) + (data[3] & 0xF0)) / 16);
		int temp = ((data[4] * 256) + (data[5] & 0xF0)) / 16;
		double altitude = tHeight / 16.0;
		double cTemp = (temp / 16.0);
		double fTemp = cTemp * 1.8 + 32;

		// Select control register
		// Active mode, OSR = 128, barometer mode
		device.write(0x26, (byte)0x39);
		Thread.sleep(1000);
		
		// Read 4 bytes of data from address 0x00(00)
		// status, pres msb1, pres msb, pres lsb
		device.read(0x00, data, 0, 4);

		// Convert the data to 20-bits
		int pres = (((data[1] & 0xFF) * 65536) + ((data[2] & 0xFF) * 256) + (data[3] & 0xF0)) / 16;
		double pressure = (pres / 4.0) / 1000.0;
		
		// Output data to screen
		System.out.printf("Pressure : %.2f kPa %n", pressure);
		System.out.printf("Altitude : %.2f m %n", altitude);
		System.out.printf("Temperature in Celsius : %.2f C %n", cTemp);
		System.out.printf("Temperature in Fahrenheit : %.2f F %n", fTemp);
	}
}