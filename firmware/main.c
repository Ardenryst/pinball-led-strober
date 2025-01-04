#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>

#include <util.h>

#define LED   B, 4
#define TX    B, 2
#define BTN   B, 3
static volatile uint8_t* pwm_channels[] = { &OCR1B, /*&OCR1A */};

// Source: https://www.mikrocontroller.net/topic/311610#3362049
int uart_putchar(char c, FILE *stream) {
  #define DEBUG_PIN B, 2

  #define DEBUG_HIGH  PORT(DEBUG_PIN) = 1
  #define DEBUG_LOW  PORT(DEBUG_PIN) = 0

  //Wartezeit in us 
  #define MINDELAY    104     // fuer 9600 bps
  // #define MINDELAY    26     // fuer 38400 bps
  // #define MINDELAY   417    // fuer 2400 bps ACHTUNG nicht mit _delay_us machtbar!!
                // zumindest nicht als einzelner aufruf!
  OUTPUT(DEBUG_PIN);
  PORT(DEBUG_PIN) = 1;

  PORT(DEBUG_PIN) = 0;
  _delay_us(MINDELAY);               // Start Bit

  for(uint8_t i=0; i<8; i++) {
    if(c & (0x01<<i)) {
      PORT(DEBUG_PIN) = 1;
    } else {
      PORT(DEBUG_PIN) = 0;
    }
    _delay_us(MINDELAY);
  }
  PORT(DEBUG_PIN) = 1;        // Stop Bit
  _delay_us(MINDELAY);

  return 0;
}

// Higher bit PWM sources:
//  - https://www.arduinoslovakia.eu/blog/2017/12/10-bitove-pwm-na-attiny85?lang=en
//  - http://www.technoblogy.com/show?1NGL

volatile int dac[SIZE(pwm_channels)];

void initPWM() {
 OCR1C = 255;
  // Timer/Counter1 doing PWM on OC1A (PB1)
  TCCR1 = 1 << PWM1A    // Pulse Width Modulator A Enable
          | 1 << COM1A0 // OC1x cleared on compare match. Set when TCNT1 = $00
          | 0 << CS10 | 0 << CS11 | 1 << CS12;  // TODO prüfen, ob man das so macht. Durch rumprobieren bei 250Hz PWM-Frew gelandet.
  TIMSK |= 1 << TOIE1; // Timer/Counter1 Overflow Interrupt Enable
  
  GTCCR = 1<<PWM1B | 1<<COM1B1; // What kind of shitty register naming is that?!
}

// 12bit DAC
// Overflow interrupt
ISR(TIMER1_OVF_vect) {
  static volatile int cycle = 0;
  static int rem[SIZE(pwm_channels)];
  for(int chan=0; chan<SIZE(pwm_channels); chan++) {
    int remain;
    if(cycle==0) {
      remain = dac[chan];
    } else {
      remain = rem[chan];
    }
    if(remain>=256) {
      *pwm_channels[chan] = 255; remain = remain - 256;
    } else {
      *pwm_channels[chan] = remain; remain = 0;
    }
    rem[chan] = remain;
  }
  cycle = (cycle+1) & 0x0F;
}

const uint16_t PROGMEM gamma_8b[] = {
  0,    0,    0,    0,    1,    1,    2,    3,    4,    6,    8,   10,   13,   16,   19,   24,
  28,   33,   39,   46,   53,   60,   69,   78,   88,   98,  110,  122,  135,  149,  164,  179,
  196,  214,  232,  252,  273,  295,  317,  341,  366,  393,  420,  449,  478,  510,  542,  575,
  610,  647,  684,  723,  764,  806,  849,  894,  940,  988, 1037, 1088, 1140, 1194, 1250, 1307,
  1366, 1427, 1489, 1553, 1619, 1686, 1756, 1827, 1900, 1975, 2051, 2130, 2210, 2293, 2377, 2463,
  2552, 2642, 2734, 2829, 2925, 3024, 3124, 3227, 3332, 3439, 3548, 3660, 3774, 3890, 4008, 4128,
  4251, 4376, 4504, 4634, 4766, 4901, 5038, 5177, 5319, 5464, 5611, 5760, 5912, 6067, 6224, 6384,
  6546, 6711, 6879, 7049, 7222, 7397, 7576, 7757, 7941, 8128, 8317, 8509, 8704, 8902, 9103, 9307,
  9514, 9723, 9936, 10151, 10370, 10591, 10816, 11043, 11274, 11507, 11744, 11984, 12227, 12473, 12722, 12975,
  13230, 13489, 13751, 14017, 14285, 14557, 14833, 15111, 15393, 15678, 15967, 16259, 16554, 16853, 17155, 17461,
  17770, 18083, 18399, 18719, 19042, 19369, 19700, 20034, 20372, 20713, 21058, 21407, 21759, 22115, 22475, 22838,
  23206, 23577, 23952, 24330, 24713, 25099, 25489, 25884, 26282, 26683, 27089, 27499, 27913, 28330, 28752, 29178,
  29608, 30041, 30479, 30921, 31367, 31818, 32272, 32730, 33193, 33660, 34131, 34606, 35085, 35569, 36057, 36549,
  37046, 37547, 38052, 38561, 39075, 39593, 40116, 40643, 41175, 41711, 42251, 42796, 43346, 43899, 44458, 45021,
  45588, 46161, 46737, 47319, 47905, 48495, 49091, 49691, 50295, 50905, 51519, 52138, 52761, 53390, 54023, 54661,
  55303, 55951, 56604, 57261, 57923, 58590, 59262, 59939, 60621, 61308, 62000, 62697, 63399, 64106, 64818, 65535
};

// more or less copy pasted and updated, with no real idea what I was doing. Seems to work OK though...
uint16_t correctGamma12(uint16_t value) {
  uint16_t y = pgm_read_word(&gamma_8b[value / 16]);
  uint16_t z1 = (value / 16 == 0) ? 0 : pgm_read_word(&gamma_8b[value / 16 - 1]);
  uint16_t z = (y - z1) / 16 * (value % 16 + 1) + z1;

  return z >> 6;
}

const uint8_t exptable5[32] PROGMEM =
  {130, 133, 136, 139, 142, 145, 148, 151, 155, 158, 161, 165, 169, 172, 176, 180,
  184, 188, 192, 196, 201, 205, 210, 214, 219, 224, 229, 234, 239, 244, 249, 255};

/*inline*/ uint16_t expvalue9(const uint16_t linear)
{/* Returns the exponential value (approx. 1.0219^x).                          *
  * argument: 9 bit unsigned (0..511)  return: 16 bit unsigned (1..65280)    */
  // look up exponential
  uint16_t exp = pgm_read_byte(&exptable5[ linear % 32 ]) << 8;
  // scale magnitude
  return exp >> (15 - linear / 32);
}

// #define BASE_BRIGHTNESS 20
// // #define MAX_BRIGHTNESS 100
// #define BRIGHTNESS_VARIANCE (100-BASE_BRIGHTNESS)

// Basishelligkeit, bezogen auf max. Umgebungshelligkeit
#define BASE_BRIGHTNESS 3000
// +/- var/2 auf Basishelligkeit
#define BRIGHTNESS_VARIANCE 2200
// Faktor für Überhöhung der Bezugshelligkeit aus der Umgebung
#define ELEVATION 1.3

#define MIN_FADE_TIME 50
#define FADE_TIME_VARIANCE 100

#define MIN_HOLD_TIME 100
#define HOLD_TIME_VARIANCE 200

struct Led {
  volatile int *dac;
//   const uint16_t MIN_BRIGHTNESS;
  // const uint16_t MAX_BRIGHTNESS;
  struct Timer fade_timer;
  uint16_t start;
  uint16_t goal;
} leds[] = {
  {.dac = &dac[0]},
};
inline void fade(struct Led *led, uint16_t from, uint16_t to, uint16_t duration) {
  resetTimer(&led->fade_timer);
  led->fade_timer.cycle = duration;
  led->start = from;
  led->goal = to;
}

// struct {
//   bool is_pushed;
//   //uint8_t samples;
//   struct Timer sample_timer;
// } button = {false, 0, {1}};

int main() {
  initMillis();
  initPWM();
  
  FILE soft_uart = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &soft_uart;
  
  OUTPUT(LED);
  OUTPUT(TX);
  INPUT_PULLUP(BTN);
  
  enum States {INIT, STANDBY, FLASH};
  struct StateMachine fsm;
  initFSM(&fsm, INIT);
  
  // #define MAX_BRIGHTNESS 4096
  // #define MIN_BRIGHTNESS 0
  // #define DURATION 10000
  
  // struct Timer timer = {.cycle=DURATION};
  // bool dir = true;
  while(true) {
    // if(checkAndResetTimer(&timer)) {
    //   dir = !dir;
    //   fade(&leds[0], dir ? MAX_BRIGHTNESS : MIN_BRIGHTNESS, dir ? MIN_BRIGHTNESS: MAX_BRIGHTNESS, DURATION);
    // }
    
//     if(checkAndResetTimer(&button.sample_timer)) {
//       if(PIN(BTN)==0) {
//         if(button.samples < 10) {
//           button.samples++;
//         } else {
//           if(!button.is_pushed) {
//             transit(&fsm, fsm.current_state == ON ? OFF : ON);
//             button.is_pushed = true;
//           }
//         }
//       } else {
//         button.samples = 0;
//         button.is_pushed = false;
//       }
//     }
//    

    struct Led *led = &leds[0];
    
    switch(updateFSM(&fsm)) {
      case INIT:
        // fade(led, 0, 0, 1000);
        transit(&fsm, STANDBY);
        break;
      case STANDBY:
        if(onEnter(&fsm)) {
          fade(led, led->goal, 0, 1000);
        }
        if(PIN(BTN)==0) {
          transit(&fsm, FLASH);
        }
        break;
      case FLASH: {
        static bool cooldown;
        static struct Timer timer = {.cycle=3000};
        if(onEnter(&fsm)) {
          fade(led, 0, BASE_BRIGHTNESS+BRIGHTNESS_VARIANCE/2, 10);
          cooldown = false;
          resetTimer(&timer);
        }
        if(checkTimer(&timer)) {
          transit(&fsm, STANDBY);
        }
        if(checkAndResetTimer(&led->fade_timer)) {
          if(cooldown) { // Helligkeit eine Weile beibehalten
            cooldown = false;
            uint16_t duration = MIN_HOLD_TIME+(rand() / (RAND_MAX / HOLD_TIME_VARIANCE + 1));
            fade(led, led->goal, led->goal, duration);
          } else {              // Neue Helligkeit setzen
            cooldown = true;
            // Halbe Helligkeit + Zufallswert mit +- Varianz/2
            int16_t to = BASE_BRIGHTNESS+(-BRIGHTNESS_VARIANCE/2 + (rand() / (RAND_MAX / BRIGHTNESS_VARIANCE + 1)));
            uint16_t duration = MIN_FADE_TIME+(rand() / (RAND_MAX / FADE_TIME_VARIANCE + 1));
//             to = 512;
//             duration = 500;
            fade(led, led->goal, to, duration);
          }
        }
        break;
      }
    }
    
    // update PWM channels
    for(unsigned int index=0; index<SIZE(leds); index++) {
      struct Led *led = &leds[index];
      
      uint16_t brightness = ((int16_t)0)+linearInterpolate(led->start, led->goal, &led->fade_timer);
      brightness = correctGamma12(brightness);
      
      ATOMIC_BLOCK(ATOMIC_FORCEON) {
        *led->dac = brightness;
      }
    }
    
//     FOR_ALL_CHANNELS {
//       #define scale(val, fac) (((uint32_t)val)*fac*4096)/4096;
//       uint16_t environ = scale(ldr_adc, ELEVATION);
//       // Mindesthelligkeit begrenzen
//       environ = min(max(environ, 500), 4096);
// //       environ = 4096;
//       
//       uint16_t brightness = ((int16_t)0)+linearInterpolate(led->start, led->goal, &led->fade_timer);
//       uint16_t led_max = (((uint32_t)environ)*(led->MAX_BRIGHTNESS))/4096;
//       uint16_t led_min = 0; //fsm.last_state == OFF ? 0 : (((uint32_t)ldr_adc)*(led->MIN_BRIGHTNESS))/1023;
//       brightness = (((uint32_t)brightness)*(led_max-led_min))/4096+led_min;
// //       if(index==0) printf("Brightness: %d\n", environ);
//       
//       ATOMIC_BLOCK(ATOMIC_FORCEON) {
//         *led->dac = brightness; //correctGamma(brightness); ldr_adc; //
//       }
//     }
  }
}
