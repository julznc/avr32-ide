
#ifndef __BSP_CLOCK_H__
#define __BSP_CLOCK_H__

#include <sysclk.h>
#include <cycle_counter/cycle_counter.h>

#define micros()            cpu_cy_2_us( Get_sys_count(), TARGET_MCUCLK_FREQ_HZ )
#define millis()            cpu_cy_2_ms( Get_sys_count(), TARGET_MCUCLK_FREQ_HZ )

#define delay_us(cnt)       cpu_delay_us(cnt, TARGET_MCUCLK_FREQ_HZ)
#define delay_ms(cnt)       cpu_delay_ms(cnt, TARGET_MCUCLK_FREQ_HZ)
#define delay               delay_ms


#endif	// __BSP_CLOCK_H__
