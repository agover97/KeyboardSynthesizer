#include "./drivers/inc/vga.h"
#include "./drivers/inc/ISRs.h"
#include "./drivers/inc/LEDs.h"
#include "./drivers/inc/audio.h"
#include "./drivers/inc/HPS_TIM.h"
#include "./drivers/inc/int_setup.h"
#include "./drivers/inc/wavetable.h"
#include "./drivers/inc/ps2_keyboard.h"
#include "./drivers/inc/HEX_displays.h"
#include <stdio.h>

float frqs[] = {130.813 ,146.832 ,164.814 ,174.614 ,195.998 ,220.000 ,246.942 ,261.626};
char maco[] = {0x1C,0x1B,0x23,0x2B,0x3B,0x42,0x4B,0x4C};
int keys = 0;
int t = 0;
int value;
int volume = 5;
int X = -1, Y =0;
int savedPixel[320][2] = {{0}};

int getSignal(int keys, int t){
	int value = 0;
	int i;
	for(i = 0; i< 8; i++){
		if(keys & 1<<i) {
			value += sine[((int)frqs[i]*t)%48000];
		}
	}
	return value;
}

int getPressedKeys(int keys){
	char data;
	int i;
	int sig = keys; 
	while(read_ps2_data_ASM(&data)){
		if( data  == 0xF0){
			while(!read_ps2_data_ASM(&data));
			if(data == 0x2A){
				 volume = volume + 1; //increase volume;
			}
			else if(data ==  0x21  &&  volume > 0){
				 volume = volume -1; //decrease volume;
			}
			else {
				for(i = 0; i<8 ; i++){
					if(data == maco[i]) {
						sig &= ~(1<<i);   //1 means key is released, we want to turn it off (set to zero) so use ~ to flip bit
						break;					//using one hot encoding
					}							
				}
			}
		}
		else{
			for(i = 0; i<8; i++){			//no break code means a new key was pressed so we have to reset our one hot encoding
				if(data == maco[i]){		//we use or so that any zeros (off) will turn on
					sig |= 1<<i ;
					break;
				}
			}
		}	
	}
	return sig;
}

int main() {
	VGA_clear_charbuff_ASM();
	VGA_clear_pixelbuff_ASM();
	int_setup(1, (int []){199});
	HPS_TIM_config_t hps_tim;
	hps_tim.tim = TIM0;				
	hps_tim.timeout = 20;			//timer set at 20us
	hps_tim.LD_en = 1;
	hps_tim.INT_en = 1;
	hps_tim.enable = 1;
	HPS_TIM_config_ASM(&hps_tim);
	while(1) {
		if(hps_tim0_int_flag){
			t = t++;
			hps_tim0_int_flag = 0;
			keys = getPressedKeys(keys);
			value =   volume * getSignal(keys, t);
			while(!audio_write_data_ASM(value,value));
			if(t % 10 == 0 ){ 
				X = (X + 1) %640;
				VGA_draw_point_ASM(X, savedPixel[X][X%2], 0x0); //erases last pixel drawn
				Y = 120 - (value>>23 ); //value of sin is so large that we have to logical shift left by 20 so it can be seen on the screen	
				VGA_draw_point_ASM( X , Y, 0xF00); 
				savedPixel[X][X%2] = Y; //draws new pixel
			}
		}
	}
	return 0;
}