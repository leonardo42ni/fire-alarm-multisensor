#include "node_zb_ep.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ha/esp_zigbee_ha_standard.h"
#include "config.h"

static esp_err_t add_basic_identify(esp_zb_cluster_list_t *cluster_list)
{
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
    };
    esp_zb_identify_cluster_cfg_t identify_cfg = {
        .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,
    };
    esp_err_t err = esp_zb_cluster_list_add_basic_cluster(
        cluster_list, esp_zb_basic_cluster_create(&basic_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (err != ESP_OK) {
        return err;
    }
    return esp_zb_cluster_list_add_identify_cluster(
        cluster_list, esp_zb_identify_cluster_create(&identify_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
}

static esp_err_t add_ep10(esp_zb_ep_list_t *ep_list)
{
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    if (cluster_list == NULL) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = add_basic_identify(cluster_list);
    if (err != ESP_OK) {
        return err;
    }
    esp_zb_temperature_meas_cluster_cfg_t temp_cfg = {
        .measured_value = (int16_t)lroundf(SPIKE_TEMP_C * ZCL_CENTI_SCALE),
        .min_value = ESP_ZB_ZCL_TEMP_MEASUREMENT_MIN_MEASURED_VALUE_MINIMUM,
        .max_value = ESP_ZB_ZCL_TEMP_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM,
    };
    esp_zb_humidity_meas_cluster_cfg_t hum_cfg = {
        .measured_value = (uint16_t)lroundf(SPIKE_HUM_PCT * ZCL_CENTI_SCALE),
        .min_value = ESP_ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MIN_MEASURED_VALUE_MINIMUM,
        .max_value = ESP_ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM,
    };
    err = esp_zb_cluster_list_add_temperature_meas_cluster(
        cluster_list, esp_zb_temperature_meas_cluster_create(&temp_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_zb_cluster_list_add_humidity_meas_cluster(
        cluster_list, esp_zb_humidity_meas_cluster_create(&hum_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (err != ESP_OK) {
        return err;
    }
    esp_zb_endpoint_config_t endpoint_cfg = {
        .endpoint = EP_TEMP,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID,
        .app_device_version = 0,
    };
    return esp_zb_ep_list_add_ep(ep_list, cluster_list, endpoint_cfg);
}

static esp_err_t add_analog_endpoint(esp_zb_ep_list_t *ep_list, uint8_t endpoint,
                                     float present_value, char *description)
{
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    if (cluster_list == NULL) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = add_basic_identify(cluster_list);
    if (err != ESP_OK) {
        return err;
    }
    esp_zb_analog_input_cluster_cfg_t analog_cfg = {
        .out_of_service = false,
        .present_value = present_value,
        .status_flags = 0,
    };
    esp_zb_attribute_list_t *attr_list = esp_zb_analog_input_cluster_create(&analog_cfg);
    if (attr_list == NULL) {
        return ESP_ERR_NO_MEM;
    }
    static uint32_t app_type;
    err = esp_zb_analog_input_cluster_add_attr(
        attr_list, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_DESCRIPTION_ID, description);
    if (err == ESP_OK && endpoint == EP_FLAGS) {
        err = esp_zb_cluster_add_attr(
            attr_list, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
            ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID,
            ESP_ZB_ZCL_ATTR_TYPE_U32,
            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
            &app_type) == ESP_OK ? ESP_OK : ESP_FAIL;
    }
    if (err != ESP_OK) {
        return err;
    }
    err = esp_zb_cluster_list_add_analog_input_cluster(
        cluster_list, attr_list, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (err != ESP_OK) {
        return err;
    }
    esp_zb_endpoint_config_t endpoint_cfg = {
        .endpoint = endpoint,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_CUSTOM_ATTR_DEVICE_ID,
        .app_device_version = 0,
    };
    return esp_zb_ep_list_add_ep(ep_list, cluster_list, endpoint_cfg);
}

esp_err_t node_zb_create_endpoints(esp_zb_ep_list_t **out_ep_list)
{
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    if (ep_list == NULL) {
        return ESP_ERR_NO_MEM;
    }
    const struct {
        uint8_t endpoint;
        float present_value;
        char *description;
    } analogs[] = {
        {EP_MQ2, SPIKE_MQ2_MV, (char *)"\x06" "MQ2_mV"},
        {EP_FLAME, SPIKE_FLAME_MV, (char *)"\x08" "flame_mV"},
        {EP_ECO2, SPIKE_ECO2_PPM, (char *)"\x08" "eco2_ppm"},
        {EP_TVOC, SPIKE_TVOC_PPB, (char *)"\x08" "tvoc_ppb"},
        {EP_FLAGS, SPIKE_FLAGS, (char *)"\x05" "flags"},
    };
    esp_err_t err = add_ep10(ep_list);
    for (size_t i = 0; err == ESP_OK && i < sizeof(analogs) / sizeof(analogs[0]); i++) {
        err = add_analog_endpoint(ep_list, analogs[i].endpoint, analogs[i].present_value,
                                  analogs[i].description);
    }
    if (err != ESP_OK) {
        return err;
    }
    *out_ep_list = ep_list;
    return ESP_OK;
}
