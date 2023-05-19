#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"
#include "cJSON.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#include "Web_driver.h"
#include "Web_driver_json.h"
#include "Web_driver_cmd.h"
#include "DataManager.h"
#include "Storage_driver.h"

static const char *TAG = "Web_driver";

//#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
//#define WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define WIFI_CHANNEL   1
#define MAX_STA_CONN   1

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file.*/
#define MAX_FILE_SIZE   (5000*1024) // 5000 KB
#define MAX_FILE_SIZE_STR "5000KB"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

#define IS_FILE_EXT(filename, ext) \
		(strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)


Web_driver_status_t status_web;
Web_driver_live_t live_web;


esp_err_t Web_wifi_init 				(void);
esp_err_t Web_http_init 				(const char *base_path);
void Web_http_stop						(httpd_handle_t server);
esp_err_t Web_wifi_stop					(void);


esp_vfs_spiffs_conf_t conf = {
     .base_path = "/www",
     .partition_label = "www",
     .max_files = 5,
     .format_if_mount_failed = false
};

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

/*!
 * @brief Initialize web component by calling init functions for wifi and http server.
 * @return `ESP_OK` if initialized
 * @return `ESP_ERR_NOT_FOUND` if partition is not present
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_init(void){
	esp_err_t ret = ESP_FAIL;

	const char* base_path = "/www";
	ret = esp_vfs_spiffs_register(&conf);

	 if(ret != ESP_OK){
		ESP_LOGE(TAG, "Failed to mount or format WWW filesystem: %s", esp_err_to_name(ret));
	 }

	ret = Web_wifi_init();
	if(ret == ESP_OK){
		ret = Web_http_init(base_path);
	}

    Web_cmd_init(CONFIG_KPPTR_MASTERKEY);
	return ret;
}

/*!
 * @brief Initialize wifi, create soft access point.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_wifi_init(void){
	esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase()); //potencjalnie niebezpieczne
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);


    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    wifi_config_t wifi_config = {
    	.ap =
    	{
    		.ssid = CONFIG_ESP_WIFI_SSID,
    		.ssid_len = strlen(CONFIG_ESP_WIFI_SSID),
    		.channel = WIFI_CHANNEL,
    		.password = CONFIG_ESP_WIFI_PASSWORD,
    		.max_connection = MAX_STA_CONN,
    		.authmode = WIFI_AUTH_WPA_WPA2_PSK
    	},
    };


    if (strlen(CONFIG_ESP_WIFI_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Soft AP initialization finished. SSID: %s password: %s channel: %d", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD, WIFI_CHANNEL);

    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));

    ESP_LOGI(TAG,"IP Address:  %s", ip4addr_ntoa(&ip_info.ip));
    ESP_LOGI(TAG,"Subnet mask: %s", ip4addr_ntoa(&ip_info.netmask));
    ESP_LOGI(TAG,"Gateway:     %s", ip4addr_ntoa(&ip_info.gw));

    return ESP_OK;
}


/*!
 * @brief Set HTTP response content type according to file extension.
 * @return `text/html` .html
 * @return `application/pdf` .pdf
 * @return `image/jpeg` .jpeg
 * @return `image/x-icon` .ico
 */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".js")) {
    	return httpd_resp_set_type(req, "text/javascript");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}


/*!
 * @brief Copies the full path into destination buffer and returns pointer to path (skipping the preceding base path).
 * @param dest
 * String with destination path
 * @param base_path
 * String with base path
 * @param uri
 * HTTP request uri
 * @param destsize
 * Szie of dest string
 */
static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{

    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }



    strcpy(dest, "");
    strlcpy(dest + 0, uri, pathlen + 1);

    ESP_LOGI(TAG, "Website path: %s", dest);

    /* Return pointer to path, skipping the base */
    return dest + 0;
}


static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/www/index.html");
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}


/*!
 * @brief Handler responsible for serving all files to the client.
 * @param req
 * HTTP request
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
static esp_err_t download_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,req->uri, sizeof(filepath));

    if(!filename){
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }


    /* If name has trailing '/', respond with directory contents */
    ESP_LOGI(TAG, "Filename: %s",filename);
    if(stat(filepath, &file_stat) == -1){
        /* If file not present on SPIFFS check if URI
         * corresponds to one of the hardcoded paths */

        ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
        /* Respond with 404 Not Found */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    if(strstr(filename, "meas.bin") != NULL)
    	Storage_blockMeasFile();
    fd = fopen(filepath, "r");

    if(!fd){
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending file: %s (%ld bytes)...", filename, file_stat.st_size);
    set_content_type_from_file(req, filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do{
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
            	fclose(fd);

                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }

        /* Keep looping till the whole file is sent */
    }while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");

    if(strstr(filename, "meas.bin") != NULL)
        	Storage_unblockMeasFile();

    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


/*!
 * @brief Handler responsible for deleting files.
 * @param req
 * HTTP request
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
static esp_err_t delete_post_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    /* Skip leading "/delete" from URI to get filename */
    /* Note sizeof() counts NULL termination hence the -1 */
    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri  + sizeof("/delete") - 1, sizeof(filepath));
    if(!filename){
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if(filename[strlen(filename) - 1] == '/'){
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if(stat(filepath, &file_stat) == -1){
        ESP_LOGE(TAG, "File does not exist : %s", filename);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File does not exist");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Deleting file : %s", filename);
    /* Delete file */
    unlink(filepath);

    fd = fopen(filepath, "w");
    if (!fd) {
    	ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/www/index.html");

    httpd_resp_sendstr(req, "File deleted successfully");
    return ESP_OK;
}


/*!
 * @brief Handler responsible for uploading files.
 * @param req
 * HTTP request
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
static esp_err_t upload_post_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    /* Skip leading "/upload" from URI to get filename */
    /* Note sizeof() counts NULL termination hence the -1 */
    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri + sizeof("/upload") - 1, sizeof(filepath));
    if (!filename) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }
    /*
    if (stat(filepath, &file_stat) == 0) {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        Respond with 400 Bad Request
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }*/

    /* File cannot be larger than a limit */
    if (req->content_len > MAX_FILE_SIZE) {
        ESP_LOGE(TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File size must be less than "
                            MAX_FILE_SIZE_STR "!");
        /* Return failure to close underlying connection else the
         * incoming file content will keep the socket busy */
        return ESP_FAIL;
    }

    fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving file : %s...", filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *buf = ((struct file_server_data *)req->user_ctx)->scratch;
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    while(remaining > 0){
    	ESP_LOGI(TAG, "Remaining size : %d", remaining);

        /* Receive the file part by part into a buffer */
        if((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0){
        	if(received == HTTPD_SOCK_ERR_TIMEOUT){
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if(received && (received != fwrite(buf, 1, received, fd))){
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    /* Close file upon upload completion */
    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/www/index.html");

    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}


/*!
 * @brief Handler responsible for serving json with status data.
 * @param req
 * HTTP request
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t jsonStatus_get_handler(httpd_req_t *req){
	char *string = Web_driver_json_statusCreate(status_web);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, string, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


/*!
 * @brief Handler responsible for serving json with live telemetry data.
 * @param req
 * HTTP request
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t jsonLive_get_handler(httpd_req_t *req){
	char *string = Web_driver_json_liveCreate(live_web);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, string, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


/*!
 * @brief Handler responsible for commands sent through wifi.
 * @param req
 * HTTP request
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t cmd_post_handler(httpd_req_t *req){
	int total_len = req->content_len;
	    int cur_len = 0;
	    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	    int received = 0;
	    if (total_len >= SCRATCH_BUFSIZE) {
	        /* Respond with 500 Internal Server Error */
	        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
	        return ESP_FAIL;
	    }
	    while (cur_len < total_len) {
	        received = httpd_req_recv(req, buf + cur_len, total_len);
	        if (received <= 0) {
	            /* Respond with 500 Internal Server Error */
	            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
	            return ESP_FAIL;
	        }
	        cur_len += received;
	    }
	    buf[total_len] = '\0';

	    Web_cmd_handler(buf);

	    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}



/*!
 * @brief Initialize HTTP server, create soft access point.
 * @param base_path
 * Base path used for data access.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_http_init(const char *base_path){

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	static struct file_server_data *server_data = NULL;
	if(server_data){
		ESP_LOGE(TAG, "File server already started");
	    return ESP_ERR_INVALID_STATE;
	}

	/* Allocate memory for server data */
	server_data = calloc(1, sizeof(struct file_server_data));
	if (!server_data) {
	   ESP_LOGE(TAG, "Failed to allocate memory for server data");
	   return ESP_ERR_NO_MEM;
	}
	strlcpy(server_data->base_path, base_path, sizeof(server_data->base_path));

	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGI(TAG, "Starting HTTP Server");
	if(httpd_start(&server, &config) != ESP_OK){
		ESP_LOGE(TAG, "Failed to start HTTP server!");
		return ESP_FAIL;
	}


	httpd_uri_t index_get = {
			.uri      = "/",
			.method   = HTTP_GET,
			.handler  = index_html_get_handler,
			.user_ctx = server_data
	};
	httpd_register_uri_handler(server, &index_get);

	httpd_uri_t jsonStatus_get = {
		    .uri      = "/status",
		    .method   = HTTP_GET,
		    .handler  = jsonStatus_get_handler,
		    .user_ctx = server_data
	};
	httpd_register_uri_handler(server, &jsonStatus_get);

	httpd_uri_t jsonLive_get = {
			.uri      = "/live",
			.method   = HTTP_GET,
			.handler  = jsonLive_get_handler,
			.user_ctx = server_data
	};
	httpd_register_uri_handler(server, &jsonLive_get);

	httpd_uri_t cmd_send = {
			    .uri      = "/cmd",
			    .method   = HTTP_POST,
			    .handler  = cmd_post_handler,
			    .user_ctx = server_data
	};
	httpd_register_uri_handler(server, &cmd_send);


	httpd_uri_t file_delete = {
			.uri       = "/delete/*",   // Match all URIs of type /delete/path/to/file
		    .method    = HTTP_POST,
		    .handler   = delete_post_handler,
		    .user_ctx  = server_data    // Pass server data as context
		};
	httpd_register_uri_handler(server, &file_delete);

	httpd_uri_t file_upload = {
			.uri       = "/upload/*",   // Match all URIs of type /upload/path/to/file
		    .method    = HTTP_POST,
		    .handler   = upload_post_handler,
		    .user_ctx  = server_data    // Pass server data as context
		};
	httpd_register_uri_handler(server, &file_upload);

	httpd_uri_t file_download = {
			.uri       = "/*",  // Match all URIs of type /path/to/file
	        .method    = HTTP_GET,
	        .handler   = download_get_handler,
	        .user_ctx  = server_data    // Pass server data as context
	};
	httpd_register_uri_handler(server, &file_download);


	ESP_LOGI(TAG, "Started HTTP server successfully");
	return ESP_OK;
}


/*!
 * @brief Turn off HTTP server
 * @param server
 * server which should be turned off
 */
void Web_http_stop(httpd_handle_t server){
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "Server stopped successfully");
    }
    else{
    	ESP_LOGW(TAG, "Server is already stopped");
    }
}


/*!
 * @brief Turn off WIFI soft access point
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_wifi_stop(void){
	esp_err_t ret = ESP_FAIL;

	ret = esp_wifi_stop();

	return ret;
}


/*!
 * @brief Turn off web component by calling deactivation functions for wifi and HTTP server.
 * @return `ESP_OK` if initialized
 * @return `ESP_FAIL` otherwise.
 */
esp_err_t Web_off(void){
	//Web_http_off();
	return ESP_OK;
}


/*!
 * @brief Exchange status data with main program to display it.
 * @param req
 * HTTP request
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
void Web_status_exchange(Web_driver_status_t EX_status){
	status_web = EX_status;
}


/*!
 * @brief Exchange live data with main program to display it.
 * @param req
 * HTTP request
 * @return `ESP_OK` if done
 * @return `ESP_FAIL` otherwise.
 */
void Web_live_exchange(Web_driver_live_t EX_live){
	live_web = EX_live;
}


esp_err_t Web_status_updateAnalog(float vbat, uint8_t ign1_cont, uint8_t ign2_cont, uint8_t ign3_cont, uint8_t ign4_cont){
    status_web.battery_voltage = vbat;
    status_web.igniters[0].continuity = ign1_cont;
	status_web.igniters[1].continuity = ign2_cont;
	status_web.igniters[2].continuity = ign3_cont;
	status_web.igniters[3].continuity = ign4_cont;

    return ESP_OK;
}


esp_err_t Web_status_updateIgniters(uint8_t ign1_fired, uint8_t ign2_fired, uint8_t ign3_fired, uint8_t ign4_fired){
    status_web.igniters[0].fired = ign1_fired;
    status_web.igniters[1].fired = ign2_fired;
    status_web.igniters[2].fired = ign3_fired;
    status_web.igniters[3].fired = ign4_fired;

    return ESP_OK;
}


esp_err_t Web_status_updateSysMgr(uint32_t timestamp_ms, uint8_t state_system, uint8_t state_analog, uint8_t state_lora, uint8_t state_adcs, uint8_t state_storage, uint8_t state_sysmgr, uint8_t state_utils, uint8_t state_web){
    live_web.timestamp 					= timestamp_ms;
    status_web.timestamp_ms 			= timestamp_ms;
    status_web.sysmgr_system_status     = state_system;    //zmiana nazwy z "system"
    status_web.sysmgr_analog_status     = state_analog;
    status_web.sysmgr_lora_status       = state_lora;
    status_web.sysmgr_adcs_status       = state_adcs;        //zmiana nazwy z main; //ADCS = Attitude Determination and Control System
    status_web.sysmgr_storage_status    = state_storage;
    status_web.sysmgr_sysmgr_status     = state_sysmgr;
    status_web.sysmgr_utils_status      = state_utils;
    status_web.sysmgr_web_status        = state_web;

    return ESP_OK;
}


esp_err_t Web_status_updateconfig(uint64_t SWversion, uint64_t serialNumber, float drougeAlt, float mainAlt){        //zakładam wykonywanie tego przy okazji odczyty konfiguracji konfiguracji, czyli na starcie i po zmienie konfiguracji
    status_web.software_version = SWversion;        //zmiana z tablicy charów na uint64 w którym zakodujemy wszystkie informacje
    status_web.serial_number = serialNumber;
    status_web.drouge_alt = drougeAlt;
    status_web.main_alt = mainAlt;

    return ESP_OK;
}


esp_err_t Web_status_updateGNSS(float lat, float lon, uint8_t fix, uint8_t sats){
    live_web.gps.latitude  = lat;        // pozmieniane lekko nazwy i dodane pole "sats"
    live_web.gps.longitude = lon;
    live_web.gps.fix  = fix;
    live_web.gps.sats = sats;

    return ESP_OK;
}


esp_err_t Web_status_updateADCS(uint8_t flightstate, float rocket_tilt){        //ADCS = Attitude Determination and Control System
    status_web.flight_state = flightstate;    //nowe
    status_web.rocket_tilt = rocket_tilt;    //zmian nazwy z angle

    return ESP_OK;
}


esp_err_t Web_live_from_DataPackage(DataPackage_t * DataPackage_ptr){
    Web_driver_live_t     live_web;

    live_web.LIS331.ax = DataPackage_ptr->sensors.accHX;
    live_web.LIS331.ay = DataPackage_ptr->sensors.accHY;
    live_web.LIS331.az = DataPackage_ptr->sensors.accHZ;
    live_web.LSM6DS32_0.ax = DataPackage_ptr->sensors.accX;
    live_web.LSM6DS32_0.ay = DataPackage_ptr->sensors.accY;
    live_web.LSM6DS32_0.az = DataPackage_ptr->sensors.accZ;
    live_web.LSM6DS32_0.gx = DataPackage_ptr->sensors.gyroX;
    live_web.LSM6DS32_0.gy = DataPackage_ptr->sensors.gyroY;
    live_web.LSM6DS32_0.gz = DataPackage_ptr->sensors.gyroZ;
    live_web.LSM6DS32_0.temperature = DataPackage_ptr->sensors.temp;
    live_web.LSM6DS32_1.ax = 10.0f;
    live_web.LSM6DS32_1.ay = 0.0f;
    live_web.LSM6DS32_1.az = 0.0f;
    live_web.LSM6DS32_1.gx = 0.0f;
    live_web.LSM6DS32_1.gy = 0.0f;
    live_web.LSM6DS32_1.gz = 0.0f;
    live_web.LSM6DS32_1.temperature = 0.0f;
    live_web.MMC5983MA.mx = DataPackage_ptr->sensors.magX;
    live_web.MMC5983MA.my = DataPackage_ptr->sensors.magY;
    live_web.MMC5983MA.mz = DataPackage_ptr->sensors.magZ;
    live_web.MS5607.altitude = 0.0f;
    live_web.MS5607.pressure = DataPackage_ptr->sensors.pressure;
    live_web.MS5607.temperature = DataPackage_ptr->sensors.temp;
    live_web.anglex = 0.0f;
    live_web.angley = 0.0f;
    live_web.anglez = 0.0f;
    live_web.gps.fix = DataPackage_ptr->sensors.gnss_fix;
    live_web.gps.latitude = DataPackage_ptr->sensors.latitude;
    live_web.gps.longitude = DataPackage_ptr->sensors.longitude;
    live_web.gps.sats = DataPackage_ptr->sensors.gnss_fix;
    Web_live_exchange(live_web);
    return ESP_OK;
}

