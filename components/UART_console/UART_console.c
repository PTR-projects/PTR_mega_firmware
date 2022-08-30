
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "UART_console.h"
#include "esp_system.h"
#include "argtable3/argtable3.h"

static const char* TAG = "Console";
#define PROMPT_STR CONFIG_IDF_TARGET




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


static struct {
    struct arg_str *arg;
    struct arg_end *end;
} do_function_cmd_args;


static int function(const char *arg)
{

	printf("%s \n",arg);
	return 0;
}

static int function_value(int argc, char **argv)
{
	do_function_cmd_args.arg = arg_str1(NULL, NULL, "<key>", "opis zmiennej");
	do_function_cmd_args.end = arg_end(2);
	int nerrors = arg_parse(argc, argv, (void **) &do_function_cmd_args);
	    if (nerrors != 0) {
	        arg_print_errors(stderr, do_function_cmd_args.end, argv[0]);
	        return 1;
	    }
	const char *arg = do_function_cmd_args.arg->sval[0];
	function(arg);
	return 0;
}


static void register_function(void)
{
    const esp_console_cmd_t cmd = {
        .command = "function",
        .help = "Function description",
        .hint = NULL,
        .func = &function_value,
		.argtable = &do_function_cmd_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}



static int do_restart_cmd(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting");
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




void register_tools(void)
{
	esp_console_register_help_command();
    register_restart();
    register_function();
    //inne funkcje

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
