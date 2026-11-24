#include "zf_common_headfile.h"
#include <string.h>
#include "Gray.h"
#include "Element.h"

#define WHITE 0xFF
#define BLACK 0x00
#define Min_Length 8 						//竖向最小长度，用于判断是否为十字路口

#define GRAY_NUM    (GRAY_W * GRAY_H)       //总共取得的点的数量

#define Max_DifAB 4 						//判断为路口时A ,B点的最大高度差
#define Key_Row   (GRAY_H - 6)				//取边界基准点所在的行

//找偏差值相关
#define Max_LoseLineTimes 5                 //找偏差时最大丢线行数，多余这个行数判断为找到的中点不可信
#define Min_RoadWeidth    34				//最小赛道宽度，当宽度小于这个值，过远，得到的中线不考虑
#define Forward_LookingLine_Max   30        //前瞻行的最大值
#define Forward_LookingLine_Min   50        //前瞻行的最小值
#define Frame_Mind     (GRAY_W >> 1)		//设定的中心
#define Confirm_Delay    3                  //确认角点存在的延迟
static const uint8 Weight[9] = {60, 70, 80, 90, 100, 90, 80, 70,  60};           //前瞻区域的权重

/*=================赛道半宽================*/
//半宽从30行一直到第50行
static uint16 Half_Width[21] = {13, 14, 15, 19, 20, 20,
								23, 23, 24, 24, 24, 25,
								26, 27, 28, 30, 31, 33,
								33, 33, 34 };

// 左侧方向偏移
static const int8 LX_Plus[8] = { 1,  1,  0, -1, -1, -1,  0,  1};
static const int8 LY_Plus[8] = { 0, -1, -1, -1,  0,  1,  1,  1};
//右侧方向偏移
static const int8 RX_Plus[8] = {-1, -1,  0,  1,  1,  1,  0, -1};
static const int8 RY_Plus[8] = { 0, -1, -1, -1,  0,  1,  1,  1};

//用于记录角点存在的帧数
static uint8 Key_PointExisi_Flag = 0;

//用于储存找到的左,右边线
int16 L_Edge[GRAY_H];
int16 R_Edge[GRAY_H];

//用于边线显示
uint16 LW_Update = 0;
uint16 LW_UpdateLast = 0;
uint8 L_Times = 0;
uint16 RW_Update = 0;
uint8 R_Times = 0;
uint16 RW_UpdateLast = 0;

//找到的角点
uint16 AX = 0;
uint16 AY = 0;

uint16 BX = 0;
uint16 BY = 0;
	
uint16 EX = 0;				//十字路口远端左侧点
uint16 EY = 0;
	
uint16 FX = 0;				//十字路口远端右侧点
uint16 FY = 0;
uint8  AB_Exist = 0;		//检查A,B是否存在
uint8  OnlyA_or_B = 0;      //十字部分只看到单个角点的标志位      1：只有A点        2： 只有B点
uint8  Out_CrossFlag = 0;   //退出十字的标志位

//补线距离
uint16 Length_A = 0;
uint16 Length_B = 0;

//截止行
uint8 End_Line = 0;        						  //截至行

//储存找到的中线
int16 Mid_Line[GRAY_H];

//偏差计算
int16 Error_Out = 0;
uint16 Forward_LookingLine_Out = 42;	//前瞻行，导出给主循环画线用（Find_Error 在中断里，不能画屏）
int16 Curve_deviation_threshold = 0;                //统计上下部分偏差值
int16 RL_Result = 0;                                //退出判据处的赛道宽度，导出给主循环显示

static uint16 Gray_Hist[256] = {0};
uint8 Image1[GRAY_W][GRAY_H] = {0};			//二值化之后的图片（下采样后 94 x 60）
uint8 NOFind_DiffFlagL = 0;							//假如置1 ，表示丢线
uint8 NOFind_DiffFlagR = 0;							//假如置1 ，表示丢线

/*====================================标准的大津法==============================*/
uint8 Normal_Gray(uint8 frame[MT9V03X_H][MT9V03X_W])	//注意输入的是灰度图
{
	uint16 i;
	uint16 j;
	uint8 Temp_Gray;
	uint32 Total_Gray = 0;			//全局灰度和
	uint32 w0 = 0;					//阈值以下像素数
	uint32 w1 = 0;
	uint32 sum0 = 0;				//阈值以下灰度和
	uint16 m0 = 0;					//阈值以下灰度均值
	uint16 m1 = 0;					//阈值以上灰度均值
	uint16 d = 0;					//两类均值的差
	uint32 score = 0;				//类间方差（放大后）
	uint32 best = 0;				//记录遍历过程中找到的最大值
	uint8  Low_gray = 0;			//记录最佳阈值（0~255）

	/*============对取得的灰度图进行下采样============*/
	for (i = 0 ;i < MT9V03X_H; i += 2)
	{
		for(j = 0 ; j< MT9V03X_W; j += 2)
		{
			Temp_Gray = frame[i][j];            //临时变量，用于读取灰度值
			Gray_Hist[Temp_Gray] ++ ;			//获取直方图
			Total_Gray += Temp_Gray;			
		}
	}

	/*
	Gray_Hist[i]
	|
	|
	|
	|
	|
	—— —— —— ———— —— —— —— i
	*/

	for( i = 0; i <= 255; i ++)
	{
		w1 = GRAY_NUM - w0;			 //w1,阈值以上像素个数
		if(w0 > 0 && w1 > 0)		// 防止w0或w1为0
		{
			m0 = (uint16)(sum0 / w0);					//背景（阈值以下）的整数均值（暗区）
			m1 = (uint16)((Total_Gray - sum0) / w1);	//前景的整数均值（亮区）
			if(m0 > m1)
			{
				d = m0 - m1;
			}
			else
			{
				d = m1 - m0;
			}
			// 类间方差 ∝ w0*w1*d*d，先右移 8 位防止 32 位溢出（比较大小结果不变）
			score = (((w0 * w1) >> 8) * d) * d;
			if(score > best)
			{
				best     = score;
				Low_gray = (uint8)i;
			}
		}
		// 为下一个阈值做增量
		w0   += Gray_Hist[i];
		sum0 += i * Gray_Hist[i];
	}
	memset (Gray_Hist ,0,sizeof(Gray_Hist));//显示一帧后对储存灰度频率的数组清零，防止灰度值累计
	return Low_gray;
}


void Find_Edge(void)   //直接处理全局 Image1（[列][行] 的二值图）
{
	int16  LH;			//当前行号
	int16  LW;			//当前边界列号
	int16  RH;
	int16  RW;
	uint8  Lfound;		// 置0 ，表示进入丢线逻辑
	uint8  Rfound;
	int16  LastX_L;		//上一个边界点
	int16  LastY_L;
	int16  LastX_R;
	int16  LastY_R;
	int16  step;		//步数计数
	int16  step1;
	int16  nx;
	int16  ny;
	int16  y;
	int16  r;
	int16  x_right;
	int16  x_left;
	uint8  re_found;	//重新找到线的标志位
	uint8  Fake_Edge = 0;   //如果是在边界点处向上延伸得到的LX，那么这个标志位置1
	int16  i;
	int16  j;


	/*===================把储存边界点的数组全部置为-1，防止中线错误出现=====================*/
	memset(L_Edge, -1, sizeof(L_Edge));
	memset(R_Edge, -1, sizeof(R_Edge));


	AB_Exist = 0;
	Length_A = 0;
	Length_B = 0;
	/*========================================左侧寻线=========================================*/
	LH = GRAY_H - 1;		//当前行号
	LW = 0;					//当前边界列号
	NOFind_DiffFlagL = 0;	//每帧开始时清除丢线标志
	Lfound = 1;				// 置0 ，表示进入丢线逻辑
	//找第一个边界
	for(i = GRAY_W / 2; i > 0 ; i --)
	{
		if(Image1[i][LH] == WHITE && Image1[i - 1][LH] == BLACK)
		{
			LW = i;
			Fake_Edge = 0;
			break ;
		}
		else if(i == 1)
		{
			Lfound = 0;
			Fake_Edge = 1;			//底边没找到：本行不写值，但继续向上搜索，由丢线重搜找回真实边界
		}
	}

	LastX_L = 0;
	LastY_L = 0;
	step = 0;

	while(LH >= 0 && NOFind_DiffFlagL == 0 && step < 400)
	{

		tft180_draw_point(LW, LH, RGB565_GREEN);
		if(Fake_Edge != 1)
		{
			L_Edge[LH] = LW;			// 角标是行数，内容是列数
		}
		
		step++;
		Lfound = 0;

		//假如爬到图像边缘并且图像上方是白色，那么向上延伸即可
		if(LW <= 2 && LH >= 3 && (Image1[LW][LH - 1] == WHITE || Image1[LW][LH - 3] == WHITE))//检测上方是否为白色像素
		{
			LastX_L = LW;
			LastY_L = LH;
			LH = LH - 1;
			Fake_Edge = 1;               //虚假的边界
			Lfound = 1;
		}

		//逆时针搜索
		if(Lfound == 0)
		{
			for(i = 0; i < 8; i++)       //设置寻找点的坐标偏移
			{
				nx = LW + LX_Plus[i];
				ny = LH + LY_Plus[i];

				//越界检查
				if(nx < 1 || nx >= GRAY_W || ny < 0 || ny >= GRAY_H)
				{
					continue;
				}
				//不返回上一个点，防止两个点之间来回震荡
				if(nx == LastX_L && ny == LastY_L)
				{
					continue;
				}

				if((Image1[nx][ny] == WHITE && Image1[nx - 1][ny] == BLACK)
					 || (ny < GRAY_H - 1 && (Image1[nx][ny] == WHITE && Image1[nx][ny + 1] == BLACK) ) )//针对横向的边界判断,(此时，只检测下边界，上边界舍弃)
				{
					LastX_L  = LW;
					LastY_L  = LH;
					LW = nx;
					LH = ny;
					Lfound = 1;
					Fake_Edge = 0;
					break ;
				}
			}
		}

		//八邻域找不到，那么在当前点的左右两边分别向外找3个像素
		if(Lfound == 0 && LH > 0)
		{
			y = LH ;
			for(r = 2; r <= 3 && !Lfound; r++)
			{
				x_right = LW + r;
				x_left  = LW - r;

				if(x_right >= 2 && x_right < GRAY_W - 1 &&
				   Image1[x_right][y] == WHITE && Image1[x_right - 1][y] == BLACK)
				{
					LastX_L = LW;
					LastY_L = LH;
					LW = x_right;
					LH = y;
					L_Edge[LH] = LW;		// 角标是行数，内容是列数
					Fake_Edge = 0;
					Lfound = 1;
					break ;
				}
				if(x_left >= 2 && x_left < GRAY_W - 1 &&
				   Image1[x_left][y] == WHITE && Image1[x_left - 1][y] == BLACK)
				{
					LastX_L = LW;
					LastY_L = LH;
					LW = x_left;
					LH = y;
					L_Edge[LH] = LW;		// 角标是行数，内容是列数
					Fake_Edge = 0;
					Lfound = 1;
					break ;
				}
			}
		}

		//丢线逻辑
		if(Lfound == 0)
		{
			NOFind_DiffFlagL = 1;		//丢线
			re_found = 0 ;				//重新找到线的标志位

			//进入丢线逻辑，从最后一次找到边界的位置向上找
			for (i = LH - 1 ; i >= 0 && re_found == 0; i--)
			{
				//横向方向寻找
				for (j = GRAY_W / 2 ; j >= 1; j--)
				{
					if ((Image1[j][i] == WHITE && Image1[j - 1 ][i] == BLACK)
						 || (Image1[j][i] == WHITE && Image1[j][i - 1] == BLACK))
					{
						LW = j;
						LH = i;
						L_Edge[LH] = LW;
						NOFind_DiffFlagL = 0 ;
						Fake_Edge = 0;
						re_found = 1 ;
						break ;
					}
				}
			}
		}
	 }

	//重置
	LH = GRAY_H - 1;

	/*===================右侧寻线==================*/
	RH = GRAY_H - 1;		//当前行号
	RW = GRAY_W - 1;        //底边没有找到，那么初始值应该设置为最右侧					
	NOFind_DiffFlagR = 0;	//每帧开始时清除丢线标志
	Rfound = 1;				//置0，表示进入丢线逻辑

	//找第一个边界
	for(i = GRAY_W / 2; i <= GRAY_W - 2 ; i ++)
	{
		if(Image1[i][RH] == WHITE && Image1[i + 1][RH] == BLACK)
		{
			RW = i;
			Fake_Edge = 0; 
			break ;
		}
		else if(i == GRAY_W - 2)
		{
			Rfound = 0;
			Fake_Edge = 1;			//底边没找到：本行不写值，但继续向上搜索，由丢线重搜找回真实边界
		}
	}
	
	LastX_R = 0;
	LastY_R = 0;
	step1 = 0;
	
	while(RH >= 0 && NOFind_DiffFlagR == 0 && step1 < 400)
	{

		tft180_draw_point(RW, RH, RGB565_CYAN);  //青色
		if(Fake_Edge != 1)
		{
			R_Edge[RH] = RW;
		}
		step1++;
		Rfound = 0;		//初始时，清零，保证逻辑可以正常进入

		//爬到图像右边界且上方是白色,贴着图像边缘直接向上延伸
		if(RW >= GRAY_W - 3 && RH >= 3 && (Image1[RW][RH - 1] == WHITE || Image1[RW][RH - 3] == WHITE))
		{
			LastX_R = RW;
			LastY_R = RH;
			RH = RH - 1;
			Fake_Edge = 1;
			Rfound = 1;
		}

		//顺时针搜索
		if(Rfound == 0)
		{
			for(i = 0; i < 8; i++)
			{
				nx = RW + RX_Plus[i];
				ny = RH + RY_Plus[i];

				//越界检查
				if(nx < 1 || nx >= GRAY_W - 1 || ny < 0 || ny >= GRAY_H)
				{
					continue;
				}
				//不返回上一个点，防止两个点之间来回震荡
				if(nx == LastX_R && ny == LastY_R)
				{
					continue;
				}

				if((Image1[nx][ny] == WHITE && Image1[nx + 1][ny] == BLACK)
					 ||( ny < GRAY_H - 1 && (Image1[nx][ny] == WHITE  &&  Image1[nx][ny + 1] == BLACK) ) )
				{
					LastX_R  = RW;
					LastY_R  = RH;
					RW = nx;
					RH = ny;
					R_Edge[RH] = RW;
					Fake_Edge = 0;
					Rfound = 1;
					break ;
				}
			}
		}

		//八邻域找不到，那么在当前点的左右两边分别向外找3个像素
		if(Rfound == 0 && RH > 0)
		{
			y = RH;
			for(r = 2; r <= 3 && !Rfound; r++)
			{
				x_right = RW + r;
				x_left  = RW - r;

				if(x_right >= 2 && x_right < GRAY_W - 1 &&
				   Image1[x_right][y] == WHITE && Image1[x_right + 1][y] == BLACK)
				{
					LastX_R = RW;
					LastY_R = RH;
					RW = x_right;
					RH = y;
					R_Edge[RH] = RW;
					Fake_Edge = 0;
					Rfound = 1;
					break ;
				}
				if(x_left >= 2 && x_left < GRAY_W - 1 &&
				   Image1[x_left][y] == WHITE && Image1[x_left + 1][y] == BLACK)
				{
					LastX_R = RW;
					LastY_R = RH;
					RW = x_left;
					RH = y;
					R_Edge[RH] = RW;
					Fake_Edge = 0;
					Rfound = 1;
					break ;
				}
			}
		}

		if(Rfound == 0 )
		{
			NOFind_DiffFlagR = 1;		//丢线
			re_found = 0;				//重新找到线的标志位
			for(i = RH - 1 ; i >= 0 && re_found == 0; i--)
			{
				for(j = GRAY_W / 2 ; j <= GRAY_W - 2 ; j++)
				{
					if ((Image1[j][i] == WHITE && Image1[j + 1 ][i] == BLACK)
						|| (Image1[j][i] == WHITE && Image1[j][i - 1] == BLACK))
					{
						RW = j;
						RH = i;
						R_Edge[RH] = RW;
						Fake_Edge = 0;
						NOFind_DiffFlagR = 0 ;
						re_found = 1 ;	//重新找到线
						break ;
					}
				}
			}
		}
     }

	//重置
	RH = GRAY_H - 1;
}


void Find_KeyPoint(int16 L_Edge[GRAY_H] , int16 R_Edge[GRAY_H])
{
	/*===============图像处理====================*/
	//1.首先逆透视再用向量法
	//2.传统一点，直接处理

	/*
	 1方法
	*/

	/*2方法*/
	//定义十字路口处，靠近摄像头的边的直角为A点，右侧为B点 ，
	
	uint8  Found_A = 0;			//A点找到标志
	uint8  Found_B = 0;			//B点找到标志
	uint16 CY = 0;				//从A向上找到的黑白交界处的黑点
	uint16 DY = 0;				//类似
	int16  i;
	int16  j;
	uint16 Dif_AB = 0;			//A,B两点的行差


	AB_Exist = 0;
	OnlyA_or_B = 0;
	Length_A = 0;			//每帧清零，避免上一帧陈旧值导致 A_Y-i 无符号下溢越界
	Length_B = 0;
	

	//找A点
	if(L_Edge[Key_Row] >= 2 && L_Edge[Key_Row] <= GRAY_W - 3 )    
	{
		AX = L_Edge[Key_Row];		//初始值设置为底边的边界点，并以此向上延伸

		//找A点
		for(i = Key_Row; i > 2; i--)
		{
			for(j = AX; j < GRAY_W - 2; j ++)
			{
				if(Image1[j][i] == WHITE && Image1[j - 2][i] == BLACK)
				{
					AX = j - 2;		//找黑点
					break;
				}
			}
			if(Image1[AX][i] == BLACK && Image1[AX][i - 2] == WHITE && Image1[AX][i - 3] == WHITE)//当前黑点，上方连续2行白（与右侧对称，抗噪）
			{
				AY = i;
				Found_A = 1;             //找到A点
				break;
			}
		}
	
		//找到A点才显示并计算左探针长度，未找到则跳过
		if(Found_A == 1)
		{
			for(i = 1; i <= 3; i ++)
			{
				for(j = 1; j <= 3; j ++)
				{
					tft180_draw_point(AX + j , AY - i, RGB565_PINK);  //显示A点
				}
			}
			
			//从A点向上找边界
			for(i = AY - 1; i > 2; i -- )
			{
				if(Image1[AX][i] == WHITE && Image1[AX][i - 2] == BLACK)
				{
					CY = i - 1;
					Length_A = AY - CY;
					break;
				}
			}
		}

	}



	//找B点
	if(R_Edge[Key_Row] >= 1 && R_Edge[Key_Row] <= GRAY_W - 3)     
	{
		BX = R_Edge[Key_Row];
		//找B点
		for(i = Key_Row; i > 2; i--)
		{
			for(j = BX; j >= 1; j--)
			{
				if(Image1[j][i] == WHITE && Image1[j + 2][i] == BLACK)
				{
					BX = j + 2;
					break;
				}
			}
			if(Image1[BX][i] == BLACK && (Image1[BX][i - 2] == WHITE && Image1[BX][i - 3] == WHITE))
			{
				BY = i;
				Found_B = 1;           //找到B点
				break;
			}
		}

		//找到B点才显示并计算右探针长度
		if(Found_B == 1)
		{
			for(i = 1; i <= 3; i ++)
			{
				for(j = -2; j <= 0; j ++)
				{
					tft180_draw_point(BX + j, BY - i, RGB565_BROWN);            //显示B点
				}
			}
			
			for(i = BY - 1; i > 2; i -- )
			{
				if(Image1[BX][i] == WHITE && Image1[BX][i - 1] == BLACK && Image1[BX][i - 2] == BLACK)
				{
					DY = i - 1;
					Length_B = BY - DY;
					break;
				}
			}
		}
	}

	// 假如两侧”探针”的长度在最短长度之上,并且，AB两点差不多在同一水平线，判定为十字路口
	if(AY < BY)//A点靠上
	{
		Dif_AB = BY - AY;
	}
	if(AY > BY)//A点靠下
	{
		Dif_AB = AY - BY;
	}


	if(Found_A && Found_B && Length_A >= Min_Length && Length_B >= Min_Length && Dif_AB < Max_DifAB)  //确定为十字路口
	{
		AB_Exist = 1;
	}
	else
	{
		AB_Exist = 0;
	}


	if(Found_A == 1 && Length_A >= Min_Length && AB_Exist == 0)          //仅左侧合格
	{
		OnlyA_or_B = 1;                         
	}
	else if (Found_B == 1 && Length_B >= Min_Length && AB_Exist == 0)    //仅右侧合格
	{
		OnlyA_or_B = 2; 
	}

}

void Find_MidLine(void)
{
	int16 i = 0;
	//找中线
	memset(Mid_Line , -1 , sizeof(Mid_Line));						//初始化为-1
	if(Distinguish_Circle_Cross(AY,BY,AB_Exist,OnlyA_or_B) == 2)    //判断为十字
	{
		Cross_Statemachine(AX, AY, BX, BY, AB_Exist,OnlyA_or_B);
	}
	else
	{
		//圆环状态机
	}
	

	for(i = GRAY_H - 1 ; i >= 0 ; i --)
	{
		if(L_Edge[i] >= 0 && R_Edge[i] > 0 )						//假如，左右两边都可以找到边界（左边界贴在图像第0列也是合法的）
		{
			Mid_Line[i] = (L_Edge[i] + R_Edge[i]) >> 1;
		}
		else if(RE_State == Cross_State_OnlyA && L_Edge[i] >= 0 && i >= 30 && i <= 49)
		{
			Mid_Line[i] = L_Edge[i] + Half_Width[i - 30];			//OnlyA：左侧为人为补出的边界，右侧丢线时用半宽兜底
			// Mid_Line[i] = (L_Edge[i] + GRAY_W) >> 1;					//OnlyA：左侧为人为补出的边界，右侧丢线时用半宽兜底
			if(Mid_Line[i] >= GRAY_H)
			{
				Mid_Line[i] = -1;                                   //超出边界，失效
			}
		}
		else if(RE_State == Cross_State_OnlyB && R_Edge[i] >= 0 && i >= 30 && i <= 49)
		{
			Mid_Line[i] = R_Edge[i] - Half_Width[i - 30];
			// Mid_Line[i] = R_Edge[i] >> 1;
			if(Mid_Line[i] <= 0)
			{
				Mid_Line[i] = -1;                                   //超出边界，失效
			}
		}
		else if(R_Edge[i] < 0 && L_Edge[i] >= 0)					//右侧丢线且左侧有效（含左边界贴第0列）
		{
			if(i >= GRAY_H - 10)
			{ 
				Mid_Line[i] = (L_Edge[i] + GRAY_W) >> 1; 
			}			
		}
		else if(L_Edge[i] <= 0 && R_Edge[i] > 0)					//左侧丢线且右侧有效
		{
			if(i >= GRAY_H - 10)
			{
				Mid_Line[i] = R_Edge[i] >> 1;
			}
		}
		
		if(Mid_Line[i] >= 0 )										//左右都丢线时为-1，不画，防止画到界外
		{
			tft180_draw_point(Mid_Line[i], i, 0xF800 );				// 坐标为下采样后的坐标，可直接显示
		}
	}
}
	
int16 Find_Error(int16 Mid_Line[], int16 L_Edge[], int16 R_Edge[])
{
	uint8 Lose_LineTimes  = 0 ;                       //丢线次数
	uint8 Forward_LookingLine = 0;                    //前瞻行，指定前进方向，因为摄像头算法有延迟，计算出结果时，小车已经前进一段距离
	int32 ErrorWeighted_Sum = 0;                      //偏差加权和
	int32 Weighted_Sum = 0;						      //权重和
	int16 Top_threshold = 0;						  //前瞻区域顶部两行偏差
	int16 Lower_threshold = 0;						  //前瞻区域底部两行偏差
	int16 i;
	End_Line = 0;        						  	  //截至行

	RL_Result = 0;
	Curve_deviation_threshold = 0;			  //判断区域上下两部分权重偏差（语句须在声明之后，C251 C89 规则）
	for(i = (GRAY_H - 2); i > 0; i --)				//从底部开始找					
	{
		if(Mid_Line[i] < 0)
		{
			Lose_LineTimes ++;
			if(Lose_LineTimes >= Max_LoseLineTimes)      //计算丢线次数
			{
				Lose_LineTimes = 0;
				End_Line = GRAY_H - 10;							
				break;
			}
		}
		else                                          //Mid_Line[i] >= 0，有效行：清零计数并检查赛道宽度
		{
			Lose_LineTimes = 0;
			if(L_Edge[i] > 0 && R_Edge[i] > 0 && (R_Edge[i] - L_Edge[i]) < Min_RoadWeidth )     //判定赛道宽度是否比最小值大
			{
				End_Line = i;
				RL_Result = (R_Edge[i] - L_Edge[i]);		//导出给主循环显示（本函数k在3ms中断里，不能直接画屏）
				break;
			}
		}
	}

	if(End_Line != 0)
	{
		Forward_LookingLine = End_Line + 3;		//只算数值，画线挪到主循环（本函数在3ms中断里跑，软SPI不可重入）
	}
	else
	{
		Forward_LookingLine = 38;                //如果End_line没找到，默认值
	}

	if(Forward_LookingLine > Forward_LookingLine_Min )
	{
		Forward_LookingLine = Forward_LookingLine_Min;
	}
	else if (Forward_LookingLine < Forward_LookingLine_Max)
	{
		Forward_LookingLine = Forward_LookingLine_Max;
	}
	Forward_LookingLine_Out = Forward_LookingLine;	//导出给主循环画前瞻线
	
	for(i = -4; i <= 4; i ++ )    //从截至行往下开始
	{
		if(Mid_Line[(Forward_LookingLine + i)] >= 0)
			{
				ErrorWeighted_Sum += ( (Mid_Line[(Forward_LookingLine + i)] - Frame_Mind) * Weight[(i + 4)]);
				Weighted_Sum +=  Weight[(i + 4)];
				if((Forward_LookingLine + i ) < (Forward_LookingLine - 2))
				{
					Top_threshold += (Mid_Line[(Forward_LookingLine + i)] - Frame_Mind);
				}
				else if((Forward_LookingLine + i) > (Forward_LookingLine +  2))
				{
					Lower_threshold += (Mid_Line[(Forward_LookingLine + i)] - Frame_Mind);
				}
			}
		else
		{
			continue;
		}
	}
	if(Top_threshold != 0 && Lower_threshold != 0)
	{
		Curve_deviation_threshold = Top_threshold - Lower_threshold;
	}

	if(Weighted_Sum > 0)			
	{
		Error_Out = (int16)(((ErrorWeighted_Sum * 5) / Weighted_Sum) * 2);		
	}
	return Error_Out;
}



/*
传入参数 1—4 A,B点的坐标
		 5：A,B点是否同时存在
		 6：A,B点哪个点单独出现 （1.A点    2.B点）

返回参数 0. 无意义
		1，判断为圆环
		2. 判断为十字
*/
uint8 Distinguish_Circle_Cross(uint16 A_Y, uint16 B_Y, uint8 AB_StateFlag, uint8 Only_AorB_StateFlag)
{
	uint8 i;
	uint8 Quallied_Num;          //左侧/右侧边界有值的行数

	Quallied_Num = 0;

	if(AB_StateFlag == 1)  //A,B点同时存在
	{
		//加延时确认
		Key_PointExisi_Flag ++;
		if(Key_PointExisi_Flag >= Confirm_Delay)
		{
			Key_PointExisi_Flag = 0;
			return 2;
		}
	}
	else
	{
		Key_PointExisi_Flag = 0;
	}


	switch (Only_AorB_StateFlag)
	{
		case 1:
			Key_PointExisi_Flag ++;
			if(Key_PointExisi_Flag >= Confirm_Delay)
			{
				Key_PointExisi_Flag = 0;
				for(i = A_Y; i >= (A_Y - 10); i --)
				{
					if(R_Edge[i] >= 0)
					{
						Quallied_Num ++;
					}
				}
				if(Quallied_Num >= 6)            //右侧边界连续，是圆环
				{
					return 1;
				}
				else
				{
					return 2;
				}
			}
		break;
	
		case 2:
			//加延时确认
			Key_PointExisi_Flag ++;
			if(Key_PointExisi_Flag >= Confirm_Delay)
			{
				Key_PointExisi_Flag = 0;
				for(i = B_Y; i >= (B_Y - 10); i --)
				{
					if(L_Edge[i] >= 0)
					{
						Quallied_Num ++;
					}
				}
				if(Quallied_Num >= 6)
				{
					return 1;
				}
				else
				{
					return 2;
				}
			}
		break;
	
		default:
		break;
	}


	return 0;
}
/*
说明
可优化的点 1.在丢线之后重新寻找的逻辑里面，可以不用从中线开始找，从最后一个找到点的位置，同时向左右两边找即可
BUG1.左侧或者右侧在其中一边丢线的时候，进入补中线的条件缺少等于0，此时，由于八邻域仍在运行见下方
		if(LW <= 2 && LH >= 3 && (Image1[LW][LH - 1] == WHITE || Image1[LW][LH - 3] == WHITE))//检测上方是否为白色像素
		{
			LastX_L = LW;
			LastY_L = LH;
			LH = LH - 1;
			Lfound = 1;
			L_Edge[LH] = LW ;
		}
可见L_Edge的值变成了0，故无法进入补中线的逻辑
BUG2.在边缘向上的途中，会把左右边界的值设为0与187，最终导致得到错误中线
修改.在八邻域判断横向部分改为仅判断十字的下边缘，假如连上边缘也一起判断，那么在一边丢线的时候，丢线一侧的爬线会不可避免的爬到另一边，目前没有好的解决办法。
*/
