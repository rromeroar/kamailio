/*
 * $Id$
 *
 * SYMSOFT OCS Diameter Support AVPs.
 * (C) 2014 Pablo Ruiz García <pruiz@netway.org>
 * Based on existing C-Diameter code from OpenIMS
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "avp_new.h"
#include "avp_new_base_data_format.h"
#include "avp_add.h"
#include "avp_get.h"
#include "avp_get_base_data_format.h"

extern struct cdp_binds *cdp;


#include "../cdp/cdp_load.h"


#include "symsoft.h"


#define CDP_AVP_DEFINITION

	#include "symsoft.h"

#undef CDP_AVP_DEFINITION



