#ifndef KII_JSON_H
#define KII_JSON_H

#include "kii.h"

typedef enum kii_json_parse_result {
    KII_JSON_PARSE_SUCCESS,
    KII_JSON_PARSE_UNEXPECTED_ARRAY,
    KII_JSON_PARSE_UNEXPECTED_OBJECT,
    KII_JSON_PARSE_INVALID
} kii_json_parse_result_t;

typedef enum kii_json_field_parse_result {
    KII_JSON_FIELD_PARSE_SUCCESS,
    KII_JSON_FIELD_PARSE_NOT_FOUND
} kii_json_field_parse_result_t;

typedef enum kii_json_field_type {
    KII_JSON_FIELD_TYPE_PRIMITIVE,
    KII_JSON_FIELD_TYPE_STRING,
    KII_JSON_FIELD_TYPE_OBJECT,
    KII_JSON_FIELD_TYPE_ARRAY
} kii_json_field_type_t;

/** json parsed field data */
typedef struct kii_json_field {
    /** parsing target key name */
    char* name;
    /** field parse result */
    kii_json_field_parse_result_t result;
    /** parsed target value type */
    kii_json_field_type_t type;
    /** start point of this field in given buffer */
    size_t start;
    /** end point of this field in given buffer */
    size_t end;
} kii_json_field_t;

/** parse json string as json object.
 *  \param [in] kii sdk instance.
 *  \param [in] pointer of json string.
 *  \param [in] length of json string.
 *  \param [inout] field of kii json parser.
 *  \return parse json result.
 */
kii_json_parse_result_t prv_kii_json_read_object(
        kii_t* kii,
        const char* json_string,
        size_t json_string_len,
        kii_json_field_t* field);

// TODO: remove this after new api is implemented..
int prv_kii_jsmn_get_tokens(
        kii_t* kii,
        const char* json_string,
        size_t json_string_len,
        jsmntok_t* tokens,
        size_t token_num);

// TODO: remove this after new api is implemented.
int prv_kii_jsmn_get_value(
        const char* json_string,
        size_t json_string_len,
        const jsmntok_t* tokens,
        const char* name,
        jsmntok_t** out_token);

#endif
