
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "UART_console.h"
#include "argtable3/argtable3.h"

static const char* TAG = "Console";
#define PROMPT_STR CONFIG_IDF_TARGET


static void register_version(void);
static void register_restart(void);

static void register_maintenance(void);

static void register_storage_erase(void);
static void register_storage_list(void);
static void register_storage_readfile(void);

/*
do_flash_erase_cmd
do_flash_dump_cmd
do_flash_select_cmd XXX <<- wybór pliku
do_flash_read_cmd
do_flash_list_cmd
do_ign_set_cmd X <- odpalenie zapalnika
do_ign_setall_cmd <- odpal wszystkie
do_ign_get_cmd
do_status_cmd
*/

//-------------------------------------------------------------------------------------------------------------
//		System
//-------------------------------------------------------------------------------------------------------------

//-------------------------------- Version ----------------------------------
static int get_version(int argc, char **argv)
{
    esp_chip_info_t info;
    esp_chip_info(&info);
    printf("IDF Version:%s\r\n", esp_get_idf_version());
    printf("Chip info:\r\n");
    printf("\tmodel:%s\r\n", info.model == CHIP_ESP32 ? "ESP32" : "Unknown");
    printf("\tcores:%d\r\n", info.cores);
    printf("\tfeature:%s%s%s%s%d%s\r\n",
           info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
           info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
           info.features & CHIP_FEATURE_BT ? "/BT" : "",
           info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
           0/*spi_flash_get_chip_size() / (1024 * 1024)*/, " MB");
    printf("\trevision number:%d\r\n", info.revision);
    return 0;
}

static void register_version(void)
{
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Get version of chip and SDK",
        .hint = NULL,
        .func = &get_version,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//-------------------- Restart -------------------------------
static int do_restart_cmd(int argc, char **argv)
{
	printf("Restarting");
    esp_restart();
}

static void register_restart(void)
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Software reset of the chip",
        .hint = NULL,
        .func = &do_restart_cmd,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//-------------------------------------------------------------------------------------------------------------
//		Software control
//-------------------------------------------------------------------------------------------------------------
//-------------------- Maintenance mode -------------------------------
static int do_maintenance_cmd(int argc, char **argv)
{
	//switch to maintenance, terminate logging to filesystem, unlock filesystem access
    printf("Successfully switch to maintenance mode!");

    return 0;
}

static void register_maintenance(void)
{
    const esp_console_cmd_t cmd = {
        .command = "maintenance",
        .help = "Switch from flightmode to maintenance mode to unlock advanced command",
        .hint = NULL,
        .func = &do_maintenance_cmd,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}


//-------------------------------------------------------------------------------------------------------------
//		Storage
//-------------------------------------------------------------------------------------------------------------

//---------------------- Storage erase --------------------------
static struct {
	struct arg_int *value;
    struct arg_end *end;
} storage_erase_function_cmd_args;

static int do_storage_erase_request(int argc, char **argv)
{
	int nerrors = arg_parse(argc, argv, (void **) &storage_erase_function_cmd_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, storage_erase_function_cmd_args.end, argv[0]);
		return 1;
	}

	// check if filesystem acces unlocked
	if(0){
		printf("\n");
		printf("Filesystem acces locked! Switch to maintenance mode to unlock this command...\n");
		return 0;
	}

	if(storage_erase_function_cmd_args.value->ival[0] == 12345){
		printf("\n");
		printf("Erasing...\n");
		//Storage_erase();
		printf("Storage erased...\n");
	} else {
		printf("\n");
		printf("Wrong magic key!\n");
	}

	return 0;
}

static void register_storage_erase(void)
{
	storage_erase_function_cmd_args.value   = arg_int1(NULL, NULL, "magickey", "Security key to unlock filesystem erase (12345)");
	storage_erase_function_cmd_args.end 	= arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "storage_erase",
        .help = "Erases filesystem and all files.",
        .hint = NULL,
        .func = &do_storage_erase_request,
		.argtable = &storage_erase_function_cmd_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//---------------------- Storage list --------------------------
static int do_storage_list_request(int argc, char **argv)
{
	// check if filesystem acces unlocked
	if(0){
		printf("\n");
		printf("Filesystem acces locked! Switch to maintenance mode to unlock this command...\n");
		return 0;
	}

	printf("Files list\n");
	//Storage_listFiles

	return 0;
}

static void register_storage_list(void)
{
    const esp_console_cmd_t cmd = {
        .command = "storage_list",
        .help = "Storage list all files",
        .hint = NULL,
        .func = &do_storage_list_request
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//---------------------- Storage read file --------------------------
static struct {
	struct arg_int *value;
    struct arg_end *end;
} storage_readfile_function_cmd_args;

static int do_storage_readfile_request(int argc, char **argv)
{
	int nerrors = arg_parse(argc, argv, (void **) &storage_readfile_function_cmd_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, storage_readfile_function_cmd_args.end, argv[0]);
		return 1;
	}

	// check if filesystem acces unlocked
	if(0){
		printf("\n");
		printf("Filesystem acces locked! Switch to maintenance mode to unlock this command...\n");
		return 0;
	}

	if((storage_readfile_function_cmd_args.value->ival[0] < 0) ||
			storage_readfile_function_cmd_args.value->ival[0] > 99){
		printf("\nFile number outside limits!\n");
		return 0;
	}
	//Check if filenum is present in the filesystem

	//Storage_readfile(storage_readfile_function_cmd_args.value->ival[0]);
	printf("\nReading file %i...\n", storage_readfile_function_cmd_args.value->ival[0]);

	return 0;
}

static void register_storage_readfile(void)
{
	storage_readfile_function_cmd_args.value = arg_int1(NULL, NULL, "filenum", "Selected file number (0-99)");
	storage_readfile_function_cmd_args.end 	 = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "storage_readfile",
        .help = "Read file from filesystem. Example \"storage_readfile 5\" will read file no 5.",
        .hint = NULL,
        .func = &do_storage_readfile_request,
		.argtable = &storage_readfile_function_cmd_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void register_tools(void)
{
	esp_console_register_help_command();
	register_version();
	register_restart();

	register_maintenance();

	register_storage_erase();
	register_storage_list();
	register_storage_readfile();

}


void UART_console_init(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    /* Register commands */

    register_tools();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

#else
#error Unsupported console type
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
