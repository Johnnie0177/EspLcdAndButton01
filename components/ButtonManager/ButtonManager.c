
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ButtonManager.h"

/*
ButtonManager
Monitors push button state, interprets click events as single click, double click, long press, or long press release, then calls callback functions accordingly.
*/

//========================
// Internal declarations
//========================

static const char *TAG = "ButtonManager";
static void ConfigurePulldownButton( int gpioPin );
static int GpioPin = -1;
static void (*ClickCb)(void) = NULL;
static void (*DoubleCb)(void) = NULL;
static void (*PressCb)(void) = NULL;
static void (*ReleaseCb)(void) = NULL;

//====================
// Public Interface
//====================

// Assign callback functions for a push button connected to the specified gpio pin.
void RegisterButton(
int gpioPin,
void (*clickCb)(void),
void (*doubleCb)(void),
void (*pressCb)(void),
void (*releaseCb)(void) )
	{
	// Assign callbacks.
	ClickCb = clickCb;
	DoubleCb = doubleCb;
	PressCb = pressCb;
	ReleaseCb = releaseCb;

	// Configure the specified gpio pin as input with pullup resistor, expecting a button press that pulls the input to ground.
	ConfigurePulldownButton( gpioPin );
	}

//====================
// Internal functions
//====================

// These declarations need to report their static-ness, but not IRAM_ATTR attribution.
static QueueHandle_t InterruptQueue;
static void ButtonEventTask( void *params );
static void IsrHandler( void *args );
static void ButtonPollingTask( void *params );
static void ButtonAction( int newButtonState );

// Configure the specified gpio pin as input with pullup resistor, expecting a button press that pulls the input to ground.
static void ConfigurePulldownButton( int gpioPin )
	{
	// Configure gpio input.
	GpioPin = gpioPin;
	ESP_LOGI( TAG, "ConfigureButton(): resetting gpio pin %d to input", GpioPin );
	gpio_set_direction( GpioPin, GPIO_MODE_INPUT );
	gpio_pulldown_dis( GpioPin );
	gpio_pullup_en( GpioPin );

	// Assign isr handler.
	// Button responds to presses and releases, via a single isr triggered on both edges.
	gpio_set_intr_type( GpioPin, GPIO_INTR_ANYEDGE );
	InterruptQueue = xQueueCreate( 10, sizeof( int ));
	xTaskCreate( ButtonEventTask, "ButtonEventTask", 2048, NULL, 1, NULL );
	gpio_install_isr_service( 0 );
	gpio_isr_handler_add( GpioPin, IsrHandler, (void *)GpioPin );

	// Make button state poller.
	xTaskCreate( ButtonPollingTask, "ButtonPollingTask", 2048, NULL, 1, NULL );
	}

// Interrupt routine responds to button state changes by (quickly) queuing events, for a task to read and process later.
static void IRAM_ATTR IsrHandler( void *args )
	{
	int gpioPin = (int)args;
	xQueueSendFromISR( InterruptQueue, &gpioPin, NULL );
	}

// Task to read button events from a queue, debounce by filtering dups, and forward actions to an interpreter.
static void ButtonEventTask( void *params )
	{
	int gpioPin;
	int priorButtonState = 1;
	int newButtonState = 1;
	while( true )
		{
		// Wait for queue event, loop back and wait again on timeout.
		if( !xQueueReceive( InterruptQueue, &gpioPin, portMAX_DELAY )) continue;

		// Got a queue event, debounce by filtering out button state dups.
		newButtonState = gpio_get_level( GpioPin );
		if( newButtonState == priorButtonState ) continue;

		// Update button state interpreter.
		ButtonAction( newButtonState );
		priorButtonState = newButtonState;
		}
	}

// Stopwatch used by button state manager and polling task.
static struct timeval stopwatchTimeval;
static unsigned long getCurrentTimeMs( void )
	{
	gettimeofday( &stopwatchTimeval, NULL );
	return( 1000 * stopwatchTimeval.tv_sec + stopwatchTimeval.tv_usec / 1000 );
	}

// Button state manager, captures event times, advances state, does not call callbacks.
static int cmState = STATE_IDLE;
static unsigned long pressTimeMs = 0;
static unsigned long releaseTimeMs = 0;
static void ButtonAction( int newButtonState )
	{
	if( newButtonState )
		{
		// Button released.
		releaseTimeMs = getCurrentTimeMs( );
		switch( cmState )
			{
			case STATE_FIRST_DOWN:
				cmState = STATE_FIRST_UP;
				break;
			case STATE_SECOND_DOWN:
				cmState = STATE_SECOND_UP;
				break;
			case STATE_LONG_PRESS:
				cmState = STATE_RELEASE;
				break;
			}
		}
	else
		{
		// Button pressed.
		pressTimeMs = getCurrentTimeMs( );
		switch( cmState )
			{
			case STATE_IDLE:
				cmState = STATE_FIRST_DOWN;
				break;
			case STATE_FIRST_UP:
				cmState = STATE_SECOND_DOWN;
				break;
			}
		}
	}

// Interpreter uses a stopwatch to measure press and release intervals.  Single clicks, double clicks, and long presses followed by eventual release, are determined from the event intervals.  Then appropriate callback functions are called.
static void ButtonPollingTask( void *params )
	{
	// Poll at approximately twice the pace of meaningful button state change intervals.
	int pollPeriodMs = 80;
	int pollPeriodTicks = pollPeriodMs / portTICK_PERIOD_MS;
	while( 1 )
		{
		vTaskDelay( pollPeriodTicks );
		unsigned long curTimeMs = getCurrentTimeMs( );
		switch( cmState )
			{
			case STATE_FIRST_DOWN:
			case STATE_SECOND_DOWN:
				// Button is down, check press time for long press.
				if( curTimeMs - pressTimeMs > 250 )
					{
					// Down long enough for a long press.
					cmState = STATE_LONG_PRESS;
					PressCb( );
					//printf( "--> ButtonPollingTask() new cmState %d\n", cmState );
					}
				// Else wait for release or next time check for long press.
				break;

			case STATE_FIRST_UP:
				// Button released from first click, check release time for single click.
				if( curTimeMs - releaseTimeMs > 150 )
					{
					// Released long enough to disqualify a double click.
					ClickCb( );
					cmState = STATE_IDLE;
					}
				// Else wait for press or next time check for single or double click.
				break;

			case STATE_SECOND_UP:
				// Button released from double click, no need to time check.
				DoubleCb( );
				cmState = STATE_IDLE;
				break;

			case STATE_RELEASE:
				// Button released from long press, no need to time check.
				ReleaseCb( );
				cmState = STATE_IDLE;
				break;
			}
		}
	}

