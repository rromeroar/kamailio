#ifndef __CLIENT_RO_CONFIG_H
#define __CLIENT_RO_CONFIG_H

typedef enum {
    RO_MODE_3GPP = 0,
    RO_MODE_SYMSOFT = 1
} ro_ccr_mode_t;

typedef struct {
    ro_ccr_mode_t mode;
    uint32_t default_validity_time;
    str origin_host;
    str origin_realm;
    str destination_realm;
    str destination_host;
    str msc_address;
    str vlr_location;
    str * service_context_id;
} client_ro_cfg;

#endif
