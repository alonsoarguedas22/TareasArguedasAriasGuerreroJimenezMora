#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <gpiod.h>
#include <mutex>

#define GPIO_CHIP "/dev/gpiochip0"
#define PIN_BOTON 17   // GPIO 17
#define PIN_LED   27   // GPIO 27

std::mutex candado; // mutex para sincronizar salida

// --- Función que imprime números del 0 al 30 ---
void imprimirNumeros() {
    for(int numero = 0; numero <= 30; numero++) {
        candado.lock();
        std::cout << "Número: " << numero << std::endl;
        candado.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// --- Función que imprime 30 letras aleatorias ---
void imprimirLetrasAleatorias() {
    const char alfabeto[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for(int i = 0; i < 30; i++) {
        char letra = alfabeto[std::rand() % 26];
        candado.lock();
        std::cout << "Letra: " << letra << std::endl;
        candado.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// --- Función para ejecutar tareas en 1 hilo (secuencial) ---
void ejecutarSecuencial() {
    auto inicio = std::chrono::high_resolution_clock::now();

    imprimirNumeros();
    imprimirLetrasAleatorias();

    auto fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio).count();
    std::cout << "Tiempo total (1 hilo secuencial): " << duracion << " ms" << std::endl;
}

// --- Función para ejecutar tareas en 2 hilos (paralelo) ---
void ejecutarParalelo() {
    auto inicio = std::chrono::high_resolution_clock::now();

    std::thread hiloNumeros(imprimirNumeros);
    std::thread hiloLetras(imprimirLetrasAleatorias);

    hiloNumeros.join();
    hiloLetras.join();

    auto fin = std::chrono::high_resolution_clock::now();
    auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio).count();
    std::cout << "Tiempo total (2 hilos paralelos): " << duracion << " ms" << std::endl;
}

int main() {
    std::srand(std::time(nullptr));

    // Abrir el chip GPIO
    gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        std::cerr << "No se pudo abrir el chip GPIO" << std::endl;
        return 1;
    }

    // Configurar línea del botón
    gpiod_line *line_boton = gpiod_chip_get_line(chip, PIN_BOTON);
    if (!line_boton) {
        std::cerr << "No se pudo obtener la línea del botón" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_input(line_boton, "programa")) {
        std::cerr << "No se pudo configurar el botón como entrada" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    // Configurar línea del LED
    gpiod_line *line_led = gpiod_chip_get_line(chip, PIN_LED);
    if (!line_led) {
        std::cerr << "No se pudo obtener la línea del LED" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_output(line_led, "programa", 0)) {
        std::cerr << "No se pudo configurar el LED como salida" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    std::cout << "Esperando señal de inicio (presiona el switch)..." << std::endl;

    // Espera hasta que el switch se presione
    while (gpiod_line_get_value(line_boton) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Switch presionado. Iniciando tareas...\n" << std::endl;

    // Ejecutar primero secuencial y luego paralelo
    ejecutarSecuencial();
    ejecutarParalelo();

    // Encender LED al finalizar
    gpiod_line_set_value(line_led, 1);
    std::cout << "\n[LED encendido] Las tareas finalizaron correctamente." << std::endl;

    // Liberar recursos
    gpiod_line_release(line_boton);
    gpiod_line_release(line_led);
    gpiod_chip_close(chip);

    return 0;
}
