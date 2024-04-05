#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "bsp/board.h"
#include "hardware/adc.h"
#include "math.h"
#include "pico/binary_info.h"

/* Choose 'C' for Celsius or 'F' for Fahrenheit. */
#define TEMPERATURE_UNITS 'C'
 
#define WIFI_SSID "Karimex"
#define WIFI_PASSWORD "K@r1m$xsp."
#define MQTT_SERVER "54.146.113.169" //"broker.emqx.io" available at https://www.emqx.com/en/mqtt/public-mqtt5-broker
 #define PUBLISH_STR_NAME "dennys/button"
#define SUBS_STR_NAME "dennys/led"

struct mqtt_connect_client_info_t mqtt_client_info=
{
  "<RA>/pico_w", /*client id*/
  NULL, /* user */
  NULL, /* pass */
  0,  /* keep alive */
  NULL, /* will_topic */
  NULL, /* will_msg */
  0,    /* will_qos */
  0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len); 
static void mqtt_request_cb(void *arg, err_t err); 
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);

float read_onboard_temperature(const char unit);
void ftoa(float n, char* res, int afterpoint);
void ftoa(float n, char* res, int afterpoint);
int intToStr(int x, char str[], int d);

void reverse(char* str, int len) ;
char tempString[6];

int main()
{
  stdio_init_all();

  /* Initialize hardware AD converter, enable onboard temperature sensor and
   *   select its channel*/
  adc_init();
  adc_set_temp_sensor_enabled(true);
  adc_select_input(4);

  printf("Iniciando...\n");

  if (cyw43_arch_init()) 
  {
    printf("falha ao iniciar chip wifi\n");
    return 1;
  }

  cyw43_arch_enable_sta_mode();
  printf("Conectando ao %s\n", WIFI_SSID);

  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
  {
    printf("Falha ao conectar\n");
    return 1;
  }
  printf("Conectado!!\n");
  
  ip_addr_t addr;
  if(!ip4addr_aton(MQTT_SERVER, &addr)){
    printf("ip error\n");
    return 1;
  }

  printf("Conectando ao MQTT\n");

  mqtt_client_t* cliente_mqtt = mqtt_client_new();

  mqtt_set_inpub_callback(cliente_mqtt, &mqtt_incoming_publish_cb, &mqtt_incoming_data_cb, NULL);

  err_t erro = mqtt_client_connect(cliente_mqtt, &addr, 1883, &mqtt_connection_cb, NULL, &mqtt_client_info); 
  
  if (erro != ERR_OK) 
  {
    printf("Erro de conexão\n");
    return 1;
  }

  printf("Conectado ao MQTT!\n");


  while(true) 
  {

    float temperature = read_onboard_temperature(TEMPERATURE_UNITS);
    printf("Onboard temperature = %.02f %c\n", temperature, TEMPERATURE_UNITS);

    //float to string
    ftoa(temperature, tempString, 2);

    if (board_button_read()) {
      mqtt_publish(cliente_mqtt, PUBLISH_STR_NAME, tempString, 5, 0, false, &mqtt_request_cb, NULL);
    }
    
    sleep_ms(300);
  }

}


/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
float read_onboard_temperature(const char unit) {
    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    if (unit == 'C') {
        return tempC;
    } else if (unit == 'F') {
        return tempC * 9 / 5 + 32;
    }

    return -1.0f;
}

// Converts a floating-point/double number to a string. 
void ftoa(float n, char* res, int afterpoint) 
{ 
    // Extract integer part 
    int ipart = (int)n; 
 
    // Extract floating part 
    float fpart = n - (float)ipart; 
 
    // convert integer part to string 
    int i = intToStr(ipart, res, 0); 
 
    // check for display option after point 
    if (afterpoint != 0) { 
        res[i] = '.'; // add dot 
 
        // Get the value of fraction part upto given no. 
        // of points after dot. The third parameter 
        // is needed to handle cases like 233.007 
        fpart = fpart * pow(10, afterpoint); 
 
        intToStr((int)fpart, res + i + 1, afterpoint); 
    } 
}

// Converts a given integer x to string str[]. 
// d is the number of digits required in the output. 
// If d is more than the number of digits in x, 
// then 0s are added at the beginning. 
int intToStr(int x, char str[], int d) 
{ 
    int i = 0; 
    while (x) { 
        str[i++] = (x % 10) + '0'; 
        x = x / 10; 
    } 
 
    // If number of digits required is more, then 
    // add 0s at the beginning 
    while (i < d) 
        str[i++] = '0'; 
 
    reverse(str, i); 
    str[i] = '\0'; 
    return i; 
}

// Reverses a string 'str' of length 'len' 
void reverse(char* str, int len) 
{ 
    int i = 0, j = len - 1, temp; 
    while (i < j) { 
        temp = str[i]; 
        str[i] = str[j]; 
        str[j] = temp; 
        i++; 
        j--; 
    } 
} 


static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    printf("data: %s\n",data);   
    char led[30];
    memset(led, "\0", 30);
    strncpy(led, data, len);

    if (strncmp(led, "on", 2) == 0) {
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    } else if (strncmp(led, "off", 3) == 0) {
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
    }
}
  
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
  printf("topic %s\n", topic);
}
 
static void mqtt_request_cb(void *arg, err_t err) { 
  printf(("MQTT client request cb: err %d\n", (int)err));
}
 
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
  printf(("Conectado ao Brokker MQTT %d\n",  (int)status));
  if (status == MQTT_CONNECT_ACCEPTED) {
    err_t erro = mqtt_sub_unsub(client, SUBS_STR_NAME, 0, &mqtt_request_cb, NULL, 1);

    if (erro == ERR_OK) 
    {
      printf("Inscrito com Sucesso!\n");
    } else {
      printf("Falha ao Inscrever!\n");
    }
  } else {
    printf("Conexão rejeitada!\n");
  }
}
 