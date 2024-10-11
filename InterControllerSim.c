#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main() {
    srand(time(NULL));
    int irq0_count = 0;
    int max_irqs = 5;  // Limita o número de sinais de IRQ0 a 5

    // Aguarda alguns segundos antes de começar a enviar comandos para garantir que o KernelSim esteja pronto
    sleep(5);

    while (1) {
        sleep(5);  // Intervalo de 5 segundos entre comandos

        // Envia IRQ0 (Time slice)
        write(STDOUT_FILENO, "0", 1);  // Usa o descritor herdado do pipe
        printf("InterControllerSim: Enviando IRQ0 (time slice)\n");
        irq0_count++;

        // Se atingiu o limite de IRQs enviados, para o envio
        if (irq0_count >= max_irqs) {
            printf("InterControllerSim: Limite de IRQs atingido, interrompendo envio.\n");
            close(STDOUT_FILENO);  // Fecha o descritor de escrita
            break;
        }

        // Envia IRQ1 (E/S em D1) com 10% de probabilidade
        if (rand() % 10 == 0) {
            write(STDOUT_FILENO, "1", 1);  // Envia IRQ1
            printf("InterControllerSim: Enviando IRQ1 (E/S em D1)\n");
        }

        // Envia IRQ2 (E/S em D2) com 5% de probabilidade
        if (rand() % 20 == 0) {
            write(STDOUT_FILENO, "2", 1);  // Envia IRQ2
            printf("InterControllerSim: Enviando IRQ2 (E/S em D2)\n");
        }
    }

    return 0;
}
