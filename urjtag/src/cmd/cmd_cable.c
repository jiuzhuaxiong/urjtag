/*
 * $Id$
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

#include <sysdep.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <urjtag/error.h>
#include <urjtag/parport.h>
#include <urjtag/tap.h>
#include <urjtag/cable.h>
#include <urjtag/chain.h>
#include <urjtag/bus.h>

#include <urjtag/cmd.h>

#include "cmd.h"

static int
cmd_cable_run (urj_chain_t *chain, char *params[])
{
    urj_cable_t *cable;
    int i;
    int paramc = urj_cmd_params (params);

    /* we need at least one parameter for 'cable' command */
    if (paramc < 2)
    {
        urj_error_set (URJ_ERROR_SYNTAX,
                       "%s: #parameters should be >= %d, not %d",
                       params[0], 2, paramc);
        return URJ_STATUS_FAIL;
    }

    /* maybe old syntax was used?  search connection type driver */
    for (i = 0; urj_tap_parport_drivers[i]; i++)
        if (strcasecmp (params[1], urj_tap_parport_drivers[i]->type) == 0)
            break;

    if (urj_tap_parport_drivers[i] != 0)
    {
        /* Old syntax was used. Swap params. */
        urj_warning ("Note: the 'cable' command syntax changed, please read the help text\n");
        if (paramc >= 4)
        {
            char *tmparam;
            tmparam = params[3];
            params[3] = params[2];
            params[2] = params[1];
            params[1] = tmparam;
        }
        else
        {
            urj_error_set (URJ_ERROR_SYNTAX,
                           "old syntax requires >= %d params, not %d",
                           4, paramc);
            return URJ_STATUS_FAIL;
        }
    }

    /* search cable driver list */
    for (i = 0; urj_tap_cable_drivers[i]; i++)
        if (strcasecmp (params[1], urj_tap_cable_drivers[i]->name) == 0)
            break;
    if (!urj_tap_cable_drivers[i])
    {
        urj_error_set (URJ_ERROR_NOTFOUND, _("Unknown cable type: '%s'"),
                       params[1]);
        return URJ_STATUS_FAIL;
    }

    if (paramc >= 3)
    {
        if (strcasecmp (params[2], "help") == 0)
        {
            urj_tap_cable_drivers[i]->help (URJ_LOG_LEVEL_NORMAL,
                                            urj_tap_cable_drivers[i]->name);
            return URJ_STATUS_OK;
        }
    }

    if (urj_bus)
    {
        URJ_BUS_FREE (urj_bus);
        urj_bus = NULL;
    }

    urj_tap_chain_disconnect (chain);

    cable = calloc (1, sizeof (urj_cable_t));
    if (!cable)
    {
        urj_error_set (URJ_ERROR_OUT_OF_MEMORY, "calloc(%zd,%zd) fails",
                       1, sizeof (urj_cable_t));
        return URJ_STATUS_FAIL;
    }

    cable->driver = urj_tap_cable_drivers[i];

    if (cable->driver->connect (++params, cable))
    {
        free (cable);
        return URJ_STATUS_FAIL;
    }

    chain->cable = cable;

    if (urj_tap_cable_init (chain->cable))
    {
        urj_tap_chain_disconnect (chain);
        return URJ_STATUS_FAIL;
    }
    urj_tap_chain_set_trst (chain, 0);
    urj_tap_chain_set_trst (chain, 1);
    urj_tap_reset (chain);

    return URJ_STATUS_OK;
}

static void
cmd_cable_help (void)
{
    int i;

    urj_log (URJ_LOG_LEVEL_NORMAL,
             _("Usage: %s DRIVER [DRIVER_OPTS]\n"
               "Select JTAG cable type.\n"
               "\n"
               "DRIVER      name of cable\n"
               "DRIVER_OPTS options for the selected cable\n"
               "\n"
               "Type \"cable DRIVER help\" for info about options for cable DRIVER.\n"
               "\n" "List of supported cables:\n"),
             "cable");

    for (i = 0; urj_tap_cable_drivers[i]; i++)
        urj_log (URJ_LOG_LEVEL_NORMAL,
                 _("%-13s %s\n"), urj_tap_cable_drivers[i]->name,
                _(urj_tap_cable_drivers[i]->description));
}

const urj_cmd_t urj_cmd_cable = {
    "cable",
    N_("select JTAG cable"),
    cmd_cable_help,
    cmd_cable_run
};
