#!/bin/bash

echo "Compilando KernelSim e InterControllerSim..."
gcc -o KernelSim KernelSim.c
gcc -o InterControllerSim InterControllerSim.c

echo "Iniciando testes..."

# Executa o KernelSim e InterControllerSim em paralelo
./KernelSim &
KERNEL_PID=$!
sleep 2  # Aguarda o KernelSim iniciar

./InterControllerSim &
INTERCONTROLLER_PID=$!

# Aguarda o KernelSim e o InterControllerSim concluírem
wait $KERNEL_PID
wait $INTERCONTROLLER_PID

echo "Testes concluídos. Verifique os logs acima para evidenciar a alternância e o envio de interrupções."
