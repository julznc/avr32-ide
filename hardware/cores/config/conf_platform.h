/**
 * \file
 *
 * \brief Development Board Configuration
 *
 * Copyright (c) 2011-2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#ifndef _CONF_SENSOR_PLATFORM_H_
#define _CONF_SENSOR_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup atavrpb_config Sensors Platform Board Configuration
 * @brief
 * Configuration constants defined for the platform are used to enable
 * peripherals and map I/O pin interfaces between sensors and the
 * development board they are paired with.
 *
 * @sa atavrsb_config
 * @{
 */

/** \name Platform Board Interrupt Priorities */
/** @{ */
#define CONFIG_GPIO_INT_LVL         0
#define CONFIG_TWIM_INT_LVL         2
/** @} */

/** \name Platform-Specific GPIO Pin Configuration */
/** @{ */
#define CONF_BOARD_SPI              /**< Map board SPI bus I/O pins */
#define CONF_BOARD_TWI              /**< Map board TWI bus I/O pins */
#define CONF_BOARD_ENABLE_USARTC0   /**< Map XMEGA-A1 USART pins */
#define CONF_BOARD_ENABLE_USARTE0   /**< Map XMEGA-B1 USART pins */
#define CONF_BOARD_ENABLE_USARTD0   /**< Map XMEGA-A3BU USART pins */
#define CONF_BOARD_COM_PORT         /**< Map UC3-L0 / UC3-A3 USART USB Virtual Com pins */
/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
