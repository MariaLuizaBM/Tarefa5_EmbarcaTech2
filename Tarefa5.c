#include <stdio.h>          // Biblioteca padrão de entrada e saída
#include "hardware/gpio.h"  // Biblioteca para GPIO
#include "hardware/i2c.h"   // Biblioteca para I2C
#include "hardware/pwm.h"   // Biblioteca para PWM
#include "lib/ssd1306.h"    // Biblioteca para o display OLED
#include "lib/font.h"       // Biblioteca para fonte do display OLED
#include "FreeRTOS.h"       // Biblioteca FreeRTOS
#include "FreeRTOSConfig.h" // Configuração do FreeRTOS
#include "task.h"           // Biblioteca para tarefas do FreeRTOS
#include <stdlib.h>         // Biblioteca para malloc/free
#include "hardware/pio.h"   // Biblioteca para PIO
#include "hardware/adc.h"   // Biblioteca para ADC
#include "queue.h"
#include "Tarefa5.pio.h" // Biblioteca gerada para o PIO

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define led_g 11
#define led_r 13
#define BUZZER_PIN 21
#define VRX_PIN 26
#define VRY_PIN 27
volatile bool modo_led = false;
#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7

typedef struct
{
    uint16_t x_pos;
    uint16_t y_pos;
} joystick_data_t;

QueueHandle_t xQueueModoOperacao;
QueueHandle_t xQueueJoystickData;

// Buffer para armazenar quais LEDs estão ligados matriz 5x5
bool led_buffer[10][NUM_PIXELS] = {
    
    //número 0
    {0, 0, 0, 0, 0, 
     0, 0, 0, 0, 0,                 
     0, 0, 0, 0, 0, 
     0, 0, 0, 0, 0, 
     0, 0, 0, 0, 0},

    //número 1
    {0, 0, 1, 0, 0,
     0, 1, 0, 1, 0,                 
     1, 0, 0, 0, 0, 
     0, 0, 0, 0, 0, 
     0, 0, 0, 0, 0},

    //número 2
    {0, 0, 1, 0, 0, 
     0, 0, 0, 0, 0, 
     0, 0, 1, 0, 0, 
     0, 0, 1, 0, 0, 
     0, 0, 1, 0, 0}
};

// Função para enviar os dados para o PIO
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Função para converter RGB para GRB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Função para exibir o número na matriz de LEDs
void exibir_numero(const bool *buffer, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);

    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(buffer[i] ? color : 0);
    }
}

// Task para a matriz de LEDs
void vMatrizTask(void *params) {
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &Tarefa5_program);
    Tarefa5_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);
    joystick_data_t joydata;
    const char *mensagem = "Modo Normal";
    bool modo;

    while (true) {
        
        int numero = 0;

        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)
        {
            if (xQueueReceive(xQueueModoOperacao, &modo, 0) == pdTRUE) {
            mensagem = modo ? "Modo Alerta" : "Modo Normal";
            }

            if(!modo_led) // Modo Normal
        {
            numero = 1;
            exibir_numero(led_buffer[numero], 0, 25, 0);
        }
        else // Modo Alerta
        {
            numero = 2;
            exibir_numero(led_buffer[numero], 25, 0, 0);
        }
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// Task para o Joystick
void vJoystickTask(void *params)
{
    adc_gpio_init(VRY_PIN); // Lê o valor do eixo X, de 0 a 4095. (nível da água)
    adc_gpio_init(VRX_PIN); // Lê o valor do eixo Y, de 0 a 4095. (volume de chuva)
    adc_init();

    joystick_data_t joydata;

    while (true)
    {

        adc_select_input(0); // GPIO 26 = ADC0
        joydata.y_pos = adc_read();

        adc_select_input(1); // GPIO 27 = ADC1
        joydata.x_pos = adc_read();

        #define NIVEL_AGUA_LIMITE 2866
        #define VOLUME_CHUVA_LIMITE 3276

        if (joydata.x_pos >= NIVEL_AGUA_LIMITE || joydata.y_pos >= VOLUME_CHUVA_LIMITE) {
        modo_led = true;
        } else {
        modo_led = false;
        }

        xQueueSend(xQueueJoystickData, &joydata, 0); // Envia o valor do joystick para a fila

        bool modo = modo_led;

        xQueueSend(xQueueModoOperacao, &modo, 0); // Envia o modo de operação para a fila
        vTaskDelay(pdMS_TO_TICKS(100));              // 10 Hz de leitura
    }
}

// Task para o LED
void vBlinkLedTask(void *params)
{
    // Inicializa os LEDs
    gpio_init(led_g);
    gpio_set_dir(led_g, GPIO_OUT);
    gpio_init(led_r);
    gpio_set_dir(led_r, GPIO_OUT);

    joystick_data_t joydata;
    const char *mensagem = "Modo Normal";
    bool modo;
    
    while (true)
    {
        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)
        {
            if (xQueueReceive(xQueueModoOperacao, &modo, 0) == pdTRUE) {
            mensagem = modo ? "Modo Alerta" : "Modo Normal";
            }

            if (!modo_led) // Modo Normal
        {
            gpio_put(led_g, true);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_put(led_g, false);
        }
        else // Modo Alerta
        {
            gpio_put(led_r, true);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_put(led_r, false);
        }
        }

    }
}

// Função para tocar o buzzer
void tocar_buzzer(uint slice, uint channel, uint freq_hz, uint duracao_ms) {
    uint32_t clock_hz = 125000000;
    uint32_t top = clock_hz / (freq_hz * 4); // Ajuste de clock para buzzer audível
    pwm_set_wrap(slice, top);
    pwm_set_chan_level(slice, channel, top / 2);
    pwm_set_enabled(slice, true);
    vTaskDelay(pdMS_TO_TICKS(duracao_ms));
    pwm_set_enabled(slice, false);
}

// Task para o buzzer
void vBuzzerTask(void *params) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);
    pwm_set_clkdiv(slice, 4.f);
    joystick_data_t joydata;
    const char *mensagem = "Modo Normal";
    bool modo;

    while (true) {
        bool verde = gpio_get(led_g);
        bool vermelho = gpio_get(led_r);

        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)
        {   
            if (xQueueReceive(xQueueModoOperacao, &modo, 0) == pdTRUE) {
            mensagem = modo ? "Modo Alerta" : "Modo Normal";
            }

            if (!modo_led) {  // Modo Normal
            tocar_buzzer(slice, channel, 1000, 200);
            vTaskDelay(pdMS_TO_TICKS(800));
        } else // Modo Alerta
        {
            // Beep rápido intermitente
            tocar_buzzer(slice, channel, 800, 100); 
            vTaskDelay(pdMS_TO_TICKS(100));
        }                           // Atualiza display
        }
    }
}

// Task para o display
void vDisplayTask(void *params)
{
    // Inicialização do I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    joystick_data_t joydata;
    char buffer_vrx[20];
    char buffer_vry[20];
    const char *mensagem = "Modo Normal";
    bool modo;

    while (true)
    {
        sprintf(buffer_vrx, "VRX: %4d", joydata.y_pos); // VRX: nível da água
        sprintf(buffer_vry, "VRY: %4d", joydata.x_pos); // VRY: volume de chuva

        if (xQueueReceive(xQueueModoOperacao, &modo, 0) == pdTRUE) {
            mensagem = modo ? "Modo Alerta" : "Modo Normal";
        }

        if (xQueueReceive(xQueueJoystickData, &joydata, portMAX_DELAY) == pdTRUE)
        {
            // Desenha no display
            ssd1306_fill(&ssd, false);                          // Limpa display
            ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);     // Moldura
            ssd1306_draw_string(&ssd, buffer_vrx, 10, 12);      // Valor de VRX
            ssd1306_draw_string(&ssd, buffer_vry, 10, 24);      // Valor de VRY
            ssd1306_line(&ssd, 5, 35, 122, 35, true);           // Linha divisória
            ssd1306_draw_string(&ssd, mensagem, 20, 45);        // Modo
            ssd1306_send_data(&ssd);                            // Atualiza display
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    // Fim do trecho para modo BOOTSEL com botão B

    stdio_init_all();

    // Cria a fila para compartilhamento de valor do joystick
    xQueueJoystickData = xQueueCreate(5, sizeof(joystick_data_t));
    xQueueModoOperacao = xQueueCreate(1, sizeof(bool));

    xTaskCreate(vMatrizTask, "Task Matriz", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vJoystickTask, "Task Joystick", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBlinkLedTask, "Task Led", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Task Buzzer", configMINIMAL_STACK_SIZE,
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Task Display", configMINIMAL_STACK_SIZE, 
        NULL, tskIDLE_PRIORITY, NULL);

    // Inicia o agendador
    vTaskStartScheduler();
    panic_unsupported();
}