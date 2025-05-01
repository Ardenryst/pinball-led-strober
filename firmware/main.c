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

// ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓
// ↓ Animation configuration ↓
// ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓

// Base brightness, relative to max ambient brightness
#define BASE_BRIGHTNESS 3000
// +/- var to base brightness
#define BRIGHTNESS_VARIANCE 3125
// Total duration of the animation in milliseconds
#define ANIMATION_DURATION 3500
// Duration of the initial light flash in milliseconds
#define INITIAL_FLASH_DURATION 100
// Brightness for the initial flash
#define INITIAL_FLASH_BRIGHTNESS 10000
// Minimum runtime of the animation in milliseconds before it can be restarted
#define MIN_ANIMATION_RUN_TIME 800
// Minimum time for brightness changes (ms)
#define MIN_FADE_TIME 1
// Additional random time for brightness changes (ms)
#define FADE_TIME_VARIANCE 10
// Minimum time to hold a brightness level (ms)
#define MIN_HOLD_TIME 50
// Additional random time to hold a brightness level (ms)
#define HOLD_TIME_VARIANCE 200
// Maximum allowed brightness
#define BRIGHTNESS_LIMIT 12000
// Parameters for power surge simulation
#define FLICKER_PROBABILITY 25    // Probability of flickering (1-100)
#define MAX_BRIGHTNESS 6500       // Maximum brightness during power surges
#define BLACKOUT_PROBABILITY 5    // Reduced probability of short total blackout
#define BLACKOUT_DURATION 150     // Duration of total blackout (ms)

// ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑
// ↑ Animation configuration ↑
// ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑

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
  uint16_t y = pgm_read_word(&gamma_8b[value / 256]);
  uint16_t z1 = (value / 256 == 0) ? 0 : pgm_read_word(&gamma_8b[value / 256 - 1]);
  uint16_t z = (y - z1) / 256 * (value % 256 + 1) + z1;

  return z;
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

struct Led {
  volatile int *dac;
  struct Timer fade_timer;
  uint16_t start;
  uint16_t goal;
} leds[] = {
  {.dac = &dac[0]},
};
void fade(struct Led *led, uint16_t from, uint16_t to, uint16_t duration) {
  resetTimer(&led->fade_timer);
  led->fade_timer.cycle = duration;
  led->start = from;
  led->goal = to;
  if (led->goal > BRIGHTNESS_LIMIT) {
    led->goal = BRIGHTNESS_LIMIT;
  }
  if (led->start > BRIGHTNESS_LIMIT) {
    led->start = BRIGHTNESS_LIMIT;
  }
}

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

  while(true) {
    struct Led *led = &leds[0];
    
    switch(updateFSM(&fsm)) {
      case INIT:
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
        static struct Timer timer = {.cycle=ANIMATION_DURATION};  // Use of configurable duration
        static struct Timer blackout_timer = {0};
        static bool in_blackout = false;
        static bool initial_flash_done = false;
        static struct Timer initial_hold_timer = {.cycle=INITIAL_FLASH_DURATION}; // Configurable hold time for initial flash
        static bool button_released = true;
        static struct Timer button_debounce_timer = {.cycle=50}; // Debounce time
        static struct Timer animation_run_timer = {.cycle=MIN_ANIMATION_RUN_TIME}; // Timer for minimum runtime
        static bool can_restart = false;
        
        // Function to restart the animation
        void restartAnimation(void) {
          fade(led, led->goal, INITIAL_FLASH_BRIGHTNESS, 1);  // Immediate very bright flash
          initial_flash_done = false;
          cooldown = false;
          resetTimer(&timer);
          resetTimer(&initial_hold_timer);
          resetTimer(&animation_run_timer);
          can_restart = false;
          in_blackout = false;
        }
        
        if(onEnter(&fsm)) {
          // Immediate bright flash at the beginning
          restartAnimation();
          button_released = false; // Mark button as pressed at start
        }
        
        // Check if minimum runtime has been reached
        if(!can_restart && checkTimer(&animation_run_timer)) {
          can_restart = true;
        }
        
        // Monitor button status for restarts
        if(PIN(BTN) != 0) {
          // Button was released
          button_released = true;
          resetTimer(&button_debounce_timer);
        } else if(button_released && checkTimer(&button_debounce_timer) && can_restart) {
          // Button was pressed again after release (debounced) and minimum runtime is reached
          button_released = false;
          restartAnimation();  // Restart animation
        }
        
        if(checkTimer(&timer)) {
          transit(&fsm, STANDBY);
        }
        
        // Total short outage (Blackout)
        if(in_blackout) {
          if(checkAndResetTimer(&blackout_timer)) {
            in_blackout = false;
            // Start with bright power surge after outage
            fade(led, 0, BASE_BRIGHTNESS + (rand() % (BRIGHTNESS_VARIANCE)), MIN_FADE_TIME);
          }
        } 
        else if(checkAndResetTimer(&led->fade_timer)) {
          if(!initial_flash_done) {
            if(checkTimer(&initial_hold_timer)) {
              // After hold time of initial flash, dim to elevated base brightness
              initial_flash_done = true;
              // Longer transition to higher base brightness
              fade(led, INITIAL_FLASH_BRIGHTNESS, BASE_BRIGHTNESS * 1.4, 500);
            } else {
              // Maintain flash for the hold time
              fade(led, INITIAL_FLASH_BRIGHTNESS, INITIAL_FLASH_BRIGHTNESS, 1);
            }
          }
          // Random chance for total outage
          else if(rand() % 100 < BLACKOUT_PROBABILITY) {
            in_blackout = true;
            blackout_timer.cycle = BLACKOUT_DURATION + (rand() % 200);
            resetTimer(&blackout_timer);
            fade(led, led->goal, 0, MIN_FADE_TIME);
          }
          // Normal animation cycle
          else if(cooldown) { // Maintain brightness for a while
            cooldown = false;
            
            // Longer hold time for more stable phases
            uint16_t duration = MIN_HOLD_TIME + (rand() % HOLD_TIME_VARIANCE);
            
            // Chance for sudden power surge
            if(rand() % 100 < FLICKER_PROBABILITY) {
              // Bright power surge, fast rise
              uint16_t peak = BASE_BRIGHTNESS + BRIGHTNESS_VARIANCE + (rand() % (MAX_BRIGHTNESS - BASE_BRIGHTNESS - BRIGHTNESS_VARIANCE));
              
              // With low probability especially bright peak
              if(rand() % 100 < 25) {
                peak = MAX_BRIGHTNESS + (rand() % 1000);  // Occasionally overshoot
              }
              
              fade(led, led->goal, peak, 1);
            } else {
              fade(led, led->goal, led->goal, duration);
            }
          } else {  // Set new brightness
            cooldown = true;
            
            // Random brightness with larger fluctuations
            int16_t base_level = BASE_BRIGHTNESS + (-BRIGHTNESS_VARIANCE + (rand() % BRIGHTNESS_VARIANCE));
            
            // More variable, mainly faster transitions
            uint16_t duration = MIN_FADE_TIME + (rand() % FADE_TIME_VARIANCE);
            
            // Occasionally simulate short, not quite as dark phases
            if(rand() % 100 < 15) {  // Lower probability for darker phases
              base_level = BASE_BRIGHTNESS / 2 + (rand() % (BASE_BRIGHTNESS / 3));  // Less dark
            }
            
            fade(led, led->goal, base_level, duration);
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
  }
}
