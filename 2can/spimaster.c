/******************************************************************************
* File:              spimaster.c
* Author:            Kevin Day
* Date:              December, 2008
* Description:       
*                    
*                    
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
*                    
* Copyright (c) 2008 Kevin Day
* All rights reserved.
*******************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "platform.h"
#include "types.h"
#include "timers.h"
#include "tasks.h"
#include "comms_generic.h"
#include "spimaster.h"


/* In the future this can be a task with interrupt-driven SPI TX.  For now
 * it is a simple call based interface. */

#define SPIPORT PORTB
#define SPIDDR  DDRB
#define MISOPIN 3
#define MOSIPIN 2
#define SCKPIN  1
#define SSPIN   0


#define MCP2515_CS 4

void spi_select_chip(spi_slave_id_t sid)
{
    if (sid == SLAVE_2515)
    {
        /* CE  low */
        PORTB &= ~(_BV(MCP2515_CS));
    }
    else
    {
        /* CE  high */
        PORTB |= (_BV(MCP2515_CS));
    }
}



void spimaster_init()
{
    /* All but MISO as output  -- including SS which is not used here */
    SPIDDR  |= ((1<<MOSIPIN)|(1<<SCKPIN)|(1<<SSPIN));
    SPIDDR  &= ~(1<<MISOPIN);

    PORTB |= _BV(MCP2515_CS);
    DDRB  |= _BV(MCP2515_CS);  /* Port B4 is the MCP2515 /CS line */

    /* Enable pullup on MISO */
    SPIPORT |= (1<<MISOPIN);

    /* SPCR: 
     *   SPE:   1 enable interface
     *   DORD:  0 MSB first
     *   MSTR:  1 master mode
     *   CPOL:  0 CLK low when idle (leading edge is rising)
     *   CPHA:  0 Sample on leading (rising) edge
     *   SPR1:  1
     *   SPR0:  1 >> Fosc/128 if !SPI2X
     */
    SPCR = _BV(SPE)|_BV(MSTR)|_BV(SPR1)|_BV(SPR0);
    
    /* SPSR: 
     *   SPI2X: 0 not double speed 
     */
    SPSR = 0;

    /* Clear status registers */
    SPSR;
    SPDR;

    spi_select_chip(SLAVE_NONE);
    delay_us(100);
}

/* only "userspace" can lock the SPI.
 * userspace or ISR can unlock.
 * So semantics are simple here */
volatile u8 spilock;
static inline void lock_spi()
{
    while (spilock)
        ;

    spilock = 1;
//    DDRG |= 1;
//    PORTG |= 1;    
}
static inline void unlock_spi()
{
//    PORTG &= ~1;
    spilock = 0;
}
    

void spi_send(spi_slave_id_t sid, u16 len, u8 *data)
{    
    lock_spi();
    SPCR &= ~(1<<SPIE);
    spi_select_chip(sid);

    u16 i;
    for(i=0; i<len; i++)
    {
        SPDR = data[i];
        while (!(SPSR & _BV(SPIF)))
            ;
    }
    /* dummy read to clear */
    SPDR;

    delay_us(40);
    spi_select_chip(SLAVE_NONE);

    delay_us(500);
    unlock_spi();
}


void spi_start(spi_slave_id_t sid)
{    
    lock_spi();
    SPCR &= ~(1<<SPIE);
    spi_select_chip(sid);
}

void spi_write(u8 len, u8 *data)
{
    u8 i;
    for(i=0; i<len; i++)
    {
        SPDR = data[i];
        while (!(SPSR & _BV(SPIF)))
            ;
    }
}

void spi_read(u8 len, u8 *data)
{
    u8 i;
    for(i=0; i<len; i++)
    {
        SPDR = 0;
        while (!(SPSR & _BV(SPIF)))
            ;
        data[i] = SPDR;
    }
}

void spi_finish()
{
    /* dummy read to clear */
    SPDR;

    delay_us(40);
    spi_select_chip(SLAVE_NONE);

    delay_us(500);
    unlock_spi();
}




struct {
    u8  active;
    u8  *data;
    u16 length;
    u8  *status;
} async;
/* status will be written 0 when complete */
void spi_send_async(spi_slave_id_t sid, u16 len, u8 *data, u8 *status)
{
    lock_spi();
    spi_select_chip(sid);

    async.active = 1;
    async.data = data;
    async.length = len - 1;
    async.status = status;
    SPSR &= ~(1<<SPIF);
    SPCR |= (1<<SPIE);
    SPDR = *async.data;
    ++async.data;
}

SIGNAL(SIG_SPI)
{
    if (async.active)
    {
        if (async.length)
        {
            SPDR = *async.data;
            ++async.data;
            --async.length;
        }
        else
        {
            async.active = 0;
            *async.status = 0;
            SPCR &= ~(1<<SPIE);
            unlock_spi();
            spi_select_chip(SLAVE_NONE);
        }
    }
}
        


