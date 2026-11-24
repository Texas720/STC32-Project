#ifndef __MOTOR_H
#define __MOTOR_H

void Motor_Init(void);
void Motor_SetSameSpeed(int16 pwm);  //占空比的计算 pwm / 10000
int16 Motor_PIControl(int16 aimspeed, int16 *l_speed, int16 *r_speed) ;           //输入的是目标pwm的值，这个值设定为目标占空比的20倍数；左右轮速度通过指针带出

#endif
