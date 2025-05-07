
#include "hardware.h"

#include <SPI.h>

// Helper class to initialize the SPI bus
struct Spi {
    Spi() {
        SPI.begin(HW::Spi::Sck, HW::Spi::Miso, HW::Spi::Mosi, -1);
    }
};