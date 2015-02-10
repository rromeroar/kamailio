#ifndef __CLIENT_RO_CONFIG_H
#define __CLIENT_RO_CONFIG_H

typedef enum {
    RO_MODE_3GPP = 0,
    RO_MODE_SYMSOFT = 1,
    RO_MODE_RFC_4006 = 2
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
    uint32_t service_parameter_type_caller;
    uint32_t service_parameter_type_called;
    uint32_t service_parameter_type_mtmo;
    uint32_t service_parameter_type_location_type;
    uint32_t service_parameter_type_location;
    uint32_t service_parameter_type_routing_case;
    uint32_t service_parameter_value_location_type;
    str service_parameter_value_location;
    uint32_t service_parameter_value_routing_case;
    uint32_t service_identifier;
    uint32_t rating_group;
} client_ro_cfg;

#endif
