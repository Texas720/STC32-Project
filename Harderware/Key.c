#include "zf_common_headfile.h"
#include "Key.h"

#define Key_1               IO_PB0
#define Key_2               IO_PB1
#define Key_3               IO_PB2
#define Key_4               IO_PB3
#define Key_5               IO_PB4

uint8 Key_Slow = 0;                

uint8 Key_Value = 0,Key_Old = 0;
uint8 Key_Down = 0,Key_Up = 0;
uint16 Angle_Set = 0;

void Key_Init()
{
    gpio_init(Key_1,GPI,1,GPI_PULL_UP);
    gpio_init(Key_2,GPI,1,GPI_PULL_UP);
    gpio_init(Key_3,GPI,1,GPI_PULL_UP);
    gpio_init(Key_4,GPI,1,GPI_PULL_UP);
    gpio_init(Key_5,GPI,1,GPI_PULL_UP);
}

uint8 Key_Read()
{
    uint8 Key_Temp = 0x00;

    if (gpio_get_level(Key_1) == 0)
        Key_Temp = 1;
    if (gpio_get_level(Key_2) == 0)
        Key_Temp = 2;
    if (gpio_get_level(Key_3) == 0)
        Key_Temp = 3;
    if (gpio_get_level(Key_4) == 0)
        Key_Temp = 4;
    if (gpio_get_level(Key_5) == 0)
        Key_Temp = 5;

    return Key_Temp;
}

void Key_Process()
{
    if (Key_Slow != 0)      return;
    Key_Slow = 1;

    Key_Value = Key_Read();
    Key_Down = Key_Value & (Key_Value ^ Key_Old);                   
    Key_Up = ~Key_Value & (Key_Value ^ Key_Old);
    Key_Old = Key_Value;

    if (Key_Down == 1)
    {
        Angle_Set += 100;
    }

    if (Key_Down == 2)
    {
        Angle_Set -= 100;
    }

    if (Key_Down == 3)
    {
        Angle_Set += 10;
    }

    if (Key_Down == 4)
    {
        Angle_Set -= 10;
    }

        
}