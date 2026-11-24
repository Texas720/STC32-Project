#include "zf_common_headfile.h"


//轮
#define ENCODER_DIR_2                 	(PWMC_ENCODER)              // 带方向编码器对应使用的编码器接口
#define ENCODER_DIR_PULSE_2       		(PWMC_ENCODER_CH1P_P40)     // PULSE 对应的引脚
#define ENCODER_DIR_DIR_2           	(PWMC_ENCODER_CH2P_P42)     // DIR 对应的引脚


//右轮
#define ENCODER_DIR_1                 	(PWMA_ENCODER)              // 带方向编码器对应使用的编码器接口 
#define ENCODER_DIR_PULSE_1            	(PWMA_ENCODER_CH1P_P60)     // PULSE 对应的引脚
#define ENCODER_DIR_DIR_1              	(PWMA_ENCODER_CH2P_P62)     // DIR 对应的引脚


void Encoder_Init(void)
{
    encoder_dir_init(ENCODER_DIR_1,ENCODER_DIR_PULSE_1,ENCODER_DIR_DIR_1);         //编码器本身有一根信号线输出方向，位置改变的时候，该信号线的电平改变
    encoder_clear_count(ENCODER_DIR_1);                                            //清除计数器的值
    encoder_dir_init(ENCODER_DIR_2,ENCODER_DIR_PULSE_2,ENCODER_DIR_DIR_2);
    encoder_clear_count(ENCODER_DIR_2);
}


//编码器旋转一圈输出1024个脉冲信号
int16 Encoder_GetL(void)
{
    int16 Temp = 0;
    Temp = -(encoder_get_count(ENCODER_DIR_2));	//本车接线下前进时原始计数为负，取反使前进为正
    encoder_clear_count(ENCODER_DIR_2);
    return Temp;
}

int16 Encoder_GetR(void)
{
    int16 Temp = 0;
    Temp = -(encoder_get_count(ENCODER_DIR_1));	//同上，保持两轮方向一致
    encoder_clear_count(ENCODER_DIR_1);
    return Temp;
}