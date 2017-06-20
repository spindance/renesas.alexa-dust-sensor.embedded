/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products.
* No other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIESREGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED
* OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY
* LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE FOR ANY DIRECT,
* INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR
* ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability
* of this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2013, 2016 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/

/***********************************************************************************************************************
* File Name    : net_thread_entry.c
* Version      : 1.0
* Device(s)    : R7FS3A77C
* Tool-Chain   : GCC ARM Embedded v4.9.3.20150529
* OS           : ThreadX
* H/W Platform : Renesas Synergy S3A7 IoT Fast Prototyping Kit
* Description  : This file implements Network functions
* Creation Date: 02/17/2017
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes <System Includes> , “Project Includes”
***********************************************************************************************************************/
#include <stdlib.h>
#include "app.h"
#include "net_thread.h"
#include "r_fmi.h"
#include "fx_api.h"
#include "nx_api.h"
#include "nx_http_server.h"
#include "nx_dhcp.h"
#include "nx_dhcp_server.h"
#include "nx_dns.h"
#include "m1_agent.h"

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
#define DO_NOT_PUBLISH_TO_MQTT false

#define APP_ERR_TRAP(a)     if(a)\
                            {\
                                g_ioport.p_api->pinWrite(leds[0], IOPORT_LEVEL_HIGH);\
                                g_ioport.p_api->pinWrite(leds[1], IOPORT_LEVEL_LOW);\
                                g_ioport.p_api->pinWrite(leds[2], IOPORT_LEVEL_LOW);\
                                g_ioport.p_api->pinWrite(leds[3], IOPORT_LEVEL_LOW);\
                                while (1);\
                            }

#define APP_SERVER_ADDRESS          (IP_ADDRESS(192, 168, 3  , 1))
#define APP_SERVER_MASK             (IP_ADDRESS(255, 255, 255, 0))
#define START_IP_ADDRESS_LIST       (IP_ADDRESS(192, 168, 3  , 100))
#define END_IP_ADDRESS_LIST         (IP_ADDRESS(192, 168, 3  , 200))
#define GW_ADDR                     (IP_ADDRESS(192, 168, 3  , 1))

#define BLOCK_SIZE                  (1536)
#define NUM_PACKETS                 (5)
#define NUM_IP_PACKETS              (10)
#define IP_THREAD_SIZE              (10 * 1024)
#define DHCP_STACK_SIZE             (2  * 1024)
#define HTTP_STACK_SIZE             (4  * 1024)

#define MQTT_USERNAME_LEN_MAX       (128)
#define MQTT_PASSWORD_LEN_MAX       (128)

/***********************************************************************************************************************
Exported global functions
***********************************************************************************************************************/
/* Export from other files */
extern VOID nx_ether_driver_eth1(NX_IP_DRIVER *);
extern VOID _fx_ram_driver(FX_MEDIA *media_ptr);

/* Export to other files */
void net_thread_entry(void);
VOID g_sf_wifi_nsal_nx0(NX_IP_DRIVER *p_driver);

/***********************************************************************************************************************
Private functions
***********************************************************************************************************************/
UINT my_get_notify(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr);
void m1_message_callback(int type, char *topic, char *msg, int length);
void publish_notification();
static void write_data_to_json_buffer(sensor_data_t data, char *buffer);

/***********************************************************************************************************************
Exported global variables
***********************************************************************************************************************/
/* Export from other files */
extern TX_THREAD net_thread;
extern TX_SEMAPHORE g_provision_lock;
extern const fmi_instance_t g_fmi0;
extern const uint16_t m1provisionend[];
extern const uint8_t provisionHtml[];

/* Export to other files */
int provisioning = 0;
bool provisioned = false;
char m1_mqtt_publish_topic[130];
char m1_mqtt_subscribe_topic[130];

/***********************************************************************************************************************
Private global variables
***********************************************************************************************************************/
FX_MEDIA               *gp_media;
NX_PACKET_POOL          g_http_packet_pool;
NX_IP                   g_http_ip;
static NX_HTTP_SERVER   g_http_server;
static NX_DHCP_SERVER   dhcp_server;
static NX_DHCP          g_dhcp;
NX_DNS                  g_dns_client;
NX_PACKET_POOL          dhcp_packet_pool;
NX_TCP_SOCKET           g_client_socket;
UCHAR                   ram_disk_memory[13*1024];
UCHAR                   ram_disk_sector_cache[512];
FX_MEDIA                ram_disk_media;

static CHAR mem_packet_pool[(BLOCK_SIZE + 32 + sizeof(NX_PACKET)) * NUM_IP_PACKETS] __attribute__ ((aligned(4)));
static CHAR mem_ip_stack[IP_THREAD_SIZE] __attribute__ ((aligned(4)));
static CHAR mem_arp[1024] __attribute__ ((aligned(4)));
static CHAR mem_http_stack[HTTP_STACK_SIZE]  __attribute__ ((aligned(4)));
static CHAR dhcp_server_stack [DHCP_STACK_SIZE];
static char dhcp_buffer_pool_memory [BLOCK_SIZE*NUM_PACKETS];

char m1_apikey[65];
char m1_password[65];
char m1_mqtt_user_id[32];
char m1_mqtt_project_id[32];
char m1_mqtt_username[MQTT_USERNAME_LEN_MAX + 1];
char m1_mqtt_password[MQTT_PASSWORD_LEN_MAX + 1];
char m1_device_id[33] = "s3a7";
uint8_t update_rate;

static ioport_port_pin_t leds[4] =
{
    IOPORT_PORT_07_PIN_00,  /* Red */
    IOPORT_PORT_07_PIN_01,  /* Yellow */
    IOPORT_PORT_07_PIN_02,  /* Green */
    IOPORT_PORT_07_PIN_03   /* Green */
};


/***********************************************************************************************************************
* Function Name: net_thread_entry
* Description  : Network thread entry function.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void net_thread_entry(void)
{
    UINT status;
    ULONG ip_status;
    ULONG dhcp_status;
    ssp_err_t ssp_err;
    UINT addresses_added;
    UCHAR provisionConfigBuffer[300];
    UCHAR network_name[50];
    ULONG actual_size;
    sf_wifi_provisioning_t prov;

    nx_system_initialize();
    status = nx_packet_pool_create(&g_http_packet_pool,
                                   "HTTP Packet Pool",
                                   (BLOCK_SIZE + 32),
                                   mem_packet_pool,
                                   sizeof(mem_packet_pool));
    APP_ERR_TRAP(status)

    tx_semaphore_get(&g_provision_lock, TX_WAIT_FOREVER);
    if (provisioning)
    {
        status = nx_packet_pool_create(&dhcp_packet_pool,
                                       "DHCP Server Pool",
                                       BLOCK_SIZE,
                                       dhcp_buffer_pool_memory,
                                       sizeof(dhcp_buffer_pool_memory));
        APP_ERR_TRAP(status)

        status = nx_ip_create(&g_http_ip, "HTTP IP Instance",
                              APP_SERVER_ADDRESS, APP_SERVER_MASK,
                              &g_http_packet_pool, g_sf_wifi_nsal_nx0,
                              mem_ip_stack, sizeof(mem_ip_stack), 2);
        APP_ERR_TRAP(status)
    }
    else
    {
        status = nx_ip_create(&g_http_ip, "HTTP IP Instance",
                              IP_ADDRESS(0,0,0,0), APP_SERVER_MASK,
                              &g_http_packet_pool, g_sf_wifi_nsal_nx0,
                              mem_ip_stack, sizeof(mem_ip_stack), 2);
        APP_ERR_TRAP(status)
    }

    status = nx_ip_fragment_enable(&g_http_ip);
    APP_ERR_TRAP(status)

    status = nx_arp_enable(&g_http_ip, mem_arp, sizeof(mem_arp));
    APP_ERR_TRAP(status)

    status = nx_tcp_enable(&g_http_ip);
    APP_ERR_TRAP(status)

    status =  nx_udp_enable(&g_http_ip);
    APP_ERR_TRAP(status)

    status = nx_icmp_enable(&g_http_ip);
    APP_ERR_TRAP(status)

    /* Wait for init to finish */
    status = nx_ip_interface_status_check(&g_http_ip, 0, NX_IP_LINK_ENABLED, &ip_status, NX_WAIT_FOREVER);
    APP_ERR_TRAP(status)

    if (provisioning)
    {
        status = nx_dhcp_server_create (&dhcp_server, &g_http_ip, dhcp_server_stack, sizeof(dhcp_server_stack),"DHCP Server", &dhcp_packet_pool);
        APP_ERR_TRAP(status)

        status = nx_dhcp_create_server_ip_address_list (&dhcp_server, 0, START_IP_ADDRESS_LIST, END_IP_ADDRESS_LIST, &addresses_added);
        APP_ERR_TRAP(status)

        status = nx_dhcp_set_interface_network_parameters (&dhcp_server, 0, APP_SERVER_MASK, GW_ADDR, GW_ADDR);
        APP_ERR_TRAP(status)
    }
    else
    {
        status = nx_dhcp_create(&g_dhcp, &g_http_ip, (CHAR *)"Netx DHCP");
        APP_ERR_TRAP(status)

        status = nx_dns_create(&g_dns_client, &g_http_ip, (UCHAR *)"Netx DNS");
        APP_ERR_TRAP(status)
    }

    if (provisioning)
    {
        status = nx_dhcp_server_start(&dhcp_server);
        APP_ERR_TRAP(status)
    }
    else
    {
        ssp_err = g_flash0.p_api->open(g_flash0.p_ctrl, g_flash0.p_cfg);
        APP_ERR_TRAP(ssp_err);

        ssp_err = g_flash0.p_api->read(g_flash0.p_ctrl, provisionConfigBuffer, 0x40100000, 199);
        APP_ERR_TRAP(ssp_err);
        for (int i = 0; i < 199; i++)
        {
            if (0xff != provisionConfigBuffer[i])
            {
                provisioned = true;
                break;
            }
        }

        ssp_err = g_flash0.p_api->close(g_flash0.p_ctrl);
        APP_ERR_TRAP(ssp_err);
        actual_size = strnlen((char*)provisionConfigBuffer, 199);

        if (provisioned)
        {
            char security[20];
            char * temp = NULL, * dst = NULL, * delimiter = NULL;

            provisionConfigBuffer[actual_size] = '\0';
            temp = (char*)provisionConfigBuffer;

            for (int i = 0; i < 7; i++)
            {
                if (!strncmp(temp, "ssid", 4))
                {
                    dst = (char*)prov.ssid;
                }
                else if (!strncmp(temp, "key", 3))
                {
                    dst = (char*)prov.key;
                }
                else if (!strncmp(temp, "apikey", 6))
                {
                    dst = m1_apikey;
                }
                else if (!strncmp(temp, "mqttuserid", 10))
                {
                    dst = m1_mqtt_user_id;
                }
                else if (!strncmp(temp, "mqttprojectid", 13))
                {
                    dst = m1_mqtt_project_id;
                }
                else if (!strncmp(temp, "password", 8))
                {
                    dst = m1_password;
                }
                else if (!strncmp(temp, "sec", 3))
                {
                    dst = security;
                }

                temp = strchr(temp, '=') + 1;
                delimiter = strchr(temp, '&');
                if (NULL == delimiter)
                {
                    delimiter = strchr(temp, '\0');
                }

                memcpy(dst, temp, (size_t)(delimiter - temp));
                dst[delimiter - temp] = '\0';
                temp = delimiter + 1;
            }

            if (!strncmp(security, "open", 4))
            {
                prov.security = SF_WIFI_SECURITY_TYPE_OPEN;
            }
            else if (!strncmp(security, "wep128bit", 9))
            {
                prov.security = SF_WIFI_SECURITY_TYPE_WEP;
            }
            else if (!strncmp(security, "wpa2", 4))
            {
                prov.security = SF_WIFI_SECURITY_TYPE_WPA2;
            }
            else if (!strncmp(security, "wpa", 3))
            {
                prov.security = SF_WIFI_SECURITY_TYPE_WPA;
            }
        }
        else
        {
            /* Dedicated AP and login for factory test */
            strcpy((char *)prov.ssid, "mediumone");
            strcpy((char *)prov.key, "cccccccc");
            strcpy(m1_apikey, "J4ZT2C4SAKLLMFTXO7BN27ZQGQ4DKMBQG4YDIZRQGQ4TAMBQ");
            strcpy(m1_mqtt_user_id, "do_DgeqWSsA");
            strcpy(m1_mqtt_project_id, "tKVrC_AfIYQ");
            strcpy(m1_password, "j9L5tPjF");
            prov.security = SF_WIFI_SECURITY_TYPE_WPA2;
        }
    }

    if (provisioning)
    {
        prov.channel = 1;
        prov.security = SF_WIFI_SECURITY_TYPE_WPA2;
        prov.mode = SF_WIFI_INTERFACE_MODE_AP;

        char supported_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";
        char wifi_ssid[16] = "iot-wifi-";
        char wifi_password[11];

        /* TODO: for more "security", skip a random number of sequence */
        for (size_t j = strlen(wifi_ssid); j < sizeof(wifi_ssid) - 1; j++)
        {
            wifi_ssid[j] = supported_chars[rand() % ((int)sizeof(supported_chars) - 1)];
        }
        wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
        for (uint8_t j = 0; j < sizeof(wifi_password) - 1; j++)
        {
            wifi_password[j] = supported_chars[rand() % ((int)sizeof(supported_chars) - 1)];
        }
        wifi_password[sizeof(wifi_password) - 1] = '\0';
        strcpy((char *)&prov.ssid[0], wifi_ssid);
        strcpy((char *)&prov.key[0], wifi_password);
    }
    else
    {
        prov.mode = SF_WIFI_INTERFACE_MODE_CLIENT;
        BufferLine(0, " Connecting to SSID:");
        BufferLine(1, (char*)prov.ssid);
        PaintText();
    }

    switch (prov.security)
    {
        case SF_WIFI_SECURITY_TYPE_WPA2:
            prov.encryption = SF_WIFI_ENCRYPTION_TYPE_CCMP;
        break;

        case SF_WIFI_SECURITY_TYPE_WPA:
            prov.encryption = SF_WIFI_ENCRYPTION_TYPE_TKIP;
        break;

        default:
            prov.encryption = SF_WIFI_ENCRYPTION_TYPE_AUTO;
        break;
    }

    do {
        ssp_err = g_sf_wifi0.p_api->provisioningSet(g_sf_wifi0.p_ctrl, &prov);
    } while (SSP_SUCCESS != ssp_err);
    APP_ERR_TRAP(ssp_err)

    /* TODO: should this DHCP stuff be before wifi association? */
    if (!provisioning)
    {
        status = nx_dhcp_start(&g_dhcp);
        APP_ERR_TRAP(status)

        /* TODO: if enabled in uSSL, this is supposedly done there */
        status = nx_dns_server_add(&g_dns_client, (8L << 24) + (8L << 16) + (8L << 8) + 8L);
        APP_ERR_TRAP(status)
        BufferLine(0, " Resolving IP address...");
        PaintText();
        nx_ip_status_check(&g_http_ip, NX_IP_ADDRESS_RESOLVED, &dhcp_status, NX_WAIT_FOREVER);

        m1_mqtt_username[0] = '\0';
        strcat(m1_mqtt_username, m1_mqtt_project_id);
        strcat(m1_mqtt_username, "/");
        strcat(m1_mqtt_username, m1_mqtt_user_id);
        m1_mqtt_password[0] = '\0';
        strcat(m1_mqtt_password, m1_apikey);
        strcat(m1_mqtt_password, "/");
        strcat(m1_mqtt_password, m1_password);

        if (!provisioned)
        {
            fmi_product_info_t *p_fmi_product_info;
            g_fmi0.p_api->productInfoGet(&p_fmi_product_info);
            sprintf(m1_device_id, "%x%x%x%x", ((unsigned int *)p_fmi_product_info->unique_id)[0],
                    ((unsigned int *)p_fmi_product_info->unique_id)[1],
                    ((unsigned int *)p_fmi_product_info->unique_id)[2],
                    ((unsigned int *)p_fmi_product_info->unique_id)[3]);
        }

        m1_mqtt_publish_topic[0] = '\0';
        strcat(m1_mqtt_publish_topic, "0/");
        strcat(m1_mqtt_publish_topic, m1_mqtt_project_id);
        strcat(m1_mqtt_publish_topic, "/");
        strcat(m1_mqtt_publish_topic, m1_mqtt_user_id);
        strcat(m1_mqtt_publish_topic, "/");
        strcat(m1_mqtt_publish_topic, m1_device_id);
        m1_mqtt_subscribe_topic[0] = '\0';
        strcat(m1_mqtt_subscribe_topic, "1/");
        strcat(m1_mqtt_subscribe_topic, m1_mqtt_project_id);
        strcat(m1_mqtt_subscribe_topic, "/");
        strcat(m1_mqtt_subscribe_topic, m1_mqtt_user_id);
        strcat(m1_mqtt_subscribe_topic, "/");
        strcat(m1_mqtt_subscribe_topic, m1_device_id);
        strcat(m1_mqtt_subscribe_topic, "/event");
    }

    g_ioport.p_api->pinWrite(leds[2], IOPORT_LEVEL_HIGH);

    if (provisioning)
    {
        /* Format the RAM disk - the memory for the RAM disk was defined above.  */
        status = fx_media_format(&ram_disk_media,
                                 _fx_ram_driver,                  /* Driver entry             */
                                 ram_disk_memory,                 /* RAM disk memory pointer  */
                                 ram_disk_sector_cache,           /* Media buffer pointer     */
                                 sizeof(ram_disk_sector_cache),   /* Media buffer size        */
                                 (CHAR*)"MY_RAM_DISK",            /* Volume Name              */
                                 1,                               /* Number of FATs           */
                                 32,                              /* Directory Entries        */
                                 0,                               /* Hidden sectors           */
                                 sizeof(ram_disk_memory) / sizeof(ram_disk_sector_cache),/* Total sectors            */
                                 sizeof(ram_disk_sector_cache),   /* Sector size              */
                                 1,                               /* Sectors per cluster      */
                                 1,                               /* Heads                    */
                                 1);                              /* Sectors per track        */
        APP_ERR_TRAP(status)

        /* Open the RAM disk.  */
        status = fx_media_open(&ram_disk_media, (CHAR*)"RAM DISK", _fx_ram_driver, ram_disk_memory, ram_disk_sector_cache, sizeof(ram_disk_sector_cache));
        APP_ERR_TRAP(status)
        gp_media = &ram_disk_media;
        FX_FILE my_file;

        status = fx_file_create(gp_media, "index.html");
        APP_ERR_TRAP(status)
        status = fx_file_open(gp_media, &my_file, "index.html", FX_OPEN_FOR_WRITE);
        APP_ERR_TRAP(status)
        status = fx_file_write(&my_file, (char*)provisionHtml, strlen((char *)provisionHtml));
        APP_ERR_TRAP(status)
        status = fx_file_close(&my_file);
        APP_ERR_TRAP(status)

        status = nx_http_server_create(&g_http_server, "HTTP Server Instance", &g_http_ip,
                                       gp_media, mem_http_stack, sizeof(mem_http_stack),
                                       &g_http_packet_pool, NX_NULL, &my_get_notify);
        APP_ERR_TRAP(status)

        status = nx_http_server_start(&g_http_server);
        APP_ERR_TRAP(status)

        sprintf((char *)provisionConfigBuffer, " SSID: %s", prov.ssid);
        BufferLine(0, (char *)provisionConfigBuffer);

        sprintf((char *)provisionConfigBuffer, " Key: %s", prov.key);
        BufferLine(1, (char *)provisionConfigBuffer);
        PaintText();

        tx_thread_suspend(&net_thread);
    }
    else
    {
        char ip_address_string[16];
        unsigned long host_ip_address;

        BufferLine(0, " Hostname resolving:");
        BufferLine(1, "mqtt2.mediumone.com");
        PaintText();

        status = nx_dns_host_by_name_get(&g_dns_client, (UCHAR *)"mqtt2.mediumone.com", &host_ip_address, 500);

        if (NX_SUCCESS != status)
        {
            BufferLine(0, " Failed hostname resolve");
            BufferLine(1, "");
            host_ip_address = (unsigned long)((167L << 24) + (114L << 16) + (77L << 8) + 228L);
        }
        else
        {
            BufferLine(0, " Hostname resolved.");
            BufferLine(1, " Connecting to MQTT...");
        }

        PaintText();

        sprintf(ip_address_string, "%d.%d.%d.%d",
                ((int)host_ip_address >> 24) & 0xff,
                ((int)host_ip_address >> 16) & 0xff,
                ((int)host_ip_address >> 8) & 0xff,
                (int)host_ip_address & 0xff);
    }

        m1_register_subscription_callback(m1_message_callback);

        status = (UINT) m1_connect("mqtt2.mediumone.com", 61620, m1_mqtt_user_id, m1_password,
                                  m1_mqtt_project_id, m1_apikey, m1_device_id, 5, 5, 30, 1);

        if (0 == status)
        {
            sprintf((char *)network_name, "Network: %s", prov.ssid);
            BufferLine(0, "MQTT connected to");
            BufferLine(1, (char *)network_name);
            PaintText();
        }

        if (provisioning)
        {
            status = nx_http_server_stop(&g_http_server);
            APP_ERR_TRAP(status)

            status = nx_http_server_delete(&g_http_server);
            APP_ERR_TRAP(status)
        }

        g_ioport.p_api->pinWrite(leds[2], IOPORT_LEVEL_LOW);

    while (1)
    {
        tx_thread_sleep(100*((ULONG)g_sensor_data.update_rate_sec));
    }
}

/***********************************************************************************************************************
* Function Name: my_get_notify
* Description  : HTTP server request notify routine to response for client request
* Arguments    : server_ptr –
*                    Pointer to HTTP Server control block
*                request_type –
*                    HTTP Server request types
*                resource –
*                    HTTP Server request types
*                packet_ptr -
*                    Pointer to HTTP packet
* Return Value : NX_SUCCESS –
*                    Success to response for client request
*                NX_HTTP_ERROR -
*                    Success to response for client request
***********************************************************************************************************************/
UINT my_get_notify(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr)
{
    ssp_err_t ssp_err;
    UINT status;
    UINT length;
    NX_PACKET *response_pkt;
    UCHAR provisionConfigBuffer[300];

    /* Look for the test resource! */
    if ((NX_HTTP_SERVER_GET_REQUEST == request_type) && (0 == strcmp(resource, "/")))
    {
        strcat(resource, "index.html");
    }
    else if(NX_HTTP_SERVER_POST_REQUEST == request_type)
    {
        /* Process multipart data */
        status = nx_http_server_content_get(server_ptr, packet_ptr, 0, (char*)provisionConfigBuffer, sizeof(provisionConfigBuffer) - 1, &length);
        if (length >= sizeof(provisionConfigBuffer))
        {
            /* TODO: return error to user */
        }

        provisionConfigBuffer[length] = '\0';
        UCHAR *urldecode = (UCHAR *)strchr((char*)provisionConfigBuffer, '%');

        while (NULL != urldecode)
        {
            status = (UINT)sscanf((char *)&urldecode[1], "%2hhx", urldecode);
            if (1 != status)
            {
                /* TODO: return error to user */
            }
            memcpy(&urldecode[1], &urldecode[3], (length - (size_t)(urldecode - provisionConfigBuffer)));
            urldecode = (UCHAR *)strchr((char *)&urldecode[1], '%');
        }

        ssp_err = g_flash0.p_api->open(g_flash0.p_ctrl, g_flash0.p_cfg);
        APP_ERR_TRAP(ssp_err);
        ssp_err = g_flash0.p_api->erase(g_flash0.p_ctrl, 0x40100000, 1);
        APP_ERR_TRAP(ssp_err);
        ssp_err = g_flash0.p_api->write(g_flash0.p_ctrl, (uint32_t)provisionConfigBuffer, 0x40100000, length + 1);
        APP_ERR_TRAP(ssp_err);
        ssp_err = g_flash0.p_api->close(g_flash0.p_ctrl);
        APP_ERR_TRAP(ssp_err);

        PaintScreen((uint8_t *) m1provisionend);

        /* TODO: this response is apparently incomplete */
        /* Generate HTTP header. */
        if (length == sizeof(provisionConfigBuffer))
        {
            status = nx_http_server_callback_generate_response_header(server_ptr, &response_pkt, NX_HTTP_STATUS_OK, 800, "text/html", "Payload too large. Server: NetX HTTP 5.3\r\n");
        }
        else
        {
            status = nx_http_server_callback_generate_response_header(server_ptr, &response_pkt, NX_HTTP_STATUS_OK, 800, "text/html", "Server: NetX HTTP 5.3\r\n");
        }

        if (NX_SUCCESS == status)
        {
            if (NX_SUCCESS != nx_http_server_callback_packet_send(server_ptr, response_pkt))
            {
                nx_packet_release(response_pkt);
            }
        }

        /* Release the received client packet.      */
        nx_packet_release(packet_ptr);

        /* Indicate the response to client is transmitted. */
        return(NX_HTTP_CALLBACK_COMPLETED);
    }

    /* Indicate we have not processed the response to client yet.*/
    return NX_SUCCESS;
}

/***********************************************************************************************************************
* Function Name: m1_message_callback
* Description  : MQTT message callback handler (trigger whenever receive any message)
* Arguments    : type –
*                    Message type
*                topic -
*                    Subscription topic
*                msg –
*                    Message content
*                length -
*                    Message length
* Return Value : None
***********************************************************************************************************************/
void m1_message_callback(int type, char *topic, char *msg, int length)
{
    switch (msg[0])
    {
        case 'R':
            update_rate = (uint8_t)(atoi(&msg[1]));
            if (update_rate <= 0)
                update_rate = 1;

            g_sensor_data.update_rate_sec = update_rate;
            break;

        default:
            break;
    }

    SSP_PARAMETER_NOT_USED(type);
    SSP_PARAMETER_NOT_USED(topic);
    SSP_PARAMETER_NOT_USED(length);
}

/***********************************************************************************************************************
* Function Name: publish_notification
* Description  : Publish notification to Renesas IoT Sandbox
* Arguments    : data
*                    - User data to be published
* Return Value : None
***********************************************************************************************************************/
void publish_notification(sensor_data_t data)
{
#if DO_NOT_PUBLISH_TO_MQTT
#warning "NOT PUBLISHING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    return;
#endif
    char eventbuf[500];

    /* Prepare user data event */
    write_data_to_json_buffer(data, eventbuf);

    /* Upload user data */
    m1_publish_event(eventbuf, NULL);
}

static void write_data_to_json_buffer(sensor_data_t data, char *buffer)
{
    sprintf(buffer, "{\"pcs\":%u}", (unsigned int)data.pcs);
}
