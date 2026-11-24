#ifndef __ENCODER_H
#define __ENCODER_H

void Encoder_Init(void);
//当占空比设定为10%的时候，每10ms调用一次Encoder_GetX,得到的值大概在180左右，速度环可以设置为占空比的20倍，即当占空比设定为10%，目标值设定为200.
int16 Encoder_GetL(void);    
int16 Encoder_GetR(void);

#endif
