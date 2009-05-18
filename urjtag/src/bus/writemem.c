/*
 * $Id$
 *
 * Written by Kent Palmkvist (kentp@isy.liu.se>, 2005.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 *
 */

#include <sysdep.h>

#include <stdint.h>
#include <string.h>

#include <urjtag/log.h>
#include <urjtag/bus.h>
#include <urjtag/flash.h>
#include <urjtag/jtag.h>

// @@@@ RFHH return status
void
urj_bus_writemem (urj_bus_t *bus, FILE *f, uint32_t addr, uint32_t len)
{
    uint32_t step;
    uint32_t a;
    int bc = 0;
    int bidx = 0;
#define BSIZE 4096
    uint8_t b[BSIZE];
    urj_bus_area_t area;
    uint64_t end;

    if (!bus)
    {
        printf (_("Error: Missing bus driver!\n"));
        return;
    }

    URJ_BUS_PREPARE (bus);

    if (URJ_BUS_AREA (bus, addr, &area) != URJ_STATUS_OK)
    {
        printf (_("Error: Bus width detection failed\n"));
        return;
    }
    step = area.width / 8;

    if (step == 0)
    {
        printf (_("Unknown bus width!\n"));
        return;
    }

    addr = addr & (~(step - 1));
    len = (len + step - 1) & (~(step - 1));

    urj_log (URJ_LOG_LEVEL_NORMAL, _("address: 0x%08lX\n"),
             (long unsigned) addr);
    urj_log (URJ_LOG_LEVEL_NORMAL, _("length:  0x%08lX\n"),
             (long unsigned) len);

    if (len == 0)
    {
        urj_log (URJ_LOG_LEVEL_NORMAL, _("length is 0.\n"));
        return;
    }

    a = addr;
    end = a + len;
    urj_log (URJ_LOG_LEVEL_NORMAL, _("writing:\n"));

    for (; a < end; a += step)
    {
        uint32_t data;
        int j;

        /* Read one block of data */
        if (bc < step)
        {
            urj_log (URJ_LOG_LEVEL_NORMAL, _("addr: 0x%08lX"),
                     (long unsigned) a);
            urj_log (URJ_LOG_LEVEL_NORMAL, "\r");
            fflush (stdout);
            if (bc != 0)
                printf (_("Data not on word boundary, NOT SUPPORTED!"));
            if (feof (f))
            {
                printf (_("Unexpected end of file!\n"));
                printf (_("Addr: 0x%08lX\n"), (long unsigned) a);
                break;
            }
            bc = fread (b, 1, BSIZE, f);
            if (!bc)
            {
                printf (_("Short read: bc=0x%X\n"), bc);
            }
            bidx = 0;

        }

        /* Write a word at  time */
        data = 0;
        for (j = step; j > 0; j--)
        {
            if (urj_big_endian)
            {
                data |= b[bidx++];
                data <<= 8;
                bc--;
            }
            else
            {
                data |= (b[bidx++] << ((step - j) * 8));
                bc--;
            }
        }

        URJ_BUS_WRITE (bus, a, data);

    }

    urj_log (URJ_LOG_LEVEL_NORMAL, _("\nDone.\n"));
}
