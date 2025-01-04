#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdbool.h>
#include <stdint.h>

#define SIZE(x)  (sizeof(x) / sizeof(x[0]))

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

//TODO umbenennen
// https://stackoverflow.com/questions/1489932/how-to-concatenate-twice-with-the-c-preprocessor-and-expand-a-macro-as-in-arg
#define PASTER(x,sep,y) x ## sep ## y
#define EVALUATOR(x,y)  PASTER(x,_,y)

#define JOIN(a, b) PASTER(a, _ ,b)

#define STR(...) #__VA_ARGS__
#define XSTR(...) STR(__VA_ARGS__)

#define IGNORE(dontcare...)

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// #define REP0(X)
// #define REP1(X) X
// #define REP2(X) REP1(X) X
// #define REP3(X) REP2(X) X
// #define REP4(X) REP3(X) X
// #define REP5(X) REP4(X) X
// #define REP6(X) REP5(X) X
// #define REP7(X) REP6(X) X
// #define REP8(X) REP7(X) X
// #define REP9(X) REP8(X) X
// #define REP10(X) REP9(X) X
// 
// #define REP(HUNDREDS,TENS,ONES,X) \
//   REP##HUNDREDS(REP10(REP10(X))) \
//   REP##TENS(REP10(X)) \
//   REP##ONES(X)

// -- HAL
// Source: https://www.mikrocontroller.net/topic/81792#742165
struct bits {
   uint8_t b0:1;
   uint8_t b1:1;
   uint8_t b2:1;
   uint8_t b3:1;
   uint8_t b4:1;
   uint8_t b5:1;
   uint8_t b6:1;
   uint8_t b7:1;
} __attribute__((__packed__));

#define __SET_TO_IN(name, bit)     DDR##name &= ~(1<<bit)
#define __SET_TO_OUT(name, bit)    DDR##name |= (1<<bit)
#define __PORT(name, bit)   ((*(volatile struct bits*)&PORT##name).b##bit)
#define __PIN(name, bit)    ((*(volatile struct bits*)&PIN##name).b##bit)
#define INPUT(name)        __SET_TO_IN(name); __PORT(name) = 0
#define INPUT_PULLUP(name) __SET_TO_IN(name); __PORT(name) = 1
#define OUTPUT(name)       __SET_TO_OUT(name)
#define PORT(name)             __PORT(name)
#define PIN(name)              __PIN(name)

// -- Timing stuff
void initMillis();
extern unsigned long millis(); //TODO braucht man das hier wirklich?
struct Timer {
  unsigned long cycle;
  unsigned long last;
  bool disabled;
};
unsigned long getDelta(struct Timer* timer);
bool checkTimer(struct Timer* timer);
void resetTimer(struct Timer* timer);
bool checkAndResetTimer(struct Timer* timer);
void disableTimer(struct Timer* timer);
// double getProgress(struct  Timer* timer);
int16_t linearInterpolate(int16_t from, int16_t to, struct Timer* timer);

// -- Finite State Machines
struct StateMachine {
  int current_state;
  int last_state;
  int next_state;
  bool state_changed;
};

void initFSM(struct StateMachine* fsm, int state);

int updateFSM(struct StateMachine* fsm);

bool onEnter(struct StateMachine* fsm);

void transit(struct StateMachine* fsm, int state);

#endif 
