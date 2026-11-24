#ifndef __ELEMENT_H
#define __ELEMENT_H

/*========状态机各状态===========*/
#define Cross_State_Normal   1        //无十字
#define Cross_State_ABExist  2        //A,B点存在
#define Cross_State_ABLost   3        //A,B点丢失，惯性补线
#define Cross_State_OnlyA    4        //只看到A点，B点丢失
#define Cross_State_OnlyB    5        //只看到B点，A点丢失

extern uint8 RE_State;
extern uint8 Temp_AorBLose;                            //用于暂存是哪边丢失.仅A点为1，仅B点为2
void Cross_Statemachine(uint16 A_X, uint16 A_Y, uint16 B_X, uint16 B_Y, uint8 AB_StateFlag, uint8 Only_AorB_StateFlag);


/*以下为状态机内部函数*/
void Cross_State_Normal_Action();       //无十字状态的响应
void Cross_State_ABExist_Action(uint16 A_X, uint16 A_Y, uint16 B_X, uint16 B_Y);            //A,B点均存在时的响应
void Cross_State_ABLost_Action();             //A,B点都丢失时的响应
uint8 Check_BackNormalState(void);                                 //判断返回普通情况的条件是否满足
uint8 Cross_State_OnlyA_Action(uint16 A_X, uint16 A_Y);            //只有A点的响应
uint8 Cross_State_OnlyB_Action(uint16 B_X, uint16 B_Y);            //只有B点的响应
uint8 Check_A_or_B_ConfirmLoseOne(uint8 Choose_AorB_Lose, int16 Point_LocationY);
uint8 Check_A_or_B_RealLose(uint8 Lose_Side, uint16 PointX, uint16 PointY);
/*=====检查单侧丢线角点是否具有真实十字的横向白色支路==========*/
//参数：1.选择哪边丢失（1，A点丢失，检查B点    2.B点丢失，检查A点），以及存在点的坐标
//返回：1为真十字单侧丢线，0为弯道伪角点或特征不足

#endif
