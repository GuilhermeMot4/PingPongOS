// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   struct task_t *filaSuspensas;
   int tid ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   char estado;
   int prioridadeDinamica;
   int prioridadeEstatica;
   int usuario_t;
   int exitCode;
   int acordar;
   unsigned int tempoInic;
   unsigned int tempoFim;
   //unsigned int inicExe;
   unsigned int fimExe;
   unsigned int tempoProcessamento;
   unsigned int ativacoes;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
   int cont; //contador do semáforo
   struct task_t *filaSuspensas; //fila de tarefas suspensas do semáforo
   int ativo; //flag destruicao semáforo
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  void *buffer; //buffer da fila
  int maxMsgs; //numero maximo de msgs
  int msgSize; //tamanho das mensagens em bytes
  int ativo;  //flag destruicao fila
  int cabeca; //indica cabeca do buffer
  int cauda;  //indica cauda do buffer
  int nMsgs; //numero atual de msgs na fila

  //semaforos
  semaphore_t s_vaga;
  semaphore_t s_buffer;
  semaphore_t s_item;
} mqueue_t ;

#endif

