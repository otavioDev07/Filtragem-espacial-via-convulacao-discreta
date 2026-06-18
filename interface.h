#ifndef IMAGEM
#define IMAGEM

#include <pthread.h>
#include <time.h>

#define KERNEL_D 15
#define KERNEL_V 7


typedef struct{
    int altura;
    int largura;
    int maxval;
    int **pixels;
} noImg;

typedef struct {
    float matriz[KERNEL_D][KERNEL_D];
} noKernel;

//struct da thread
typedef struct {
    int id;
    noImg *imgEntrada;
    noImg *imgSaida;
    noKernel *kernel;
    int ini;
    int fim;
} thread;

int **criarMatriz(int altura, int largura);
void liberarMatriz(int **matriz, int altura);

noImg *lerPGM(const char *file);
void salvarPGM(const char *file, noImg *imagem);
void liberarImagem(noImg *imagem);

void *filtragemEspacial(void *arg);

#endif

