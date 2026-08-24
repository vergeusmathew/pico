#include "pico/stdlib.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "Gen1Hz.pio.h"

// Wrap these functions in an extern "C" block inside tft_spi.cpp 
// so the C++ compiler exports simple, clean function names for lcd.cpp to link with
extern "C" {
//https://github.com/raspberrypi/pico-micropython-examples/blob/master/pio/pio_1hz.py
	uint Generate1HzOnLED(PIO pio, int gpio)
	{
		static const float pio_freq = 2000; //Hz
		uint offset = pio_add_program(pio, &Gen1Hz_program);    //Gen1Hz.pio
		uint sm = pio_claim_unused_sm(pio, true);
		pio_gpio_init(pio, gpio);
		
		pio_sm_config c = Gen1Hz_program_get_default_config(
													 offset);
		float div = (float)clock_get_hz(clk_sys)/pio_freq;

		sm_config_set_clkdiv(&c, div);
		sm_config_set_set_pins(&c, gpio, 1);
		pio_sm_set_consecutive_pindirs(pio,sm,gpio,1,true);
		pio_sm_init(pio, sm, offset, &c);
		pio_sm_set_enabled(pio, sm, true);
		
		return sm;
	}
} // End of extern "C"
