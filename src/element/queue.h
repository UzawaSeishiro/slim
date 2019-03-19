/*
 * queue.h
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group
 *                                          <lmntal@ueda.info.waseda.ac.jp>
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *    3. Neither the name of the Ueda Laboratory LMNtal Group nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

/** @author Masato Gocho
 *  library for queue / parallel queue
 */

#ifndef LMN_QUEUE_H
#define LMN_QUEUE_H

/**
 * @ingroup Element
 * @defgroup Queue
 * @{
 */

#include "../lmntal.h"
#include "lmntal_thread.h"

typedef struct Queue Queue;
struct Node {
  LmnWord v;
  Node *next;
  Node(LmnWord v);
  ~Node();
};

struct Queue {
  Queue();
  Queue(BOOL lock_type);
  ~Queue();
  Node *head;
  Node *tail;
  BOOL qlock;
  unsigned long enq_num, deq_num;
  pthread_mutex_t enq_mtx, deq_mtx;
  void enqueue(LmnWord v);
  void enqueue_push_head(LmnWord v);
  LmnWord dequeue();
  BOOL is_empty();
  void lock(BOOL is_enq);
  void unlock(BOOL is_enq);
  void clear();
  unsigned long entry_num();

};



/* single dequeue(reader), single enqueue(writer) */
#define LMN_Q_SRSW 0
/* single dequeue(reader), multiple enqueue(writer) */
#define LMN_Q_SRMW 1
/* multiple dequeue(reader), single enqueue(writer) */
#define LMN_Q_MRSW 2
/* multiple dequeue(reader), multiple enqueue(writer) */
#define LMN_Q_MRMW 4

/** ==========
 *  DeQue (KaWaBaTa code)
 */

struct Deque {
  Deque(unsigned int init_size);
  void init(unsigned int init_size);
  void extend();
  void push_head(LmnWord keyp);
  LmnWord *tbl;
  unsigned int head, tail, cap;
};

typedef struct Deque Deque;
typedef LmnWord deq_data_t;

#define deq_cap(V) ((V)->cap)
#define deq_head(V) ((V)->head)
#define deq_tail(V) ((V)->tail)
#define deq_num(V)                                                             \
  ((V)->tail > (V)->head ? (V)->tail - (V)->head - 1                           \
                         : (V)->cap - (V)->head + (V)->tail - 1)
#define deq_is_empty(V) (deq_num(V) == 0)
#define DEQ_DEC(X, C) (X = X != 0 ? X - 1 : C - 1)
#define DEQ_INC(X, C) (X = X != C - 1 ? X + 1 : 0)

static inline void deq_push_tail(Deque *deq, LmnWord keyp);
static inline LmnWord deq_pop_head(Deque *deq);
static inline LmnWord deq_pop_tail(Deque *deq);
static inline LmnWord deq_peek_head(const Deque *deq);
static inline LmnWord deq_peek_tail(const Deque *deq);
static inline LmnWord deq_get(const Deque *deq, unsigned int i);
static inline void deq_clear(Deque *deq);
static inline void deq_destroy(Deque *deq);
static inline void deq_free(Deque *deq);
static inline unsigned long deq_space(Deque *deq);
static inline unsigned long deq_space_inner(Deque *deq);
static inline void deq_print(Deque *deq);

LmnWord deq_pop_n(Deque *deq, unsigned int n);
BOOL deq_contains(const Deque *deq, LmnWord keyp);
Deque *deq_copy(Deque *deq);
void deq_reverse(Deque *deq);
void deq_resize(Deque *deq, unsigned int size, deq_data_t val);
void deq_sort(const Deque *deq, int (*compare)(const void *, const void *));

/*  */
static inline void deq_push_tail(Deque *deq, LmnWord keyp) {
  if (deq_num(deq) == deq->cap - 1) {
    deq->extend();
  }
  (deq->tbl)[deq_tail(deq)] = keyp;
  DEQ_INC(deq->tail, deq->cap);
}

/* pop */
static inline LmnWord deq_pop_head(Deque *deq) {
  LmnWord ret;
  LMN_ASSERT(deq_num(deq) > 0);

  DEQ_INC(deq->head, deq->cap);
  ret = deq->tbl[deq->head];
  return ret;
}

static inline LmnWord deq_pop_tail(Deque *deq) {
  LMN_ASSERT(deq_num(deq) > 0);
  DEQ_DEC(deq->tail, deq->cap);
  return deq->tbl[deq->tail];
}

/* peek */
static inline LmnWord deq_peek_head(const Deque *deq) {
  unsigned int x = deq->head;
  return deq->tbl[DEQ_INC(x, deq->cap)];
}

static inline LmnWord deq_peek_tail(const Deque *deq) {
  unsigned int x = deq->tail;
  return deq->tbl[DEQ_DEC(x, deq->cap)];
}

/* peak (no assertion) */
static inline LmnWord deq_get(const Deque *deq, unsigned int i) {
  return deq->tbl[i];
}

/* pop all elements from deq */
static inline void deq_clear(Deque *deq) {
  deq->head = 0;
  deq->tail = 1;
}

/* destroy */
static inline void deq_destroy(Deque *deq) { LMN_FREE(deq->tbl); }

/* free */
static inline void deq_free(Deque *deq) {
  LMN_FREE(deq->tbl);
  LMN_FREE(deq);
}

static inline unsigned long deq_space_inner(Deque *deq) {
  return deq_cap(deq) * sizeof(deq_data_t);
}

static inline unsigned long deq_space(Deque *deq) {
  return sizeof(struct Deque) + deq_space_inner(deq);
}

static inline void deq_print(Deque *deq) {
  unsigned int i;
  FILE *f = stdout;
  fprintf(f, "cap=%u, head=%u, tail=%u, num=%u\n[", deq->cap, deq->head,
          deq->tail, deq_num(deq));
  for (i = 0; i < deq->cap; i++)
    fprintf(f, "%lu, ", deq->tbl[i]);
  fprintf(f, "]\n");
}

/* @} */

#endif
