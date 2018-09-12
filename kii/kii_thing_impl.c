#include "kii_thing_impl.h"
#include "kii_impl.h"
#include "kii_json_utils.h"
#include <string.h>

kii_code_t _thing_authentication(
        kii_t* kii,
        const char* vendor_thing_id,
        const char* password)
{
    khc_set_host(&kii->_khc, kii->_app_host);
    // /api/apps/{appid}/oauth2/token
    int path_len = snprintf(kii->_rw_buff, kii->_rw_buff_size, "/api/apps/%s/oauth2/token", kii->_app_id);
    if (path_len >= kii->_rw_buff_size) {
        return KII_ERR_TOO_LARGE_DATA;
    }
    khc_set_path(&kii->_khc, kii->_rw_buff);
    khc_set_method(&kii->_khc, "POST");

    // Request headers.
    khc_slist* headers = NULL;
    int x_app_len = snprintf(kii->_rw_buff, kii->_rw_buff_size, "X-Kii-Appid: %s", kii->_app_id);
    if (x_app_len >= kii->_rw_buff_size) {
        return KII_ERR_TOO_LARGE_DATA;
    }
    headers = khc_slist_append(headers, kii->_rw_buff, x_app_len);

    char ct[] = "Content-Type: application/vnd.kii.OauthTokenRequest+json";
    headers = khc_slist_append(headers, ct, strlen(ct));

    headers = khc_slist_append(headers, _APP_KEY_HEADER, strlen(_APP_KEY_HEADER));

    // Request body.
    char esc_vid[strlen(vendor_thing_id) * 2 + 1];
    char esc_pass[strlen(password) * 2 + 1];
    kii_escape_str(vendor_thing_id, esc_vid, sizeof(esc_vid));
    kii_escape_str(password, esc_pass, sizeof(esc_vid));

    int content_len = snprintf(
        kii->_rw_buff,
        kii->_rw_buff_size,
        "{\"username\":\"VENDOR_THING_ID:%s\", \"password\":\"%s\", \"grant_type\":\"password\"}",
        esc_vid, esc_pass);
    if (content_len >= 256) {
        khc_slist_free_all(headers);
        return KII_ERR_TOO_LARGE_DATA;
    }

    // Content-Length.
    char cl_h[128];
    int cl_h_len = snprintf(cl_h, 128, "Content-Length: %d", content_len);
    if (cl_h_len >= 128) {
        khc_slist_free_all(headers);
        return KII_ERR_TOO_LARGE_DATA;
    }
    headers = khc_slist_append(headers, cl_h, cl_h_len);
    khc_set_req_headers(&kii->_khc, headers);

    kii->_rw_buff_req_size = content_len;

    khc_code code = khc_perform(&kii->_khc);
    khc_slist_free_all(headers);

    return _convert_code(code);
}

kii_code_t _register_thing_with_id(
        kii_t* kii,
        const char* vendor_thing_id,
        const char* password,
        const char* thing_type)
{
    khc_set_host(&kii->_khc, kii->_app_host);
    int path_len = snprintf(kii->_rw_buff, kii->_rw_buff_size, "/api/apps/%s/things", kii->_app_id);
    if (path_len >= kii->_rw_buff_size) {
        return KII_ERR_TOO_LARGE_DATA;
    }
    khc_set_path(&kii->_khc, kii->_rw_buff);
    khc_set_method(&kii->_khc, "POST");

    // Request headers.
    khc_slist* headers = NULL;
    int x_app_len = snprintf(kii->_rw_buff, kii->_rw_buff_size, "X-Kii-Appid: %s", kii->_app_id);
    if (x_app_len >= kii->_rw_buff_size) {
        return KII_ERR_TOO_LARGE_DATA;
    }
    headers = khc_slist_append(headers, kii->_rw_buff, x_app_len);
    headers = khc_slist_append(headers, _APP_KEY_HEADER, strlen(_APP_KEY_HEADER));
    char ct[] = "Content-Type: application/vnd.kii.ThingRegistrationAndAuthorizationRequest+json";
    headers = khc_slist_append(headers, ct, strlen(ct));

    // Request body.
    char esc_vid[strlen(vendor_thing_id) * 2 + 1];
    char esc_pass[strlen(password) * 2 + 1];
    char esc_type[strlen(thing_type) * 2 + 1];
    kii_escape_str(vendor_thing_id, esc_vid, sizeof(esc_vid));
    kii_escape_str(password, esc_pass, sizeof(esc_vid));
    kii_escape_str(thing_type, esc_type, sizeof(esc_type));

    int content_len = snprintf(
        kii->_rw_buff,
        kii->_rw_buff_size,
        "{\"_vendorThingID\":\"%s\", \"_thingType\":\"%s\", \"_password\":\"%s\"}",
        esc_vid, esc_type, esc_pass);
    if (content_len >= 256) {
        khc_slist_free_all(headers);
        return KII_ERR_TOO_LARGE_DATA;
    }

    // Content-Length.
    char cl_h[128];
    int cl_h_len = snprintf(cl_h, 128, "Content-Length: %d", content_len);
    if (cl_h_len >= 128) {
        khc_slist_free_all(headers);
        return KII_ERR_TOO_LARGE_DATA;
    }
    headers = khc_slist_append(headers, cl_h, cl_h_len);
    khc_set_req_headers(&kii->_khc, headers);

    kii->_rw_buff_req_size = content_len;

    khc_code code = khc_perform(&kii->_khc);
    khc_slist_free_all(headers);

    return _convert_code(code);
}