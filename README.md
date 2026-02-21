# EspLcdAndButton01
Freenove wroom esp32s2 reading a push button and writing to lcd display 1602.

Push button connected from gpio pin 5 to ground.  Lcd display 1602 connected to 5v, ground, and gpio pins 13 and 14, for CL and DA respectively.

A button manager and lcd manager are implemented as separate idf components.  (Not all munged into main, as with prior assignments.)  Main receives button click events from the button manager, and writes corresponding messages to lcd display.

