
#include <string.h>
#include "esp_log.h"
#include "ButtonManager.h"
#include "LcdManager.h"

static const char *TAG = "Main";

//====================
// Button Callbacks
//====================

void ClickCb( void )
	{
	LcdWriteRow( 0, "Single click" );
	}

void DoubleCb( void )
	{
	LcdWriteRow( 0, "Double click" );
	}

void PressCb( void )
	{
	LcdWriteRow( 0, "Long press..." );
	}

void ReleaseCb( void )
	{
	LcdWriteRow( 0, "Button released" );
	}

//====================
// Main
//====================

void app_main( void )
	{
	ESP_LOGI( TAG, "Initializing i2c" );
	InitializeI2c( );
	ESP_LOGI( TAG, "Scanning i2c for devices" );
	ScanI2c( );
	ESP_LOGI( TAG, "Initializing lcd display" );
	LcdInitialize( );

	ESP_LOGI( TAG, "Writing hello message" );
	LcdWriteRow( 0, "Hello" );
	LcdWriteRow( 1, "Johnnie" );
	//SetLcdCursor( 1, 0 );

	int gpioPin = 5;
	//ESP_LOGI( TAG, "main() calling ButtonManagerTestMain() gpio: %d", gpioPin );
	//ButtonManagerTestMain( gpioPin );
	ESP_LOGI( TAG, "Registering push button on gpio: %d", gpioPin );
	RegisterButton( gpioPin, ClickCb, DoubleCb, PressCb, ReleaseCb );
	ESP_LOGI( TAG, "Push the button, observe click events, go nuts" );
	}

