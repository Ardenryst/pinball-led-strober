#include "util.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

// -- Timing stuff
// Source: https://github.com/Tekl7/Millis
#define CTC_REG             TCCR0A
#define CTC_BIT_MSK         _BV(WGM01)
#define PRESCALER_REG       TCCR0B
#define PRESCALER_BITS_MSK  _BV(CS00) | _BV(CS01)
#define PRESCALER           64
#define OCREG               OCR0A
#define OCIE_REG            TIMSK
#define	OCIE_BIT_MSK        _BV(OCIE0A)
#define OCINT_VECT          TIMER0_COMPA_vect

static volatile uint32_t timer_millis;

void initMillis() {
  // Set CTC mode
  CTC_REG |= CTC_BIT_MSK;
  // Set prescaler
  PRESCALER_REG |= PRESCALER_BITS_MSK;
  // Set OCRnx value
  OCREG = ((F_CPU / PRESCALER) / 1000) - 1;
  // Enable output compare interrupt
  OCIE_REG |= OCIE_BIT_MSK;
  
//   TCCR0A = (1<<WGM01);
//   TIMSK |= (1<<OCIE0A);
//   OCR0A = 103;
//   TCCR0B = (1<<CS01);
}

uint32_t millis() {
  uint32_t millis_return;
  // Ensures this cannot be disrupted
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    millis_return = timer_millis;
  }
  return millis_return;
}
ISR(OCINT_VECT) {
  timer_millis++; //TODO
}

unsigned long getDelta(struct Timer* timer) {
  return millis() - timer->last;
}
bool checkTimer(struct Timer* timer) {
  unsigned long now = millis();
  return timer->disabled || (getDelta(timer) >= timer->cycle);
}
void resetTimer(struct Timer* timer) {
  timer->disabled = false;
  timer->last = millis();
}
bool checkAndResetTimer(struct Timer* timer) {
  if(checkTimer(timer)) {
    resetTimer(timer);
    return true;
  }
  return false;
}
void disableTimer(struct Timer* timer) {
  timer->disabled = true;
}
// double getProgress(struct  Timer* timer) {
//   unsigned long delta = getDelta(timer);
//   return (delta >= timer->cycle) ? 1.0 : delta / (double)timer->cycle;
// }

int16_t linearInterpolate(int16_t from, int16_t to, struct Timer* timer) {
  unsigned long delta_t = getDelta(timer);
  if(delta_t >= timer->cycle) {
    return to;
  } else {
    return (from * (timer->cycle - delta_t)) / timer->cycle + (to * delta_t) / timer->cycle;
  }
}

// -- Finite State Machines
void initFSM(struct StateMachine* fsm, int state) {
  fsm->current_state = state;
  fsm->last_state = -1;
  fsm->next_state = -1;
}

int updateFSM(struct StateMachine* fsm) {
  fsm->state_changed = false;
  fsm->last_state = fsm->current_state;
  if(fsm->next_state != -1) {
    fsm->current_state = fsm->next_state;
    fsm->next_state = -1;
    fsm->state_changed = true;
  }
  return fsm->current_state;
}

bool onEnter(struct StateMachine* fsm) {
  return fsm->state_changed;
//   return fsm->current_state != fsm->last_state;
}

void transit(struct StateMachine* fsm, int state) {
  fsm->next_state = state;
}
