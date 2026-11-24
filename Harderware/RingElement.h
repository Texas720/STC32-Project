#ifndef __RINGELEMENT_H
#define __RINGELEMENT_H


#define No_Detecte_Ring                      0      //普通状态：  没有检测到圆环
#define Ring_State_Normal                    1      //圆环状态1： 用于圆环中
#define Ring_State_RingEntry_Confirmation    2      //圆环状态2： 进环确认 + （A,B点补线）
#define Ring_State_RingEntry_Patchline       3      //圆环状态3： C点拉线进环
#define Ring_State_Ready_take_outRing        4      //圆环状态4： 准备出圆环
#define Ring_State_Ring_Removal_Patching     5      //圆环状态5： 出环补线（A点补线 + BC点补线）


/*
说明：传入的参数分别为：A点坐标，B点坐标，哪个点单独存在（1： 只有A点   2： 只有B点）
*/
void Ring_Statemachine(uint16 A_X, uint16 A_Y, uint16 B_X, uint16 B_Y, uint8 Only_AorB_StateFlag);


/*========以下为圆环内部函数=========*/

uint8 Confirm_Ring_No_KeyPoint(uint8 Ring_Side);
uint8 Ring_Check_A_or_B_RealLose(uint8 Lose_Side, uint16 PointX, uint16 PointY);
uint8 Ring_Check_A_or_B_ConfirmLoseOne(uint8 Choose_AorB_Lose, int16 Point_LocationY);

#endif
