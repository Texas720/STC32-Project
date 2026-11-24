#ifndef __GRAY_H
#define __GRAY_H

#include "zf_common_typedef.h"
#include "zf_device_mt9v03x.h"		// 用到 MT9V03X_W / MT9V03X_H（188 x 120）

/* 下采样之后的图像尺寸：94 x 60 */
#define GRAY_W      94			//下采样之后的宽度
#define GRAY_H      60		    //下采样之后的高度

/* 二值化之后的图像：Image1[列][行]，0xFF = 白，0x00 = 黑 */
extern uint8 Image1[GRAY_W][GRAY_H];

/* 左右边线 / 中线：角标是行号，内容是列号，-1 表示该行没找到 */
extern int16 L_Edge[GRAY_H];
extern int16 R_Edge[GRAY_H];
extern int16 Mid_Line[GRAY_H];
extern uint16 Length_A;
extern uint16 Length_B;
extern uint8 End_Line;

/* 归一化偏差，Out 放大了 100 倍 */
extern int16 Error;
extern int16 Out;

/* 丢线标志 */
extern uint8 NOFind_DiffFlagL;
extern uint8 NOFind_DiffFlagR;

extern int16 Curve_deviation_threshold;
extern int16 RL_Result;					//退出判据处的赛道宽度，主循环显示用
extern uint16 Forward_LookingLine_Out;	//前瞻行，主循环画紫色前瞻线用

uint8 Simple_Gray(uint16 frame[]);		// 输入 RGB565 原图，返回阈值
uint8 Normal_Gray(uint8 frame[MT9V03X_H][MT9V03X_W]);	//注意输入的是灰度图,返回阈值
void Find_Edge(void);  // 直接对全局 Image1（下采样后二值化图像）寻线
void Find_KeyPoint(int16 L_Edge[GRAY_H] , int16 R_Edge[GRAY_H]);
void Find_MidLine(void);
int16 Find_Error(int16 Mid_Line[], int16 L_Edge[], int16 R_Edge[]);
uint8 Distinguish_Circle_Cross(uint16 A_Y, uint16 B_Y, uint8 AB_StateFlag, uint8 Only_AorB_StateFlag);

#endif
