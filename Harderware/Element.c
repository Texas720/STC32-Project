#include "zf_common_headfile.h"
#include "Gray.h"
#include "Element.h"


#define Min_Distance    10   //补线之后第一个黑色像素到底边的最小距离，小于这个距离，退出补线
#define WHITE 0xFF
#define BLACK 0x00


#define FindEF_Span          3        //边界最大高差
#define FindEF_MinMove       2        //跨度内边界高度的最小位移
#define FindEF_Confirm_Cols  3        //连续N列都满足位移条件才确认为拐点
#define Cross_Top_Gap        2        //找到十字白色区域上边界后，再向上偏移的行数，用于定位远端拐点
#define EdgeSkip             3        //跳过画面最外侧N列
#define LowEdge_Start       55        //A,B点丢失时，检测开始的最低行
#define Max_RoadWidth       75       //检测是否从十字模式退出时，赛道的最大宽度
#define Min_Roadwidth       50       //检测是否从十字模式退出时，赛道的最小宽度
#define Qualified_Times      5        //检测有多少行符合上面的标准


#define CheckAorBLose_Confirm_Cols     5    //N列满足条件，则判断为只是弯道而非十字
#define CheckfromOnly_to_Normal_Cols   3    //从单侧丢失跳到正常状态时，判断状态是否稳定的延时
#define Mid_Linex                     (GRAY_W / 2)    //画面中心  
#define Key_Row                       (GRAY_H - 6)    //取边界基准点所在的列
#define WhiteBlock_Num                 50             //左右两侧的白色像素个数的最小值
#define Real_AorB_LoseCon              2              //符合上方条件的最小行数
#define Expansion_BlackBlock           4              //膨胀点周围黑点数
#define Max_Expansion_Step             10             //最大膨胀次数

/*======延时帧数设置============*/
#define AB_Confirm_Frames   1         //连续N帧AB存在才判定为十字
#define AB_Lost_Frames      0         //连续N帧AB丢失才进入惯性补线
#define Exit_Delay_Frames   0         //惯性补线退出延时帧数
#define Exit_Check_Frames   3         //满足End_Line行边界存在N次才确认退出


/*=========暂存的左右两侧斜率与初始状态======== */
static uint8 Cross_State = Cross_State_Normal;
static uint8 Back_Normal_StateFlag = 0;            //从惯性返回普通状态的标志位，置1表示返回普通状态
static int16 KL = 0;
static int16 KR = 0; 
static uint8 AB_Exist_Count = 0;                    //连续AB存在帧数
static uint8 AB_Lost_Count = 3;                     //连续AB丢失帧数
static uint8 Exit_Delay_Count = 0;                  //惯性补线退出延时计数
static uint8 Wrong_judgment_AorBLostFlag = 0;       //弯道与十字单侧补线冲突时的退出标志位，置1退出

/*=======找角点的时候，对图像边缘进行膨胀=======*/
static int8 L_expansionX_Plus[5] = {1, 1, 0, -1, -1};     //膨胀时，对当前点周围5个点进行检测
static int8 L_expansionY_Plus[5] = {0, -1, -1, -1, 0};

static int8 R_expansionX_Plus[5] = {-1, -1, 0, 1, 1};
static int8 R_expansiony_Plus[5] = {0, -1, -1, -1, 0};

/*========A,B点丢失的时候，暂存的点============*/

//上一次合规的E点
static uint8 Last_E_PointX = 0;
static uint8 Last_E_PointY = 0;       

//上一次合规的F点
static uint8 Last_F_PointX = 0;
static uint8 Last_F_PointY = 0;  

//Last_* 是否已被有效缓存（避免初值 0 被当成锚点）
static uint8 E_LastValid = 0;
static uint8 F_LastValid = 0;


uint8 RE_State = 0;                                 //用于查看Cross_State
uint8 Temp_AorBLose = 0;                            //用于暂存是哪边丢失

void Cross_Statemachine(uint16 A_X, uint16 A_Y, uint16 B_X, uint16 B_Y, uint8 AB_StateFlag, uint8 Only_AorB_StateFlag)
{   
    RE_State = Cross_State;


    switch(Cross_State)
    {
    case Cross_State_Normal:
        Cross_State_Normal_Action();
        if(AB_StateFlag == 1)                       //A,B点存在
        {
            AB_Exist_Count++;
            AB_Lost_Count = 0;
            if(AB_Exist_Count >= AB_Confirm_Frames)
            {
                Cross_State = Cross_State_ABExist;
                AB_Exist_Count = 0;
            }
        }
        else if(Only_AorB_StateFlag == 1 && (Check_A_or_B_RealLose(2, A_X, A_Y) == 1))              //只有A点存在
        {
            Cross_State = Cross_State_OnlyA;
        }
        else if(Only_AorB_StateFlag == 2 && (Check_A_or_B_RealLose(1, B_X, B_Y) == 1))             //只有B点存在
        {
            Cross_State = Cross_State_OnlyB;
        }
        else
        {
            AB_Exist_Count = 0;
        }
        
        break;

    case Cross_State_ABExist:
        Cross_State_ABExist_Action(A_X, A_Y, B_X, B_Y);
        Last_E_PointX = 0;
        Last_E_PointY = 0;
        Last_F_PointX = 0;
        Last_F_PointY = 0;
        E_LastValid = 0;
        F_LastValid = 0;
        if(AB_StateFlag == 0)                      //A,B点丢失，连续帧确认防抖
        {
            AB_Lost_Count++;
            AB_Exist_Count = 0;
            if(AB_Lost_Count >= AB_Lost_Frames)
            {
                Cross_State = Cross_State_ABLost;
                AB_Lost_Count = 0;
                Exit_Delay_Count = 0;
            }
        }
        else
        {
            AB_Lost_Count = 0;
        }
        break;
    
    case Cross_State_ABLost:
        Cross_State_ABLost_Action();
        Back_Normal_StateFlag = Check_BackNormalState();
        if(Back_Normal_StateFlag == 1)             //退出惯性补线，加延时防抖
        {
            Exit_Delay_Count++;
            if(Exit_Delay_Count >= Exit_Delay_Frames)
            {
                Cross_State = Cross_State_Normal;
                Exit_Delay_Count = 0;
            }
        }
        else
        {
            Exit_Delay_Count = 0;
            if(AB_StateFlag == 1)                  //AB重新出现，连续帧确认后回到ABExist
            {
                AB_Exist_Count++;
                if(AB_Exist_Count >= AB_Confirm_Frames)
                {
                    Cross_State = Cross_State_ABExist;
                    AB_Exist_Count = 0;
                }
            }
            else
            {
                AB_Exist_Count = 0;
            }
        }
        break;
    

    case Cross_State_OnlyA:
        Wrong_judgment_AorBLostFlag  = Cross_State_OnlyA_Action(A_X, A_Y);
        Back_Normal_StateFlag = Check_BackNormalState();
        if(Back_Normal_StateFlag == 1  || Wrong_judgment_AorBLostFlag == 1)
        {
            Cross_State = Cross_State_Normal;
        }
        else if(AB_StateFlag == 1)
        {
            //加延时
            AB_Exist_Count ++;
            if(AB_Exist_Count >= AB_Confirm_Frames)
            {
              Cross_State = Cross_State_ABExist; 
              AB_Exist_Count = 0; 
            }
            
        }
        else if(AB_StateFlag == 0)
        {
            AB_Exist_Count = 0;
        }
        break;


    case Cross_State_OnlyB:
    Wrong_judgment_AorBLostFlag = Cross_State_OnlyB_Action(B_X, B_Y);
    Back_Normal_StateFlag = Check_BackNormalState();
    if(Back_Normal_StateFlag == 1  || Wrong_judgment_AorBLostFlag == 1)
    {
        Cross_State = Cross_State_Normal;
    }
    else if(AB_StateFlag == 1)                     //检测到AB存在
    {
        //加入延时
        AB_Exist_Count ++; 
        if(AB_Exist_Count >= AB_Confirm_Frames)
        {
            Cross_State = Cross_State_ABExist;
            AB_Exist_Count = 0;
        }
        
    }
    else if(AB_StateFlag == 0)                   //AB丢失
    {
        AB_Exist_Count = 0;
    }
    break;


    default:
        break;
    }
}



void Cross_State_Normal_Action()              //无十字状态的响应
{
    KL = 0;
    KR = 0;
}




/*=========A,B点均存在时的响应============*/
void Cross_State_ABExist_Action(uint16 A_X, uint16 A_Y, uint16 B_X, uint16 B_Y)             
{   
     int16 x;
     int16 i;
     int16 j;
     int16 Temp_KL = 0;
     int16 Temp_KR = 0;  

	if(A_Y <= GRAY_H - 6 && L_Edge[A_Y + 1] > 0 && L_Edge[Key_Row] > 0)     //向上一点，避免小车视角不够大而导致底边没有边界线
	{
	    // 这里K_L被放大了100倍,用于避免浮点运算
        if((A_Y != Key_Row - 1) && L_Edge[A_Y] != -1 && L_Edge[Key_Row] != -1 )               // 分母下限保护：A_Y=Key_Row-1 时分母为 0 会除零
        {
            Temp_KL = ((int32)(L_Edge[A_Y + 1] - L_Edge[Key_Row]) * 100) / (Key_Row - (int16)A_Y - 1);
            KL = Temp_KL;
        }
	}


	if(B_Y <= GRAY_H - 6 && R_Edge[B_Y + 1] > 0 && R_Edge[Key_Row] > 0)
	{
		    //放大可以避免浮点运算
        if((B_Y != Key_Row - 1) && R_Edge[B_Y] != -1  &&  R_Edge[Key_Row] != -1 )                  // 分母下限保护：B_Y=Key_Row-1 时分母为 0 会除零
        {
            Temp_KR = ((int32)(R_Edge[B_Y + 1] - R_Edge[Key_Row]) * 100) / (Key_Row - (int16)B_Y - 1);
            KR = Temp_KR;
        }
	}

    

		for(i = 1 ; i < Length_A && i < (int16)A_Y; i ++)
		{
             x = (int16)(A_X + 1);	//与边界线统一，初始值为点附近白点的值
			 x = (int16)((int32)(A_X + 1) + (((int32)KL * (int32)i) / 100));//补线的X等于AX + 算出来的偏移
			 if(x < 1)   x = 1;
			 if(x > GRAY_W - 3) x = GRAY_W - 3;
             //加保护。补出来的线一定被夹在AB之间
             if(x >= A_X)
             {
                L_Edge[A_Y - i] = x;
			    tft180_draw_point(x, A_Y - i, RGB565_BLUE); 
             }
             else
             {
                L_Edge[A_Y - i] = -1;
             }

		}

		for(j = 1 ; j < Length_B && j < (int16)B_Y; j ++)
	    {
             x = (int16)(B_X - 1);
			 x = (int16)((int32)(B_X - 1) + (((int32)KR * (int32)j) / 100));
			 if(x < 1)   x = 1;
			 if(x > GRAY_W - 3) x = GRAY_W - 3;
             if(x <= B_X)
             {
                R_Edge[B_Y - j] = x;
			    tft180_draw_point(x, B_Y - j, RGB565_BLUE);
             }
             else
             {
                R_Edge[B_Y - j] = -1;
             }


	    }
}


/*=========只有A点的响应=========*/
uint8 Cross_State_OnlyA_Action(uint16 A_X, uint16 A_Y)
{
    
    uint16 i;
    uint16 j;
    uint16 E_PointX = 0;
    uint16 E_PointY = 0;
    uint8  E_Found  = 0;
    int16  Base_X = 0;          //补线锚点坐标
    int16  Base_Y = 0;          //补线锚点坐标
    int16  Lx_Plus = 0;
    uint8  Out_Flag = 0;
    int16  State4_KL     = 0;                     //缓存左侧补线斜率
    uint8  E_SlopeValid  = 0;                     //E侧斜率是否成功算出过


    //B点丢失：以A点所在列向上找十字上边界，再向上Cross_Top_Gap行，向右找右侧边界点F

    if(A_Y > 3 && A_Y < GRAY_H && A_X > 0 && A_X < GRAY_W)              
    {
        // for(j = A_X; j < (GRAY_W - 2); j ++)
        for(j = A_X; j < (Mid_Linex + 10); j ++)
        {
            E_PointY = 0;                       //每列重置，避免沿用上一列的行号
            for(i = A_Y; i > 2; i --)
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
        else if(A_X != 0 && A_Y != 0 && A_Y > E_PointY)             //基准行不存在，使用A点坐标
        {
            Base_X = A_X;
            Base_Y = A_Y;
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
            Out_Flag = Check_A_or_B_ConfirmLoseOne(2,j);
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

    return Out_Flag;
}

/*=========只有B点的响应=========*/
uint8 Cross_State_OnlyB_Action(uint16 B_X, uint16 B_Y)
{
    uint16 i;
    uint16 j;
    uint16 F_PointX = 0;
    uint16 F_PointY = 0;
    uint8  F_Found  = 0;
    int16  Base_X = 0;          //补线锚点坐标
    int16  Base_Y = 0;          //补线锚点坐标
    int16  Rx_Plus = 0;
    uint8  Out_Flag = 0;
    int16  State5_KR     = 0;                     //右侧补线斜率
    uint8  F_SlopeValid  = 0;                     //F侧斜率是否成功算出过

    //A点丢失：从B点所在列起逐列向左，每列向上找十字上边界，再上移Cross_Top_Gap行判定右边界点F
    if(B_Y > 3 && B_Y < GRAY_H && B_X > 0 && B_X < GRAY_W)
    {
        // for(j = B_X; j > 2; j --)              
        for(j = B_X; j > (Mid_Linex - 10); j --)   //必要：限制搜索宽度
        {
            F_PointY = 0;                       //每列重置，避免沿用上一列的行号
            for(i = B_Y; i > 2; i --)
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
        else if(B_X != 0 && B_Y != 0 && B_Y > F_PointY)   //基准行不存在，使用B点坐标
        {
            Base_X = B_X;
            Base_Y = B_Y;
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
            Out_Flag = Check_A_or_B_ConfirmLoseOne(1,i);
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

    return Out_Flag;
}


/*============AB点丢失的时候的响应===============*/
void Cross_State_ABLost_Action()             
{
    uint8 i;
    uint8 j; 
    uint8 E_PointX = 0;
    uint8 E_PointY = 0;
    uint8 F_PointX = 0;
    uint8 F_PointY = 0;
    uint8 E_Found = 0;
    uint8 F_Found = 0;
    int16 Lx_Plus = 0;
    int16 Rx_Plus = 0;
    uint8 E_CountBlack = 0;         //在一轮统计中，E点周围黑点个数
    uint8 F_CountBlack = 0;         //在一轮统计中，F点周围黑点个数
    uint8 Normal_EPoitn_Flag = 0;  //找到正常E点的标志位
    uint8 Normal_FPoitn_Flag = 0;  //找到正常F点的标志位
    uint8 Expansion_Step = 0;      //膨胀步数，限制最大膨胀次数


    F_Found = 0;
    E_Found = 0;
    Normal_EPoitn_Flag = 0;
    Normal_FPoitn_Flag = 0;

    //E点从左边开始，向上找黑白跳变点，再由这一行向右找黑白跳变点
    for(j = EdgeSkip; j < (GRAY_W - 2); j ++)
    {
        E_PointY = 0;                       
        for(i = LowEdge_Start; i > 2; i --)
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

    
    //F点：从画面右侧开始扫，与E点镜像

    for(j = (GRAY_W - EdgeSkip); j >2; j --)               //逐列向左，j>=3保证j-2不越界
    {
        F_PointY = 0;                                       //每列重置，避免沿用上一列的行号
        for(i = LowEdge_Start; i > 2; i --)
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

    //初步判定之后，对E点进行合规性判断
    if(E_Found == 1 && E_PointX >= 3 && E_PointX <= GRAY_W - 4 && E_PointY + 2 < GRAY_H
        && Image1[E_PointX][E_PointY + 2] == WHITE
        && Image1[E_PointX - 3][E_PointY] == BLACK
        && Image1[E_PointX + 3][E_PointY] == WHITE)
    {
        Normal_EPoitn_Flag = 1;
    }

    //对F点进行合规性判断
    if(F_Found == 1 && F_PointX >= 3 && F_PointX <= GRAY_W - 4 && F_PointY + 2 < GRAY_H
        && Image1[F_PointX][F_PointY + 2] == WHITE
        && Image1[F_PointX - 3][F_PointY] == WHITE
        && Image1[F_PointX + 3][F_PointY] == BLACK)
    {
        Normal_FPoitn_Flag = 1;
    }

    //双侧都合规且高差在范围内时，暂存锚点供后续帧兜底用
    if(Normal_EPoitn_Flag == 1 && Normal_FPoitn_Flag == 1 && ((E_PointY >= F_PointY ? (E_PointY - F_PointY) : (F_PointY - E_PointY)) <= FindEF_Span))
    {
        Last_E_PointX = E_PointX;
        Last_E_PointY = E_PointY;
        Last_F_PointX = F_PointX;
        Last_F_PointY = F_PointY;
        E_LastValid = 1;
        F_LastValid = 1;
    }
    
    //E点自身找到且合规才膨胀
    if(Normal_EPoitn_Flag == 1)
    {
        Expansion_Step = 0;
        while(E_PointX < GRAY_W - 2 && Expansion_Step < Max_Expansion_Step)      //限制右移范围
        {
            E_CountBlack = 0;                                   
            for(i = 0; i < 5; i ++)
            {
                if(Image1[(int8)E_PointX + L_expansionX_Plus[i]][(int8)E_PointY + L_expansionY_Plus[i]] == BLACK)
                {
                    E_CountBlack ++;
                }
            }
            if(E_CountBlack >= Expansion_BlackBlock)            //采满5点后再判定
            {

                E_PointX = E_PointX + 1;
                Image1[E_PointX][E_PointY] = BLACK;    //补黑点
                Expansion_Step ++;
            }
            else
            {
                break;
            }
        }
    }

    //F点自身找到且合规才膨胀
    if(Normal_FPoitn_Flag == 1)
    {
        Expansion_Step = 0;
        while(F_PointX > 1 && Expansion_Step < Max_Expansion_Step)              //限制左移范围，防止膨胀过多
        {
            F_CountBlack = 0;                                   //每轮重新统计5个点
            for(j = 0; j < 5; j ++)
            {
                if(Image1[(int8)F_PointX + R_expansionX_Plus[j]][(int8)F_PointY + R_expansiony_Plus[j]] == BLACK)
                {
                    F_CountBlack ++;
                }
            }
            if(F_CountBlack >= Expansion_BlackBlock)            
            {
                F_PointX = F_PointX - 1;
                Image1[F_PointX][F_PointY] = BLACK;    //补黑点
                Expansion_Step ++;
            }
            else
            {
                break;
            }
        }
    }
   


    for(i = E_PointY; i < (GRAY_H - 1); i ++)
    {
        if(E_Found == 1)
        {
            if(Normal_EPoitn_Flag == 1)
            {
                // Lx_Plus = (int16)((int32)(E_PointX) - (((int32)KL * ((int32)i - (int32)E_PointY)) / 100));           //以E为锚点向下延伸，KL本身含符号
                Lx_Plus = E_PointX;
            }
            else if(Normal_EPoitn_Flag == 0)
            {
                if(E_LastValid == 1)
                {
                    // Lx_Plus = (int16)((int32)(Last_E_PointX) - (((int32)KL * ((int32)i - (int32)Last_E_PointY)) / 100));           //以E为锚点向下延伸，KL本身含符号
                    Lx_Plus = Last_E_PointX;
                }
            }
            if(Lx_Plus < 1) 
            {
            Lx_Plus = 1;
            }  
            if(Lx_Plus > GRAY_W - 3)
            {
                Lx_Plus = GRAY_W - 3;
            }  
            L_Edge[i] = Lx_Plus;
            tft180_draw_point((L_Edge[i]), i, RGB565_BLUE);  
        }
        else
        {
            break;
        }
    	
    }

    for (j = F_PointY; j < (GRAY_H - 1); j ++)
    {
        if(F_Found == 1)
        {
            if(Normal_FPoitn_Flag == 1)
            {
                Rx_Plus = F_PointX;
            }
            else if(Normal_FPoitn_Flag == 0)
            {
                if(F_LastValid == 1)
                {
                    Rx_Plus = Last_F_PointX;
                }
            }
            if(Rx_Plus < 1)
            {
                Rx_Plus = 1;
            }   
            if(Rx_Plus > GRAY_W - 3) 
            {
            Rx_Plus = GRAY_W - 3; 
            } 
            R_Edge[j] = Rx_Plus;
            tft180_draw_point((R_Edge[j]), j,RGB565_BLUE);
        }
        else
        {
            break;
        }
    }

}

/*确认进入A/B单个丢失逻辑*/
//输入参数1： 选择A点或者B点丢失， 1为A丢失， 2为B丢失
//输入参数2： 当前点的Y轴位置
uint8 Check_A_or_B_ConfirmLoseOne(uint8 Choose_AorB_Lose, int16 Point_LocationY)
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


 /*===========检查是否要返回普通状态===========*/
uint8 Check_BackNormalState(void)             
{
   
    uint8 i;
    uint8 Now_Line_Width; 
    uint8 Break_Flag = 0;
    uint8 Qualified_Num = 0;
    uint8 L_startX = 0;
    uint8 R_StopX = 0;


    if(End_Line < 10)               
    {
        return 0;
    }

    for(i = 0; i < 10; i ++)
    {
        for(L_startX = Mid_Linex; L_startX > 2; L_startX -= 2) //找左边起始点
        {
            if(Image1[L_startX][End_Line - i] == BLACK)
            {
                break;
            }
            else if(L_startX < 2)
            {
                L_startX = 2;
                break;
            }
        }

        for(R_StopX = Mid_Linex; R_StopX < GRAY_W - 3; R_StopX += 2) //找右边起始点
        {
            if(Image1[R_StopX][End_Line - i] == BLACK)
            {
                break;
            }
            else if(R_StopX > GRAY_W - 3)
            {
                R_StopX = GRAY_W - 3;
                break;
            }
        }

        if(R_StopX > L_startX)
        {
            Now_Line_Width = R_StopX - L_startX;
            if(Now_Line_Width >= Min_Roadwidth && Now_Line_Width <= Max_RoadWidth)      //检测赛道长度是否在合理范围
            {
                Qualified_Num ++;
            }
        }
        

        if(Qualified_Num >= Qualified_Times)
        {
            Qualified_Num = 0;
            Break_Flag = 1;
            return Break_Flag;  
        }

    }

    return Break_Flag;
}

/*=====检查单侧丢线角点是否具有真实十字的横向白色支路==========*/
//参数：1.选择哪边丢失（1，A点丢失，检查B点    2.B点丢失，检查A点），以及存在点的坐标
//返回：1为真十字单侧丢线，0为弯道伪角点或特征不足
uint8 Check_A_or_B_RealLose(uint8 Lose_Side, uint16 PointX, uint16 PointY)
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


//优化思路
/*
1，状态3中，对E,F点合规的判断改成，E点下方左侧应该有一段白色延伸，F点下方右侧应该有一段白色延伸，并且E,F点要同时存在，且间距不能相差太大
避免使用过期的A,B点坐标，否则准确度不高
*/
