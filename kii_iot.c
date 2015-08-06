#include "kii_iot.h"

#include <kii.h>

#include <stdio.h>
#include <string.h>


/* If your environment does not have assert, you must set NOASSERT define. */
#ifdef NOASSERT
  #define M_KII_IOT_ASSERT(s)
#else
  #include <assert.h>
  #define M_KII_IOT_ASSERT(s) assert(s)
#endif

#define EVAL(f, v) f(v)
#define TOSTR(s) #s
#define ULONG_MAX_STR EVAL(TOSTR, ULONG_MAX)
#define ULONGBUFSIZE (sizeof(ULONG_MAX_STR) / sizeof(char))

#define CONST_STRLEN(str) sizeof(str) - 1
#define APP_PATH "iot-api/apps"
#define ONBOARDING_PATH "onboardings"
#define CONTENT_TYPE_VENDOR_THING_ID "application/vnd.kii.onboardingWithVendorThingIDByThing+json"

#define M_KII_IOT_APPEND_CONST_STR(kii, str)  \
    { \
        size_t size = CONST_STRLEN(str); \
        if (kii_api_call_append_body(kii, str, size) != 0) {  \
            M_KII_LOG(kii->kii_core.logger_cb("request size overflowed.\n")); \
            return KII_FALSE; \
        } \
    }

#define M_KII_IOT_APPEND_STR(kii, str) \
    { \
        size_t size = strlen(str); \
        if (kii_api_call_append_body(kii, str, size) != 0) {  \
            M_KII_LOG(kii->kii_core.logger_cb("request size overflowed.\n")); \
            return KII_FALSE; \
        } \
    }


#define M_KII_IOT_APPEND_STR_WITH_LEN(kii, str, size)   \
    { \
        if (kii_api_call_append_body(kii, str, size) != 0) {  \
            M_KII_LOG(kii->kii_core.logger_cb("request size overflowed.\n")); \
            return KII_FALSE; \
        } \
    }

static kii_json_parse_result_t prv_kii_iot_json_read_object(
        kii_t* kii,
        const char* json_string,
        size_t json_string_size,
        kii_json_field_t *fields)
{
    kii_json_t kii_json;
    kii_json_parse_result_t retval;
    char error_message[50];

    memset(&kii_json, 0, sizeof(kii_json));
    kii_json.error_string_buff = error_message;
    kii_json.error_string_length =
        sizeof(error_message) / sizeof(error_message[0]);

    kii_json.resource = &(kii->kii_json_resource);
    retval = kii_json_read_object(&kii_json, json_string, json_string_size,
                fields);

    if (retval != KII_JSON_PARSE_SUCCESS) {
        M_KII_LOG(kii->kii_core.logger_cb(
                "fail to parse json: result=%d, message=%s\n",
                retval, kii_json.error_string_buff));
    }
    return retval;
}


static int prv_iot_parse_onboarding_response(kii_t* kii)
{
    kii_json_field_t fields[3];

    M_KII_IOT_ASSERT(kii != NULL);

    memset(fields, 0, sizeof(fields));
    fields[0].name = "thingID";
    fields[0].type = KII_JSON_FIELD_TYPE_STRING;
    fields[0].field_copy.string = kii->kii_core.author.author_id;
    fields[0].field_copy_buff_size = sizeof(kii->kii_core.author.author_id) /
            sizeof(kii->kii_core.author.author_id[0]);
    fields[1].name = "accessToken";
    fields[1].type = KII_JSON_FIELD_TYPE_STRING;
    fields[1].field_copy.string = kii->kii_core.author.access_token;
    fields[1].field_copy_buff_size = sizeof(kii->kii_core.author.access_token) /
            sizeof(kii->kii_core.author.access_token[0]);
    fields[2].name = NULL;

    if (prv_kii_iot_json_read_object(kii, kii->kii_core.response_body,
                    strlen(kii->kii_core.response_body), fields) !=
            KII_JSON_PARSE_SUCCESS) {
        return -1;
    }

    return 0;
}

kii_bool_t init_kii_iot(
        kii_iot_t* kii_iot,
        const char* app_id,
        const char* app_key,
        const char* app_host,
        char* mqtt_buff,
        size_t mqtt_buff_size,
        char* command_handler_buff,
        size_t command_handler_buff_size,
        char* state_updater_buff,
        size_t state_updater_buff_size,
        KII_IOT_ACTION_HANDLER action_handler)
{
    M_KII_IOT_ASSERT(kii_iot != NULL);
    M_KII_IOT_ASSERT(app_id != NULL);
    M_KII_IOT_ASSERT(app_key != NULL);
    M_KII_IOT_ASSERT(app_host != NULL);
    M_KII_IOT_ASSERT(mqtt_buff != NULL);
    M_KII_IOT_ASSERT(command_handler_buff != NULL);
    M_KII_IOT_ASSERT(state_updater_buff != NULL);

    memset(kii_iot, 0x00, sizeof(kii_iot_t));
    memset(mqtt_buff, 0x00, mqtt_buff_size);
    memset(command_handler_buff, 0x00, command_handler_buff_size);
    memset(state_updater_buff, 0x00, state_updater_buff_size);

    // Initialize command_handler
    if (kii_init(&kii_iot->command_handler, app_host, app_id, app_key) != 0) {
        return KII_FALSE;
    }
    kii_iot->command_handler.kii_core.http_context.buffer =
        command_handler_buff;
    kii_iot->command_handler.kii_core.http_context.buffer_size =
        command_handler_buff_size;

    kii_iot->command_handler.mqtt_buffer = mqtt_buff;
    kii_iot->command_handler.mqtt_buffer_size = mqtt_buff_size;

    kii_iot->command_handler.app_context = (void*)action_handler;

    // Initialize state_updater
    if (kii_init(&kii_iot->state_updater, app_host, app_id, app_key) != 0) {
        return KII_FALSE;
    }
    kii_iot->state_updater.kii_core.http_context.buffer = state_updater_buff;
    kii_iot->state_updater.kii_core.http_context.buffer_size =
        state_updater_buff_size;

    return KII_TRUE;
}

static int prv_kii_iot_get_key_and_value_from_json(
        kii_t* kii,
        const char* json_string,
        size_t json_string_len,
        char** out_key,
        char** out_value,
        size_t* out_key_len,
        size_t* out_value_len)
{
    jsmn_parser parser;
    int parse_result = JSMN_ERROR_NOMEM;
#ifdef KII_JSON_FIXED_TOKEN_NUM
    jsmntok_t tokens[KII_JSON_FIXED_TOKEN_NUM];
    size_t tokens_num = sizeof(tokens) / sizeof(tokens[0]);
#else
    jsmntok_t* tokens = kii->kii_json_resource.tokens;
    size_t tokens_num = kii->kii_json_resource.tokens_num;
#endif

    jsmn_init(&parser);

    parse_result = jsmn_parse(&parser, json_string, json_string_len, tokens,
            tokens_num);
    if (parse_result >= 0) {
        if (tokens[0].type != JSMN_OBJECT || tokens[0].size < 2) {
            M_KII_LOG(kii->kii_core.logger_cb("action must be json object.\n"));
            return -1;
        }
        if (tokens[1].type != JSMN_STRING) {
            M_KII_LOG(kii->kii_core.logger_cb("invalid json object.\n"));
            return -1;
        }
        *out_key = (char*)(json_string + tokens[1].start);
        *out_key_len = tokens[1].end - tokens[1].start;
        *out_value = (char*)(json_string + tokens[2].start);
        *out_value_len = tokens[2].end - tokens[2].start;
        return 0;
    } else if (parse_result == JSMN_ERROR_NOMEM) {
        M_KII_LOG(kii->kii_core.logger_cb(
                "Not enough tokens were provided.\n"));
        return -1;
    } else if (parse_result == JSMN_ERROR_INVAL) {
        M_KII_LOG(kii->kii_core.logger_cb(
                "Invalid character inside JSON string.\n"));
        return -1;
    } else if (parse_result == JSMN_ERROR_PART) {
        M_KII_LOG(kii->kii_core.logger_cb(
                "The string is not a full JSON packet, more bytes expected.\n"));
        return -1;
    } else {
        M_KII_LOG(kii->kii_core.logger_cb("Unexpected error.\n"));
        return -1;
    }
}

static void received_callback(kii_t* kii, char* buffer, size_t buffer_size) {
    kii_json_field_t fields[7];
    kii_json_field_t action[2];
    char* actions_str = NULL;
    size_t actions_len = 0;
    char index[ULONGBUFSIZE];
    size_t i = 0;

    memset(fields, 0x00, sizeof(fields));
    fields[0].path = "/thingTypeSchema";
    fields[0].type = KII_JSON_FIELD_TYPE_STRING;
    fields[0].field_copy.string = NULL;
    fields[1].path = "/thingTypeSchemaVersion";
    fields[1].type = KII_JSON_FIELD_TYPE_STRING;
    fields[1].field_copy.string = NULL;
    fields[2].path = "/targetType";
    fields[2].type = KII_JSON_FIELD_TYPE_STRING;
    fields[2].field_copy.string = NULL;
    fields[3].path = "/target";
    fields[3].type = KII_JSON_FIELD_TYPE_STRING;
    fields[3].field_copy.string = NULL;
    fields[4].path = "/failIfOffline";
    fields[4].type = KII_JSON_FIELD_TYPE_BOOLEAN;
    fields[5].path = "/actions";
    fields[5].type = KII_JSON_FIELD_TYPE_ARRAY;
    fields[5].field_copy.string = NULL;
    fields[6].path = NULL;

    if (prv_kii_iot_json_read_object(kii, buffer, buffer_size, fields)
            != KII_JSON_PARSE_SUCCESS) {
        M_KII_LOG(kii->kii_core.logger_cb("fail to parse received message.\n"));
        return;
    }

    // TODO: Check schema and schema version.
    // TODO: What should I do if failIfOffline is true?

    if (strncmp("thing", buffer + fields[2].start,
                    fields[2].end - fields[2].start) != 0) {
        *(buffer + fields[2].end) = '\0';
        M_KII_LOG(kii->kii_core.logger_cb("target type mismatches: %s.\n",
                        buffer + fields[2].start));
        return;
    }
    if (strncmp(kii->kii_core.author.author_id, buffer + fields[3].start,
                    fields[3].end - fields[3].start) != 0) {
        *(buffer + fields[3].end) = '\0';
        M_KII_LOG(kii->kii_core.logger_cb("target id mismatches: %s.\n",
                        buffer + fields[3].start));
        return;
    }

    actions_str = buffer + fields[5].start;
    actions_len = fields[5].end - fields[5].start;
    action[0].path = index;
    for (i = 0; action[0].result == KII_JSON_PARSE_SUCCESS; ++i) {
        sprintf(index, "/[%lu]", i);
        switch (prv_kii_iot_json_read_object(kii, actions_str, actions_len,
                        action)) {
            case KII_JSON_PARSE_SUCCESS:
            {
                KII_IOT_ACTION_HANDLER handler =
                    (KII_IOT_ACTION_HANDLER)kii->app_context;
                char* key;
                char* value;
                size_t key_len, value_len;
                char key_swap, value_swap;
                char error[EMESSAGE_SIZE + 1];
                if (prv_kii_iot_get_key_and_value_from_json(kii,
                            actions_str + action[0].start,
                            action[0].end - action[0].start, &key, &value,
                            &key_len, &value_len) != 0) {
                    *(actions_str + action[0].end) = '\0';
                    M_KII_LOG(kii->kii_core.logger_cb(
                            "fail to parse action: %s.\n",
                            actions_str + action[0].start));
                    return;
                }
                key_swap = key[key_len];
                value_swap = value[value_len];
                key[key_len] = '\0';
                value[value_len] = '\0';
                if ((*handler)(key, value, error) != KII_FALSE) {
                    // TODO: construct message.
                } else {
                    // TODO: construct message.
                }
                key[key_len] = key_swap;
                value[value_len] = value_swap;
            }
            case KII_JSON_PARSE_PARTIAL_SUCCESS:
                // This must be end of array.
                break;
            case KII_JSON_PARSE_ROOT_TYPE_ERROR:
            case KII_JSON_PARSE_INVALID_INPUT:
            default:
                M_KII_LOG(kii->kii_core.logger_cb("unexpected error.\n"));
                return ;
        }
    }
    return;
}

kii_bool_t onboard_with_vendor_thing_id(
        kii_iot_t* kii_iot,
        const char* vendor_thing_id,
        const char* password,
        const char* thing_type,
        const char* thing_properties
        )
{
    kii_t* kii = &kii_iot->command_handler;
    char resource_path[64];

    if (sizeof(resource_path) / sizeof(resource_path[0]) <=
            CONST_STRLEN(APP_PATH) + CONST_STRLEN("/") +
            strlen(kii->kii_core.app_id) + CONST_STRLEN(ONBOARDING_PATH)) {
        M_KII_LOG(kii->kii_core.logger_cb(
                "resource path is longer than expected.\n"));
        return KII_FALSE;
    }
    sprintf(resource_path, "%s/%s/%s", APP_PATH, kii->kii_core.app_id,
            ONBOARDING_PATH);

    if (kii_api_call_start(kii, "POST", resource_path,
                    CONTENT_TYPE_VENDOR_THING_ID, KII_FALSE) != 0) {
        M_KII_LOG(kii->kii_core.logger_cb(
            "fail to start api call.\n"));
    }
    M_KII_IOT_APPEND_CONST_STR(kii, "{\"vendorThingID\":\"");
    M_KII_IOT_APPEND_STR(kii, vendor_thing_id);
    M_KII_IOT_APPEND_CONST_STR(kii, "\",\"thingPassword\":\"");
    M_KII_IOT_APPEND_STR(kii, password);
    M_KII_IOT_APPEND_CONST_STR(kii, "\"");
    if (thing_type != NULL) {
        M_KII_IOT_APPEND_CONST_STR(kii, ",\"thingType\":\"");
        M_KII_IOT_APPEND_STR(kii, thing_type);
        M_KII_IOT_APPEND_CONST_STR(kii, "\"");
    }
    if (thing_properties != NULL) {
        M_KII_IOT_APPEND_CONST_STR(kii, ",\"thingProperties\":\"");
        M_KII_IOT_APPEND_STR(kii, thing_properties);
        M_KII_IOT_APPEND_CONST_STR(kii, "\"");
    }
    M_KII_IOT_APPEND_CONST_STR(kii, "}");

    if (kii_api_call_run(kii) != 0) {
        M_KII_LOG(kii->kii_core.logger_cb("fail to run api.\n"));
        return KII_FALSE;
    }

    if (prv_iot_parse_onboarding_response(kii) != 0) {
        M_KII_LOG(kii->kii_core.logger_cb("fail to parse resonse.\n"));
        return KII_FALSE;
    }

    if (kii_push_start_routine(kii, 0, 0, received_callback) != 0) {
        return KII_FALSE;
    }

    return KII_TRUE;
}

kii_bool_t onboard_with_thing_id(
        kii_iot_t* kii_iot,
        const char* thing_id,
        const char* password
        )
{
    kii_t* kii = &kii_iot->command_handler;
    char resource_path[64];

    if (sizeof(resource_path) / sizeof(resource_path[0]) <=
            CONST_STRLEN(APP_PATH) + CONST_STRLEN("/") +
            strlen(kii->kii_core.app_id) + CONST_STRLEN(ONBOARDING_PATH)) {
        M_KII_LOG(kii->kii_core.logger_cb(
                "resource path is longer than expected.\n"));
        return KII_FALSE;
    }
    sprintf(resource_path, "%s/%s/%s", APP_PATH, kii->kii_core.app_id,
            ONBOARDING_PATH);

    if (kii_api_call_start(kii, "POST", resource_path,
                    CONTENT_TYPE_VENDOR_THING_ID, KII_FALSE) != 0) {
        M_KII_LOG(kii->kii_core.logger_cb(
            "fail to start api call.\n"));
    }
    M_KII_IOT_APPEND_CONST_STR(kii, "{\"thingID\":\"");
    M_KII_IOT_APPEND_STR(kii, thing_id);
    M_KII_IOT_APPEND_CONST_STR(kii, "\",\"thingPassword\":\"");
    M_KII_IOT_APPEND_STR(kii, password);
    M_KII_IOT_APPEND_CONST_STR(kii, "\"}");

    if (kii_api_call_run(kii) != 0) {
        M_KII_LOG(kii->kii_core.logger_cb("fail to run api.\n"));
        return KII_FALSE;
    }

    if (prv_iot_parse_onboarding_response(kii) != 0) {
        M_KII_LOG(kii->kii_core.logger_cb("fail to parse resonse.\n"));
        return KII_FALSE;
    }

    if (kii_push_start_routine(kii, 0, 0, received_callback) != 0) {
        return KII_FALSE;
    }

    return KII_TRUE;
}
