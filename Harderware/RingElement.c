#include "zf_common_headfile.h"
#include "Gray.h"
#include "RingElement.h"

#define WHITE                             0xFF      //白色
#define BLACK                             0x00      //黑色

#define Mid_Linex                          47           //中线位置
#define Cross_Top_Gap                       2           //找到十字白色区域上边界后，再向上偏移的行数，用于定位远端拐点 
#define EdgeSkip                            3           //跳过画面最外侧N列
#define Max_Detecte_Length                 15           //最远检测距离
#define Min_LoseCount                      10           //圆环的最小断裂数
#define Min_Reply                           5           //确认断裂后，边界点重新出现的个数
#define Key_Row                       (GRAY_H - 6)      //取边界基准点所在的列    
       

#define WhiteBlock_Num                      50      //左右两侧的白色像素个数的最小值   
#define Real_AorB_LoseCon                   2       //符合上方条件的最小行数  
#define CheckAorBLose_Confirm_Cols          5       //N列满足条件，则判断为只是弯道而非十字

static uint8 Ring_State = No_Detecte_Ring;
static uint8 Choose_LeftorRight = 0;                //选择方向（1. 左圆环      2.右圆环）

void Ring_Statemachine(uint16 A_X, uint16 A_Y, uint16 B_X, uint16 B_Y, uint8 Only_AorB_StateFlag)            //右侧圆环状态机（进入8字第一个圆环）
{
    

    switch (Ring_State)
    {
        case No_Detecte_Ring:
            /**/
            /*TIps：在弯道接圆环的时候，A,B点可能找不到，仍需要一个A,B点之外的判据*/
            if(Only_AorB_StateFlag == 1 || (Confirm_Ring_No_KeyPoint(1) == 1))              //进入左侧圆环的条件
            {
                Ring_State = 2;
                Choose_LeftorRight = 1;
            }
            else if(Only_AorB_StateFlag == 2  || (Confirm_Ring_No_KeyPoint(2) == 2))        //进入右侧圆环的条件
            {
                Ring_State = 2;
                Choose_LeftorRight = 2;
            }
        
            break;

        case Ring_State_Normal:
            /**/
            break;
        
        case Ring_State_RingEntry_Confirmation:
            if(Choose_LeftorRight == 1)
            {
                RingEntry_Confirmation_Action(1, A_X, A_Y);
            }
            else if(Choose_LeftorRight == 2)
            {
                RingEntry_Confirmation_Action(2, B_X, B_Y);
            }
            
            /**/
            break;

        case Ring_State_RingEntry_Patchline:
            /**/
            break;
        
        case Ring_State_Ready_take_outRing:
            /**/
            break;
        
        case Ring_State_Ring_Removal_Patching:
            /**/
            break;

    default:
        break;
    }
}


/*============确认是是圆环的响应 + 入环补线===============*/
/*
    参数1：选择是左还是右侧圆环  1.左圆环    2，右圆环
    参数2，3    传入点的坐标
    返回值：
*/
uint8 RingEntry_Confirmation_Action(uint8 Flag ,uint16 Point_X, uint16 Point_Y)
{
    //A,B点补线
    uint16 i;
    uint16 j;
    uint16 F_PointX = 0;
    uint16 F_PointY = 0;
    uint8  F_Found  = 0;
    int16  Base_X = 0;                             //补线锚点坐标
    int16  Base_Y = 0;                             //补线锚点坐标
    int16  Rx_Plus = 0;
    uint8  Out_Flag = 0;
    int16  State5_KR     = 0;                     //右侧补线斜率
    uint8  F_SlopeValid  = 0;                     //F侧斜率是否成功算出过

    uint16 E_PointX = 0;
    uint16 E_PointY = 0;
    uint8  E_Found  = 0;
    int16  Lx_Plus = 0;
    int16  State4_KL     = 0;                     //缓存左侧补线斜率
    uint8  E_SlopeValid  = 0;                     //E侧斜率是否成功算出过


    //还缺少

    if (Flag == 1)
    {
        //B点丢失：以A点所在列向上找十字上边界，再向上Cross_Top_Gap行，向右找右侧边界点F
        if(Point_Y > 3 && Point_Y < GRAY_H && Point_X > 0 && Point_X < GRAY_W)              
        {
            for(j = Point_X; j < (Mid_Linex + 10); j ++)
            {
                E_PointY = 0;                       //每列重置，避免沿用上一列的行号
                for(i = Point_Y; i > 2; i --)
                {
                    if(Image1[j][i] == WHITE && Image1[j][i - 1] == BLACK && Image1[j][i - 2] == BLACK)
                    {
                        E_PointY = i;
                        break;
                    }
                }

                if(E_PointY <= Cross_Top_Gap + 1)
                {
                    continue;
                }

                E_PointY = E_PointY - Cross_Top_Gap;    //统一成上移后的判定行，斜率/补线起点都用它

                if(Image1[j][E_PointY] == BLACK
                    && Image1[j + 1][E_PointY] == WHITE 
                    && Image1[j + 2][E_PointY] == WHITE)
                {
                    E_PointX = j;
                    E_Found = 1;
                    break;
                }
            }
        }

        //补线
        if(E_Found == 1)
        {
            
            if(L_Edge[Key_Row] > 0)                    //假如边界基准行，存在L_Edge[Key_Row] > 0,即存在有效值
            {
                Base_X = L_Edge[Key_Row];
                Base_Y = Key_Row;
            }
            else if(Point_X != 0 && Point_Y != 0 && Point_Y > E_PointY)             //基准行不存在，使用A点坐标
            {
                Base_X = Point_X;
                Base_Y = Point_Y;
            }
            
            if(Base_Y >E_PointY)
            {
                State4_KL = (int16)(((int32)((int16)(E_PointX + 1) - Base_X) * 100) / (Base_Y - (int16)E_PointY));
                E_SlopeValid = 1;                                       //计算出正确斜率，可以缓存
            }
        }
        
        for (j = E_PointY; j < (GRAY_H - 1); j ++)
        {
            if(E_Found == 1 && E_SlopeValid == 1)
            {
                Lx_Plus =  (int16)((E_PointX + 1) - (((int32)State4_KL * ((int32)j - (int32)E_PointY)) / 100));
                if(Lx_Plus < 1)
                {
                    Lx_Plus = 1;
                }
                if(Lx_Plus > GRAY_W - 3)
                {
                    Lx_Plus = GRAY_W - 3;
                }
                Out_Flag = Ring_Check_A_or_B_ConfirmLoseOne(2,j);
                if(Out_Flag == 1)
                {
                    return Out_Flag;
                }
                else
                {
                    L_Edge[j] = Lx_Plus;
                    tft180_draw_point(L_Edge[j], j, RGB565_BLUE);
                }
            }
            else
            {
                break;
            }
        }
    }

    else if (Flag == 2)
    {
        //A点丢失：从B点所在列起逐列向左，每列向上找十字上边界，再上移Cross_Top_Gap行判定右边界点B
        if(Point_Y > 3 && Point_Y < GRAY_H && Point_X > 0 && Point_X < GRAY_W)
        {              
            for(j = Point_X; j > (Mid_Linex - 10); j --)   //必要：限制搜索宽度
            {
                F_PointY = 0;                       //每列重置，避免沿用上一列的行号
                for(i = Point_Y; i > 2; i --)
                {
                    if(Image1[j][i] == WHITE && Image1[j][i - 1] == BLACK && Image1[j][i - 2] == BLACK)
                    {
                        F_PointY = i;
                        break;
                    }
                }

                if(F_PointY <= Cross_Top_Gap + 1)
                {
                    continue;
                }

                F_PointY = F_PointY - Cross_Top_Gap;    //统一成上移后的判定行，斜率/补线起点都用它

                if(Image1[j][F_PointY] == BLACK
                    && Image1[j - 1][F_PointY] == WHITE
                    && Image1[j - 2][F_PointY] == WHITE)
                {
                    F_PointX = j;
                    F_Found = 1;
                    break;
                }
            }
        }

        if(F_Found == 1)
        {
            if(R_Edge[Key_Row] > 0)                    //优先用底行真实右边界
            {
                Base_X = R_Edge[Key_Row];
                Base_Y = Key_Row;
            }
            else if(Point_X != 0 && Point_Y != 0 && Point_Y > F_PointY)   //基准行不存在，使用B点坐标
            {
                Base_X = Point_X;
                Base_Y = Point_Y;
            }

            if(Base_Y > F_PointY)
            {
                State5_KR = (int16)(((int32)((int16)F_PointX - Base_X) * 100) / (Base_Y - (int16)F_PointY));
                F_SlopeValid = 1;
            }
        }

        for(i = F_PointY; i < (GRAY_H - 1); i ++)
        {
            if(F_Found == 1 && F_SlopeValid == 1)
            {
                Rx_Plus = (int16)((int32)F_PointX - (((int32)State5_KR * ((int32)i - (int32)F_PointY)) / 100));   //以F为锚点向下补线
                if(Rx_Plus < 1)
                {
                    Rx_Plus = 1;
                }
                if(Rx_Plus > GRAY_W - 3)
                {
                    Rx_Plus = GRAY_W - 3;
                }
                Out_Flag = Ring_Check_A_or_B_ConfirmLoseOne(1,i);
                if(Out_Flag == 1)
                {
                    return Out_Flag;
                }
                else
                {
                    R_Edge[i] = Rx_Plus;
                    tft180_draw_point(R_Edge[i], i, RGB565_BLUE);
                }
            }
            else
            {
                break;
            }
        }
    }
    
    
    

    return Out_Flag;
}


/*=========当角点找不到时，判断是不是圆环=========*/ 
//输入参数：圆环方向（1：左圆环       2：右圆环）
//返回值：  0：不是圆环   1：是左圆环    2：是右圆环
uint8 Confirm_Ring_No_KeyPoint(uint8 Ring_Side)
{
    uint8 i;
    uint8 Lose_Count = 0;           //有断裂的次数   
    uint8 Reply_Count = 0;          //检测为孤立段的次数         
    uint8 Step;                 //检测阶段1:检测第一段断裂    2：检测孤立段   

    if(Ring_Side == 1)          //左圆环
    {
        //在一段距离以内，判断右侧边界是否单调，左边边界是不是有断裂
        for(i = (GRAY_H - 1); i > Max_Detecte_Length; i --)
        {
            if(R_Edge[i] >= 0 && L_Edge[i] < 0 && Step == 0)     //有断裂
            {
                Lose_Count ++;
                if(Lose_Count >= Min_LoseCount)   //确认是断裂
                {
                  Step = 1; 
                  Lose_Count = 0; 
                }
            }
            if(Step == 1)
            {
                if(L_Edge[i] >= 0 && R_Edge[i] >= 0)  //检测是否有孤立段
                {
                    Reply_Count ++;
                }
            }
            
            if(Reply_Count >= Min_Reply)            //确认是孤立段
            {
                Reply_Count = 0;    
                Step = 0;
                return 1;                          
            }

        }
    }
    if(Ring_Side == 2)          //右圆环
    {
        //在一段距离以内，判断左侧边界是否单调，右边边界是不是有断裂
        for(i = (GRAY_H - 1); i > Max_Detecte_Length; i --)
        {
            if(L_Edge[i] >= 0 && R_Edge[i] < 0 && Step == 0)     //有断裂
            {
                Lose_Count ++;
                if(Lose_Count >= Min_LoseCount)   //确认是断裂
                {
                  Step = 1; 
                  Lose_Count = 0; 
                }
            }
            if(Step == 1)
            {
                if(L_Edge[i] >= 0 && R_Edge[i] >= 0)  //检测是否有孤立段
                {
                    Reply_Count ++;
                }
            }
            
            if(Reply_Count >= Min_Reply)            //确认是孤立段
            {
                Reply_Count = 0;    
                Step = 0;
                return 2;                          
            }

        }
    }


    //检测为非圆环
    return 0;
}



/*确认进入A/B单个丢失逻辑*/
//输入参数1： 选择A点或者B点丢失， 1为A丢失， 2为B丢失
//输入参数2： 当前点的Y轴位置
uint8 Ring_Check_A_or_B_ConfirmLoseOne(uint8 Choose_AorB_Lose, int16 Point_LocationY)
{
    
    static int16 LastL_StopX = 0;             //A点丢失时，左侧Ledge的历史值
    static int16 LastR_StopX = 0;             //B点丢失时，右测Redge的历史值
    static int16 NowL_StopX = 0;              //A点丢失时，左侧Ledge的更新值
    static int16 NowR_StopX = 0;              //B点丢失时，右侧Redge的更新值
    static int16 Qualified_Num = 0;
    uint8 Out_AorBlose_Flag = 0;


    Out_AorBlose_Flag = 0;
    
    //逻辑：补线的时候，检查左或者右边界点外扩趋势，假如外扩趋势大于2个像素/行，且连续3次，判定为弯道
    
    if(Choose_AorB_Lose == 1)               // A点丢失
    {
        if(L_Edge[Point_LocationY] >= 0)
        {
            LastL_StopX = NowL_StopX;
            NowL_StopX  = L_Edge[Point_LocationY];
            if(LastL_StopX >= NowL_StopX && LastL_StopX - NowL_StopX >= 3)
            {
                Qualified_Num ++;
                if(Qualified_Num >= CheckAorBLose_Confirm_Cols)
                {
                    Qualified_Num = 0;
                    Out_AorBlose_Flag = 1;
                }
            }
        }
        else
        {
            Out_AorBlose_Flag = 0;
        }

    }

    
    if(Choose_AorB_Lose == 2)              //B点丢失
    {
         if(R_Edge[Point_LocationY] >= 0)
        {
            LastR_StopX = NowR_StopX;
            NowR_StopX  = R_Edge[Point_LocationY];
            if(LastR_StopX <= NowR_StopX && NowR_StopX - LastR_StopX >= 3)
            {
                Qualified_Num ++;
                if(Qualified_Num >= CheckAorBLose_Confirm_Cols)
                {
                    Qualified_Num = 0;
                    Out_AorBlose_Flag = 1;
                }
            }
        }
        else
        {
            Out_AorBlose_Flag = 0;
        }

    }   


    return Out_AorBlose_Flag;
}



/*=====检查单侧丢线角点是否具有真实十字的横向白色支路==========*/
//参数：1.选择哪边丢失（1，A点丢失，检查B点    2.B点丢失，检查A点），以及存在点的坐标
//返回：1为真十字单侧丢线，0为弯道伪角点或特征不足
uint8 Ring_Check_A_or_B_RealLose(uint8 Lose_Side, uint16 PointX, uint16 PointY)
{
    int16 i;
    int16 j;
    uint8 Quali_Times;                //朝外连续白长度符合要求的行数
    uint8 White_Length;               //当前行从角点朝外的连续白长度
    uint8 Out_Flag;

    Quali_Times = 0;
    Out_Flag = 0;

    if((Lose_Side != 1 && Lose_Side != 2) || PointX >= GRAY_W || PointY < 3 || PointY >= GRAY_H)
    {
        return 0;
    }

    for(i = 1; i < 4; i ++)
    {
        White_Length = 0;

        if(Lose_Side == 1)               //A点丢失，从B点向左检查横向支路
        {
            for(j = (int16)PointX; j > 0; j --)
            {
                if(Image1[j][(int16)PointY - i] == BLACK)
                {
                    break;
                }
                White_Length ++;
            }
        }
        else                             //B点丢失，从A点向右检查横向支路
        {
            for(j = (int16)PointX; j < GRAY_W; j ++)
            {
                if(Image1[j][(int16)PointY - i] == BLACK)
                {
                    break;
                }
                White_Length ++;
            }
        }

        if(White_Length >= WhiteBlock_Num)
        {
            Quali_Times ++;
        }
    }

    if(Quali_Times >= Real_AorB_LoseCon)
    {
        Out_Flag = 1;
    }
    return Out_Flag;
}



/*
    TIPS；可以写一个标志位，让各个圆环和状态机同时只能有一个响应
*/
