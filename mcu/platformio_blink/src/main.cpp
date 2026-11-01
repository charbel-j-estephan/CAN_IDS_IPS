/* Blink both user LEDs on the STM32F407VET6 black board.
 *
 * The LEDs are on PA6 and PA7 and light when the pin is LOW. They take turns,
 * so whichever LED your board has will visibly blink. If neither does, your
 * board uses a different pin: change LED_A / LED_B.
 */
#include <Arduino.h>

#define LED_A PA6
#define LED_B PA7

void setup()
{
    pinMode(LED_A, OUTPUT);
    pinMode(LED_B, OUTPUT);
}

void loop()
{
    digitalWrite(LED_A, LOW);    // A on
    digitalWrite(LED_B, HIGH);   // B off
    delay(500);
    digitalWrite(LED_A, HIGH);
    digitalWrite(LED_B, LOW);
    delay(500);
}
