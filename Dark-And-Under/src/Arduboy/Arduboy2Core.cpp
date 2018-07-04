/**
 * @file Arduboy2Core.cpp
 * \brief
 * The Arduboy2Core class for Arduboy hardware initilization and control.
 */

#include "Arduboy2Core.h"
#include <avr/wdt.h>

const uint8_t PROGMEM lcdBootProgram[] = {
  // boot defaults are commented out but left here in case they
  // might prove useful for reference
  //
  // Further reading: https://www.adafruit.com/datasheets/SSD1306.pdf
  //
  // Display Off
  // 0xAE,

  #if !defined OLED_SH1106 // Not supported on SH1106 
  // Set Display Clock Divisor v = 0xF0
  // default is 0x80
    0xD5, 0xF0,
  #endif
  // Set Multiplex Ratio v = 0x3F
  // 0xA8, 0x3F,

  // Set Display Offset v = 0
  // 0xD3, 0x00,

  // Set Start Line (0)
  // 0x40,

  // Charge Pump Setting v = enable (0x14)
  // default is disabled
  0x8D, 0x14,

  // Set Segment Re-map (A0) | (b0001)
  // default is (b0000)
  0xA1,

  // Set COM Output Scan Direction
  0xC8,

  // Set COM Pins v
  // 0xDA, 0x12,

  // Set Contrast v = 0xCF
  0x81, 0xCF,
	
  // Set Precharge = 0xF1
  0xD9, 0xF1,

  // Set VCom Detect
  // 0xDB, 0x40,

  // Entire Display ON
  // 0xA4,

  // Set normal/inverse display
  // 0xA6,

  // Display On
  0xAF,

#if defined OLED_SH1106 // required for SH1106 only
  //Set column address for left most pixel 
  OLED_SET_COLUMN_ADDRESS_LO 
#else // these commands are only supported by SSD1306 and SSD1309
  // set display mode = horizontal addressing mode (0x00) 
  0x20, 0x00,

  // set col address range
  // 0x21, 0x00, COLUMN_ADDRESS_END,

  // set page address range
  // 0x22, 0x00, PAGE_ADDRESS_END
#endif  
#if defined OLED_SSD1309 //required for SSD1309
  0x21, 0x00, COLUMN_ADDRESS_END
#endif
};

Arduboy2Core::Arduboy2Core() { }

void Arduboy2Core::boot()
{
  #ifdef ARDUBOY_SET_CPU_8MHZ
  // ARDUBOY_SET_CPU_8MHZ will be set by the IDE using boards.txt
  setCPUSpeed8MHz();
  #endif

  // Select the ADC input here so a delay isn't required in initRandomSeed()
  ADMUX = RAND_SEED_IN_ADMUX;

  bootPins();
  bootSPI();
  bootOLED();
  bootPowerSaving();
}

#ifdef ARDUBOY_SET_CPU_8MHZ
// If we're compiling for 8MHz we need to slow the CPU down because the
// hardware clock on the Arduboy is 16MHz.
// We also need to readjust the PLL prescaler because the Arduino USB code
// likely will have incorrectly set it for an 8MHz hardware clock.
void Arduboy2Core::setCPUSpeed8MHz()
{
  uint8_t oldSREG = SREG;
  cli();                // suspend interrupts
  PLLCSR = _BV(PINDIV); // dissable the PLL and set prescale for 16MHz)
  CLKPR = _BV(CLKPCE);  // allow reprogramming clock
  CLKPR = 1;            // set clock divisor to 2 (0b0001)
  PLLCSR = _BV(PLLE) | _BV(PINDIV); // enable the PLL (with 16MHz prescale)
  SREG = oldSREG;       // restore interrupts
}
#endif

// Pins are set to the proper modes and levels for the specific hardware.
// This routine must be modified if any pins are moved to a different port
void Arduboy2Core::bootPins()
{
#ifdef ARDUBOY_10
  // Port B INPUT_PULLUP or HIGH
  PORTB |= _BV(RED_LED_BIT) |
  #ifndef ARDUINO_AVR_PROMICRO
           _BV(GREEN_LED_BIT) |
  #endif
		   _BV(BLUE_LED_BIT) | _BV(B_BUTTON_BIT);
  // Port B INPUT or LOW (none)
  // Port B inputs
  DDRB &= ~(_BV(B_BUTTON_BIT));
  // Port B outputs
  DDRB |= _BV(RED_LED_BIT) | 
  #ifndef ARDUINO_AVR_PROMICRO
          _BV(GREEN_LED_BIT) | 
  #endif
		  _BV(BLUE_LED_BIT) | _BV(SPI_MOSI_BIT) | _BV(SPI_SCK_BIT);

  // Port C
  // Speaker: Not set here. Controlled by audio class

  // Port D INPUT_PULLUP or HIGH
  #ifdef ARDUINO_AVR_PROMICRO
    PORTD |= _BV(CS_BIT) | _BV(GREEN_LED_BIT);
  #else
    PORTD |= _BV(CS_BIT);
  #endif
  // Port D INPUT or LOW
  PORTD &= ~(_BV(RST_BIT));
  // Port D inputs (none)
  // Port D outputs
  DDRD |= _BV(RST_BIT) | _BV(CS_BIT) |
  #ifdef ARDUINO_AVR_PROMICRO
    _BV(GREEN_LED_BIT) |
  #endif
  _BV(DC_BIT);

  // Port E INPUT_PULLUP or HIGH
  PORTE |= _BV(A_BUTTON_BIT);
  // Port E INPUT or LOW (none)
  // Port E inputs
  DDRE &= ~(_BV(A_BUTTON_BIT));
  // Port E outputs (none)

  // Port F INPUT_PULLUP or HIGH
  PORTF |= _BV(LEFT_BUTTON_BIT) | _BV(RIGHT_BUTTON_BIT) |
           _BV(UP_BUTTON_BIT) | _BV(DOWN_BUTTON_BIT);
  // Port F INPUT or LOW
  PORTF &= ~(_BV(RAND_SEED_IN_BIT));
  // Port F inputs
  DDRF &= ~(_BV(LEFT_BUTTON_BIT) | _BV(RIGHT_BUTTON_BIT) |
            _BV(UP_BUTTON_BIT) | _BV(DOWN_BUTTON_BIT) |
            _BV(RAND_SEED_IN_BIT));
  // Port F outputs (none)

#elif defined(AB_DEVKIT)

  // Port B INPUT_PULLUP or HIGH
  PORTB |= _BV(LEFT_BUTTON_BIT) | _BV(UP_BUTTON_BIT) | _BV(DOWN_BUTTON_BIT) |
           _BV(BLUE_LED_BIT);
  // Port B INPUT or LOW (none)
  // Port B inputs
  DDRB &= ~(_BV(LEFT_BUTTON_BIT) | _BV(UP_BUTTON_BIT) | _BV(DOWN_BUTTON_BIT));
  // Port B outputs
  DDRB |= _BV(BLUE_LED_BIT) | _BV(SPI_MOSI_BIT) | _BV(SPI_SCK_BIT);

  // Port C INPUT_PULLUP or HIGH
  PORTC |= _BV(RIGHT_BUTTON_BIT);
  // Port C INPUT or LOW (none)
  // Port C inputs
  DDRC &= ~(_BV(RIGHT_BUTTON_BIT));
  // Port C outputs (none)

  // Port D INPUT_PULLUP or HIGH
  PORTD |= _BV(CS_BIT);
  // Port D INPUT or LOW
  PORTD &= ~(_BV(RST_BIT));
  // Port D inputs (none)
  // Port D outputs
  DDRD |= _BV(RST_BIT) | _BV(CS_BIT) | _BV(DC_BIT);

  // Port E (none)

  // Port F INPUT_PULLUP or HIGH
  PORTF |= _BV(A_BUTTON_BIT) | _BV(B_BUTTON_BIT);
  // Port F INPUT or LOW
  PORTF &= ~(_BV(RAND_SEED_IN_BIT));
  // Port F inputs
  DDRF &= ~(_BV(A_BUTTON_BIT) | _BV(B_BUTTON_BIT) | _BV(RAND_SEED_IN_BIT));
  // Port F outputs (none)
  // Speaker: Not set here. Controlled by audio class

#endif
}

void Arduboy2Core::bootOLED()
{
  // reset the display
  delayShort(5); // reset pin should be low here. let it stay low a while
  bitSet(RST_PORT, RST_BIT); // set high to come out of reset
  delayShort(5); // wait a while

  // select the display (permanently, since nothing else is using SPI)
  bitClear(CS_PORT, CS_BIT);

  // run our customized boot-up command sequence against the
  // OLED to initialize it properly for Arduboy
  LCDCommandMode();
  for (uint8_t i = 0; i < sizeof(lcdBootProgram); i++) {
    SPItransfer(pgm_read_byte(lcdBootProgram + i));
  }
  LCDDataMode();
}
/* replaced by define macro compiling to single instruction
inline void Arduboy2Core::LCDDataMode()
{
  bitSet(DC_PORT, DC_BIT);
}
*//* replaced by define macro compiling to single instruction
void Arduboy2Core::LCDCommandMode()
{
  bitClear(DC_PORT, DC_BIT);
}
*/
// Initialize the SPI interface for the display
void Arduboy2Core::bootSPI()
{
// master, mode 0, MSB first, CPU clock / 2 (8MHz)
  SPCR = _BV(SPE) | _BV(MSTR);
  SPSR = _BV(SPI2X);
}

// Write to the SPI bus (MOSI pin)
void Arduboy2Core::SPItransfer(uint8_t data)
{
  SPDR = data;
  /*
   * The following NOP introduces a small delay that can prevent the wait
   * loop form iterating when running at the maximum speed. This gives
   * about 10% more speed, even if it seems counter-intuitive. At lower
   * speeds it is unnoticed.
   */
  asm volatile("nop");
  while (!(SPSR & _BV(SPIF))) { } // wait
}

void Arduboy2Core::safeMode()
{
  if (buttonsState() == UP_BUTTON)
  {
    digitalWriteRGB(RED_LED, RGB_ON);

    // prevent the bootloader magic number from being overwritten by timer 0
    // when a timer variable overlaps the magic number location
    power_timer0_disable();

    while (true) { }
  }
}


/* Power Management */

void Arduboy2Core::idle()
{
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();
}

void Arduboy2Core::bootPowerSaving()
{
  // disable Two Wire Interface (I2C) and the ADC
  PRR0 = _BV(PRTWI) | _BV(PRADC);
  // disable USART1
  PRR1 = _BV(PRUSART1);
  // All other bits will be written with 0 so will be enabled
}

// Shut down the display
void Arduboy2Core::displayOff()
{
  LCDCommandMode();
  SPItransfer(0xAE); // display off
  SPItransfer(0x8D); // charge pump:
  SPItransfer(0x10); //   disable
  delayShort(250);
  bitClear(RST_PORT, RST_BIT); // set display reset pin low (reset state)
}

// Restart the display after a displayOff()
void Arduboy2Core::displayOn()
{
  bootOLED();
}

uint8_t Arduboy2Core::width() { return WIDTH; }

uint8_t Arduboy2Core::height() { return HEIGHT; }


/* Drawing */

void Arduboy2Core::paint8Pixels(uint8_t pixels)
{
  SPItransfer(pixels);
}

void Arduboy2Core::paintScreen(const uint8_t *image)
{
#if defined OLED_SH1106	
  for (uint8_t i = 0; i < HEIGHT / 8; i++)
  {
  	LCDCommandMode();
  	SPDR = (OLED_SET_PAGE_ADDRESS + i);
	while (!(SPSR & _BV(SPIF)));
  	SPDR = (OLED_SET_COLUMN_ADDRESS_HI); // we only need to reset hi nibble to 0
	while (!(SPSR & _BV(SPIF)));
  	LCDDataMode();
  	for (uint8_t j = WIDTH; j > 0; j--)
      {
  		SPDR = pgm_read_byte(*(image++));
		while (!(SPSR & _BV(SPIF)));
      }
  }
#else //default OLED SSD1306
  for (int i = 0; i < (HEIGHT*WIDTH)/8; i++)
  {
    SPItransfer(pgm_read_byte(image + i));
  }
#endif  
}

// paint from a memory buffer, this should be FAST as it's likely what
// will be used by any buffer based subclass
void Arduboy2Core::paintScreen(uint8_t image[], bool clear)
{
#if defined OLED_SH1106
  for (uint8_t i = 0; i < HEIGHT / 8; i++)
  {
  	LCDCommandMode();
  	SPDR = (OLED_SET_PAGE_ADDRESS + i);
	while (!(SPSR & _BV(SPIF)));
  	SPDR = (OLED_SET_COLUMN_ADDRESS_HI); // we only need to reset hi nibble to 0
	while (!(SPSR & _BV(SPIF)));
  	LCDDataMode();
  	for (uint8_t j = WIDTH; j > 0; j--)
  	{
  		SPDR = *(image);
  		if (clear) *(image) = 0;
		*(image++);
		while (!(SPSR & _BV(SPIF)));
  	}
  }	
#else	
  uint8_t c;
  int i = 0;

  if (clear)
  {
    SPDR = image[i]; // set the first SPI data byte to get things started
    image[i++] = 0;  // clear the first image byte
  }
  else
    SPDR = image[i++];

  // the code to iterate the loop and get the next byte from the buffer is
  // executed while the previous byte is being sent out by the SPI controller
  while (i < (HEIGHT * WIDTH) / 8)
  {
    // get the next byte. It's put in a local variable so it can be sent as
    // as soon as possible after the sending of the previous byte has completed
    if (clear)
    {
      c = image[i];
      // clear the byte in the image buffer
      image[i++] = 0;
    }
    else
      c = image[i++];

    while (!(SPSR & _BV(SPIF))) { } // wait for the previous byte to be sent

    // put the next byte in the SPI data register. The SPI controller will
    // clock it out while the loop continues and gets the next byte ready
    SPDR = c;
  }
  while (!(SPSR & _BV(SPIF))) { } // wait for the last byte to be sent
  #endif
}

void Arduboy2Core::blank()
{
#if defined OLED_SH1106
  for (uint8_t i = 0; i < HEIGHT / 8; i++)
  {
  	LCDCommandMode();
  	SPDR = (0xB0 + i); 				//Select OLED PAGE
	while (!(SPSR & _BV(SPIF)));
  	SPDR = (0x10);     				//Select COLUMN address high nibble of column address
	while (!(SPSR & _BV(SPIF)));
  	LCDDataMode();
  	for (uint8_t j = WIDTH; j > 0; j--)
  	{
      SPDR = 0;
      while (!(SPSR & _BV(SPIF)));
  	}
  }	
#else	
  for (int i = 0; i < (HEIGHT*WIDTH)/8; i++)
    SPItransfer(0x00);
#endif
}

void Arduboy2Core::sendLCDCommand(uint8_t command)
{
  LCDCommandMode();
  SPItransfer(command);
  LCDDataMode();
}

// invert the display or set to normal
// when inverted, a pixel set to 0 will be on
void Arduboy2Core::invert(bool inverse)
{
  sendLCDCommand(inverse ? OLED_PIXELS_INVERTED : OLED_PIXELS_NORMAL);
}

// turn all display pixels on, ignoring buffer contents
// or set to normal buffer display
void Arduboy2Core::allPixelsOn(bool on)
{
  sendLCDCommand(on ? OLED_ALL_PIXELS_ON : OLED_PIXELS_FROM_RAM);
}

// flip the display vertically or set to normal
void Arduboy2Core::flipVertical(bool flipped)
{
  sendLCDCommand(flipped ? OLED_VERTICAL_FLIPPED : OLED_VERTICAL_NORMAL);
}

// flip the display horizontally or set to normal
void Arduboy2Core::flipHorizontal(bool flipped)
{
  sendLCDCommand(flipped ? OLED_HORIZ_FLIPPED : OLED_HORIZ_NORMAL);
}

/* RGB LED */

void Arduboy2Core::setRGBled(uint8_t red, uint8_t green, uint8_t blue)
{
#ifdef ARDUBOY_10 // RGB, all the pretty colors
  // inversion is necessary because these are common annode LEDs
  analogWrite(RED_LED, 255 - red);
  analogWrite(GREEN_LED, 255 - green);
  analogWrite(BLUE_LED, 255 - blue);
#elif defined(AB_DEVKIT)
  // only blue on DevKit, which is not PWM capable
  (void)red;    // parameter unused
  (void)green;  // parameter unused
  bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, blue ? RGB_ON : RGB_OFF);
#endif
}

void Arduboy2Core::digitalWriteRGB(uint8_t red, uint8_t green, uint8_t blue)
{
#ifdef ARDUBOY_10
  bitWrite(RED_LED_PORT, RED_LED_BIT, red);
  bitWrite(GREEN_LED_PORT, GREEN_LED_BIT, green);
  bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, blue);
#elif defined(AB_DEVKIT)
  // only blue on DevKit
  (void)red;    // parameter unused
  (void)green;  // parameter unused
  bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, blue);
#endif
}

void Arduboy2Core::digitalWriteRGB(uint8_t color, uint8_t val)
{
#ifdef ARDUBOY_10
  if (color == RED_LED)
  {
    bitWrite(RED_LED_PORT, RED_LED_BIT, val);
  }
  else if (color == GREEN_LED)
  {
    bitWrite(GREEN_LED_PORT, GREEN_LED_BIT, val);
  }
  else if (color == BLUE_LED)
  {
    bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, val);
  }
#elif defined(AB_DEVKIT)
  // only blue on DevKit
  if (color == BLUE_LED)
  {
    bitWrite(BLUE_LED_PORT, BLUE_LED_BIT, val);
  }
#endif
}

/* Buttons */

uint8_t Arduboy2Core::buttonsState()
{
  uint8_t buttons;

  // using ports here is ~100 bytes smaller than digitalRead()
#ifdef AB_DEVKIT
  // down, left, up
  buttons = ((~PINB) & B01110000);
  // right button
  //buttons = buttons | (((~PINC) & B01000000) >> 4);
  if ((PINC & B01000000) == 0) buttons |= 0x04; //compiles to shorter and faster code
  // A and B
  //buttons = buttons | (((~PINF) & B11000000) >> 6);
  if ((PINF & B10000000) == 0) buttons |= 0x02; //compiles to shorter and faster code
  if ((PINF & B01000000) == 0) buttons |= 0x01; 
#elif defined(ARDUBOY_10)
  // up, right, left, down
  buttons = ((~PINF) & B11110000);
  // A (left)
  //buttons = buttons | (((~PINE) & B01000000) >> 3);
  if ((PINE & B01000000) == 0) {buttons |= 0x08;} //compiles to shorter and faster code
  // B (right)
  // buttons = buttons | (((~PINB) & B00010000) >> 2);
  if ((PINB & B00010000) == 0) {buttons |= 0x04;} //compiles to shorter and faster code
#endif
#if defined ENABLE_BOOTLOADER_KEYS
  //bootloader button combo
  if (buttons == (LEFT_BUTTON | UP_BUTTON | A_BUTTON | B_BUTTON))
  { cli();
	//set magic boot key
    *(uint8_t *)0x0800 = 0x77;
	*(uint8_t *)0x0801 = 0x77;
	//enable and trigger watchdog by timeout
	wdt_enable(WDTO_15MS); 
	for (;;);
  }
#endif
  return buttons;
}

// delay in ms with 16 bit duration
void Arduboy2Core::delayShort(uint16_t ms)
{
  delay((unsigned long) ms);
}

