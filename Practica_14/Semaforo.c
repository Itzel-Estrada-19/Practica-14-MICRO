#include "pico/stdlib.h"
#include "pico/multicore.h"

// Pines para semáforos vehículos A y B
#define A_RED 0
#define A_YELLOW 1
#define A_GREEN 2
#define B_RED 3
#define B_YELLOW 4
#define B_GREEN 5

// Pines para semáforos peatonales A y B
#define PED_A_RED 8
#define PED_A_GREEN 9
#define PED_B_RED 6
#define PED_B_GREEN 7

// Botones peatonales
#define BTN_PED_A 11
#define BTN_PED_B 10

// Pines de segmentos del display (A-G + DP)
int segment_pins[8] = {16, 17, 18, 19, 20, 21, 22, 23};

// Pines de control de dígitos del display
int digit_pins[2] = {24, 25};

// Dígitos para display de cátodo común
const uint8_t segment_digits[10] = {
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4
    0b01101101,  // 5
    0b01111101,  // 6
    0b00000111,  // 7
    0b01111111,  // 8
    0b01101111   // 9
};

volatile bool request_ped_A = false;
volatile bool request_ped_B = false;

// -------------------------- Inicialización --------------------------

void init_display() {
    for (int i = 0; i < 8; i++) {
        gpio_init(segment_pins[i]);
        gpio_set_dir(segment_pins[i], GPIO_OUT);
    }
    for (int i = 0; i < 2; i++) {
        gpio_init(digit_pins[i]);
        gpio_set_dir(digit_pins[i], GPIO_OUT);
    }
}

void init_gpio() {
    int leds[] = {
        A_RED, A_YELLOW, A_GREEN,
        B_RED, B_YELLOW, B_GREEN,
        PED_A_RED, PED_A_GREEN,
        PED_B_RED, PED_B_GREEN
    };
    for (int i = 0; i < 10; i++) {
        gpio_init(leds[i]);
        gpio_set_dir(leds[i], GPIO_OUT);
        gpio_put(leds[i], 0); // Leds apagados por defecto
    }

    gpio_init(BTN_PED_A);
    gpio_set_dir(BTN_PED_A, GPIO_IN);
    gpio_pull_up(BTN_PED_A);

    gpio_init(BTN_PED_B);
    gpio_set_dir(BTN_PED_B, GPIO_IN);
    gpio_pull_up(BTN_PED_B);

    init_display();

    // Encender luces rojas peatonales por defecto
    gpio_put(PED_A_RED, 1);
    gpio_put(PED_B_RED, 1);
}

// -------------------------- Utilidades --------------------------

void set_vehicle_lights(bool isA, bool red, bool yellow, bool green) {
    gpio_put(isA ? A_RED : B_RED, red);
    gpio_put(isA ? A_YELLOW : B_YELLOW, yellow);
    gpio_put(isA ? A_GREEN : B_GREEN, green);
}

void set_pedestrian_lights(bool isA, bool red, bool green) {
    gpio_put(isA ? PED_A_RED : PED_B_RED, red);
    gpio_put(isA ? PED_A_GREEN : PED_B_GREEN, green);
}

// Lectura de botones SIN inversión
void check_buttons(bool allow_a, bool allow_b) {
    static bool prev_button_a = false;
    static bool prev_button_b = false;

    bool button_a = !gpio_get(BTN_PED_A); // Botón A
    bool button_b = !gpio_get(BTN_PED_B); // Botón B

    if (button_a && !prev_button_a && allow_a) {
        request_ped_A = true;
    }
    if (button_b && !prev_button_b && allow_b) {
        request_ped_B = true;
    }

    prev_button_a = button_a;
    prev_button_b = button_b;
}

// -------------------------- Display --------------------------

void display_number(int number) {
    int tens = number / 10;
    int units = number % 10;

    for (int cycle = 0; cycle < 20; cycle++) {
        // Decenas
        for (int i = 0; i < 8; i++)
            gpio_put(segment_pins[i], (segment_digits[tens] >> i) & 1);
        gpio_put(digit_pins[0], 0);
        gpio_put(digit_pins[1], 1);
        sleep_ms(5);
        gpio_put(digit_pins[0], 1);

        // Unidades
        for (int i = 0; i < 8; i++)
            gpio_put(segment_pins[i], (segment_digits[units] >> i) & 1);
        gpio_put(digit_pins[1], 0);
        gpio_put(digit_pins[0], 1);
        sleep_ms(5);
        gpio_put(digit_pins[1], 1);
    }
}

void clear_display() {
    for (int i = 0; i < 8; i++) gpio_put(segment_pins[i], 0);
    gpio_put(digit_pins[0], 1);
    gpio_put(digit_pins[1], 1);
}

// -------------------------- Peatón con parpadeo rojo --------------------------

void handle_pedestrian_display(bool isA) {
    int red_pin = isA ? PED_A_RED : PED_B_RED;

    gpio_put(red_pin, 0);                // Apagar rojo para iniciar parpadeo
    set_pedestrian_lights(isA, 0, 1);    // Encender verde

    for (int i = 10; i >= 0; i--) {
        gpio_put(red_pin, i % 2);        // Parpadeo rojo
        display_number(i);
        sleep_ms(1000);
    }

    for (int i = 0; i < 3; i++) {
        gpio_put(red_pin, 0);
        clear_display();
        sleep_ms(300);
        gpio_put(red_pin, 1);
        display_number(0);
        sleep_ms(300);
    }

    clear_display();
    set_pedestrian_lights(isA, 1, 0); // Apagar verde, encender rojo fijo
}

// -------------------------- Fase del semáforo --------------------------

void phase(bool direction) {
    if (direction) {
        // Vehículo A en verde
        set_vehicle_lights(false, 1, 0, 0);
        set_vehicle_lights(true, 0, 0, 1);

        if (request_ped_B) {
            request_ped_B = false;
            handle_pedestrian_display(false);
        }

        for (int i = 0; i < 100; i++) {
            check_buttons(false, true);
            sleep_ms(100);
        }

        set_vehicle_lights(true, 0, 1, 0);
        sleep_ms(2000);
        set_vehicle_lights(true, 1, 0, 0);

    } else {
        // Vehículo B en verde
        set_vehicle_lights(true, 1, 0, 0);
        set_vehicle_lights(false, 0, 0, 1);

        if (request_ped_A) {
            request_ped_A = false;
            handle_pedestrian_display(true);
        }

        for (int i = 0; i < 100; i++) {
            check_buttons(true, false);
            sleep_ms(100);
        }

        set_vehicle_lights(false, 0, 1, 0);
        sleep_ms(2000);
        set_vehicle_lights(false, 1, 0, 0);
    }
}

// -------------------------- Core secundario --------------------------

void core1_main() {
    while (true) {
        check_buttons(true, true); // Revisión constante
        sleep_ms(50);
    }
}

// -------------------------- Programa principal --------------------------

int main() {
    stdio_init_all();
    init_gpio();

    multicore_launch_core1(core1_main); // Activar segundo núcleo

    bool direction = true;
    while (true) {
        phase(direction);
        direction = !direction;
    }

    return 0;
}