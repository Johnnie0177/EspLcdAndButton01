#ifndef LCD1602_I2C_H
#define LCD1602_I2C_H

#include <unistd.h>

void LcdInitialize( void );
void LcdWriteRow( int rowNum, const char *rowStr );
void LcdSetCursor( u_int8_t y, u_int8_t x );

void InitializeI2c( void );
void ScanI2c( void );

#endif
