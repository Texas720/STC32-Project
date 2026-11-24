#include "zf_common_headfile.h"
#include "Element.h"
#define Servo_PWMStream   PWME_CH1P_PA0
#define Servo_Mid   780
#define Servo_Max   870                         //最大脉宽（最左侧）
#define Servo_Min   680                         //最小脉宽（最右侧）

/*---------------- 转向PD参数 ----------------*/
#define SERVO_KP        0.600f                       //比例系数（按新行程 190/1040≈0.18 等比缩放）
#define SERVO_KP2       0.0004f                     //平方项系数：小偏差几乎不起作用，大偏差增益非线性放大
#define SERVO_KD        0.10f                       //微分系数(经3:7一阶滤波，抗图像噪声)
#define SERVO_OUT_MAX   (Servo_Max - Servo_Mid)     //输出限幅上限 +90（左）
#define SERVO_OUT_MIN   (Servo_Min - Servo_Mid)     //输出限幅下限 -100（右）

void Servo_Init(void)
{
    pwm_init(Servo_PWMStream,50,Servo_Mid);
}

void Servo_SetAngle(uint16 PWM)
{
    //出口限幅保护
    if(PWM > Servo_Max) PWM = Servo_Max;
    if(PWM < Servo_Min) PWM = Servo_Min;
    pwm_set_duty(Servo_PWMStream,PWM);
}


int16 Servo_pidTurn(int16 Error)
{
    static int16 error_last = 0;    //上一帧偏差
    static float out_d = 0;         //滤波后的微分项
    int16 error;
    float f;                        //error*|error|，保留符号
    float out;

    error = -Error;                 //和原增量式保持同方向

    f = (float)error * (error >= 0 ? error : -error);                 //e*|e|，C251的int是16位，先转float防溢出
    out_d = (3 * SERVO_KD * (error - error_last) + 7 * out_d) / 10;   //3:7一阶滤波：平滑单帧误差跳变，避免反向脉冲
    error_last = error;

    out = SERVO_KP * error + SERVO_KP2 * f + out_d;

    //输出限幅
    if(out > SERVO_OUT_MAX) out = SERVO_OUT_MAX;
    if(out < SERVO_OUT_MIN) out = SERVO_OUT_MIN;

    Servo_SetAngle((uint16)(Servo_Mid + (int16)out));   //中值叠加，Servo_SetAngle内还有一层行程保护

    return (int16)out;
}