#include "zf_common_headfile.h"
#include "Gray.h"
#include "Servo.h"
#include "Motor.h"
#include "Element.h"
#include "Key.h"

// #define Servo_RunningTimes   400      //[已注释] 400 × 3ms = 1200ms 硬打方向时长

int16 Temp_Error = 0;
int16 Temp_Speed = 0;
int16 L_Speed = 0;
int16 R_Speed = 0;
int16 Turn_Result = 0;
uint8 Tick = 0;                       // 1s 节拍标志：中断置1，主循环读完清零
//转向环
// void pit_tim0(void)
// {
//     if(Servo_ReturnNormal == 0)
//     {
//         Temp_Error = Find_Error(Mid_Line,L_Edge,R_Edge);
//         Turn_Result = Servo_pidTurn(Temp_Error);	//转向一体控制，返回值用于屏幕显示
//     }
//     else if(Servo_ReturnNormal == 1)
//     {
//         Servo_SetAngle(Servo_Mid);
//     }

// }


void pit_tim0(void)
{
    Temp_Error = Find_Error(Mid_Line, L_Edge, R_Edge);   
    Turn_Result = Servo_pidTurn(Temp_Error);  
}

void pit_tim1(void)
{
    Temp_Speed = Motor_PIControl(100, &L_Speed, &R_Speed);
}

void pit_tim2(void)
{
    Tick = 1;
}

void pit_tim3(void)             //按键定时器
{
    static uint16 TIM0_Count = 0;

    //缓速系统
    if (++Key_Slow >= 10)       Key_Slow = 0;

    //计算帧数
    if (++TIM0_Count >= 1000)
    {
        tft180_show_uint16(0,80,Angle_Set);
		TIM0_Count = 0;
    }
}