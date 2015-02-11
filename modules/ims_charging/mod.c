/*
 * mod.c
 *
 *  Created on: 21 Feb 2013
 *      Author: jaybeepee
 */

#include "mod.h"
#include "../../sr_module.h"
#include "../../modules/dialog_ng/dlg_load.h"
#include "../../modules/dialog_ng/dlg_hash.h"
#include "../cdp/cdp_load.h"
#include "../cdp_avp/mod_export.h"
#include "../../parser/parse_to.h"
#include "stats.h"
#include "ro_timer.h"
#include "ro_session_hash.h"
#include "ims_ro.h"
#include "config.h"
#include "dialog.h"

MODULE_VERSION

/* parameters */
int ro_ccr_mode = 0; // 3GPP
char* ro_origin_host_s = "scscf.ims.smilecoms.com";
char* ro_origin_realm_s = "ims.smilecoms.com";
char* ro_destination_realm_s = "ims.smilecoms.com";
char* ro_destination_host_s = "hss.ims.smilecoms.com";
char* ro_msc_address_s = 0;
char* ro_vlr_location_s = 0;
char* ro_service_context_id_root_s = "32260@3gpp.org";
char* ro_service_context_id_ext_s = "ext";
char* ro_service_context_id_mnc_s = "01";
char* ro_service_context_id_mcc_s = "001";
char* ro_service_context_id_release_s = "8";
static int ro_session_hash_size = 4096;
int ro_timer_buffer = 5;
int interim_request_credits = 30;
int default_validity_time = 0;
int service_parameter_type_caller = 0; // 0 = not used
int service_parameter_type_called = 0; // 0 = not used
int service_parameter_type_mtmo = 0; // 0 = not used
int service_parameter_type_location_type = 0; // 0 = not used
int service_parameter_type_location = 0; // 0 = not used
int service_parameter_type_routing_case = 0; // 0 = not used
int service_parameter_value_location_type = 1;
char* service_parameter_value_location_s = "34111111111";
int service_parameter_value_routing_case = 1;
int service_identifier = 1000;
int rating_group = 0; // 0 = no rating group
client_ro_cfg cfg;

struct cdp_binds cdpb;
struct dlg_binds dlgb;
cdp_avp_bind_t *cdp_avp;
struct tm_binds tmb;

char* rx_dest_realm_s = "ims.smilecoms.com";
str rx_dest_realm;
/* Only used if we want to force the Ro peer usually this is configured at a stack level and the first request uses realm routing */
//char* rx_forced_peer_s = "";
str ro_forced_peer;
int ro_auth_expiry = 7200;
int cdp_event_latency = 1; /*flag: report slow processing of CDP callback events or not - default enabled */
int cdp_event_threshold = 500; /*time in ms above which we should report slow processing of CDP callback event - default 500ms*/
int cdp_event_latency_loglevel = 0; /*log-level to use to report slow processing of CDP callback event - default ERROR*/

stat_var *initial_ccrs;
stat_var *interim_ccrs;
stat_var *final_ccrs;
stat_var *successful_initial_ccrs;
stat_var *successful_interim_ccrs;
stat_var *successful_final_ccrs;
stat_var *ccr_responses_time;
stat_var *billed_secs;
stat_var *killed_calls;
stat_var *ccr_timeouts;

/** module functions */
static int mod_init(void);
static int mod_child_init(int);
static void mod_destroy(void);
static int w_ro_ccr(struct sip_msg *msg, str* route_name, str* direction, str* charge_type, str* unit_type, int reservation_units);
//void ro_session_ontimeout(struct ro_tl *tl);

static int ro_fixup(void **param, int param_no);

static cmd_export_t cmds[] = {
		{ "Ro_CCR", 	(cmd_function) w_ro_ccr, 5, ro_fixup, 0, REQUEST_ROUTE },
		{ 0, 0, 0, 0, 0, 0 }
};

static param_export_t params[] = {
		{ "ccr_mode",				INT_PARAM,			&ro_ccr_mode				},
		{ "hash_size", 				INT_PARAM,			&ro_session_hash_size 		},
		{ "interim_update_credits",	INT_PARAM,			&interim_request_credits 	},
		{ "default_validity_time",	INT_PARAM,			&default_validity_time		},
		{ "timer_buffer", 			INT_PARAM,			&ro_timer_buffer 			},
		{ "ro_forced_peer", 		STR_PARAM, 			&ro_forced_peer.s 			},
		{ "ro_auth_expiry",			INT_PARAM, 			&ro_auth_expiry 			},
		{ "cdp_event_latency", 		INT_PARAM,			&cdp_event_latency 			}, /*flag: report slow processing of CDP
																						callback events or not */
		{ "cdp_event_threshold", 	INT_PARAM, 			&cdp_event_threshold 		}, /*time in ms above which we should
																						report slow processing of CDP callback event*/
		{ "cdp_event_latency_log", 	INT_PARAM, 			&cdp_event_latency_loglevel },/*log-level to use to report
																						slow processing of CDP callback event*/
		{ "origin_host", 			STR_PARAM, 			&ro_origin_host_s 			},
		{ "origin_realm", 			STR_PARAM,			&ro_origin_realm_s 			},
		{ "destination_realm", 		STR_PARAM,			&ro_destination_realm_s 	},
		{ "destination_host", 		STR_PARAM,			&ro_destination_host_s 		},
		{ "msc_address",			STR_PARAM,			&ro_msc_address_s				},
		{ "vlr_location",			STR_PARAM,			&ro_vlr_location_s				},
		{ "service_context_id_root",STR_PARAM,			&ro_service_context_id_root_s 	},
		{ "service_context_id_ext", STR_PARAM,			&ro_service_context_id_ext_s 	},
		{ "service_context_id_mnc", STR_PARAM,			&ro_service_context_id_mnc_s 	},
		{ "service_context_id_mcc", STR_PARAM,			&ro_service_context_id_mcc_s 	},
		{ "service_context_id_release",	STR_PARAM, 		&ro_service_context_id_release_s},
        { "service_parameter_type_caller",	INT_PARAM,			&service_parameter_type_caller		},
        { "service_parameter_type_called",	INT_PARAM,			&service_parameter_type_called		},
        { "service_parameter_type_mtmo",	INT_PARAM,			&service_parameter_type_mtmo		},
        { "service_parameter_type_location_type",	INT_PARAM,			&service_parameter_type_location_type		},
        { "service_parameter_type_location",	INT_PARAM,			&service_parameter_type_location		},
        { "service_parameter_type_routing_case",	INT_PARAM,			&service_parameter_type_routing_case		},
        { "service_parameter_value_location_type",	INT_PARAM,			&service_parameter_value_location_type		},
        { "service_parameter_value_location",	STR_PARAM,			&service_parameter_value_location_s		},
        { "service_parameter_value_routing_case",	INT_PARAM,			&service_parameter_value_routing_case		},
        { "service_identifier",	INT_PARAM,			&service_identifier		},
        { "rating_group",	INT_PARAM,			&rating_group		},
        { 0, 0, 0 }
};

stat_export_t charging_stats[] = {
    {"initial_ccrs", STAT_NO_RESET, &initial_ccrs},
    {"interim_ccrs", STAT_NO_RESET, &interim_ccrs},
    {"final_ccrs", STAT_NO_RESET, &final_ccrs},
    {"successful_initial_ccrs", STAT_NO_RESET, &successful_initial_ccrs},
    {"successful_interim_ccr", STAT_NO_RESET, &successful_interim_ccrs},
    {"successful_final_ccrs", STAT_NO_RESET, &successful_final_ccrs},
    {"failed_initial_ccrs", STAT_IS_FUNC, (stat_var**) get_failed_initial_ccrs},
    {"failed_interim_ccr", STAT_IS_FUNC, (stat_var**) get_failed_interim_ccrs},
    {"failed_final_ccrs", STAT_IS_FUNC, (stat_var**) get_failed_final_ccrs},
    {"ccr_avg_response_time", STAT_IS_FUNC, (stat_var**) get_ccr_avg_response_time},
    {"ccr_responses_time", STAT_NO_RESET, &ccr_responses_time},
    {"billed_secs", STAT_NO_RESET, &billed_secs},
    {"killed_calls", STAT_NO_RESET, &killed_calls},
    {"ccr_timeouts", STAT_NO_RESET, &ccr_timeouts},
    {0, 0, 0}
};

/** module exports */
struct module_exports exports = { MOD_NAME, DEFAULT_DLFLAGS, /* dlopen flags */
		cmds, 		/* Exported functions */
		params, 	/* Exported params */
		charging_stats,	/* exported statistics */
		0, 			/* exported MI functions */
		0, 			/* exported pseudo-variables */
		0, 			/* extra processes */
		mod_init, 	/* module initialization function */
		0,
		mod_destroy, 	/* module destroy functoin */
		mod_child_init 	/* per-child init function */
};

int fix_parameters_symsoft() {
	if (!ro_msc_address_s) {
		LM_ERR("fix_parameters: msc_address param is mandatory for SYMSOFT mode\n");
		return 0;
	}
	cfg.msc_address.s = ro_msc_address_s;
	cfg.msc_address.len = strlen(ro_msc_address_s);

	if (!ro_vlr_location_s) {
		LM_ERR("fix_parameters: vlr_location param is mandatory for SYMSOFT mode\n");
		return 0;
	}
	cfg.vlr_location.s = ro_vlr_location_s;
	cfg.vlr_location.len = strlen(ro_vlr_location_s);
}

int fix_parameters() {
	cfg.mode = (ro_ccr_mode_t)ro_ccr_mode;
	cfg.default_validity_time = default_validity_time;

	cfg.origin_host.s = ro_origin_host_s;
	cfg.origin_host.len = strlen(ro_origin_host_s);

	cfg.origin_realm.s = ro_origin_realm_s;
	cfg.origin_realm.len = strlen(ro_origin_realm_s);

	cfg.destination_realm.s = ro_destination_realm_s;
	cfg.destination_realm.len = strlen(ro_destination_realm_s);

	cfg.destination_host.s = ro_destination_host_s;
	cfg.destination_host.len = strlen(ro_destination_host_s);

	if (cfg.mode == RO_MODE_SYMSOFT && !fix_parameters_symsoft()) {
		LM_ERR("unable to set SYMSOFT parameters correctly\n");
		return 0;
	}

	cfg.service_context_id = shm_malloc(sizeof(str));
	if (!cfg.service_context_id) {
		LM_ERR("fix_parameters:not enough shm memory\n");
		return 0;
	}
	cfg.service_context_id->len = strlen(ro_service_context_id_ext_s)
			+ strlen(ro_service_context_id_mnc_s)
			+ strlen(ro_service_context_id_mcc_s)
			+ strlen(ro_service_context_id_release_s)
			+ strlen(ro_service_context_id_root_s) + 5;
	cfg.service_context_id->s =
			pkg_malloc(cfg.service_context_id->len * sizeof (char));
	if (!cfg.service_context_id->s) {
		LM_ERR("fix_parameters: not enough memory!\n");
		return 0;
	}
	// FIXME: Use ccr_mode to select wether we should take svc_ctx_ext into account or not. (pruiz)
	cfg.service_context_id->len = strlen(ro_service_context_id_ext_s) == 0 ?
			sprintf(cfg.service_context_id->s, "%s", ro_service_context_id_root_s) :
			sprintf(cfg.service_context_id->s,
			"%s.%s.%s.%s.%s", ro_service_context_id_ext_s,
			ro_service_context_id_mnc_s, ro_service_context_id_mcc_s,
			ro_service_context_id_release_s, ro_service_context_id_root_s);
	if (cfg.service_context_id->len < 0) {
		LM_ERR("fix_parameters: error while creating service_context_id\n");
		return 0;
	}

    cfg.service_parameter_type_caller = service_parameter_type_caller;
    cfg.service_parameter_type_called = service_parameter_type_called;
    cfg.service_parameter_type_mtmo = service_parameter_type_mtmo;
    cfg.service_parameter_type_location_type = service_parameter_type_location_type;
    cfg.service_parameter_type_location = service_parameter_type_location;
    cfg.service_parameter_type_routing_case = service_parameter_type_routing_case;

    cfg.service_parameter_value_location_type = service_parameter_value_location_type;
    cfg.service_parameter_value_location.s = service_parameter_value_location_s;
    cfg.service_parameter_value_location.len = strlen(service_parameter_value_location_s);
    cfg.service_parameter_value_routing_case = service_parameter_value_routing_case;

    cfg.service_identifier = service_identifier;
    cfg.rating_group = rating_group;

	return 1;
}

static int mod_init(void) {
	int n;
	load_dlg_f load_dlg;
	load_tm_f load_tm;

	if (!fix_parameters()) {
		LM_ERR("unable to set Ro configuration parameters correctly\n");
		goto error;
	}

	/* bind to the tm module */
	if (!(load_tm = (load_tm_f) find_export("load_tm", NO_SCRIPT, 0))) {
		LM_ERR("Can not import load_tm. This module requires tm module\n");
		goto error;
	}
	if (load_tm(&tmb) == -1)
		goto error;

	if (!(load_dlg = (load_dlg_f) find_export("load_dlg", 0, 0))) { /* bind to dialog module */
		LM_ERR("can not import load_dlg. This module requires Kamailio dialog module.\n");
	}
	if (load_dlg(&dlgb) == -1) {
		goto error;
	}

	if (load_cdp_api(&cdpb) != 0) { /* load the CDP API */
		LM_ERR("can't load CDP API\n");
		goto error;
	}

	if (load_dlg_api(&dlgb) != 0) { /* load the dialog API */
		LM_ERR("can't load Dialog API\n");
		goto error;
	}

	cdp_avp = load_cdp_avp(); /* load CDP_AVP API */
	if (!cdp_avp) {
		LM_ERR("can't load CDP_AVP API\n");
		goto error;
	}

	/* init timer lists*/
	if (init_ro_timer(ro_session_ontimeout) != 0) {
		LM_ERR("cannot init timer list\n");
		return -1;
	}

	/* initialized the hash table */
	for (n = 0; n < (8 * sizeof(n)); n++) {
		if (ro_session_hash_size == (1 << n))
			break;
		if (ro_session_hash_size < (1 << n)) {
			LM_WARN("hash_size is not a power of 2 as it should be -> rounding from %d to %d\n", ro_session_hash_size, 1 << (n - 1));
			ro_session_hash_size = 1 << (n - 1);
		}
	}

	if (init_ro_session_table(ro_session_hash_size) < 0) {
		LM_ERR("failed to create ro session hash table\n");
		return -1;
	}

	/* register global timer */
	if (register_timer(ro_timer_routine, 0/*(void*)ro_session_list*/, 1) < 0) {
		LM_ERR("failed to register timer \n");
		return -1;
	}

	 /* register statistics */
	if (register_module_stats(exports.name, charging_stats) != 0) {
		LM_ERR("failed to register core statistics\n");
		return -1;
	}

	/*if (register_stat(MOD_NAME, "ccr_responses_time", &ccr_responses_time, 0)) {
		LM_ERR("failed to register core statistics\n");
		return -1;
	}*/

	return 0;

error:
	LM_ERR("Failed to initialise ims_qos module\n");
	return RO_RETURN_FALSE;

}

static int mod_child_init(int rank) {
	return 0;
}

static void mod_destroy(void) {

}

static int w_ro_ccr(struct sip_msg *msg, str* route_name, str* direction, str* charge_type, str* unit_type, int reservation_units) {
	/* PSEUDOCODE/NOTES
	 * 1. What mode are we in - terminating or originating
	 * 2. check request type - 	IEC - Immediate Event Charging
	 * 							ECUR - Event Charging with Unit Reservation
	 * 							SCUR - Session Charging with Unit Reservation
	 * 3. probably only do SCUR in this module for now - can see event based charging in another component instead (AS for SMS for example, etc)
	 * 4. Check a dialog exists for call, if not we fail
	 * 5. make sure we dont already have an Ro Session for this dialog
	 * 6. create new Ro Session
	 * 7. register for DLG callback passing new Ro session as parameter - (if dlg torn down we know which Ro session it is associated with)
	 *
	 *
	 */
	cfg_action_t* cfg_action;
	tm_cell_t *t;
	unsigned int tindex = 0,
				 tlabel = 0;

	LM_DBG("Ro CCR initiated: direction:%.*s, charge_type:%.*s, unit_type:%.*s, reservation_units:%i, route_name:%.*s",
			direction->len, direction->s,
			charge_type->len, charge_type->s,
			unit_type->len, unit_type->s,
			reservation_units,
			route_name->len, route_name->s);

    if (msg->first_line.type != SIP_REQUEST) {
    	LM_ERR("Ro_CCR() called from SIP reply.");
    	return -1;
    }

	LM_DBG("Looking for route block [%.*s]\n", route_name->len, route_name->s);

	int ri = route_get(&main_rt, route_name->s);
	if (ri < 0) {
		LM_ERR("unable to find route block [%.*s]\n", route_name->len, route_name->s);
		return RO_RETURN_ERROR;
	}
	
	cfg_action = main_rt.rlist[ri];
	if (!cfg_action) {
		LM_ERR("empty action lists in route block [%.*s]\n", route_name->len, route_name->s);
		return RO_RETURN_ERROR;
    }

	//before we send lets suspend the transaction
	t = tmb.t_gett();
	if (t == NULL || t == T_UNDEFINED) {
		if (tmb.t_newtran(msg) < 0) {
			LM_ERR("cannot create the transaction for CCR async\n");
			return RO_RETURN_ERROR;
		}
		t = tmb.t_gett();
		if (t == NULL || t == T_UNDEFINED) {
			LM_ERR("cannot lookup the transaction\n");
			return RO_RETURN_ERROR;
		}
	}

	LM_DBG("Suspending SIP TM transaction\n");
	if (tmb.t_suspend(msg, &tindex, &tlabel) < 0) {
		LM_ERR("failed to suspend the TM processing\n");
		return RO_RETURN_ERROR;
	}

	return Ro_Send_CCR(msg, direction, charge_type, unit_type, reservation_units, cfg_action, tindex, tlabel);
}

static int ro_fixup(void **param, int param_no) {
	str s;
	unsigned int num;

	if (param_no > 0 && param_no <= 4) {
		return fixup_var_str_12(param, param_no);
	} else if (param_no == 5) {
		/*convert to int */
		s.s = (char*)*param;
		s.len = strlen(s.s);
		if (str2int(&s, &num)==0) {
			pkg_free(*param);
			*param = (void*)(unsigned long)num;
			return 0;
		}
		LM_ERR("Bad reservation units: <%s>n", (char*)(*param));
		return E_CFG;
	}
	return 0;
}
