// Copyright (c) Microsoft. All rights reserved.

using System;
using System.Threading;
using Windows.UI.Xaml.Controls;
using Windows.Devices.Enumeration;
using Windows.Devices.I2c;

namespace MPL3115A2
{
    struct ALPRTEMP
	{
		public double A;
        	public double P;
		public double C;
		public double F;
	};	/// <summary>
		/// App that reads data over I2C from an MPL3115A2, Precision Altimeter
		/// </summary>

	public sealed partial class MainPage : Page
	{
		// MPL3115A2 registers
		private const byte ALPRTEMP_I2C_ADDR = 0x60;		// I2C address of the MPL3115A2
		private const byte ALPRTEMP_REG_STATUS = 0x00;		// Status Register
		private const byte ALPRTEMP_REG_CONF = 0x13;		// Configuration register
		private const byte ALPRTEMP_REG_CNTR1 = 0x26;		// Control register 1

		private I2cDevice I2CAlprtemp;
        	private Timer periodicTimer;

		public MainPage()
		{
			this.InitializeComponent();

			// Register for the unloaded event so we can clean up upon exit
			Unloaded += MainPage_Unloaded;

			// Initialize the I2C bus, Altimeter, Barometer, and Temperature Sensor, and timer
			InitI2CAlprtemp();
		}

		private async void InitI2CAlprtemp()
		{
			// Precision Altimeter
			string aqs = I2cDevice.GetDeviceSelector();		// Get a selector string that will return all I2C controllers on the system
			var dis = await DeviceInformation.FindAllAsync(aqs);	// Find the I2C bus controller device with our selector string
			if (dis.Count == 0)
			{
				Text_Status.Text = "No I2C controllers were found on the system";
				return;
			}

			var settings = new I2cConnectionSettings(ALPRTEMP_I2C_ADDR);
			settings.BusSpeed = I2cBusSpeed.FastMode;
			I2CAlprtemp = await I2cDevice.FromIdAsync(dis[0].Id, settings);	// Create an I2C Device with our selected bus controller and I2C settings
			if (I2CAlprtemp == null)
			{
				Text_Status.Text = string.Format(
					"Slave address {0} on I2C Controller {1} is currently in use by " +
					"another application. Please ensure that no other applications are using I2C.",
				settings.SlaveAddress,
				dis[0].Id);
				return;
			}

			/*
				Initialize the Precision Altimeter, MPL3115A2:
				For this device, we create 2-byte write buffers:
				The first byte is the register address we want to write to.					The second byte is the contents that we want to write to the register.
			*/

			byte[] WriteBuf_Conf = new byte[] { ALPRTEMP_REG_CONF, 0x07 };		// 0x07 sets data ready event enabled for altitude, pressure, temperature
			byte[] WriteBuf_Cntr1 = new byte[] { ALPRTEMP_REG_CNTR1, 0x39 };	// 0x39 sets barometer Mode, OSR = 128, Active Mode

			// Write the register settings
			try
			{
				I2CAlprtemp.Write(WriteBuf_Cntr1);
				I2CAlprtemp.Write(WriteBuf_Conf);
			}

			// If the write fails display the error and stop running
			catch (Exception ex)
			{
				Text_Status.Text = "Failed to communicate with device: " + ex.Message;
				return;
			}

			// Create a timer to read data every 300ms
			periodicTimer = new Timer(this.TimerCallback, null, 0, 300);
		}

		private void MainPage_Unloaded(object sender, object args)
		{
			// Cleanup
			I2CAlprtemp.Dispose();
		}

		private void TimerCallback(object state)
		{
			string aText, pText, cText, fText;
			string addressText, statusText;

			// Read and format Altimeter, Barometer, Hygrometer and Thermometer data
			try
			{
				ALPRTEMP Alprtemp = ReadI2CAlprtemp();
				addressText = "I2C Address of the Precision Altimeter MPL3115A2: 0x60";
				pText = String.Format("Pressure: {0:F2} kPa", Alprtemp.P);
                		aText = String.Format("Altitude: {0:F2} m", Alprtemp.A);
                		cText = String.Format("Temperature in Celsius: {0:F2} °C", Alprtemp.C);
				fText = String.Format("Temperature in Fahrenheit: {0:F2} °F", Alprtemp.F);
				statusText = "Status: Running";
			}
			catch (Exception ex)
			{
				pText = "Pressure: Error";
                		aText = "Altitude: Error";
                		cText = "Temperature in Celsius: Error";
				fText = "Temperature in Fahrenheit: Error";
				statusText = "Failed to read from Altimeter, Barometer and Temperature Sensor: " + ex.Message;
			}

			// UI updates must be invoked on the UI thread
			var task = this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
			{
				Text_Pressure.Text = pText;
                		Text_Altitude.Text = aText;
                		Text_Temperature_in_Celsius.Text = cText;
				Text_Temperature_in_Fahrenheit.Text = fText;
				Text_Status.Text = statusText;
			});
		}

		private ALPRTEMP ReadI2CAlprtemp()
		{	
			// Precision Altimeter
			byte[] RegAddrBuf = new byte[] { ALPRTEMP_REG_STATUS };	// Read data from the Status address
			byte[] ReadBuf = new byte[6];				// We read 6 bytes sequentially to get all 2 two-byte pressure/altitude and temperature registers in one read

			/*
				Read from the Precision Altimeter 
				We call WriteRead() so we first write the address of the Status I2C register, then read Altitude and Temperature data registers
			*/
			I2CAlprtemp.WriteRead(RegAddrBuf, ReadBuf);

			/*
				In order to get the raw 16-bit data values, we need to concatenate two 8-bit bytes from the I2C read for each axis.
			*/
			uint AlprtempRawP = (uint)((ReadBuf[1] & 0xFF) * 65536);
			AlprtempRawP |= (uint)((ReadBuf[2] & 0xFF) * 256);
			AlprtempRawP |= (uint)(ReadBuf[3] & 0xF0);
			uint AlprtempRawC = (uint)((ReadBuf[4] & 0xFF) * 256);
			AlprtempRawC |= (uint)(ReadBuf[5] & 0xF0);
			

			// Conversions using formulas provided
			double pressure = (AlprtempRawP >> 6) + (((AlprtempRawP >> 4) & 0x03) / 4.0);
			pressure = pressure / 1000;
			double altitude = 44330.77 * (1 - Math.Pow(pressure / 101326, 0.1902632));
			altitude = altitude / 100;
			double ctemp = AlprtempRawC / 256.0;
			double ftemp = (ctemp * 1.8) + 32.0;

			ALPRTEMP Alprtemp;
			Alprtemp.P = pressure;
			Alprtemp.A = altitude;
			Alprtemp.C = ctemp;
			Alprtemp.F = ftemp;

			return Alprtemp;
		}
	}
}
