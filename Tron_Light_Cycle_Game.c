#include <unistd.h>
#include <stdio.h>

// if using CPUlator, you should copy+paste contents of the file below instead of using #include
#ifndef __SYSTEM_INFO__
#define __SYSTEM_INFO__

#define DE10LITE 0 // change to 0 for CPUlator or DE1-SoC, 1 for DE10-Lite

/* do not change anything after this line */

#if DE10LITE
 #define BOARD				"DE10-Lite"
 #define MAX_X		160
 #define MAX_Y		120
 #define YSHIFT		  8
#else
 #define MAX_X		320
 #define MAX_Y		240
 #define YSHIFT		  9
#endif

/* Memory */
#define SDRAM_BASE			0x00000000
#define SDRAM_END			0x03FFFFFF
#define FPGA_PIXEL_BUF_BASE	0x08000000
#define FPGA_PIXEL_BUF_END	0x0800FFFF
#define FPGA_CHAR_BASE		0x09000000
#define FPGA_CHAR_END		0x09001FFF
	
#define TICK_SPEED 			10000000

/* Devices */
#define LED_BASE			0xFF200000
#define LEDR_BASE			0xFF200000
#define HEX3_HEX0_BASE		0xFF200020
#define HEX5_HEX4_BASE		0xFF200030
#define SW_BASE				0xFF200040
#define KEY_BASE			0xFF200050
#define JP1_BASE			0xFF200060
#define ARDUINO_GPIO		0xFF200100
#define ARDUINO_RESET_N		0xFF200110
#define JTAG_UART_BASE		0xFF201000
#define TIMER_BASE			0xFF202000
#define TIMER_2_BASE		0xFF202020
#define MTIMER_BASE			0xFF202100
#define RGB_RESAMPLER_BASE  0xFF203010
#define PIXEL_BUF_CTRL_BASE	0xFF203020
#define CHAR_BUF_CTRL_BASE	0xFF203030
#define ADC_BASE			0xFF204000
#define ACCELEROMETER_BASE	0xFF204020

/* Nios V memory-mapped registers */
#define MTIME_BASE          0xFF202100

#endif

typedef uint16_t pixel_t;
volatile pixel_t *pVGA = (pixel_t *)FPGA_PIXEL_BUF_BASE;

volatile uint32_t* KEY_ptr = (uint32_t*)KEY_BASE;
//button address linking + flip-flops for pos-edge detection

volatile uint32_t* seven_seg = (uint32_t*)HEX3_HEX0_BASE;
volatile uint32_t* seven_seg2 = (uint32_t*)HEX5_HEX4_BASE;
//seven segment address linking

volatile uint32_t* switches = (uint32_t*)SW_BASE;
//switches address linking

volatile uint32_t* led = (uint32_t*)LEDR_BASE;
//led linking

volatile uint32_t *mtime_ptr = (uint32_t*)MTIMER_BASE;
uint64_t PERIOD = (uint64_t) TICK_SPEED; //100M apparently. Gonna assume that means 100 million 
//sets up timer address linking

int score[2] = {0, 0};
//tracking player scores

int firm[2] = {0, 0};
//pos-edge triggers
	
int player_replace[1] = {0};
//boolean to see if I should replace player 1 with AI

int coords[4] = {30, 110, 60, 40}; 
//x1 = 30, x2 = 110, y1 = 60, y2 = 40;
//player coords

int dir[2] = {1, 3};
//int dir = 1, dir2 = 3;
//direction indications
//1 - right, 2 - up, 3 - left, 4 - down

int alive[2] = {1, 1};
//int alive = 1, alive2 = 1;
//booleans to see if both players are alive

const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;

const pixel_t pink = 0xf81f;
const pixel_t purple = 0x8811;

/* STARTER CODE BELOW. FEEL FREE TO DELETE IT AND START OVER */

void drawPixel( int y, int x, pixel_t colour )
{
	*(pVGA + (y<<YSHIFT) + x ) = colour;
}

pixel_t makePixel( uint8_t r8, uint8_t g8, uint8_t b8 )
{
	// inputs: 8b of each: red, green, blue
	const uint16_t r5 = (r8 & 0xf8)>>3; // keep 5b red
	const uint16_t g6 = (g8 & 0xfc)>>2; // keep 6b green
	const uint16_t b5 = (b8 & 0xf8)>>3; // keep 5b blue
	return (pixel_t)( (r5<<11) | (g6<<5) | b5 );
}

void rect( int y1, int y2, int x1, int x2, pixel_t c )
{
	for( int y=y1; y<y2; y++ )
		for( int x=x1; x<x2; x++ )
			drawPixel( y, x, c );
}

void tri (int y1, int y2, int x1, int x2, int ori, pixel_t c) 
{
	
	if (ori == 1)
	    for( int y=y1; y<y2; y++ )
		    for( int x=x1; x< (y2-y) + x1; x++ )
			    drawPixel( y, x, c );
	else if (ori == 2)
		for( int y=y1; y<y2; y++ )
		    for( int x=x1; x< (y-y1) + x1; x++ )
			    drawPixel( y, x, c );
	else if (ori == 3)
		for( int x=x1; x<x2; x++ )
		    for( int y=y1; y< (x-x1) + y1; y++ )
			    drawPixel( y, x, c );
	else if (ori == 4)
	    for( int x=x1; x<x2; x++ )
		    for( int y=y1; y<(x2-x) + y1; y++ )
			    drawPixel( y, x, c );
}

int check(int y, int x, int dir) {
	
	int safe = 1;
	
	if (dir >= 5) dir = 1;
	if (dir <= 0) dir = 4;
	
	if (dir == 1 && *(pVGA + (y<<YSHIFT) + x + 1) != blk) safe = 0;
	if (dir == 2 && *(pVGA + ((y-1)<<YSHIFT) + x) != blk) safe = 0;
	if (dir == 3 && *(pVGA + (y<<YSHIFT) + x - 1) != blk) safe = 0;
	if (dir == 4 && *(pVGA + ((y+1)<<YSHIFT) + x) != blk) safe = 0;
	
	return safe;
}

int check2(int y, int x, int dir) {
	
	int safe = 1;
	
	if (dir >= 5) dir = 1;
	if (dir <= 0) dir = 4;
	
	if (dir == 1 && *(pVGA + (y<<YSHIFT) + x + 2) != blk) safe = 0;
	if (dir == 2 && *(pVGA + ((y-2)<<YSHIFT) + x) != blk) safe = 0;
	if (dir == 3 && *(pVGA + (y<<YSHIFT) + x - 2) != blk) safe = 0;
	if (dir == 4 && *(pVGA + ((y+2)<<YSHIFT) + x) != blk) safe = 0;
	
	return safe;
}
	
void reset() {
	
	rect( 2, 120, 2, 160, wht );
	rect( 4, 120 - 2, 4, 160 - 2, blk);
	//border making note -> (160, 120)
	
	rect (30, 60, 100, 101, wht);
	rect (60, 61, 100, 120, wht);
	rect (40, 41, 31, 70, wht);
	rect (80, 81, 10, 40, wht);
	
	rect(41, 80, 132, 133, wht);
	rect(40, 80, 50, 51, wht);
	rect(50, 70, 80, 81, wht);
	rect(60, 61, 70, 90, wht);
	
	rect(95, 96, 120, 150, wht);
	rect(100, 120, 60, 61, wht);
	rect(99, 100, 50, 60, wht);
	
	rect (20, 21, 70, 75, wht);
	rect (25, 35, 89, 90, wht);
	tri (25, 35, 70, 75, 2, wht);
	tri (0, 20, 70, 90, 3, wht);
	
	rect(90, 95, 110, 115, wht);
	rect(105, 110, 95, 100, wht);
	rect(110, 115, 80, 85, wht);
	rect(93, 97, 70, 75, wht);
	rect(80, 85, 90, 95, wht);

	tri(20, 30, 20, 30, 1, wht);
	tri(90, 100, 20, 30, 2, wht);
	tri(20, 30, 120, 130, 1, wht);
	
	//obstacles making / testing functions
	
	*(coords) = 30;
	*(coords+1) = 110;
	*(coords+2) = 60;
	*(coords+3) = 40;
	//x1, x2, y1, y2
	
	*(dir) = 1;
	*(dir + 1) = 3;
	//dir, dir2
	
	*(alive) = 1;
	*(alive + 1) = 1;
	//alive, alive2
	
	//initializing begining values
}

int seg(int score) {
	
	if (score == 0) return 63;
	if (score == 1) return 6;
	if (score == 2) return 91;
	if (score == 3) return 79;
	if (score == 4) return 102;
	if (score == 5) return 109;
	if (score == 6) return 125;
	if (score == 7) return 7;
	if (score == 8) return 127;
	if (score == 9) return 111;
	
	return 0;
}

void set_mtimer(volatile uint32_t *time_ptr, uint64_t new_time64) {
	
	*(time_ptr + 0) = (uint32_t)0;
	//sets the lower half of the timer to low, 
	//preventing hi from incrementing
	//before low is set (the 64 bits of the timer)
	
	*(time_ptr + 1) = (uint32_t) (new_time64>>32);
	//sets the high part to be the new_time shifted
	
	*(time_ptr + 0) = (uint32_t)new_time64;
	//sets the low part to be the unshifted ones
	
}
//Writing the time 

uint64_t get_mtimer (volatile uint32_t *time_ptr) {
	
	uint32_t mtime_h, mtime_l;
	//since can only read 32 bits at a time, needs two
	
	do {
		
		mtime_h = *(time_ptr + 1); //reads the upper bits w/ pointer arith
		mtime_l = *(time_ptr + 0); //reads the lower bits
		
	} while (mtime_h != *(time_ptr + 1));
	//keep going in case upper incremented during this
	
	return ((uint64_t)mtime_h << 32) | mtime_l;
	//bit-wise or to turn the two back into 64 bits
	
}
//reading the time

void setup_mtimecmp() {
	
	uint64_t mtime64 = get_mtimer(mtime_ptr);
	//reads current time
	
	mtime64 = (mtime64/PERIOD + 1) * PERIOD;
	//computes when the next period ends
	
	set_mtimer(mtime_ptr + 2, mtime64);
	//writes the first mtimecmp to +2
	
}
//sets up when each period is ig?

//setup for the timer stuff

void mtimer_ISR(void) {
	
	uint64_t mtimecmp64 = get_mtimer(mtime_ptr + 2);
		//reads the mtimecmp
	
	mtimecmp64 += PERIOD;
		//sets the future irq to be one period from now
	
	set_mtimer(mtime_ptr+2, mtimecmp64);
		//writes the next mtimecmp
	
	//if (*led != 0) *led = 0;
	
	if (*led != 4) *led = 4;
	else if (*led != 8) *led = 8;
	
	if (*player_replace == 1) {
			
				if (!check2(*(coords+2), *(coords), *dir) && !check2(*(coords+2), *(coords), *dir + 1) 
					&& !check2(*(coords+2), *(coords), *dir-1)) {

					if(!check(*(coords+2), *(coords), *dir) && !check(*(coords+2), *(coords), *dir+1)) *dir-=1;
					else if (!check(*(coords+2), *(coords), *dir)) *dir+=1;

				} else if(!check2(*(coords+2), *(coords), *dir) && !check2(*(coords+2), *(coords), *dir+1)) {

					if(!check(*(coords+2), *(coords), *dir) && !check(*(coords+2), *(coords), *dir+1)) *dir-=1;
					else if (!check(*(coords+2), *(coords), *dir)) *dir+=1;
					else *dir+=1;

				} else if (!check2(*(coords+2), *(coords), *dir)) {

					*dir+=1;
					
					if(!check(*(coords+2), *(coords), *dir) && !check(*(coords+2), *(coords), *dir+1)
					   && !check(*(coords+2), *(coords), *dir-1)) *dir += 2;
					else if(!check(*(coords+2), *(coords), *dir) && !check(*(coords+2), *(coords), *dir+1)) *dir-=1;
					else if (!check(*(coords+2), *(coords), *dir)) *dir+=1;

				} else if(!check(*(coords+2), *(coords), *dir) && !check(*(coords+2), *(coords), *dir+1)) *dir-=1;
				else if (!check(*(coords+2), *(coords), *dir)) *dir+=1;

				if (*dir <= 0) *dir = 4;
				if (*dir >= 5) *dir = 1;
			}
			//player AI replacement;
			
			if(!check2(*(coords+3), *(coords+1), *(dir+1)) && !check2(*(coords+3), *(coords+1), *(dir+1)+1) 
           && !check2(*(coords+3), *(coords+1), *(dir+1)-1)){
                
				if(!check(*(coords+3), *(coords+1), *(dir+1)) && !check(*(coords+3), *(coords+1), *(dir+1)+1)) *(dir+1)-=1;
				else if (!check(*(coords+3), *(coords+1), *(dir+1))) *(dir+1)+=1;

			} else if(!check2(*(coords+3), *(coords+1), *(dir+1)) && !check2(*(coords+3), *(coords+1), *(dir+1)+1)) {

				*(dir+1)-=1;
				if(!check(*(coords+3), *(coords+1), *(dir+1)) && !check(*(coords+3), *(coords+1), *(dir+1)+1)) *(dir+1)-=1;
				else if (!check(*(coords+3), *(coords+1), *(dir+1))) *(dir+1)+=1;

			} else if (!check2(*(coords+3), *(coords+1), *(dir+1))) {

				*(dir+1)+=1;

				if(!check(*(coords+3), *(coords+1), *(dir+1)) && !check(*(coords+3), *(coords+1), *(dir+1)+1) 
				   && !check(*(coords+3), *(coords+1), *(dir+1)-1)) *(dir+1) += 2;
				else if(!check(*(coords+3), *(coords+1), *(dir+1)) && !check(*(coords+3), *(coords+1), *(dir+1)+1)) *(dir+1)-=1;
				else if (!check(*(coords+3), *(coords+1), *(dir+1))) *(dir+1)+=1;

			} else if(!check(*(coords+3), *(coords+1), *(dir+1)) && !check(*(coords+3), *(coords+1), *(dir+1)+1)) *(dir+1)-=1;
			else if (!check(*(coords+3), *(coords+1), *(dir+1))) *(dir+1) += 1;
			
			if (*(dir+1) <= 0) *(dir+1) = 4;
			if (*(dir+1) >= 5) *(dir+1) = 1;

			//AI movement algorithm ig?
			//if front blocked, check left. If left blocked too, check right.

			if (*dir == 1) *(coords) += 1;
			else if (*dir == 4) *(coords+2) += 1;
			else if (*dir == 3) *(coords) -= 1;
			else if (*dir == 2) *(coords+2) -= 1;

			if (*(dir+1) == 1) *(coords+1) += 1;
			else if (*(dir+1) == 4) *(coords+3) += 1;
			else if (*(dir+1) == 3) *(coords+1) -= 1;
			else if (*(dir+1) == 2) *(coords+3) -= 1;

			//movement code

			if (*(pVGA + (*(coords+2)<<YSHIFT) + *(coords+0)) != blk) {
				*(alive) = 0;
			}
			if (*(pVGA + (*(coords+3)<<YSHIFT) + *(coords+1)) != blk) {
				*(alive+1) = 0;
			}
			if ((pVGA + (*(coords+2)<<YSHIFT) + *(coords+0)) == 
				(pVGA + (*(coords+3)<<YSHIFT) + *(coords+1))) {
				*(alive+1) = 0;
				*(alive) = 0;
			}
			// death checks

			drawPixel(*(coords+2), *(coords+0), pink);
			drawPixel(*(coords+3), *(coords+1), purple);
	
}

void key_ISR(void) {
	
	if (*player_replace == 0) {

		int press = *(KEY_ptr + 3);   // edge capture register
		
		if (*led == 1) {
			
			*led = 0;
			*dir -= 1;
			
		} else if (*led == 2) {
			
			*led = 0;
			*dir += 1;
			
		} else if (press & 0x1) {
			
			*led = 1;
			*dir += 1;
			
		} else if (press & 0x2) {
			
			*led = 2;
			*dir -= 1;
		}

		*(KEY_ptr + 3) = press;  // clear interrupt by writing back

		if (*dir <= 0) *dir = 4;
		if (*dir >= 5) *dir = 1;
		//player movement check
		
	} else {
		int press = *(KEY_ptr + 3);
		*(KEY_ptr + 3) = press;
	}
	
}

static
	void handler(void) __attribute__ ((interrupt("machine")));
//function prototype I am fairly certain

void handler(void) {
		
	int mcause_value;
	
	__asm__ volatile("csrr %0, mcause" : "=r" (mcause_value) );
	
	if (mcause_value == 0x80000007) mtimer_ISR();
	else if (mcause_value == 0x80000012) key_ISR();
	

}
//Exception handler

void setup_cpu_irqs(uint32_t new_mie_value) {
	
	uint32_t mstatus_value, mtvec_value, old_mie_value;
	
	mstatus_value = 0b1000; //bit mask
	mtvec_value = (uint32_t) &handler; // sets trap address
	
	__asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value));
	//disables IRQs
	
	__asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));
	//sets up the handler I think?
	
	__asm__ volatile("csrr %0, mie" : "=r"(old_mie_value));
	__asm__ volatile("csrc mie, %0" :: "r"(old_mie_value));
	__asm__ volatile("csrs mie, %0" :: "r"(new_mie_value));
	//reads the old one, removes it, then sets the new one
	
	__asm__ volatile("csrs mstatus, %0" :: "r"(mstatus_value));
	//renables the IRQs
	
}
//Setting up the CPU IRQs

int main()
{
	printf( "start\n" );
	
	setup_mtimecmp();
	setup_cpu_irqs(0x00040080);

	// enable interrupts for all keys
	*(KEY_ptr + 2) = 0x3;
	*(KEY_ptr + 3) = 0x3;
	
	*seven_seg = seg(*(score));
	*seven_seg2 = seg(*(score+1));
	
	reset();
	
	while (*score < 9 && *(score + 1) < 9) {
				
		PERIOD = (*switches + 1)*TICK_SPEED;
		
		if (!( *alive && *(alive+1) )) {
			
			if (*(alive)) {
				*score +=1 ;
				*seven_seg = seg(*(score));
			}
			if (*(alive+1)) {
				*(score+1) +=1 ;
				*seven_seg2 = seg(*(score+1));
			}
			
			reset();
		}	
		
	}
	
	if (*score == 9)	rect( 2, MAX_Y, 2, MAX_X, pink );
	else if (*(score+1) == 9)	rect( 2, MAX_Y, 2, MAX_X, purple );
	
	printf( "done\n" );
}