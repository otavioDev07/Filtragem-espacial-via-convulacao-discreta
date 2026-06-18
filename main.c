#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"

int **criarMatriz(int altura, int largura){
    int **matriz = malloc(altura * sizeof(int *)); //aloca as linhas
    if (!matriz) return NULL;

    //alocar as colunas agora
    for (int i = 0; i < altura; i++){
        matriz[i] = malloc(largura * sizeof(int));
        if (!matriz[i]){ //evitar memory leak
            for (int j = 0; j < i; j++) free(matriz[j]);
            free(matriz);
            return NULL;
        }  
    }
    return matriz;
}

void liberarMatriz(int **matriz, int altura){
    if (!matriz) return;
    for (int i = 0; i < altura; i++) free(matriz[i]);
    free(matriz);
}

void liberarImagem(noImg *imagem){
    if (!imagem) return;
    if (imagem->pixels) {
        liberarMatriz(imagem->pixels, imagem->altura);
    }
    free(imagem);   
}

noImg *lerPGM(const char *file){
    if (!file) return NULL;
    FILE *pgm = fopen(file, "r");
    if (!pgm) return NULL;
    noImg *image = malloc(sizeof(noImg));
    if (!image){
        fclose(pgm);
        return NULL;
    }

    //limpeza do cabeçalho'
    //identificar se o arquivo de entrada é um PGM P2
    char formato[3]; 
    fscanf(pgm, "%2s", formato);

    if (strcmp(formato, "P2") != 0){
        printf("O arquivo de entrada não é do tipo P2");
        fclose(pgm);
        free(image);
        return NULL;
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), pgm); //consome o /n após

    do {
        fgets(buffer, sizeof(buffer), pgm);
    } while (buffer[0] == '#');
    
    //le e define a largura e altura
    sscanf(buffer, "%d %d", &image->largura, &image->altura);
    //depois de largura e altura, le e define o maxval (tom de branco puro da imagem)
    fscanf(pgm, "%d", &image->maxval);

    //alocar a matriz na memória
    image->pixels = criarMatriz(image->altura, image->largura);
    if (!image->pixels){
        fclose(pgm);
        free(image);
        return NULL;
    }

    for (int i = 0; i < image->altura; i++) {
        for (int j = 0; j < image->largura; j++)
        {
            fscanf(pgm,"%d", &image->pixels[i][j]);
        }
    }

    //agora a struct está cheia
    fclose(pgm);
    return image;
}

void salvarPGM(const char *file, noImg *imagem){
    if (!file || !imagem || !imagem->pixels) return;
    
    FILE *pgm = fopen(file, "w");
    if (!pgm) {
        printf("Arquivo de saida nao criado: %s\n", file);
        return;
    }

    fprintf(pgm, "P2\n");
    fprintf(pgm, "# Resultado\n");
    fprintf(pgm, "%d %d\n", imagem->largura, imagem->altura);
    fprintf(pgm, "%d\n", imagem->maxval);

    for (int i = 0; i < imagem->altura; i++) {
        for (int j = 0; j < imagem->largura; j++) {
            fprintf(pgm, "%d ", imagem->pixels[i][j]);
        }
        fprintf(pgm, "\n");
    }

    fclose(pgm);
}

void *filtragemEspacial(void *arg){
    thread *t = (thread *)arg;
    noImg *entrada = t->imgEntrada;
    noImg *saida = t->imgSaida;
    noKernel *k = t->kernel;

    for (int x = t->ini; x < t->fim; x++){
        for (int y = 0; y < entrada->largura; y++){
            //tratamento de borda
            if (x < KERNEL_V || x >= entrada->altura - KERNEL_V || y < KERNEL_V || y >= entrada->largura - KERNEL_V){
                saida->pixels[x][y] = 0;
                continue;
            }
            
            //convulação
            float soma = 0.0f;
            for (int i = -KERNEL_V; i <= KERNEL_V; i++){
                for (int j = -KERNEL_V; j <= KERNEL_V; j++){
                    soma += entrada->pixels[x+i][y+j] * k->matriz[i + KERNEL_V][j + KERNEL_V];
                }
            }

            //clipping
            if (soma < 0.0f){
                saida->pixels[x][y] = 0;
            } 
            else if (soma > 255.0f){
                saida->pixels[x][y] = 255;
            } else {
                saida->pixels[x][y] = (int)soma;
            }
        }
    }
    pthread_exit(NULL);   
}


int main(int argc, char *argv[]){
    if (argc != 4 ){
        printf("Argumentos inválidos");
        return 1;
    }

    char *caminhoEntrada = argv[1];
    char *caminhoSaida = argv[2];
    int numThreads = atoi(argv[3]); //faz um casting do numero de threads para int

    if (numThreads < 1){
        printf("Numero de threads deve ser maior ou igual a 1");
        return 1;
    }


    noImg *imgEntrada = lerPGM(caminhoEntrada);
    if (!imgEntrada){
        printf("Imagem nao carregada");
        return 1;
    }

    //alocacao da imagem de saida
    noImg *imgSaida = malloc(sizeof(noImg));
    imgSaida->altura = imgEntrada->altura;
    imgSaida->largura = imgEntrada->largura;
    imgSaida->maxval = imgEntrada->maxval;
    imgSaida->pixels = criarMatriz(imgSaida->altura, imgSaida->largura);

    // preenchiemtno do kernel
    noKernel kernelLaplace;
    for (int i = 0; i < KERNEL_D; i++){
        for (int j = 0; j < KERNEL_D; j++){
            kernelLaplace.matriz[i][j] = (i == KERNEL_V && j ==  KERNEL_V) ? 224.0f : -1.0f; //coloca o 224 no centro e -1 no resto
        }
    }

    pthread_t *idThreads = malloc(numThreads * sizeof(pthread_t));
    thread *parameters = malloc(numThreads * sizeof(thread));

    int faixas = imgEntrada->altura / numThreads;
    int resto = imgEntrada->altura % numThreads;
    int linhaAtual = 0;

    for(int i = 0; i < numThreads; i++){
        parameters[i].id = i;
        parameters[i].imgEntrada = imgEntrada;
        parameters[i].imgSaida = imgSaida;
        parameters[i].kernel = &kernelLaplace;
        parameters[i].ini = linhaAtual;
        parameters[i].fim = linhaAtual + faixas + (i == numThreads - 1 ? resto : 0); // se a thread atual for a ultima, processa o resto junto da faixa. Isso evita segmentation fault
        linhaAtual = parameters[i].fim;
    } 

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < numThreads; i++){
        pthread_create(&idThreads[i], NULL, filtragemEspacial, &parameters[i]);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(idThreads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double tempoTotal = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; //multiplica pro 10^9 para transformar em segundos
    printf("Tempo de execucao: %.6f segundos (Threads: %d)\n", tempoTotal, numThreads);

    salvarPGM(caminhoSaida, imgSaida);

    
    free(idThreads);
    free(parameters);
    liberarImagem(imgEntrada);
    liberarImagem(imgSaida);

    return 0;
    }