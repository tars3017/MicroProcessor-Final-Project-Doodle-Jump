#include "hx_drv_tflm.h"
#include "synopsys_wei_delay.h"
#include "synopsys_wei_i2c_oled1306.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


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


signed char block_row[5] = {4, 2, 1, 5, 3};
signed char block_col[5] = {25, 41, 65, 77, 85};

unsigned int extend_TI;
// unsigned char random_num1 = 15;
unsigned char my_rand(char a, char b) {
    return a + (rand() % (b-a));
}

signed char min(signed char a, signed char b) {
    return (a < b ? a : b);
}
signed char max(signed char a, signed char b) {
    return (a > b ? a : b);
}
// void T0_isr(void) {
//     TH0 = (65536-1000) / 256;
//     TL0 = (65536-1000) % 256;
//     extend_TI = (extend_TI+1) % 2;
//     if (extend_TI == 0) {
//         readMPU6050_AccelData(&accel_data[0]);

//         for (int i = 0; i < doodle_row; ++i) {
//             OLED_SetCursor(doodle_now_row+i, doodle_now_col);
//             for (int j = 0; j < doodle_col; ++j) {
//                 oledSendData(0x00);
//             }
//         }

//         if (accel_data[0] > 3000) {
//             shift_cnt++;
//         }
//         else if (accel_data[0] < -3000) {
//             shift_cnt--;
//         }
//         if (shift_cnt > shift_thres) {
//             doodle_now_row += 1;
//             shift_cnt = 0;
//         }
//         else if (shift_cnt < -shift_thres) {
//             doodle_now_row -= 1;
//             shift_cnt = 0;
//         }
//         if (doodle_now_row == 7) doodle_now_row = 6;
//         else if (doodle_now_row == -1) doodle_now_row = 0;
//         doodle_now_row = (doodle_now_row + 8) % 8;

//         doodle_now_col += jump_dist[doodle_traj_idx];
//         doodle_traj_idx = (doodle_traj_idx + 1) % (jump_len);

//         OLED_SetCursor(0, 20);
//         for (int k = 0; k < 5; ++k) {
//             OLED_DisplayChar(block_col[k] / 10 + '0');
//             OLED_DisplayChar(block_col[k] % 10 + '0');
//             OLED_DisplayChar(' ');
//         }
//         // if (doodle_now_col < 0) OLED_DisplayChar('-');
//         // OLED_DisplayChar(doodle_now_col/10 + '0');
//         // OLED_DisplayChar(doodle_now_col%10 + '0');
//         // OLED_DisplayChar(' ');
//         // if (doodle_pre_col < 0) OLED_DisplayChar('-');
//         // OLED_DisplayChar(doodle_pre_col/10 + '0');
//         // OLED_DisplayChar(doodle_pre_col%10 + '0');
//         // OLED_DisplayChar(' ');
//         // OLED_DisplayChar(block_col[0] / 10 + '0');
//         // OLED_DisplayChar(block_col[0] % 10 + '0');
//         if (lock_with_doodle == -1) {
//             for (int k = 0; k < 5; ++k) {
//                 if (doodle_now_row >= block_row[k]-doodle_row && doodle_now_row <= block_row[k]+bar_row) {
//                     if (doodle_pre_col > block_col[k] && doodle_now_col < block_col[k]) {
//                         if (bar_shift == 0 || block_col[k] - doodle_now_col < bar_shift) {
//                             lock_with_doodle = k;
//                         }
//                     }
//                 }
//             }
//         }
//         else {
//             bar_shift = block_col[lock_with_doodle] - doodle_now_col; 
//         }
//         doodle_pre_row = doodle_now_row;
//         doodle_pre_col = doodle_now_col;
//     }

//     // print bar
//     for (int k = 0; k < 5; ++k) {
//         for (int i = 0; i < bar_row; ++i) {
//             // clear last bar
//             OLED_SetCursor(block_row[k]+i, block_col[k]);
//             for (int j = 0; j < bar_col; ++j) {
//                 oledSendData(0x00);
//             }
                
            
//             if (block_col[k]-bar_shift > 0) {
//                 OLED_SetCursor(block_row[k]+i, block_col[k]-bar_shift);
//                 for (int j = 0; j < bar_col; ++j) {
//                     oledSendData(bar[i*bar_col+j]);
//                 }
//             }
//         }
//         block_col[k] -= bar_shift;
//         if (block_col[k] <= 0) {
//             block_col[k] = rand(50 + random_range_sel * 8, 80 + random_range_sel * 8);
//             random_range_sel = (random_range_sel + 1) % 5;
//         }
//     }
//     if (doodle_now_col == 0) {
//         bar_shift = 0;
//         lock_with_doodle = -1;
//     }
    
//     for (int i = 0; i < doodle_row; ++i) {
//         OLED_SetCursor(doodle_now_row+i, doodle_now_col);
//         for (int j = 0; j < doodle_col; ++j) {
//             oledSendData(doodle[i*doodle_col+j]);
//         }
//     }
// }

void main(void)
{		 
    const int doodle_row = 2;
    const int doodle_col = 16;

    const int bar_row = 2;
    const int bar_col = 8;
    int doodle_traj_idx = 0;
    signed char doodle_now_row = 3, doodle_now_col = 0;
    signed char doodle_pre_row = 3, doodle_pre_col = 0;
    int bar_shift = 0;
    int shift_cnt = 0;
    signed char lock_with_doodle = -1;
    char random_range_sel = 0, unlock = 0;
    char shift_thres = 1;
    srand(time(0));
    hx_drv_share_switch(SHARE_MODE_I2CM);
    OLED_Init();
    OLED_Clear();

    hx_drv_uart_initial(UART_BR_115200);
    if (hx_drv_accelerometer_initial() != HX_DRV_LIB_PASS) {
        hx_drv_uart_print("Accelerometer Initialize Fail\n");
    }
    else {
        hx_drv_uart_print("Accelerometer Initialize Success\n");
    }

    for (int k = 0; k < 5; ++k) {
        for (int i = 0; i < bar_row; ++i) {
            OLED_SetCursor(block_row[k]+i, block_col[k]);
            for (int j = 0; j < bar_col; ++j) {
                oledSendData(bar[i*bar_col+j]);
            }
        }
    }
    
    float x, y, z;
    while(1) {
        int available_cnt = hx_drv_accelerometer_available_count();
        for (int i = 0; i < available_cnt; ++i) {
            hx_drv_accelerometer_receive(&x, &y, &z);
        }


        extend_TI = (extend_TI+1) % 2;
        // if (extend_TI == 0) {
            // readMPU6050_AccelData(&accel_data[0]);

            for (int i = 0; i < doodle_row; ++i) {
                OLED_SetCursor(doodle_now_row+i, doodle_now_col);
                for (int j = 0; j < doodle_col; ++j) {
                    oledSendData(0x00);
                }
            }

            if (y > 0.1) {
                shift_cnt--;
                // doodle_now_row--;
            }
            else if (y < -0.1) {
                shift_cnt++;
                // doodle_now_row++;
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
                    if (doodle_now_row >= block_row[k]-doodle_row/2 && doodle_now_row <= block_row[k]+bar_row-doodle_row/2) {
                        if (doodle_pre_col >= block_col[k]+bar_col && doodle_now_col <= block_col[k]+bar_col) {
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
        // }

        for (int i = 0; i < doodle_row; ++i) {
            OLED_SetCursor(doodle_now_row+i, doodle_now_col);
            for (int j = 0; j < doodle_col; ++j) {
                oledSendData(doodle[i*doodle_col+j]);
            }
        }
        // print bar
        unlock = 0;
        for (int k = 0; k < 5; ++k) {
            for (int i = 0; i < bar_row; ++i) {
                // clear last bar
                OLED_SetCursor(block_row[k]+i, block_col[k]);
                for (int j = 0; j < bar_col; ++j) {
                    oledSendData(0x00);
                }
                    
                
                if (block_col[k]-bar_shift > bar_col) {
                    OLED_SetCursor(block_row[k]+i, block_col[k]-bar_shift);
                    for (int j = 0; j < bar_col; ++j) {
                        oledSendData(bar[i*bar_col+j]);
                    }
                }
            }
            block_col[k] -= bar_shift;
            if (block_col[k] <= bar_col) {
                block_col[k] = my_rand(70 + random_range_sel * 30, 90 + random_range_sel * 30);
                random_range_sel = (random_range_sel + 1) % 2;
                if (k == lock_with_doodle) {
                    unlock = 1;
                }
            }
        }
        if (unlock) {
            bar_shift = 0;
            lock_with_doodle = -1;
        }
        


        // uint8_t x_sign = '+';
        // int32_t x_int_buf = y * 10;
        // uint32_t pos_x = 0;
        // uint32_t flo_x = 0;
        // if (x_int_buf < 0) {
        //     x_sign = '-';
        //     x_int_buf = x_int_buf * (-1);
        // }
        // pos_x = x_int_buf / 10;
        // flo_x = x_int_buf % 10;
        // uint8_t x_char[11];
        // OLED_SetCursor(1, 20);
        // oledSendData(x_sign);
        // oledSendData(pos_x + '0');
        // oledSendData('.');
        // oledSendData(flo_x + '0');
        // sprintf(x_char, "x=%c%1d.%1d ", x_sign, pos_x, flo_x);
        // hx_drv_uart_print(x_char);
    }
}
