/*
Guilherme Ferreira Mota
GRR20197268
P12 - Fila de mensagens
*/

#include <stdlib.h>
#include <stdio.h>

#include "ppos.h"

#define TAM 5

typedef struct filaint_t
{
   struct filaint_t *prev ;
   struct filaint_t *next ;
   int valor ;

} filaint_t ;

filaint_t *buffer;

filaint_t *item;

semaphore_t  s_buffer, s_item, s_vaga;	

//corpo do produtor
void produtor(void * arg){
   
   while (1)
   {
      task_sleep (1000);
      item = (filaint_t *)malloc(sizeof(filaint_t));
      item->valor = rand() % 100;
      item->next = NULL;
      item->prev = NULL;
      
      sem_down(&s_vaga);
      sem_down(&s_buffer);
      //insere item no buffer
      queue_append((queue_t **) &buffer, (queue_t *) item);
      printf ("%s produziu %d\n\n", (char *) arg, item->valor);
      sem_up(&s_buffer);
      sem_up(&s_item);
   }
}

//corpo do consumidor
void consumidor(void * arg){
   while (1)
   {
   	sem_down(&s_item);
      sem_down(&s_buffer);
      item = buffer;
      //remove primeiro item do buffer
      queue_remove((queue_t **) &buffer, (queue_t *) buffer);
      sem_up(&s_buffer);
      sem_up(&s_vaga);

      printf ("%s consumiu %d\n\n", (char *) arg, item->valor);
      task_sleep (1000);
   }
}

int main(){

	task_t p1, p2, p3, c1, c2;

   //inicialização do SO
	ppos_init ();

	// cria semaforos
	sem_create (&s_buffer, 1);
	sem_create (&s_item, 0);
	sem_create (&s_vaga, TAM);

	// cria tarefas
	task_create (&p1, produtor, "p1");
	task_create (&p2, produtor, "p2");
	task_create (&p3, produtor, "p3");
	task_create (&c1, consumidor, "        c1");
	task_create (&c2, consumidor, "        c2");

   //aguarda tarefa finalizar
   task_join(&p1);

	task_exit(0);

	exit (0);

}