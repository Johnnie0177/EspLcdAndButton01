#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#define STATE_IDLE			0
#define STATE_FIRST_DOWN	1
#define STATE_FIRST_UP		2
#define STATE_SECOND_DOWN	3
#define STATE_SECOND_UP		4
#define STATE_LONG_PRESS	5
#define STATE_RELEASE		6

void RegisterButton(
	int gpioPin,
	void (*clickCb)(void),
	void (*doubleCb)(void),
	void (*pressCb)(void),
	void (*releaseCb)(void) );

void ButtonManagerTestMain( int gpioPin );

#endif
