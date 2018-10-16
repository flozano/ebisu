#include "example.h"

#include <tio.h>
#include <jkii.h>

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <pthread.h>
#include <unistd.h>
#include "sys_cb_impl.h"
#include "sock_cb_linux.h"

static pthread_mutex_t m_mutex;

static void print_help() {
    printf("sub commands: [onboard|update]\n\n");
    printf("to see detail usage of sub command, execute ./exampleapp {subcommand} --help\n\n");

    printf("onboard with vendor-thing-id\n");
    printf("./exampleapp onboard --vendor-thing-id={vendor thing id} --password={password}\n\n");

    printf("update.\n"
            "./exampleapp update --firmware-version --thing-type --vendor-thing-id={vendor thing id} --password={password} \n\n");
}

void updater_init(
        tio_updater_t* updater,
        char* buffer,
        int buffer_size,
        void* sock_ssl_ctx,
        jkii_resource_t* resource)
{
    tio_updater_init(updater);

    tio_updater_set_app(updater, EX_APP_ID, EX_APP_SITE);

    tio_updater_set_cb_task_create(updater, task_create_cb_impl);
    tio_updater_set_cb_delay_ms(updater, delay_ms_cb_impl);

    tio_updater_set_buff(updater, buffer, buffer_size);

    tio_updater_set_cb_sock_connect(updater, sock_cb_connect, sock_ssl_ctx);
    tio_updater_set_cb_sock_send(updater, sock_cb_send, sock_ssl_ctx);
    tio_updater_set_cb_sock_recv(updater, sock_cb_recv, sock_ssl_ctx);
    tio_updater_set_cb_sock_close(updater, sock_cb_close, sock_ssl_ctx);

    tio_updater_set_interval(updater, 30);

    kii_set_json_parser_resource(&updater->_kii, resource);
}

const char send_file[] = "state.json";

typedef struct {
    size_t file_size;
    size_t file_read;
} updater_file_context_t;

size_t updater_cb_state_size(void* userdata)
{
    char c;
    updater_file_context_t* ctx = (updater_file_context_t*)userdata;

    printf("Send file?[y/n]: ");
    if ((c = getchar()) == 'y') {
        struct stat st;
        if (stat(send_file, &st) == 0) {
            ctx->file_size = st.st_size;
            ctx->file_read = 0;
            return st.st_size;
        } else {
            printf("failed to get stat\n");
        }
    }
    if (c != '\n') {
        while(getchar() != '\n');
    }
    return 0;
}

size_t updater_cb_read(char *buffer, size_t size, size_t count, void *userdata)
{
    updater_file_context_t* ctx = (updater_file_context_t*)userdata;
    FILE* fp;

    fp = fopen(send_file, "rb");
    if (fp == NULL) {
        printf("fopen error.\n");
        return 0;
    }

    if (fseek(fp, ctx->file_read, SEEK_SET) != 0) {
        printf("fseek error.\n");
        fclose(fp);
        return 0;
    }

    size_t read_size = fread(buffer, 1, size * count, fp);
    if (read_size > 0) {
        ctx->file_read += read_size;
    }

    fclose(fp);

    printf("updater_cb_read: %ld / %ld\n", ctx->file_read, ctx->file_size);
    return read_size;
}

void handler_init(
        tio_handler_t* handler,
        char* kii_buffer,
        int kii_buffer_size,
        void* http_ssl_ctx,
        char* mqtt_buffer,
        int mqtt_buffer_size,
        void* mqtt_ssl_ctx,
        jkii_resource_t* resource)
{
    tio_handler_init(handler);

    tio_handler_set_app(handler, EX_APP_ID, EX_APP_SITE);

    tio_handler_set_cb_task_create(handler, task_create_cb_impl);
    tio_handler_set_cb_delay_ms(handler, delay_ms_cb_impl);

    tio_handler_set_http_buff(handler, kii_buffer, kii_buffer_size);

    tio_handler_set_cb_sock_connect_http(handler, sock_cb_connect, http_ssl_ctx);
    tio_handler_set_cb_sock_send_http(handler, sock_cb_send, http_ssl_ctx);
    tio_handler_set_cb_sock_recv_http(handler, sock_cb_recv, http_ssl_ctx);
    tio_handler_set_cb_sock_close_http(handler, sock_cb_close, http_ssl_ctx);

    tio_handler_set_mqtt_buff(handler, mqtt_buffer, mqtt_buffer_size);

    tio_handler_set_cb_sock_connect_mqtt(handler, sock_cb_connect, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_send_mqtt(handler, sock_cb_send, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_recv_mqtt(handler, sock_cb_recv, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_close_mqtt(handler, sock_cb_close, mqtt_ssl_ctx);

    tio_handler_set_keep_alive_interval(handler, 0);

    kii_set_json_parser_resource(&handler->_kii, resource);
}

tio_bool_t tio_action_handler(tio_action_t* action, tio_action_err_t* err, void* userdata)
{
    printf("tio_action_handler called\n");
    printf("%.*s: %.*s\n", action->alias_length, action->alias, action->action_name_length, action->action_name);
    return KII_TRUE;
}

int main(int argc, char** argv)
{
    char* subc = argv[1];
    /*
    tio_command_handler_resource_t command_handler_resource;
    tio_state_updater_resource_t state_updater_resource;
    tio_system_cb_t sys_cb;
    char command_handler_buff[EX_COMMAND_HANDLER_BUFF_SIZE];
    char state_updater_buff[EX_STATE_UPDATER_BUFF_SIZE];
    char mqtt_buff[EX_MQTT_BUFF_SIZE];
    tio_t tio;
    kii_bool_t result;
    */

    tio_updater_t updater;
    tio_handler_t handler;
    char updater_buff[EX_STATE_UPDATER_BUFF_SIZE];
    socket_context_t updater_ctx;
    char kii_buff[EX_COMMAND_HANDLER_BUFF_SIZE];
    socket_context_t http_ctx;
    char mqtt_buff[EX_MQTT_BUFF_SIZE];
    socket_context_t mqtt_ctx;
    jkii_token_t updater_tokens[256];
    jkii_resource_t updater_resource = {updater_tokens, 256};
    jkii_token_t tokens[256];
    jkii_resource_t resource = {tokens, 256};
    updater_file_context_t updater_file_ctx;
    kii_code_t result;

    memset(updater_buff, 0x00, sizeof(char) * EX_STATE_UPDATER_BUFF_SIZE);
    updater_init(
            &updater,
            updater_buff,
            EX_STATE_UPDATER_BUFF_SIZE,
            &updater_ctx,
            &updater_resource);
    memset(kii_buff, 0x00, sizeof(char) * EX_COMMAND_HANDLER_BUFF_SIZE);
    memset(mqtt_buff, 0x00, sizeof(char) * EX_MQTT_BUFF_SIZE);
    handler_init(
            &handler,
            kii_buff,
            EX_COMMAND_HANDLER_BUFF_SIZE,
            &http_ctx,
            mqtt_buff,
            EX_MQTT_BUFF_SIZE,
            &mqtt_ctx,
            &resource);

    if (pthread_mutex_init(&m_mutex, NULL) != 0) {
        printf("fail to get mutex.\n");
        exit(1);
    }

    if (argc < 2) {
        printf("too few arguments.\n");
        print_help();
        exit(1);
    }

    /* Parse command. */
    if (strcmp(subc, "onboard") == 0) {
        char* vendorThingID = NULL;
        char* password = NULL;
        while(1) {
            struct option longOptions[] = {
                {"vendor-thing-id", required_argument, 0, 0},
                {"password", required_argument, 0, 1},
                {"help", no_argument, 0, 2},
                {0, 0, 0, 0}
            };
            int optIndex = 0;
            int c = getopt_long(argc, argv, "", longOptions, &optIndex);
            const char* optName = longOptions[optIndex].name;
            if (c == -1) {
                if (vendorThingID == NULL) {
                    printf("neither vendor-thing-id is specified.\n");
                    exit(1);
                }
                if (password == NULL) {
                    printf("password is not specifeid.\n");
                    exit(1);
                }
                printf("program successfully started!\n");
                result = kii_ti_onboard(
                        &handler._kii,
                        vendorThingID,
                        password,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
                if (result != KII_ERR_OK) {
                    printf("failed to onboard.\n");
                    exit(1);
                }
                break;
            }
            printf("option %s : %s\n", optName, optarg);
            switch(c) {
                case 0:
                    vendorThingID = optarg;
                    break;
                case 1:
                    password = optarg;
                    break;
                case 2:
                    printf("usage: \n");
                    printf("onboard --vendor-thing-id={ID of the thing} --password={password of the thing}\n");
                    break;
                default:
                    printf("unexpected usage.\n");
            }
            if (strcmp(optName, "help") == 0) {
                exit(0);
            }
        }
        tio_handler_start(&handler, NULL, tio_action_handler, NULL);
/*
    } else if (strcmp(subc, "get") == 0) {
        char* vendorThingID = NULL;
        char* thingID = NULL;
        char* password = NULL;
        int getFirmwareVersion = 0;
        int getThingType = 0;
        while (1) {
            struct option longOptions[] = {
                {"vendor-thing-id", required_argument, 0, 0},
                {"thing-id", required_argument, 0, 1},
                {"password", required_argument, 0, 2},
                {"firmware-version", no_argument, 0, 3},
                {"thing-type", no_argument, 0, 4},
                {"help", no_argument, 0, 5},
                {0, 0, 0, 0}
            };
            int optIndex = 0;
            int c = getopt_long(argc, argv, "", longOptions, &optIndex);
            if (c == -1) {
                break;
            }
            switch (c) {
                case 0:
                    vendorThingID = optarg;
                    break;
                case 1:
                    thingID = optarg;
                    break;
                case 2:
                    password = optarg;
                    break;
                case 3:
                    getFirmwareVersion = 1;
                    break;
                case 4:
                    getThingType = 1;
                    break;
                case 5:
                    printf("usage: \n"
                            "get --vendor-thing-id={ID of the thing} "
                            "--password={password of the thing} "
                            "--thing-type "
                            "--firmware-version\n");
                    exit(0);
                    break;
            }
        }
        if (vendorThingID == NULL && thingID == NULL) {
            printf("neither vendor-thing-id and thing-id are specified.\n");
            exit(1);
        }
        if (password == NULL) {
            printf("password is not specifeid.\n");
            exit(1);
        }
        if (vendorThingID != NULL && thingID != NULL) {
            printf("both vendor-thing-id and thing-id is specified.  either of one should be specified.\n");
            exit(1);
        }
        if (getFirmwareVersion == 0 && getThingType == 0) {
            printf("--firmware-version or --thing-type must be specified.\n");
            exit(1);
        }
        if (init_tio(
                &tio,
                EX_APP_ID,
                EX_APP_KEY,
                EX_APP_SITE,
                &command_handler_resource,
                &state_updater_resource,
                &sys_cb) == KII_FALSE) {
            printf("fail to initialize.\n");
            exit(1);
        }
        if (vendorThingID != NULL) {
            if (kii_ti_onboard(
                    &kii,
                    vendorThingID,
                    password,
                    NULL,
                    NULL,
                    NULL,
                    NULL) != KII_ERR_OK) {
                printf("fail to onboard.\n");
                exit(1);
            }
        } else {
            if (onboard_with_thing_id(
                    &tio,
                    thingID,
                    password,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL) == KII_FALSE) {
                printf("fail to onboard.\n");
                exit(1);
            }
        }

        if (getFirmwareVersion != 0) {
            char firmwareVersion[64];
            tio_error_t error;
            if (get_firmware_version(
                    &tio,
                    firmwareVersion,
                    sizeof(firmwareVersion) / sizeof(firmwareVersion[0]),
                    &error) == KII_FALSE) {
                printf("get_firmware_version is failed: %d\n", error.code);
                if (error.code == TIO_ERROR_HTTP) {
                    printf("status code=%d, error code=%s\n",
                            error.http_status_code,
                            error.error_code);
                }
                exit(0);
            }
            printf("firmware version=%s\n", firmwareVersion);
        }
        if (getThingType != 0) {
            char thingType[64];
            tio_error_t error;
            if (get_thing_type(
                    &tio,
                    thingType,
                    sizeof(thingType) / sizeof(thingType[0]),
                    &error) == KII_FALSE) {
                printf("get_thing_type is failed: %d\n", error.code);
                if (error.code == TIO_ERROR_HTTP) {
                    printf("status code=%d, error code=%s\n",
                            error.http_status_code,
                            error.error_code);
                }
                exit(0);
            }
            printf("thing type=%s\n", thingType);
        }
        exit(0);
*/
    } else if (strcmp(subc, "update") == 0) {
        char* vendorThingID = NULL;
        char* password = NULL;
        char* firmwareVersion = NULL;
        char* thingType = NULL;
        while (1) {
            struct option longOptions[] = {
                {"vendor-thing-id", required_argument, 0, 0},
                {"password", required_argument, 0, 1},
                {"firmware-version", required_argument, 0, 2},
                {"thing-type", required_argument, 0, 3},
                {"help", no_argument, 0, 4},
                {0, 0, 0, 5}
            };
            int optIndex = 0;
            int c = getopt_long(argc, argv, "", longOptions, &optIndex);
            if (c == -1) {
                break;
            }
            switch (c) {
                case 0:
                    vendorThingID = optarg;
                    break;
                case 1:
                    password = optarg;
                    break;
                case 2:
                    firmwareVersion = optarg;
                    break;
                case 3:
                    thingType = optarg;
                    break;
                case 4:
                    printf("usage: \n"
                            "update --vendor-thing-id={ID of the thing} "
                            "--password={password of the thing} "
                            "--thing-type={thing type "
                            "--firmware-version={firmware version}\n");
                    exit(0);
                    break;
            }
        }
        if (vendorThingID == NULL) {
            printf("neither vendor-thing-id is specified.\n");
            exit(1);
        }
        if (password == NULL) {
            printf("password is not specifeid.\n");
            exit(1);
        }
        result = kii_ti_onboard(
                &updater._kii,
                vendorThingID,
                password,
                thingType,
                firmwareVersion,
                NULL,
                NULL);
        if (result != KII_ERR_OK) {
            printf("failed to onboard.\n");
            exit(1);
        }

        tio_updater_start(
                &updater,
                NULL,
                updater_cb_state_size,
                &updater_file_ctx,
                updater_cb_read,
                &updater_file_ctx);
    } else {
        print_help();
        exit(0);
    }

    /* run forever. TODO: Convert to daemon. */
    while(1){ sleep(1); };

    /*
     * This sample application keeps mutex from the start to end
     * of the applicatoin process. So we don't implement destry.
     * pthread_mutex_destroy(&m_mutex);
    */
}

/* vim: set ts=4 sts=4 sw=4 et fenc=utf-8 ff=unix: */

