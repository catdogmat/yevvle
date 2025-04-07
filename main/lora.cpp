#include <Arduino.h>
#include <HardwareSerial.h>

#include "hardware.h"

#include "esp32-hal-log.h"

#define RX 2 // Connect to GPS Module's TX pin
#define TX 1 // Connect to GPS Module's RX pin
#define BAND_RATE 9600 // Baud rate for GPS Module

void lora() {
  if constexpr (!HW::kHasLora) {
    return;
  }
  HardwareSerial ss(2); // Use UART2
  ss.begin(BAND_RATE, SERIAL_8N1, RX, TX); // Initialize the serial port
  // If there is data in the receive buffer of the hardware serial
  while (true)
  {
      // Read a byte of data from the buffer
      char gpsData = ss.read();

      // Send the read data to the Arduino IDE serial monitor
      ESP_LOGE("GPS", "%c", gpsData);
  }
  return;
};
