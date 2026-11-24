#ifndef __PIT_H
#define __PIT_H

void pit_tim0(void);
void pit_tim1(void);
void pit_tim2(void);
void pit_tim3(void);             //按键定时器

extern uint8 Tick;                    // 1s 节拍标志
extern int16 Temp_Error;
extern Turn_Result;
extern int16 L_Speed;
extern int16 R_Speed;
// extern uint16 PWM1;                              //[已注释] 硬打方向 PWM

#endif 
