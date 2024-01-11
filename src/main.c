/***************************************************************************************************
                                   ExploreEmbedded Copyright Notice 
 ****************************************************************************************************
 * File:   oled_i2c.c
 * Version: 16.0
 * Author: ExploreEmbedded
 * Website: http://www.exploreembedded.com/wiki
 * Description: SSD1306 I2C OLED library to display strings, numbers, graphs and logos

This code has been developed and tested on ExploreEmbedded boards.  
We strongly believe that the library works on any of development boards for respective controllers. 
Check this link http://www.exploreembedded.com/wiki for awesome tutorials on 8051,PIC,AVR,ARM,Robotics,RTOS,IOT.
ExploreEmbedded invests substantial time and effort developing open source HW and SW tools, to support consider 
buying the ExploreEmbedded boards.

The ExploreEmbedded libraries and examples are licensed under the terms of the new-bsd license(two-clause bsd license).
See also: http://www.opensource.org/licenses/bsd-license.php
EXPLOREEMBEDDED DISCLAIMS ANY KIND OF HARDWARE FAILURE RESULTING OUT OF USAGE OF LIBRARIES, DIRECTLY OR
INDIRECTLY. FILES MAY BE SUBJECT TO CHANGE WITHOUT PRIOR NOTICE. THE REVISION HISTORY CONTAINS THE INFORMATION 
RELATED TO UPDATES.

Permission to use, copy, modify, and distribute this software and its documentation for any purpose
and without fee is hereby granted, provided that this copyright notices appear in all copies 
and that both those copyright notices and this permission notice appear in supporting documentation.
 **************************************************************************************************/ 

#include "8051.h"
#include "oled_i2c.h"
#include "i2c.h"
#include "MPU6050.h"
#include "delay.h"

#define doodle_row 2
#define doodle_col 16

#define bar_row 2
#define bar_col 8

int data_buf;
char i = 0;

int accel_data[3] = {0, 0, 0};  // Stores the 16-bit signed accelerometer sensor output
int gyro_data[3] = {0, 0, 0};   // Stores the 16-bit signed gyro sensor output


// unsigned char const doodle[] = {
//         0xF8, 0x04, 0x02, 0x02, 0x01, 0x01, 0x01, 0x11, 0x01, 0x12, 0x04, 0x18, 0xA0, 0xA0, 0xA0, 0xE0,
//         0x0F, 0xFA, 0x8A, 0x0A, 0xFA, 0x8A, 0x0A, 0xFA, 0x8A, 0x0A, 0x8A, 0x8F, 0x00, 0x00, 0x00, 0x01
// };

unsigned char const doodle [] = {
    0xB6, 0x92, 0x92, 0x92, 0xFF, 0x01, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x81, 0x01, 0x02, 0x0C, 0xF0,
    0x0D, 0x04, 0x04, 0x04, 0x0F, 0x08, 0x0F, 0x88, 0xF0, 0x80, 0xF0, 0x8A, 0x08, 0x04, 0x02, 0x01
};

unsigned char const bar [] = {
    0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x7F, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00
};
// size 41
#define jump_len 12
signed char const jump_dist [] = {
    24, 16, 16, 8, 4, 1,
    -1, -4, -8, -16, -16, -24
};


signed char block_row[5] = {4, 2, 1, 5, 1};
signed char block_col[5] = {25, 41, 65, 77, 85};

unsigned int extend_TI;
int doodle_traj_idx = 0;
signed char doodle_now_row = 3, doodle_now_col = 0;
signed char doodle_pre_row = 3, doodle_pre_col = 0;
int bar_shift = 0;
int shift_cnt = 0;
signed char lock_with_doodle = -1;
#define shift_thres 0
unsigned char random_num = 15;
signed char random_range_sel = 0;
unsigned char rand(char a, char b) {
    char lsb = ((random_num >> 6) ^ (random_num >> 4) ^ (random_num >> 1)) & 1;
    random_num = (random_num << 1) | (lsb); 
    return a + (random_num % (b-a));
}

signed char min(signed char a, signed char b) {
    return (a < b ? a : b);
}
signed char max(signed char a, signed char b) {
    return (a > b ? a : b);
}
void T0_isr(void) __interrupt (1) {
    TH0 = (65536-1000) / 256;
    TL0 = (65536-1000) % 256;
    extend_TI = (extend_TI+1) % 2;
    if (extend_TI == 0) {
        readMPU6050_AccelData(&accel_data[0]);

        for (int i = 0; i < doodle_row; ++i) {
            OLED_SetCursor(doodle_now_row+i, doodle_now_col);
            for (int j = 0; j < doodle_col; ++j) {
                oledSendData(0x00);
            }
        }

        if (accel_data[0] > 3000) {
            shift_cnt++;
        }
        else if (accel_data[0] < -3000) {
            shift_cnt--;
        }
        if (shift_cnt > shift_thres) {
            doodle_now_row += 1;
            shift_cnt = 0;
        }
        else if (shift_cnt < -shift_thres) {
            doodle_now_row -= 1;
            shift_cnt = 0;
        }
        if (doodle_now_row == 7) doodle_now_row = 6;
        else if (doodle_now_row == -1) doodle_now_row = 0;
        doodle_now_row = (doodle_now_row + 8) % 8;

        doodle_now_col += jump_dist[doodle_traj_idx];
        doodle_traj_idx = (doodle_traj_idx + 1) % (jump_len);

        OLED_SetCursor(0, 20);
        for (int k = 0; k < 5; ++k) {
            OLED_DisplayChar(block_col[k] / 10 + '0');
            OLED_DisplayChar(block_col[k] % 10 + '0');
            OLED_DisplayChar(' ');
        }
        // if (doodle_now_col < 0) OLED_DisplayChar('-');
        // OLED_DisplayChar(doodle_now_col/10 + '0');
        // OLED_DisplayChar(doodle_now_col%10 + '0');
        // OLED_DisplayChar(' ');
        // if (doodle_pre_col < 0) OLED_DisplayChar('-');
        // OLED_DisplayChar(doodle_pre_col/10 + '0');
        // OLED_DisplayChar(doodle_pre_col%10 + '0');
        // OLED_DisplayChar(' ');
        // OLED_DisplayChar(block_col[0] / 10 + '0');
        // OLED_DisplayChar(block_col[0] % 10 + '0');
        if (lock_with_doodle == -1) {
            for (int k = 0; k < 5; ++k) {
                if (doodle_now_row >= block_row[k]-doodle_row && doodle_now_row <= block_row[k]+bar_row) {
                    if (doodle_pre_col > block_col[k] && doodle_now_col < block_col[k]) {
                        if (bar_shift == 0 || block_col[k] - doodle_now_col < bar_shift) {
                            lock_with_doodle = k;
                        }
                    }
                }
            }
        }
        else {
            bar_shift = block_col[lock_with_doodle] - doodle_now_col; 
        }
        doodle_pre_row = doodle_now_row;
        doodle_pre_col = doodle_now_col;
    }

    // print bar
    for (int k = 0; k < 5; ++k) {
        for (int i = 0; i < bar_row; ++i) {
            // clear last bar
            OLED_SetCursor(block_row[k]+i, block_col[k]);
            for (int j = 0; j < bar_col; ++j) {
                oledSendData(0x00);
            }
                
            
            if (block_col[k]-bar_shift > 0) {
                OLED_SetCursor(block_row[k]+i, block_col[k]-bar_shift);
                for (int j = 0; j < bar_col; ++j) {
                    oledSendData(bar[i*bar_col+j]);
                }
            }
        }
        block_col[k] -= bar_shift;
        if (block_col[k] <= 0) {
            block_col[k] = rand(50 + random_range_sel * 8, 80 + random_range_sel * 8);
            random_range_sel = (random_range_sel + 1) % 5;
        }
    }
    if (doodle_now_col == 0) {
        bar_shift = 0;
        lock_with_doodle = -1;
    }
    
    for (int i = 0; i < doodle_row; ++i) {
        OLED_SetCursor(doodle_now_row+i, doodle_now_col);
        for (int j = 0; j < doodle_col; ++j) {
            oledSendData(doodle[i*doodle_col+j]);
        }
    }
}

void main(void)
{		 
    TMOD = 0x01;
    TH0 = (65536-1000) / 256;
    TL0 = (65536-1000) % 256;
    ET0 = 1;
    EA = 1;

    SDA = 1;
    SCL = 1;
    OLED_Init();		  // Check oled_i2c.c file for SCL,SDA pin connection
    MPU6050_INIT();
    OLED_Clear();
    for (int k = 0; k < 5; ++k) {
        for (int i = 0; i < bar_row; ++i) {
            OLED_SetCursor(block_row[k]+i, block_col[k]);
            for (int j = 0; j < bar_col; ++j) {
                oledSendData(bar[i*bar_col+j]);
            }
        }
    }
    
    TR0 = 1;
    while(1) {
        // readMPU6050_GyroData(&gyro_data[0]);

        // delay_ms(80);
        // 1G of gravity: 17500



    }
}
