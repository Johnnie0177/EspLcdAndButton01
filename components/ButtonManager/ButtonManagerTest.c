
#include <stdio.h>
#include "esp_log.h"
#include "ButtonManager.h"

static const char *TAG = "ButtonTest";

// Trivial callback functions report themselves.
void Click( void )
	{
	ESP_LOGI( TAG, "Single click event" );
	}
void Double( void )
	{
	ESP_LOGI( TAG, "Double click event" );
	}
void LongPress( void )
	{
	ESP_LOGI( TAG, "Long press initiated" );
	}
void Release( void )
	{
	ESP_LOGI( TAG, "Long press released" );
	}

// Entry point to test the button manager, use for testing and as a reference implementation.  Do not use in conjunction with direct calls to the button manager, they'll collide.
void ButtonManagerTestMain( int gpioPin )
	{
	ESP_LOGI( TAG, "ButtonManagerTestMain() registering push button on gpio %d", gpioPin );
	RegisterButton( gpioPin, Click, Double, LongPress, Release );
	ESP_LOGI( TAG, "Press the button on gpio %d, events will be logged here", gpioPin );
	}

