
////////////////////////////////////////////////////////////////////
//每日一拜，好过省赛                            每日多拜，稳赢国赛//
//                          _ooOoo_                               //
//                         o8888888o                              //
//                         88" . "88                              //
//                         (| ^_^ |)                              //
//                         O\  =  /O                              //
//                      ____/`---'\____                           //
//                    .'  \\|     |//  `.                         //
//                   /  \\|||  :  |||//  \                        //
//                  /  _||||| -:- |||||-  \                       //
//                  |   | \\\  -  /// |   |                       //
//                  | \_|  ''\---/''  |   |                       //
//                  \  .-\__  `-`  ___/-. /                       //
//                ___`. .'  /--.--\  `. . ___                     //
//              ."" '<  `.___\_<|>_/___.'  >'"".                  //
//            | | :  `- \`.;`\ _ /`;.`/ - ` : | |                 //
//            \  \ `-.   \_ __\ /__ _/   .-` /  /                 //
//      ========`-.____`-.___\_____/___.-`____.-'========         //
//                           `=---='                              //
//      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^        //
//             佛祖保佑     永无BUG      车魂大悦                  //
////////////////////////////////////////////////////////////////////




/*********************************************************************************************************************
* STC32G144K Opensourec Library 即（STC32G144K 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2025 SEEKFREE 逐飞科技
*
* 本文件是STC32G144K开源库的一部分
*
* STC32G144K 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          
* 公司名称          成都逐飞科技有限公司
* 版本信息          查看 libraries/doc 文件夹内 version 文件 版本说明
* 开发环境          MDK FOR C251
* 适用平台          STC32G144K
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者           备注
* 2025-11-20        大W            first version
********************************************************************************************************************/
#include "zf_common_headfile.h"
#include "Gray.h"
#include "Servo.h"
#include "Key.h"
#include "pit.h"
#include "Motor.h"
#include "Encoder.h"
#include "Element.h"

#define LED1                    (IO_P52 )

#define WIFI_SSID_TEST          "TP-LINK_5FFF"
#define WIFI_PASSWORD_TEST      NULL
#define WHITE  0xFF
#define BLACK  0x00
#define interval_OTSU           4
int Output;

int16 kp,kd;

int16 yaw=0;
int16 acc_y=0;
int16 acc_y_filt;

//找到的边界
extern int16 L_Edge[GRAY_H];
extern int16 R_Edge[GRAY_H];
// extern uint8 Servo_ReturnNormal;                   //[已注释] 舵机回正标志
extern int16 Temp_Error;


extern uint16 Angle_Set;
//找到的最佳阈值
uint8 Final_Gray;
int16 Out = 0;

void tft180_showcolorTest(void)
{
	uint8 i = 0;
	uint16 color[3];
	color[0] = RGB565_RED;
	color[1] = RGB565_PINK;
	color[2] = RGB565_BLUE;
	for(i = 0; i < 3; i ++)
	{
		tft180_clear(color[i]);
		tft180_show_string(60,60,"Texas720");
		system_delay_ms(500);
	} 
}


void main(void)
{
		uint8 i;                        
		uint8 j;
		uint8 K = 0;
		uint8 FPS = 0; 
		uint8 Flag1 ;
		clock_init(SYSTEM_CLOCK_96M); 				// 时钟配置及系统初始化<务必保留>
		debug_init();                       		// 调试串口信息初始化
		mt9v03x_init();
		mt9v03x_set_exposure_time(100);
		tft180_init();
		Servo_Init();
		Key_Init();
		pit_ms_init(TIM1_PIT, 3, pit_tim0);
		pit_ms_init(TIM2_PIT, 10, pit_tim1);
		pit_ms_init(TIM3_PIT, 1000, pit_tim2);
		Motor_Init();
		Encoder_Init();
		

	while (1)
	{
		Flag1 = Tick;                       // 读 1s 节拍标志
		// tft180_show_string(20,62,"LS");
		// tft180_show_string(20,92,"RS");
		// tft180_show_int16(40, 62, L_Speed);
		// tft180_show_int16(40, 92, R_Speed);
		
		if(Flag1 == 1)
		{
			Tick = 0;
			tft180_show_uint8(60,130,FPS);		//1s 到，显示并清零帧计数
			FPS = 0;

		}

		if(mt9v03x_finish_flag == 1)
		{
			K ++;
			mt9v03x_finish_flag = 0;                 // 清除帧完成标志，等待下一帧
			if(K == 2)
			{
				K = 0;
				Final_Gray = Normal_Gray(mt9v03x_image);
			}
			
			//灰度图二值化
			for(i = 0 ; i < GRAY_H; i ++)				//行数（下采样后）
			{
				for(j = 0; j < GRAY_W; j ++)			//列数（下采样后）
				{
					if(mt9v03x_image[i * 2][j * 2] > Final_Gray)
					{
						Image1[j][i] = WHITE;			// Image1 是 [列][行]，与 Find_Edge 的用法保持一致
					}
					else
					{
						Image1[j][i] = BLACK;
					}
				}

			}
			// 直接显示原灰度图并按阈值二值，避免把列优先的 Image1 当行优先数组读取
			tft180_show_gray_image(0, 0, mt9v03x_image[0], MT9V03X_W, MT9V03X_H, MT9V03X_W / 2, MT9V03X_H / 2, Final_Gray);
			Find_Edge();
			Find_KeyPoint(L_Edge, R_Edge);
			Find_MidLine();
			//紫色前瞻线在主循环画（Find_Error 在 3ms 中断里，画屏会和这里的灰度图传输冲突）
			for(j = 0; j < GRAY_W; j ++)
			{
				tft180_draw_point(j, Forward_LookingLine_Out, RGB565_PURPLE);
			}
			FPS ++ ;
		}


		//   tft180_show_int16(0,90,Turn_Result);
		//   tft180_show_uint8(0,130,RE_State);
		  
		  
	}
	


}
