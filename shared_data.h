#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <windows.h>

#define SHARED_MEMORY_NAME "WormsSharedMemory"
#define TAM_CAMPO 20
#define ALCANCE_MAXIMO 5

typedef struct {
    volatile int fim_jogo;
    volatile int vez;
    
    int p1; // Posicao Jogador 1
    int p2; // Posicao Jogador 2
    int hp1; // HP Jogador 1
    int hp2; // HP Jogador 2
    int ataques1; // Ataques restantes Jogador 1
    int ataques2; // Ataques restantes Jogador 2
    int curas1; // Curas restantes Jogador 1
    int curas2; // Curas restantes Jogador 2
    
    char nome1[50];
    char nome2[50];

    // Sincronizacao
    HANDLE hMutex;
    
} SharedData;

#endif // SHARED_DATA_H
