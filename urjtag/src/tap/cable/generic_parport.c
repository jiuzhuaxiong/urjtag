/*
 * $Id: generic.c 1003 2008-02-10 10:00:30Z kawk $
 *
 * Copyright (C) 2003 ETC s.r.o.
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
 * Written by Marcel Telka <marcel@telka.sk>, 2003.
 *
 */

#include <urjtag/sysdep.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <urjtag/cable.h>
#include <urjtag/chain.h>
#include <urjtag/parport.h>

#include <urjtag/cmd.h>

#include "generic.h"
#include "generic_parport.h"

#undef VERBOSE

#ifdef VERBOSE
static void
print_vector (int len, char *vec)
{
    int i;
    for (i = 0; i < len; i++)
        printf ("%c", vec[i] ? '1' : '0');
}
#endif

int
urj_tap_cable_generic_parport_connect (char *params[], urj_cable_t *cable)
{
    urj_tap_cable_generic_params_t *cable_params;
    urj_parport_t *port;
    int i;

    if (urj_cmd_params (params) < 3)
    {
        printf (_("not enough arguments!\n"));
        return 1;
    }

    /* search parport driver list */
    for (i = 0; urj_tap_parport_drivers[i]; i++)
        if (strcasecmp (params[1], urj_tap_parport_drivers[i]->type) == 0)
            break;
    if (!urj_tap_parport_drivers[i])
    {
        printf (_("Unknown port driver: %s\n"), params[1]);
        return 2;
    }

    /* set up parport driver */
    port = urj_tap_parport_drivers[i]->connect ((const char **) &params[2],
                                                urj_cmd_params (params) - 2);

    if (port == NULL)
    {
        printf (_("Error: Cable connection failed!\n"));
        return 3;
    }

    cable_params = malloc (sizeof *cable_params);
    if (!cable_params)
    {
        printf (_("%s(%d) malloc failed!\n"), __FILE__, __LINE__);
        urj_tap_parport_drivers[i]->parport_free (port);
        return 4;
    }

    cable->link.port = port;
    cable->params = cable_params;
    cable->chain = NULL;

    return 0;
}

void
urj_tap_cable_generic_parport_free (urj_cable_t *cable)
{
    cable->link.port->driver->parport_free (cable->link.port);
    free (cable->params);
    free (cable);
}

void
urj_tap_cable_generic_parport_done (urj_cable_t *cable)
{
    urj_tap_parport_close (cable->link.port);
}

void
urj_tap_cable_generic_parport_help (const char *cablename)
{
    printf (_("Usage: cable %s parallel PORTADDR\n"
#if ENABLE_LOWLEVEL_PPDEV
              "   or: cable %s ppdev PPDEV\n"
#endif
#if HAVE_DEV_PPBUS_PPI_H
              "   or: cable %s ppi PPIDEV\n"
#endif
              "\n" "PORTADDR   parallel port address (e.g. 0x378)\n"
#if ENABLE_LOWLEVEL_PPDEV
              "PPDEV      ppdev device (e.g. /dev/parport0)\n"
#endif
#if HAVE_DEV_PPBUS_PPI_H
              "PPIDEF     ppi device (e.g. /dev/ppi0)\n"
#endif
              "\n"),
#if ENABLE_LOWLEVEL_PPDEV
            cablename,
#endif
#if HAVE_DEV_PPBUS_PPI_H
            cablename,
#endif
            cablename);
}
