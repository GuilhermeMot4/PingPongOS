/*
Guilherme Ferreira Mota
GRR20197268
P12 - Fila de mensagens
*/

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#include "ppos.h"
#include "queue.h"

//#define DEBUG

#define STACKSIZE 64*1024

task_t Main, Dispatcher;
task_t *CurrentTask;
task_t *filaTasks, *filaDormindo;
int contID = 0;
int contTasks = 0;
int contTicks;

unsigned int relogio = 0;

struct sigaction action;

struct itimerval timer;

unsigned int systime(){
	return relogio;
}

void task_setprio (task_t *task, int prio){
	//ajusta o valor da prioridade estatica da task
	if (task == NULL){
		CurrentTask->prioridadeEstatica = prio;
		CurrentTask->prioridadeDinamica = CurrentTask->prioridadeEstatica;	
		
	}else{
		task->prioridadeEstatica = prio;
		task->prioridadeDinamica = task->prioridadeEstatica;
			
	}
}

int task_getprio (task_t *task){
	//retorna valor da prioridade estatica da task
	if (task == NULL){
		return CurrentTask->prioridadeEstatica;
	}else{
		return task->prioridadeEstatica;
	}
}

task_t * scheduler(){
	
	if (filaTasks == NULL){
		return NULL;
	}

	task_t *aux = filaTasks->next;
	task_t *menor = filaTasks;

	//encontra valor com maior prioridade (no caso, o menor)
	while(aux != filaTasks){
		//encontra valor com maior prioridade dinamica
		if (aux->prioridadeDinamica < menor->prioridadeDinamica){
			menor = aux;
		} //desempate considerando maior prioridade estatica
		else if (aux->prioridadeDinamica == menor->prioridadeDinamica){
			if (aux->prioridadeEstatica < menor->prioridadeEstatica){
				menor = aux;
			}
		}
		aux = aux->next;
	}

	//reseta a prioridade dinamica do escolhido
	menor->prioridadeDinamica = menor->prioridadeEstatica;

	//atualiza a prioridade dinamica dos demais
	aux = menor->next;
	while(aux != menor){
		aux->prioridadeDinamica = aux->prioridadeDinamica - 1;
		aux = aux->next;
	}

	return menor;

}

void acordarTarefas(){
	
	if (filaDormindo != NULL){
		task_t *aux = filaDormindo;
		do{
			//verifica se a tarefa pode acordar
			if (aux->acordar <= systime()){
				queue_remove((queue_t **) &filaDormindo, (queue_t *) aux);
				aux->estado = 'p';
				queue_append((queue_t **) &filaTasks, (queue_t *) aux);

				aux = filaDormindo;
			}else{
				aux = aux->next;
			}

		}while(aux != filaDormindo && filaDormindo != NULL); //percorre a fila
	}
}

void BodyDispatcher (){

	//enquanto tiver tarefas
   while(contTasks > 0){

   	//verifica se as tarefas adormecedias já podem acordar
    acordarTarefas();

   	//escolhe a proxima tarefa com maior prioridade (menor valor)
   	task_t *proxima = scheduler();
   	task_t *aux;

   	if(proxima != NULL){

   		contTicks = 20;

   		//remove da fila e faz a troca de contexto
   		queue_remove((queue_t **) &filaTasks, (queue_t *) proxima);
		task_switch(proxima);

		//ao retornar, avalia se veio de task_yield ou task_exit
		switch(proxima->estado){
			case 'p': //task_yield
				queue_append((queue_t **) &filaTasks, (queue_t *) proxima);
			break;
			case 't': //task_exit

					aux = proxima->filaSuspensas; //primeiro elemento da fila

					//libera as tasks suspensas e as coloca novamente na fila de prontas
					while(aux != NULL){
						queue_remove((queue_t **) &proxima->filaSuspensas, (queue_t *) aux);
						aux->estado = 'p';
						queue_append((queue_t **) &filaTasks, (queue_t *) aux);

						aux = proxima->filaSuspensas;
					}
				//libera pilha 
				free(proxima->context.uc_stack.ss_sp);
			break;
		}

   	}
   }

   //encerra dispatcher
   task_exit(0);
}

void tratador(int signum){

	relogio++;

	//se a tarefa atual for de usuario
	if (CurrentTask->usuario_t == 1){
		contTicks--;

		//retorna controle pro dispatcher ao final do contador
		if (contTicks == 0){
			task_yield();
		}
	}
	

}

void ppos_init(){
	//desabilita buffer da saida padrao
	setvbuf (stdout, 0, _IONBF, 0);

	//inicializa descritor de tarefa main

	task_create(&Main, NULL, NULL);

	CurrentTask = &Main; //atualiza tarefa atual para main

	contTicks = 20; //inicia contador de ticks

	//registra a acao para o sinal de timer SIGALRM
	action.sa_handler = tratador ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
	    perror ("Erro em sigaction: ") ;
	    exit (1) ;
 	}

 	// ajusta valores do temporizador
	timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
	timer.it_value.tv_sec  = 0 ;         // primeiro disparo, em segundos
	timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
	timer.it_interval.tv_sec  = 0 ;      // disparos subsequentes, em segundos

  	// arma o temporizador ITIMER_REAL (vide man setitimer)
	if (setitimer (ITIMER_REAL, &timer, 0) < 0)
	{
  		perror ("Erro em setitimer: ") ;
   		exit (1) ;
    }

	task_create(&Dispatcher, &BodyDispatcher, NULL);
	contTasks--;
	queue_remove((queue_t **) &filaTasks, (queue_t *) &Dispatcher);

	//define dispatcher como task de sistema
	Dispatcher.usuario_t = 0;


	//ativa o dispatcher
	task_yield();

	#ifdef DEBUG
		printf("ppos_init: inicializando o sistema\n");
	#endif
}

int task_create (task_t *task, void (*start_routine)(void *),  void *arg){

	char *stack;

	//inicializa variaveis
	task->prev = NULL;
	task->next = NULL;
	task->tid = contID; //insere id
	task->estado = 'p';
	task->prioridadeEstatica = 0;
	task->prioridadeDinamica = task->prioridadeEstatica;
	task->usuario_t = 1; //define task criada como de usuario
	task->tempoInic = 0;
	task->tempoProcessamento = 0;
	task->ativacoes = 0;

	contID++; //atualiza contador dos id
	contTasks++;

	getcontext (&task->context); //salva o contexto atual na variavel

	//aloca pilha do contexto
	stack = malloc(STACKSIZE);

	if(stack){
		task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0 ;
	}else{
		perror("Erro criação da pilha\n");
		return -1;
	}
	//ajusta os valores do contexto
	makecontext (&task->context, (void*)(*start_routine), 1, arg) ;


	#ifdef DEBUG
	printf("task_create: criou tarefa %d\n", task->tid);
	#endif


	queue_append((queue_t **) &filaTasks, (queue_t *) task);

	//retorna id criado
	return task->tid;

}

int task_id(){

	#ifdef DEBUG
		printf("task_id: retornando id %d da tarefa atual\n", CurrentTask->tid);
	#endif
	//retorna id da tarefa atual
	return CurrentTask->tid;
}


int task_switch (task_t *task){

	task_t *aux = CurrentTask; //salva tarefa atual
	CurrentTask = task; //atualiza a tarefa atual para task recebida na funcao

	aux->tempoProcessamento += systime() - aux->fimExe;

	task->ativacoes++;
	task->fimExe = systime();

	#ifdef DEBUG
		printf("task_switch: trocando contexto %d -> %d\n", aux->tid, task->tid);
	#endif
	//realiza troca de contexto entre a tarefa atual e a recebida na funcao
	if(swapcontext(&aux->context, &task->context) == -1){
		perror("Erro na troca de contexto\n");
		//em caso de erro na troca, volta a tarefa atual
		CurrentTask = aux;
		return -1;
	}

	return 0;
}

void task_yield (){

	task_switch(&Dispatcher);
}

void task_exit (int exit_code){

	CurrentTask->exitCode = exit_code;

	CurrentTask->tempoFim = systime();

	CurrentTask->tempoProcessamento += systime() - CurrentTask->fimExe;

	printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", CurrentTask->tid, CurrentTask->tempoFim - CurrentTask->tempoInic, CurrentTask->tempoProcessamento, CurrentTask->ativacoes);

	#ifdef DEBUG
		printf("task_exit: tarefa %d sendo encerrada\n", CurrentTask->tid);
	#endif
	//termina tarefa atual e transfere para main

	if(CurrentTask->tid == 1){
		task_switch(&Main);
	}else{
		contTasks--;
		CurrentTask->estado = 't';
		task_switch(&Dispatcher);
	}
	
}

int task_join (task_t *task){

	//verifica se a task é valida
	if (task == NULL){
		return -1;
	}

	//verifica se a task já encerrou
	if (task->estado == 't'){
		return task->exitCode;
	}

	//suspende a task corrente e adiciona na fila de suspensas da task enviada
	CurrentTask->estado = 's';

	queue_append((queue_t **) &task->filaSuspensas, (queue_t *) CurrentTask);

	//volta ao dispatcher
	task_yield();

	//retorna exitcode
	return task->exitCode;
}

void task_sleep (int t){
	//suspende a task corrente, calcula tempo de "acordar" e adiciona na fila de suspensas
	CurrentTask->estado = 's';
	CurrentTask->acordar = systime() + t;
	queue_append((queue_t **) &filaDormindo, (queue_t *) CurrentTask);

	//volta ao dispatcher
	task_yield();
}

int sem_create (semaphore_t *s, int value){

	//verifica se semáforo não é nulo
	if (s == NULL){
		return -1;
	}

	//verifica se semáforo já não foi criado
	if(s->ativo == 1){
		return -1;
	}

	//inicializa semáforo, tornando-o válido
	s->cont = value;
	s->filaSuspensas = NULL;
	s->ativo = 1;
	
	return 0;

}

int sem_down (semaphore_t *s){

	//verifica se o semáforo é válido
	if (s == NULL || s->ativo == 0){
		return -1;
	}

	s->cont--; //decrementa contador do semáforo
	if (s->cont < 0){ //se contador do semáforo ainda for negativo, suspende a tarefa corrente
		CurrentTask->estado = 's';
		queue_append((queue_t **) &s->filaSuspensas, (queue_t *) CurrentTask);

		//volta ao dispatcher
		task_yield();

		//se o semáforo foi destruído, interrompe o sem_down
		if(s->ativo == 0){
			return -1;
		}
	}

	return 0;

}

int sem_up (semaphore_t *s){

	task_t *aux;

	//verifica se o semáforo é válido
	if (s == NULL || s->ativo == 0){
		return -1;
	}

	//incrementa contador do semáforo
	s->cont++;
	if (s->cont <= 0){ //se contador do semáforo ainda for negativo, acorda a primeira tarefa da fila de suspensas
		aux = s->filaSuspensas;

		queue_remove((queue_t **) &s->filaSuspensas, (queue_t *) aux);
		aux->estado = 'p';
		queue_append((queue_t **) &filaTasks, (queue_t *) aux);
	}

	return 0;

}


int sem_destroy (semaphore_t *s){

	//verifica se o semáforo é válido
	if (s == NULL || s->ativo == 0){
		return -1;
	}

	//flag para invalidar(destruir) o semáforo
	s->ativo = 0;

	task_t *aux = s->filaSuspensas; 

	//percorre a fila de suspensas do semáforo e acorda todas as tarefas
	while(aux != NULL){
		queue_remove((queue_t **) &s->filaSuspensas, (queue_t *) aux);
		aux->estado = 'p';
		queue_append((queue_t **) &filaTasks, (queue_t *) aux);

		aux = s->filaSuspensas;
	}

	return 0;

}

int mqueue_create (mqueue_t *queue, int max, int size){

	//verifica se a fila não é nula
	if (queue == NULL){
		return -1;
	}

	//verifica se a fila já não foi criada
	if(queue->ativo == 1){
		return -1;
	}

	//inicializa fila, tornando-a válida

	queue->buffer = malloc(max * size); //aloca espaço para o tipo de dado recebido
  	queue->maxMsgs = max;
  	queue->msgSize = size;
 	queue->ativo = 1;
  	queue->cabeca = 0;
  	queue->cauda = 0;
  	queue->nMsgs = 0;

  	//inicializa semaforos

	sem_create (&queue->s_buffer, 1);
	sem_create (&queue->s_item, 0);
	sem_create (&queue->s_vaga, max);

	//volta ao dispatcher
	task_yield();

	
	return 0;

}

int mqueue_send (mqueue_t *queue, void *msg){

	//verifica se a fila não é nula
	if (queue == NULL){
		return -1;
	}

	//verifica se a fila não foi destruida
	if(queue->ativo == 0){
		return -1;
	}

	sem_down(&queue->s_vaga);
	sem_down(&queue->s_buffer);

	//insere item na calda do buffer
	memcpy(queue->buffer + (queue->cauda * queue->msgSize), msg ,queue->msgSize);
	queue->cauda = (1 + queue->cauda) % queue->maxMsgs; //atualiza posicao da cauda (buffer circular)
	queue->nMsgs++; //incrementa n de mensagens 


	sem_up(&queue->s_buffer);
	sem_up(&queue->s_item);

	return 0;

}

int mqueue_recv (mqueue_t *queue, void *msg){

	//verifica se a fila não é nula
	if (queue == NULL){
		return -1;
	}

	//verifica se a fila não foi destruida
	if(queue->ativo == 0){
		return -1;
	}

	sem_down(&queue->s_item);
    sem_down(&queue->s_buffer);

    //retira item da cabeca do buffer
 	memcpy(msg, queue->buffer + (queue->cabeca * queue->msgSize) ,queue->msgSize);
	queue->cabeca = (1 + queue->cabeca) % queue->maxMsgs; //atualiza posicao da cabeca (buffer circular)
	queue->nMsgs--; //decrementa n de mensagens


  	sem_up(&queue->s_buffer);
  	sem_up(&queue->s_vaga);

  	return 0;

}

int mqueue_destroy (mqueue_t *queue){

	//verifica se a fila não é nula
	if (queue == NULL){
		return -1;
	}

	//verifica se a fila não foi destruida
	if(queue->ativo == 0){
		return -1;
	}

	queue->ativo = 0; //flag para invalidar(destruir) a fila
  	queue->cabeca = 0;
  	queue->cauda = 0;
  	queue->nMsgs = 0;

  	//libera buffer
  	free(queue->buffer);

  	//destroi os semaforos da fila
	sem_destroy(&queue->s_buffer);
	sem_destroy(&queue->s_item);
	sem_destroy(&queue->s_vaga);

  	return 0;

}

int mqueue_msgs (mqueue_t *queue){

	//verifica se a fila não é nula
	if (queue == NULL){
		return -1;
	}

	//verifica se a fila não foi destruida
	if(queue->ativo == 0){
		return -1;
	}

	//retorna n de msgs no buffer
	return queue->nMsgs;

}





