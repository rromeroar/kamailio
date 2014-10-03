#include "Ro_data.h"
#include "config.h"


#define str_dup(dst,src,mem) \
do {\
	if ((src).len) {\
		(dst).s = mem##_malloc((src).len);\
		if (!(dst).s){\
			LM_ERR("Error allocating %d bytes in %s!\n",(src).len,#mem);\
			(dst).len = 0;\
			goto out_of_memory;\
		}\
		memcpy((dst).s,(src).s,(src).len);\
		(dst).len = (src).len;\
	}else{\
		(dst).s=0;(dst).len=0;\
	}\
} while (0)

/**
 * Frees a str content.
 * @param x - the str to free
 * @param mem - type of memory that the content is using (shm/pkg)
 */
#define str_free(x,mem) \
do {\
	if ((x).s) mem##_free((x).s);\
	(x).s=0;(x).len=0;\
} while(0)

extern client_ro_cfg cfg;


/**
 * Gets the username part of a SIP URI
 * @param sip_uri - the SIP URI
 * @returns str with the username, or NULL in case of error
 */
str * get_username_from_sip_uri(str * sip_uri) {
    str * username;

    if (sip_uri == NULL) {
        LM_ERR("sip_uri is NULL\n");
    } else if (sip_uri->len < 4 || strncmp(sip_uri->s, "sip:", 4) != 0) {
        LM_ERR("sip_uri is not a valid SIP URI\n");
    } else {
        char * username_start = sip_uri->s + 4;
        char * username_end = memchr(username_start, '@', sip_uri->len - 4);

        if (username_end == NULL) {
            LM_ERR("sip_uri is not a valid SIP URI\n");
        } else {
            str x = {username_start, username_end - username_start};
            str_dup_ptr(username, x, pkg);
        }
    }

    return username;

out_of_memory:
    LM_ERR("out of pkg memory\n");
    return NULL;
}

event_type_t * new_event_type(str * sip_method, str * event, uint32_t * expires) {
    event_type_t * x = 0;

    mem_new(x, sizeof (event_type_t), pkg);
    if (sip_method && sip_method->s)
        str_dup_ptr(x->sip_method, *sip_method, pkg);
    if (event && event->s)
        str_dup_ptr(x->event, *event, pkg);
    if (expires && *expires != 0) {
        mem_new(x->expires, sizeof (uint32_t), pkg);
        *(x->expires) = *expires;
    }
    return x;

out_of_memory:
    LM_ERR("out of pkg memory\n");
    event_type_free(x);
    return NULL;
}

time_stamps_t * new_time_stamps(time_t *sip_request_timestamp, uint32_t *sip_request_timestamp_fraction, time_t *sip_response_timestamp, uint32_t *sip_response_timestamp_fraction) {

    time_stamps_t * x = 0;

    mem_new(x, sizeof (time_stamps_t), pkg);

    if (sip_request_timestamp && *sip_request_timestamp > 0) {
        mem_new(x->sip_request_timestamp, sizeof (time_t), pkg);
        *(x->sip_request_timestamp) = *sip_request_timestamp;
    }

    if (sip_request_timestamp_fraction && *sip_request_timestamp_fraction > 0) {
        mem_new(x->sip_request_timestamp_fraction, sizeof (uint32_t), pkg);
        *(x->sip_request_timestamp_fraction) = *sip_request_timestamp_fraction;
    }

    if (sip_response_timestamp && *sip_response_timestamp > 0) {
        mem_new(x->sip_response_timestamp, sizeof (time_t), pkg);
        *(x->sip_response_timestamp) = *sip_response_timestamp;
    }

    if (sip_response_timestamp_fraction && *sip_response_timestamp_fraction > 0) {
        mem_new(x->sip_response_timestamp_fraction, sizeof (uint32_t), pkg);
        *(x->sip_response_timestamp_fraction) = *sip_response_timestamp_fraction;
    }


    return x;

out_of_memory:
    LM_ERR("out of pkg memory\n");
    time_stamps_free(x);
    return 0;
}

ims_information_t * new_ims_information(event_type_t * event_type, time_stamps_t * time_stamps, str * user_session_id, str * outgoing_session_id, str * calling_party, str * called_party, str * icid, str * orig_ioi, str * term_ioi, int node_role) {

    str_list_slot_t *sl = 0;
    ims_information_t *x = 0;
    ioi_list_element_t * ioi_elem = 0;

    mem_new(x, sizeof (ims_information_t), pkg);

    x->event_type = event_type;
    x->time_stamps = time_stamps;

    mem_new(x->role_of_node, sizeof (int32_t), pkg);
    *(x->role_of_node) = node_role;

    //x->node_functionality = cfg.node_func;

    if (outgoing_session_id && outgoing_session_id->s)
        str_dup_ptr(x->outgoing_session_id, *outgoing_session_id, pkg);

    if (user_session_id && user_session_id->s)
        str_dup_ptr(x->user_session_id, *user_session_id, pkg);

    if (calling_party && calling_party->s) {
        mem_new(sl, sizeof (str_list_slot_t), pkg);
        str_dup(sl->data, *calling_party, pkg);
        WL_APPEND(&(x->calling_party_address), sl);
    }

    if (called_party && called_party->s)
        str_dup_ptr(x->called_party_address, *called_party, pkg);

    //WL_FREE_ALL(&(x->called_asserted_identity),str_list_t,pkg);
    //str_free_ptr(x->requested_party_address,pkg);

    if ((orig_ioi && orig_ioi->s) || (term_ioi && term_ioi->s)) {
        mem_new(ioi_elem, sizeof (ioi_list_element_t), pkg);
        if (orig_ioi)
            str_dup_ptr_ptr(ioi_elem->info.originating_ioi, orig_ioi, pkg);
        if (term_ioi)
            str_dup_ptr_ptr(ioi_elem->info.terminating_ioi, term_ioi, pkg);
    }

    if (icid && icid->s)
        str_dup_ptr(x->icid, *icid, pkg);

    return x;

out_of_memory:
    LM_ERR("out of pkg memory\n");
    ims_information_free(x);
    return NULL;
}

service_information_t * new_service_information(ims_information_t * ims_info, subscription_id_t * subscription) {
    service_information_t * x = 0;
    subscription_id_list_element_t * sl = 0;

    mem_new(x, sizeof (service_information_t), pkg);

    x->ims_information = ims_info;
    if (subscription) {
        mem_new(sl, sizeof (subscription_id_list_element_t), pkg);
        subscription_id_list_t_copy(&(sl->s), subscription, pkg);
        WL_APPEND(&(x->subscription_id), sl);
    }

    return x;

out_of_memory:
    LM_ERR("new service information: out of pkg memory\n");
    service_information_free(x);
    return 0;
}

voice_service_information_t * new_voice_service_information(ims_information_t * ims_info, subscription_id_t * subscription) {
    voice_service_information_t * x = 0;
    str * called_party_number_address = 0;

    mem_new(x, sizeof (voice_service_information_t), pkg);

    x->traffic_case = AVP_Traffic_Case_MO;
    str_dup(x->msc_address, cfg.msc_address, pkg);

    if ((called_party_number_address = get_username_from_sip_uri(ims_info->called_party_address)) == NULL)
        goto error;

    x->called_party_number.number_plan = AVP_Number_Plan_MSISDN;
    x->called_party_number.number_type = AVP_Number_Type_International;
    str_dup(x->called_party_number.address_data, *called_party_number_address, pkg);
    str_dup_ptr(x->location_information, cfg.vlr_location, pkg);
    x->call_service_type = AVP_Call_Service_Type_Voice;

    str_free_ptr(called_party_number_address, pkg);

    return x;

out_of_memory:
    LM_ERR("new voice service information: out of pkg memory\n");
error:
    str_free_ptr(called_party_number_address, pkg);
    voice_service_information_free(x);
    return 0;
}

Ro_CCR_t * new_Ro_CCR(int32_t acc_record_type, str * user_name, ims_information_t * ims_info, subscription_id_t * subscription) {


    Ro_CCR_t *x = 0;

    service_information_t * service_info = 0;
    voice_service_information_t * voice_service_info = 0;

    mem_new(x, sizeof (Ro_CCR_t), pkg);

    str_dup(x->origin_host, cfg.origin_host, pkg);
    str_dup(x->origin_realm, cfg.origin_realm, pkg);
    str_dup(x->destination_realm, cfg.destination_realm, pkg);
    x->acct_record_type = acc_record_type;

    if (user_name) {
        str_dup_ptr_ptr(x->user_name, user_name, pkg);
    }

    if (cfg.service_context_id && cfg.service_context_id->s)
        str_dup_ptr(x->service_context_id, *(cfg.service_context_id), pkg);

    if (cfg.mode == RO_MODE_3GPP && ims_info) {
        if (!(service_info = new_service_information(ims_info, subscription)))
            goto error;
    } else if (cfg.mode == RO_MODE_SYMSOFT && ims_info) {
        if (!(voice_service_info = new_voice_service_information(ims_info, subscription)))
            goto error;
    }

    x->service_information = service_info;
    x->voice_service_information = voice_service_info;
    service_info = 0;
    voice_service_info = 0;

    return x;

out_of_memory:
    LM_ERR("out of pkg memory\n");
error:
    Ro_free_CCR(x);
    service_information_free(service_info);
    voice_service_information_free(voice_service_info);

    return 0;
}

void event_type_free(event_type_t *x) {
    if (!x) return;
    str_free_ptr(x->sip_method, pkg);
    str_free_ptr(x->event, pkg);
    mem_free(x->expires, pkg);
    mem_free(x, pkg);
}

void time_stamps_free(time_stamps_t *x) {
    if (!x) return;
    mem_free(x->sip_request_timestamp, pkg);
    mem_free(x->sip_request_timestamp_fraction, pkg);
    mem_free(x->sip_response_timestamp, pkg);
    mem_free(x->sip_response_timestamp_fraction, pkg);
    mem_free(x, pkg);
}

void ims_information_free(ims_information_t *x) {
    if (!x) return;

    event_type_free(x->event_type);

    mem_free(x->role_of_node, pkg);
    str_free_ptr(x->user_session_id, pkg);
    str_free_ptr(x->outgoing_session_id, pkg);

    WL_FREE_ALL(&(x->calling_party_address), str_list_t, pkg);
    str_free_ptr(x->called_party_address, pkg);
    WL_FREE_ALL(&(x->called_asserted_identity), str_list_t, pkg);
    str_free_ptr(x->requested_party_address, pkg);

    time_stamps_free(x->time_stamps);

    WL_FREE_ALL(&(x->as_info), as_info_list_t, pkg);

    WL_FREE_ALL(&(x->ioi), ioi_list_t, pkg);
    str_free_ptr(x->icid, pkg);

    str_free_ptr(x->service_id, pkg);

    WL_FREE_ALL(&(x->service_specific_info), service_specific_info_list_t, pkg);

    mem_free(x->cause_code, pkg);

    mem_free(x, pkg);
}

void service_information_free(service_information_t *x) {
    if (!x) return;

    WL_FREE_ALL(&(x->subscription_id), subscription_id_list_t, pkg);
    ims_information_free(x->ims_information);

    mem_free(x, pkg);
}

void E164_number_free(E164_number_t *x) {
    if (!x) return;

    str_free(x->address_data, pkg);
    mem_free(x, pkg);
}

void voice_service_information_free(voice_service_information_t *x) {
    if (!x) return;

    str_free(x->msc_address, pkg);
    str_free(x->called_party_number.address_data, pkg);
    str_free_ptr(x->location_information, pkg);

    mem_free(x, pkg);
}

void Ro_free_CCR(Ro_CCR_t *x) {
    if (!x) return;

    str_free(x->origin_host, pkg);
    str_free(x->origin_realm, pkg);
    str_free(x->destination_realm, pkg);

    str_free_ptr(x->user_name, pkg);
    mem_free(x->acct_interim_interval, pkg);
    mem_free(x->origin_state_id, pkg);
    mem_free(x->event_timestamp, pkg);

    str_free_ptr(x->service_context_id, pkg);

    service_information_free(x->service_information);
    voice_service_information_free(x->voice_service_information);

    mem_free(x, pkg);
}

void Ro_free_CCA(Ro_CCA_t *x) {
    if (!x) return;

    if (x->mscc->final_unit_action) {
        mem_free(x->mscc->final_unit_action, pkg);
    }
    mem_free(x->mscc->granted_service_unit, pkg);
    mem_free(x->mscc, pkg);
    mem_free(x, pkg);
}



