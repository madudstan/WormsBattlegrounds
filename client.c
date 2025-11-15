#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <math.h>
#include "shared_data.h"

#pragma comment(lib,"ws2_32.lib")

#define PORT 12345

SharedData *shared_data = NULL;
HANDLE hMapFile = NULL;

SOCKET sock = INVALID_SOCKET;

void mostrar_regras(){
    printf("\n================================== REGRAS GERAIS ==================================\n");
    printf("JOGADOR 1: Seu turno eh o '1'. JOGADOR 2: Seu turno eh o '2'.\n");
    printf("ALCANCE DE ATAQUE: Maximo 5 casas de distancia.\n");
    printf("Poder de Cura: +20 HP (Max: 100). Quantidade inicial de Curas: 2\n");
    printf("=================================== COMANDOS ======================================\n");
    printf("MOVER L| MOVER R| CURAR\n");
    printf("ATACAR BAZUCA (Dano: 15) | ATACAR ESPINGARDA (Dano: 25) | ATACAR GRANADA (Dano: 40)\n");
    printf("=================================================================================\n\n");
}

void mostrar_vida(int hp) {
    int total=20, preenchido, i;
    preenchido=(hp>0?(hp*total)/100:0);
    putchar('[');
    for(i=0;i<preenchido;i++) putchar('#');
    for(i=preenchido;i<total;i++) putchar('-');
    putchar(']');
}

void mostrar_campo() {
    int i;
    
    system("cls");
    mostrar_regras();
    printf("\n================ CAMPO ==================\n");
    for(i=0;i<TAM_CAMPO;i++){
        if(i==shared_data->p1) putchar('1');
        else if(i==shared_data->p2) putchar('2');
        else putchar('-');
    }
    printf("\n");
    printf("%s (Voce): ",shared_data->nome2); mostrar_vida(shared_data->hp2); printf(" %d/100\n",shared_data->hp2);
    printf("%s (Op): ",shared_data->nome1); mostrar_vida(shared_data->hp1); printf(" %d/100\n",shared_data->hp1);
    printf("Ataques: %d | %d, Curas: %d | %d\n",shared_data->ataques1,shared_data->ataques2,shared_data->curas1,shared_data->curas2);
    printf("Vez do jogador %d\n",shared_data->vez);
    printf("========================================\n");
}

int processar_ataque(const char *buf, int atacante_pos, int defensor_pos, int *ataques_ptr, int *hp_defensor_ptr){
    int dano=0;
    if(strstr(buf,"BAZUCA")) dano=15;
    else if(strstr(buf,"ESPINGARDA")) dano=25;
    else if(strstr(buf,"GRANADA")) dano=40;
    else return 0;

    if(*ataques_ptr>0){
        if(abs(atacante_pos-defensor_pos)<=ALCANCE_MAXIMO){
            *hp_defensor_ptr-=dano;
            if(*hp_defensor_ptr<0) *hp_defensor_ptr=0;
            printf("Ataque acertou! Dano: %d\n",dano);
        }else printf("Ataque fora de alcance!\n");
        (*ataques_ptr)--;
        return 1;
    }
    printf("Sem ataques restantes.\n");
    return 0;
}

void menu_jogador2(char *buf){
    int escolha;
    while(1){
        mostrar_campo();
        printf("\n%s (VOCE), escolha uma acao:\n",shared_data->nome2);
        printf("1. MOVER Esquerda (L)\n2. MOVER Direita (R)\n3. CURAR (+20 HP) - Restantes: %d\n",shared_data->curas2);
        printf("4. ATACAR: BAZUCA (15)\n5. ATACAR: ESPINGARDA (25)\n6. ATACAR: GRANADA (40)\n");
        printf("----------------------------------------\nEscolha: ");
        fflush(stdout);
        if(scanf("%d",&escolha)!=1){ int c; while((c=getchar())!='\n'&&c!=EOF); continue; }
        int c; while((c=getchar())!='\n'&&c!=EOF);
        
        if(escolha==1 && shared_data->p2>0){ strcpy(buf,"MOVER L"); return; }
        if(escolha==2 && shared_data->p2<TAM_CAMPO-1){ strcpy(buf,"MOVER R"); return; }
        if(escolha==3 && shared_data->curas2>0){ strcpy(buf,"CURAR"); return; }
        if(escolha==4 && shared_data->ataques2>0){ strcpy(buf,"ATACAR BAZUCA"); return; }
        if(escolha==5 && shared_data->ataques2>0){ strcpy(buf,"ATACAR ESPINGARDA"); return; }
        if(escolha==6 && shared_data->ataques2>0){ strcpy(buf,"ATACAR GRANADA"); return; }
    }
}

int main(){
    WSADATA wsaData;
    struct sockaddr_in server_addr;
    char buf[128];
    int r;
    int last_vez = 0; 

    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,  
        FALSE,                 
        SHARED_MEMORY_NAME);   

    if (hMapFile == NULL) {
        printf("Could not open file mapping object (%d).\n", GetLastError());
        return 1;
    }

    shared_data = (SharedData *)MapViewOfFile(
        hMapFile,                
        FILE_MAP_ALL_ACCESS,     
        0,
        0,
        sizeof(SharedData));

    if (shared_data == NULL) {
        printf("Could not map view of file (%d).\n", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0) return 1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");

    if(connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        printf("Erro ao conectar ao servidor.\n"); 
        UnmapViewOfFile(shared_data);
        CloseHandle(hMapFile);
        return 1;
    }

    printf("Escolha seu nome: ");
    fgets(shared_data->nome2,50,stdin);
    shared_data->nome2[strcspn(shared_data->nome2,"\r\n")]=0;
    sprintf(buf,"NOME %s",shared_data->nome2);
    send(sock,buf,(int)strlen(buf),0);

    r=recv(sock,buf,sizeof(buf)-1,0);
    if(r>0){ buf[r]=0; sscanf(buf,"NOMES %s %s",shared_data->nome1,shared_data->nome2); }
    
    mostrar_campo();
    printf("Aguardando jogada de %s...\n",shared_data->nome1);


    while(!shared_data->fim_jogo){
        
        WaitForSingleObject(shared_data->hMutex, INFINITE);
        
        if(shared_data->vez==2){
            
            ReleaseMutex(shared_data->hMutex); 
            menu_jogador2(buf);
            WaitForSingleObject(shared_data->hMutex, INFINITE); 
            
            if (strcmp(buf,"CURAR")==0 && shared_data->curas2>0) { 
                shared_data->hp2+=20; 
                if(shared_data->hp2>100) shared_data->hp2=100; 
                shared_data->curas2--; 
                shared_data->vez=1; 
            }
            else if (strcmp(buf,"MOVER L")==0 && shared_data->p2>0) { 
                shared_data->p2--; 
                shared_data->vez=1; 
            }
            else if (strcmp(buf,"MOVER R")==0 && shared_data->p2<TAM_CAMPO-1) { 
                shared_data->p2++; 
                shared_data->vez=1; 
            }
            else if (strncmp(buf,"ATACAR",6)==0) {
                if (processar_ataque(buf, shared_data->p2, shared_data->p1, &shared_data->ataques2, &shared_data->hp1)) {
                    shared_data->vez=1;
                }
            }
            
            if (shared_data->hp1<=0) { 
                send(sock,"FIM P2",6,0); 
                shared_data->fim_jogo=1; 
            }
            
        }else{
            if (last_vez != shared_data->vez) {
                mostrar_campo();
                printf("Aguardando jogada de %s...\n",shared_data->nome1);
                last_vez = shared_data->vez;
            }
            
            unsigned long non_blocking = 1;
            ioctlsocket(sock, FIONBIO, &non_blocking);
            
            r = recv(sock, buf, sizeof(buf)-1, 0);
            
            unsigned long blocking = 0;
            ioctlsocket(sock, FIONBIO, &blocking);
            
            if (r > 0) {
                buf[r] = 0;
                if (strncmp(buf,"FIM",3)==0) {
                    shared_data->fim_jogo = 1;
                }
            }
        }
        
        ReleaseMutex(shared_data->hMutex);

        if (shared_data->vez == 1 && !shared_data->fim_jogo) {
            Sleep(100);
        }
    }

    WaitForSingleObject(shared_data->hMutex, INFINITE);
    mostrar_campo();
    if (shared_data->hp1 <= 0) {
        printf("\n========================================\n");
        printf("O JOGADOR 2 (%s) VENCEU!\n", shared_data->nome2);
        printf("========================================\n");
    } else if (shared_data->hp2 <= 0) {
        printf("\n========================================\n");
        printf("O JOGADOR 1 (%s) VENCEU!\n", shared_data->nome1);
        printf("========================================\n");
    }
    ReleaseMutex(shared_data->hMutex);

    UnmapViewOfFile(shared_data);
    CloseHandle(hMapFile);
    closesocket(sock);
    WSACleanup();
    
    return 0;
}
