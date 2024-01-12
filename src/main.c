#include "hx_drv_tflm.h"
#include "synopsys_wei_delay.h"
#include "synopsys_wei_i2c_oled1306.h"
#include "synopsys_wei_gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define C3  130.81
#define Cs3 138.59
#define D3  146.83
#define Ds3 155.56
#define E3  164.81
#define F3  174.61
#define Fs3 185
#define G3  196
#define Gs3 207.65
#define A3  220.0
#define As3 233.08
#define B3  246.94

#define C4  261.63
#define Cs4 277.18
#define D4  293.66
#define Ds4 311.13
#define E4  329.63
#define F4  349.23
#define Fs4 369.99
#define G4  392.00
#define Gs4 415.30
#define A4  440.00
#define As4 466.16
#define B4  493.88

#define C5  523.25
#define Cs5 554.37
#define D5  587.33
#define Ds5 622.25
#define E5  659.26
#define F5  698.46
#define Fs5 739.99
#define G5  783.99
#define Gs5 830.61
#define A5  880.00
#define As5 932.33
#define B5  987.77
#define D6  1174.7
#define Fs6 1480.0
#define F6  1369.9
#define E6  1318.5
#define Cs6 1108.7
#define C6  1046.5
#define Bs5 1017.1
#define Es6 1357.7
#define Cs7 2217.5
#define G6  1568.0
#define C7  2093.0
#define C2  65.4

float Square1[] ={B4, F5, 0, F5, F5, 0, F5, 0, E5, 0, D5, 0, C5, 0, 0, 0, 0, 0, 0, 0};

float Square2[] ={G4, B4, 0, B4, B4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

float Triangle[]={E3, 0, 0, 0, E3, 0, F3, 0, G4, 0, A4, E4, 0, E4, 0, C2, 0, 0};



/* define parameters */
#define delay_def 24

#define SAMPLE_RATE 8000

#define STEP (SAMPLE_RATE*0.15)

#define AMP 500

#define audio_length 21600
int8_t audio_buf[audio_length] = {0};

/* declare gpio pins */
hx_drv_gpio_config_t hal_gpio_0;  //BCLK
hx_drv_gpio_config_t hal_gpio_1;  //LRCLK
hx_drv_gpio_config_t hal_gpio_2;  //DIN

volatile int delay(unsigned int j);
void i2s_reflash(void);
void GPIO_INIT(void);

static int16_t i2s_data_buf = 0;
static int16_t i2s_r_buffer;
static int16_t i2s_l_buffer;

volatile int buf;



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
uint16_t random_start = 115;
unsigned my_rand(unsigned a, unsigned b) {
    uint16_t lsb = ((random_start >> 6) ^ (random_start >> 4) ^ (random_start >> 1)) & 1u;
    random_start = (random_start >> 1) & (lsb << 15); 
    return a + (random_start % (b-a));
}

signed char min(signed char a, signed char b) {
    return (a < b ? a : b);
}
signed char max(signed char a, signed char b) {
    return (a > b ? a : b);
}


void main(void)
{		 
    const int doodle_row = 2;
    const int doodle_col = 16;

    const int bar_row = 2;
    const int bar_col = 8;
    int doodle_traj_idx = 0;
    signed char doodle_now_row = 3, doodle_now_col = 2 * bar_col;
    signed char doodle_pre_row = 3, doodle_pre_col = 2 * bar_col;
    int bar_shift = 0;
    int shift_cnt = 0;
    signed char lock_with_doodle = -1;
    char random_range_sel = 0, unlock = 0;
    char shift_thres = 1;
    char first_loop = 1, game_end = 0;
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

    i2s_data_buf = 0;
    int num = sizeof(Square1)/sizeof(Square1[0]);
    int N = num * STEP;
    int BUFSIZE = sizeof(int8_t) * N;

    int n, tmp;
    float tmp2;
    int index = 0;
    for(n=0; n<num; n++){ //music synthesize
        float s1 = Square1[n];
        float s2 = Square2[n];
        float t =  Triangle[n];
        float ratio;
        if (n > num/2) ratio = 0.2;

        int s;
        for(s=0; s<STEP; s++){
        int16_t sample = 0;

        // Square 1
        tmp = (s1 * s / SAMPLE_RATE) * 2;
        tmp2 = 1.0*s/STEP;  // the progress of the note
        if(tmp2 <= 0.4) tmp2 = 1; else tmp2 = 1-(tmp2-0.4)/0.6;
        if(s1 == 0)             sample += 0;
        else if(tmp % 2 == 0)   sample += tmp2*AMP;
        else                    sample += tmp2*(-1 * AMP);

        // Square 2
        tmp = (s2 * s / SAMPLE_RATE) * 2;
        tmp2 = 1.0*s/STEP;  // the progress of the note
        if(tmp2 <= 0.4) tmp2 = 1; else tmp2 = 1-(tmp2-0.4)/0.6;
        if(s2 == 0)             sample += 0;
        else if(tmp % 2 == 0)   sample += tmp2*AMP;
        else                    sample += tmp2*(-1 * AMP);

        // Triangle
        tmp = (t * s / SAMPLE_RATE);  // round down to integer
        tmp2 = (t * s / SAMPLE_RATE) - tmp; // the progress of the note
        if(t == 0)              sample += 0;
        else if(tmp2 <= 0.25)   sample += tmp2 * 4 * AMP;
        else if(tmp2 <= 0.75)   sample += AMP - (tmp2-0.25) * 4 * AMP;
        else                    sample += (tmp2 - 0.75) * 4 * AMP - AMP;

        audio_buf[index] = sample >> 8;
        index ++;

        if(index >= audio_length)
            index = audio_length - 1;
        }
    }
	
	GPIO_INIT(); //initialize gpios

    for (int k = 0; k < 5; ++k) {
        for (int i = 0; i < bar_row; ++i) {
            OLED_SetCursor(block_row[k]+i, block_col[k]);
            for (int j = 0; j < bar_col; ++j) {
                oledSendData(bar[i*bar_col+j]);
            }
        }
    }
    

    // for(index = 0; index < audio_length; index ++)
    // {
    //     i2s_data_buf = audio_buf[index] << 8; // 8-bit data expands into 16-bit data, with an 8-bit zero at LSB
    //     i2s_reflash();  // 8kHz I2S
    // }
    float x, y, z;
    while(1) {

        if (game_end == 0) {
            int available_cnt = hx_drv_accelerometer_available_count();
            for (int i = 0; i < available_cnt; ++i) {
                hx_drv_accelerometer_receive(&x, &y, &z);
            }


            extend_TI = (extend_TI+1) % 2;
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
            if (doodle_now_col == 2 * bar_col && lock_with_doodle == -1) {
                game_end = 1;
            }

            // OLED_SetCursor(0, 20);
            // for (int k = 0; k < 5; ++k) {
            //     OLED_DisplayChar(block_col[k] / 10 + '0');
            //     OLED_DisplayChar(block_col[k] % 10 + '0');
            //     OLED_DisplayChar(' ');
            // }
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
            if (jump_dist[doodle_traj_idx] < 0 && lock_with_doodle == -1) {
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
            else if (jump_dist[doodle_traj_idx] < 0) {
                bar_shift = block_col[lock_with_doodle] - doodle_now_col; 
            }
            else {
                lock_with_doodle = -1;
                bar_shift = 0;
            }
            doodle_traj_idx = (doodle_traj_idx + 1) % (jump_len);
            doodle_pre_row = doodle_now_row;
            doodle_pre_col = doodle_now_col;

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
                    block_col[k] = my_rand(70 + random_range_sel * 30, 90 + random_range_sel * 10);
                    random_range_sel = (random_range_sel + 1) % 3;
                    if (k == lock_with_doodle) {
                        unlock = 1;
                    }
                }
            }
            if (unlock) {
                bar_shift = 0;
                lock_with_doodle = -1;
            }
        }
        else {
            for(index = 0; index < audio_length; index ++)
            {
                i2s_data_buf = audio_buf[index] << 8; // 8-bit data expands into 16-bit data, with an 8-bit zero at LSB
                i2s_reflash();  // 8kHz I2S
            }
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


volatile int delay(unsigned int j)
{
	for(int delay_i = 0; delay_i < j; delay_i ++)
		buf = buf + delay_i;
	return buf;
}

void i2s_reflash(void)
{
	int i;
	i2s_r_buffer = i2s_data_buf;
	i2s_l_buffer = i2s_data_buf;

  for(i = 15; i >= 0; i --)
	{
		if(i2s_l_buffer & 0x8000){
      hal_gpio_set(&hal_gpio_2, GPIO_PIN_SET);
    }
		else{
      hal_gpio_set(&hal_gpio_2, GPIO_PIN_RESET);
    }
		i2s_l_buffer <<= 1;


		delay(delay_def);
		hal_gpio_set(&hal_gpio_0, GPIO_PIN_SET);  //BCLK
		delay(delay_def);

		hal_gpio_set(&hal_gpio_0, GPIO_PIN_RESET);  //BCLK
    if(i == 1){
			hal_gpio_set(&hal_gpio_1, GPIO_PIN_SET);  //LRCLK
    }
	}
	
  for(i = 15; i >= 0; i --)
	{
		if(i2s_r_buffer & 0x8000){
			hal_gpio_set(&hal_gpio_2, GPIO_PIN_SET); //DIN
    }
		else{
			hal_gpio_set(&hal_gpio_2, GPIO_PIN_RESET); //DIN	
    }
		i2s_r_buffer <<= 1;

		delay(delay_def);
		hal_gpio_set(&hal_gpio_0, GPIO_PIN_SET);  //BCLK
		delay(delay_def);

		hal_gpio_set(&hal_gpio_0, GPIO_PIN_RESET);  //BCLK
    if(i == 1){
			hal_gpio_set(&hal_gpio_1, GPIO_PIN_RESET);  //LRCLK
    }
	}
}

void GPIO_INIT(void)
{ 
  //BCLK
  if ( hal_gpio_init(&hal_gpio_0, HX_DRV_PGPIO_0, HX_DRV_GPIO_OUTPUT, GPIO_PIN_RESET)==HAL_OK){
    hx_drv_uart_print("GPIO0 Initialized: OK\n");
  }
  else {
    hx_drv_uart_print("GPIO0 Initialized: Error\n");
  }

  //LRCLK
  if (hal_gpio_init(&hal_gpio_1, HX_DRV_PGPIO_1, HX_DRV_GPIO_OUTPUT, GPIO_PIN_RESET)==HAL_OK){ 
    hx_drv_uart_print("GPIO1 Initialized: OK\n");
  }
  else {
    hx_drv_uart_print("GPIO1 Initialized: Error\n");
  }
  
  //DIN
  if (hal_gpio_init(&hal_gpio_2, HX_DRV_PGPIO_2, HX_DRV_GPIO_OUTPUT, GPIO_PIN_RESET)==HAL_OK){ 
    hx_drv_uart_print("GPIO2 Initialized: OK\n");
  }
  else {
    hx_drv_uart_print("GPIO2 Initialized: Error\n");
  }
}
