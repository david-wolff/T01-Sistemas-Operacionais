#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_APPS 3

pid_t app_pids[NUM_APPS];
int app_completed[NUM_APPS] = {0};  // Flag para indicar se cada aplicação terminou
int current_app = -1;
int pipe_fd[2];  // Descritores do pipe

// Função para selecionar a próxima aplicação ativa
int get_next_active_app() {
    int next_app = (current_app + 1) % NUM_APPS;
    for (int i = 0; i < NUM_APPS; i++) {
        if (app_completed[next_app] == 0) {
            return next_app;  // Retorna o próximo processo ativo
        }
        next_app = (next_app + 1) % NUM_APPS;
    }
    return -1;  // Todos os processos terminaram
}

void handle_command(char command) {
    switch (command) {
        case '0':
            printf("KernelSim: Recebido comando IRQ0 (time slice)\n");

            if (current_app != -1 && app_completed[current_app] == 0) {
                kill(app_pids[current_app], SIGSTOP);  // Interrompe a aplicação atual
                printf("KernelSim: Interrompendo aplicação A%d\n", current_app + 1);
            }

            // Seleciona a próxima aplicação ativa
            int next_app = get_next_active_app();
            if (next_app != current_app) {  // Garante que não é a mesma aplicação sendo reiniciada
                current_app = next_app;
                if (current_app != -1) {
                    kill(app_pids[current_app], SIGCONT);
                    printf("KernelSim: Iniciando aplicação A%d\n", current_app + 1);  // Mensagem de início apenas uma vez
                }
            }

            // Verifica se todas as aplicações terminaram
            if (current_app == -1) {
                printf("KernelSim: Todas as aplicações terminaram. Encerrando KernelSim.\n");
                close(pipe_fd[0]);  // Fecha o descritor de leitura antes de encerrar
                exit(0);
            }
            break;
        case '1':
            printf("KernelSim: Recebido comando IRQ1 (E/S em D1)\n");
            break;
        case '2':
            printf("KernelSim: Recebido comando IRQ2 (E/S em D2)\n");
            break;
        default:
            printf("KernelSim: Comando desconhecido: %c\n", command);
            break;
    }
}

// Handler para verificar se o processo filho terminou
void handle_child_exit(int sig) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);  // Checa se algum processo filho terminou

    if (pid > 0) {
        for (int i = 0; i < NUM_APPS; i++) {
            if (app_pids[i] == pid) {
                app_completed[i] = 1;  // Marca a aplicação como concluída
                printf("KernelSim: Aplicação A%d terminou.\n", i + 1);
            }
        }
    }

    if (get_next_active_app() == -1) {
        printf("KernelSim: Todas as aplicações terminaram. Encerrando KernelSim.\n");
        close(pipe_fd[0]);  // Fecha o descritor de leitura
        exit(0);  // Encerra o KernelSim
    }
}

int main() {
    // Cria o pipe
    if (pipe(pipe_fd) == -1) {
        perror("Falha ao criar o pipe");
        exit(EXIT_FAILURE);
    }

    // Fork para criar o processo do InterControllerSim
    pid_t intercontroller_pid = fork();
    if (intercontroller_pid == 0) {
        // Processo filho (InterControllerSim)
        close(pipe_fd[0]);  // Fecha o descritor de leitura no InterControllerSim
        execl("./InterControllerSim", "InterControllerSim", NULL);  // Executa o InterControllerSim
        perror("Falha ao iniciar o InterControllerSim");
        exit(EXIT_FAILURE);
    }

    close(pipe_fd[1]);  // Fecha o descritor de escrita no KernelSim

    // Configura handlers de sinais
    signal(SIGCHLD, handle_child_exit);  // Handler para verificar se o processo filho terminou

    // Cria processos de aplicação
    for (int i = 0; i < NUM_APPS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Processo de aplicação
            char app_num[2];
            sprintf(app_num, "%d", i + 1);
            execl("./app", "app", app_num, NULL);
            exit(0);
        } else {
            app_pids[i] = pid;
        }
    }

    // Inicia a primeira aplicação
    current_app = get_next_active_app();
    if (current_app != -1) {
        kill(app_pids[current_app], SIGCONT);
        printf("KernelSim: Iniciando aplicação A%d\n", current_app + 1);
    }

    // KernelSim fica aguardando comandos do InterControllerSim
    char command;
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], &command, 1)) > 0) {  // Lê comandos do pipe
        handle_command(command);  // Processa o comando
    }

    // Verifica se o InterControllerSim encerrou e o pipe foi fechado
    if (bytes_read == 0) {
        printf("KernelSim: O pipe foi fechado, encerrando KernelSim.\n");
        close(pipe_fd[0]);  // Fecha o descritor de leitura
        exit(0);
    } else if (bytes_read < 0) {
        perror("Erro ao ler do pipe");
        close(pipe_fd[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
