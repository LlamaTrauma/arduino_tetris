#include <Adafruit_TFTLCD.h>

// Extend the TFT_LCD library with some more stuff
// Requires the private methods in Adafruit_TFTLCD.h to be changed to protected

class Ext_TFTLCD : public Adafruit_TFTLCD {
  using Adafruit_TFTLCD::Adafruit_TFTLCD;
  public:
   void Ext_TFTLCD::pushColorsFromPRGMEM(uint16_t* addr, uint16_t l);
   void Ext_TFTLCD::pushEfficientColor(uint8_t color, uint8_t count, bool begin);
   uint16_t Ext_TFTLCD::getScanline();
   void Ext_TFTLCD::initFramerate();
  //  void Ext_TFTLCD::fillScreen(uint16_t c);
   void Ext_TFTLCD::setPageAddr(uint16_t s, uint16_t e);
   void Ext_TFTLCD::setColumnAddr(uint16_t s, uint16_t e);
};