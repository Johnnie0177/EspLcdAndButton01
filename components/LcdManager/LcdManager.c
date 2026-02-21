
#include <unistd.h>
#include "driver/i2c.h"
#include "LcdManager.h"

#define TIMEOUT_MS		1000
#define DELAY_MS		1000

#define I2C_PORT 		I2C_NUM_0
// Lcd 1602 resides on i2c address 0x27 or 0x3F, depending on the board model.
#define I2C_ADDRESS 	0x27
#define SDA_PIN 		14
#define SCL_PIN 		13

// i2c bit		7	6	5	4	3	2	1	0
// LCD pin		7	6	5	4	LED	E	R/W	RS

//====================
// Functions
//====================

// Public Interface
void LcdInitialize( void );
void LcdWriteRow( int rowNum, const char *rowStr );

// Lcd Communication
void LcdSetCursor( u_int8_t y, u_int8_t x );
void SendLcdCommand( char cmd );
void SendLcdChar( char ch );

// I2c Communication
void ScanI2c( void );
void InitializeI2c( void );

//====================
// Lcd Display
//====================

static int NumRows = 2;
static int RowWidth = 16;

void LcdWriteRow( int rowNum, const char *rowStr )
	{
	// Assign display row to write, default to top row if input is out of range.
	int displayRow = rowNum;
	if( displayRow < 0 || displayRow >= NumRows )
		{
		displayRow = 0;
		}

	// Write string into specified row, padded with spaces to clear prior contents.
	LcdSetCursor( (u_int8_t)displayRow, 0 );
	int i = 0;
	for( ; i < RowWidth; i++ )
		{
		if( rowStr[ i ] == 0 ) break;
		SendLcdChar( rowStr[ i ]);
		}
	for( ; i < RowWidth; i++ )
		{
		SendLcdChar( ' ' );
		}
	}

// Commands constructed from bit meaning:
// pin			7	6	5	4	3	2	1	0
// func set		0	0	1	DL	N	F	*	*
// DL	-> 4 pin -> 1
// N	-> display line 0 = 1 line, 1 = 2 lines
// F	-> f=0, 5x8

// pin			7	6	5	4	3	2	1	0
// func set		0	0	1	DL	N	F	*	*
// 0x20			0	0	1	0	0	0	0	0
// 0x28			0	0	1	0	1	0	0	0
// 0x30			0	0	1	1	0	0	0	0
// 0x80			1	0	0	0	0	0	0	0

#define CMD_4_BIT_MODE 0x20
#define CMD_2_LINES    0x28
#define CMD_INITIALIZE 0x30
#define CMD_SET_CURSOR 0x80

// pin			7	6	5	4	3	2	1	0
// display		0	0	0	0	1	D	C	B
// D = display	On/Off
// C = cursor	On/Off
// B = Blink	On/Off

// pin			7	6	5	4	3	2	1	0
// display		0	0	0	0	1	D	C	B
// 0x01			0	0	0	0	0	0	0	1
// 0x06			0	0	0	0	0	1	1	0
// 0x08			0	0	0	0	1	0	0	0
// 0x0C			0	0	0	0	1	1	0	0
// 0x0F			0	0	0	0	1	1	1	1

#define CMD_CLEAR_DISPLAY 0x01
#define CMD_ENTRY_MODE    0x06
#define CMD_DISPLAY_OFF   0x08
#define CMD_DISPLAY_ON    0x0C
#define CMD_ON_AND_BLINK  0x0F

void LcdInitialize( void )
	{
	// 4 bit initialization
	usleep( 50000 ); // wait for >40ms
	SendLcdCommand( CMD_INITIALIZE );
	usleep( 4500 ); // wait for >4.1ms
	SendLcdCommand( CMD_INITIALIZE );
	usleep( 200 ); // wait for >100us
	SendLcdCommand( CMD_INITIALIZE );
	usleep( 200 );
	SendLcdCommand( CMD_4_BIT_MODE );
	usleep( 200 );

	// Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	SendLcdCommand( CMD_2_LINES );
	usleep( 1000 );

	// Display on/off control --> D=0, C=0, B=0 ---> display off
	SendLcdCommand( CMD_DISPLAY_OFF );
	usleep( 1000 );

	// Clear display => 0x01
	SendLcdCommand( CMD_CLEAR_DISPLAY );
	usleep( 1000 );
	usleep( 1000 );

	// Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	SendLcdCommand( CMD_ENTRY_MODE );
	usleep( 1000 );

	// Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
	//SendLcdCommand( CMD_DISPLAY_ON );

	// Show cursor, blink
	SendLcdCommand( CMD_ON_AND_BLINK );
	usleep( 2000 );
	}

void LcdSetCursor( u_int8_t y, u_int8_t x )
	{
	//First Line: Addresses 0x00 to 0x0F
	//Second Line: Addresses 0x40 to 0x4F
	u_int8_t ddram_addr = 0x40 * y + x;
	SendLcdCommand( CMD_SET_CURSOR | ddram_addr );
	usleep( 40 );
	}

//====================
// Lcd Communication
//====================

void SendLcdCommand( char cmd )
	{
	esp_err_t err;
	char data_u, data_l;
	uint8_t data_t[ 4 ];
	data_u = ( cmd & 0xF0 );
	data_l = (( cmd << 4 ) & 0xF0 );
	data_t[ 0 ] = data_u | 0x0C; // en=1, rs=0, 0xC = b1100
	data_t[ 1 ] = data_u | 0x08; // en=0, rs=0, 0x8 = b1000
	data_t[ 2 ] = data_l | 0x0C; // en=1, rs=0, 0xC = b1100
	data_t[ 3 ] = data_l | 0x08; // en=0, rs=0, 0x8 = b1000

	err= i2c_master_write_to_device( I2C_PORT, I2C_ADDRESS, data_t, 4, 1000 );
	if( err != 0 )
		{
		printf( "Error %d in command 0x%02X\n", err, cmd );
		}
	}

void SendLcdChar( char ch )
	{
	esp_err_t err;
	char data_u, data_l;
	uint8_t data_t[ 4 ];
	data_u = ( ch & 0xF0 );
	data_l = (( ch << 4 ) & 0xF0 );
	data_t[ 0 ] = data_u | 0x0D; // en=1, rs=1, 0xD = b1101
	data_t[ 1 ] = data_u | 0x09; // en=0, rs=1, 0x9 = b1001
	data_t[ 2 ] = data_l | 0x0D; // en=1, rs=1, 0xD = b1101
	data_t[ 3 ] = data_l | 0x09; // en=0, rs=1, 0x9 = b1001

	err= i2c_master_write_to_device( I2C_PORT, I2C_ADDRESS, data_t, 4, 1000 );
	if( err != 0 )
		{
		printf( "Error %d in char 0x%02X\n", err, ch );
		}
	}

//====================
// I2c Communication
//====================

void InitializeI2c( void )
	{
	printf( "Initializing i2c communication, da and cl on pins %d and %d\n", SDA_PIN, SCL_PIN );
	i2c_config_t conf =
		{
		.mode = I2C_MODE_MASTER,
		.sda_io_num = SDA_PIN,
		.scl_io_num = SCL_PIN,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 400000,
		};
	i2c_param_config( I2C_PORT, &conf );
	ESP_ERROR_CHECK( i2c_driver_install( I2C_PORT, I2C_MODE_MASTER, 0, 0, 0 ));
	}

void ScanI2c( void )
	{
	int ret;
	uint8_t rx_data;
	printf( "Scanning i2c bus for devices:\n" );
	int addr = 0;
	printf( "\t0x%02X:", addr );
	while( 1 )
		{
		rx_data = 0;
		ret = i2c_master_read_from_device(
			I2C_PORT, addr, &rx_data, 1, TIMEOUT_MS / portTICK_PERIOD_MS );
		if( ret == 0 )
			{
			printf( " 0x%02X", addr );
			}
		else
			{
			printf( "    ." );
			}
		addr++;
		if( addr >= 0x7F )
			{
			break;
			}
		if( addr % 8 == 0 )
			{
			printf( "\n\t0x%02X:", addr );
			}
		}
	printf( "\n" );
	}

