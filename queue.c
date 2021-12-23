/*
Guilherme Ferreira Mota
GRR20197268
P0 - Biblioteca de Filas
*/


#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

int queue_append(queue_t **queue, queue_t *elem) {

    if(queue == NULL){

        #ifdef DEBUG
            printf("queue_append: fila não existe\n");
        #endif

        return -1;
    }

    if(elem == NULL){

        #ifdef DEBUG
            printf("queue_append: elemento não existe\n");
        #endif

        return -2;
    }

    if(elem->prev != NULL || elem->next != NULL){

        #ifdef DEBUG
            printf("queue_append: elemento já existe em outra fila\n");
        #endif

        return -3;
    }

    //Inserção em uma fila vazia
    if(*queue == NULL){
        *queue = elem;
        (*queue)->prev = elem;
        (*queue)->next = elem;
    //Inserção em uma fila com 1 ou mais elementos
    }else{
        (*queue)->prev->next = elem;
        elem->prev = (*queue)->prev;
        elem->next = (*queue);
        (*queue)->prev = elem;
    }
    return 0;
}

int queue_remove(queue_t **queue, queue_t *elem){

    if(queue == NULL){

        #ifdef DEBUG
            printf("queue_remove: fila não existe\n");
        #endif

        return -1;
    }

    if(*queue == NULL){

        #ifdef DEBUG
            printf("queue_remove: fila vazia\n");
        #endif

        return -2;
    }

    if(elem == NULL){

        #ifdef DEBUG
            printf("queue_remove: elemento não existe\n");
        #endif

        return -3;
    }

    //Remoção do primeiro elemento da fila
    if(*queue == elem){
        //Apenas 1 elemento
        if((*queue)->prev == *queue && (*queue)->next == *queue){
            (*queue)->prev = NULL;
            (*queue)->next = NULL;
            (*queue) = NULL;
        //Mais de 1 elemento
        }else{
            //Ajusta os ponteiros da fila
            queue_t *ultimo = (*queue)->prev;
            *queue = (*queue)->next;
            (*queue)->prev = ultimo;
            ultimo->next = (*queue);
        }
    //Remoção de outro elemento da fila
    }else{
        queue_t *aux = (*queue)->next;

        //Percorre a fila procurando elemento
        while(aux != *queue && aux != elem){
            aux = aux->next;
        }

        //Se chegar ao começa da fila novamente
        if(aux == *queue){

            #ifdef DEBUG
                printf("queue_remove: elemento não pertence a fila indicada\n");
            #endif

            return -4;
        //Se achar elemento na fila
        }else{
            //Ajusta os ponteiros da fila
            aux->next->prev = aux->prev;
            aux->prev->next = aux->next;
        }

    }

    //Isola o elemento removido sem destruir
    elem->prev = NULL;
    elem->next = NULL;

    return 0;
} 

void queue_print(char *name, queue_t *queue, void print_elem(void *)) {

    queue_t *aux = queue;
 
    printf("%s  [", name);

    //Se fila estiver vazia
    if(queue == NULL){
        printf("]\n");
        return ;
    }

    //Se fila não estiver vazia
    print_elem(aux);

    aux = queue->next;

    //Percorre a fila imprimindo os elementos
    while(aux != queue){
        printf(" ");
        print_elem(aux);
        aux = aux->next;
    }

    printf("]\n");
  
}

int queue_size(queue_t *queue){
    
    if (queue == NULL){

        #ifdef DEBUG
            printf("queue_size: fila vazia\n");
        #endif

        return 0;
    }else{
        //Se não estiver vazia, deve ter pelo menos tam = 1
        int tam = 1;
        queue_t *aux = queue->next;
        //Percorre a fila incrementando o tamanho
        while(aux != queue){
            tam++;
            aux = aux->next;
        }
        return tam;  
    }
}