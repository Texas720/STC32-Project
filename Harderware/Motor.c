#include "zf_common_headfile.h"
#include "Encoder.h"

#define L_MotorIO IO_P76
#define L_MotorPWM  PWMB_CH4_P77
#define R_MotorIO IO_P74
#define R_MotorPWM  PWMB_CH2_P75
#define Max_OutPutPWM    9000.0
#define Speed_Kp   2.00
#define Speed_Ki   1.00

void Motor_Init(void)
{
    gpio_init(L_MotorIO,GPO,1,GPO_PUSH_PULL);
    pwm_init(L_MotorPWM,17000,0);
    gpio_init(R_MotorIO,GPO,0,GPO_PUSH_PULL);
    pwm_init(R_MotorPWM,17000,0);
}

void Motor_SetSameSpeed(int16 pwm)    
{
	if(pwm < 0)     pwm = 0;			//负占空比无意义，且传给 uint32 形参会回绕成满速
	if(pwm > 10000) pwm = 10000;
    pwm_set_duty(L_MotorPWM,pwm);
    pwm_set_duty(R_MotorPWM,pwm);
}

int16 Motor_PIControl(int16 aimspeed, int16 *l_speed, int16 *r_speed)            //输入的是目标pwm的值，这个值设定为目标占空比的20倍数；速度通过指针带出，避免编码器被重复读取清零
{
	static int16 SpeedError1 = 0;	//上一次的误差，增量式 PI 必须跨调用保持
	static int16 SpeedError0 = 0;	//本次误差
	static float Result_Pwm  = 0;	//输出累积量，必须跨调用保持
    int16 SpeedTerget;
    int16 SpeedActual;
    int16 AverageSpeed;


    *l_speed = Encoder_GetL();
	*r_speed = Encoder_GetR();
    SpeedTerget = aimspeed;
	AverageSpeed = (*l_speed + *r_speed) >> 1;

	SpeedActual = AverageSpeed ;//实际值
	SpeedError1 = SpeedError0 ;
	SpeedError0 = SpeedTerget - SpeedActual ;

	Result_Pwm += Speed_Kp * (SpeedError0 - SpeedError1 ) + Speed_Ki * SpeedError0 ;//输出平均PWM

	if(Result_Pwm >= Max_OutPutPWM)
	{
		Result_Pwm = Max_OutPutPWM;
	}
	if(Result_Pwm <= 0)				//下限钳位，防止负值转成巨大无符号占空比
	{
		Result_Pwm = 0;
	}

    Motor_SetSameSpeed((int16) Result_Pwm);
	return (int16)Result_Pwm;
}
