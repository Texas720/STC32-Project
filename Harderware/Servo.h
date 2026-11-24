#ifndef __SERVO_H
#define __SERVO_H

#define Servo_PWMStream   PWME_CH1P_PA0
#define Servo_Mid   780
#define Servo_Max   870                         //最大脉宽（最左侧）
#define Servo_Min   680                         //最小脉宽（最右侧）


void Servo_Init(void);
void Servo_SetAngle(uint16 PWM);
int16 Servo_pidTurn(int16 Error);


#endif
