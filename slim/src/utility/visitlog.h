/*
 * visitlog.h
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group <lmntal@ueda.info.waseda.ac.jp>
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
 */

#ifndef LMN_VISITLOG_H
#define LMN_VISITLOG_H

#include "lmntal.h"
#include "vector.h"
#include "atom.h"
#include "membrane.h"
#include "error.h"
#include "util.h"
#ifndef TIME_OPT
# include "st.h"
#endif
#include <limits.h>


#define VISITLOG_INIT_N       (1)

/* LMNtal�Υץ����ʥ��ȥࡢ��ˤ򥭡��ˤ������ơ��֥� */
struct ProcessTbl {
  unsigned long n;
  unsigned long size;
#ifdef TIME_OPT
  LmnWord *tbl;
#else
  st_table_t tbl;
#endif
};

#define process_tbl_entry_num(P)  ((P)->n)
#ifdef TIME_OPT
void proc_tbl_expand_sub(ProcessTbl p, unsigned long n);
#define proc_tbl_expand(p, n)                                                  \
  if ((p)->size <= (n)) {                                                      \
    proc_tbl_expand_sub(p, n);                                                 \
  }
#endif


/**
 * Function ProtoTypes
 */

void       proc_tbl_init_with_size(ProcessTbl p, unsigned long size);
void       proc_tbl_init(ProcessTbl p);
ProcessTbl proc_tbl_make_with_size(unsigned long size);
ProcessTbl proc_tbl_make(void);
void       proc_tbl_destroy(ProcessTbl p);
void       proc_tbl_free(ProcessTbl p);
void       proc_tbl_clear(ProcessTbl p);
int        proc_tbl_foreach(ProcessTbl p,
                            int(*func)(LmnWord key, LmnWord val, LmnWord arg),
                            LmnWord arg);
BOOL       proc_tbl_eq(ProcessTbl a, ProcessTbl b);

static inline void proc_tbl_put(ProcessTbl p, LmnWord key, LmnWord value);
static inline void proc_tbl_put_atom(ProcessTbl p, LmnSAtom atom, LmnWord value);
static inline void proc_tbl_put_mem(ProcessTbl p, LmnMembrane *mem, LmnWord value);
static inline int  proc_tbl_put_new(ProcessTbl p, LmnWord key, LmnWord value);
static inline int  proc_tbl_put_new_atom(ProcessTbl p, LmnSAtom atom, LmnWord value);
static inline int  proc_tbl_put_new_mem(ProcessTbl p, LmnMembrane *mem, LmnWord value);
static inline void proc_tbl_put_new_hlink(ProcessTbl p, HyperLink *hl, LmnWord value);
static inline void proc_tbl_unput(ProcessTbl p, LmnWord key);
static inline void proc_tbl_unput_atom(ProcessTbl p, LmnSAtom atom);
static inline void proc_tbl_unput_mem(ProcessTbl p, LmnMembrane *mem);
static inline int  proc_tbl_get(ProcessTbl p, LmnWord key, LmnWord *value);
static inline int  proc_tbl_get_by_atom(ProcessTbl p, LmnSAtom atom, LmnWord *value);
static inline int  proc_tbl_get_by_mem(ProcessTbl p, LmnMembrane *mem, LmnWord *value);
static inline int  proc_tbl_get_by_hlink(ProcessTbl p, HyperLink *hl, LmnWord *value);
static inline BOOL proc_tbl_contains(ProcessTbl p, LmnWord key);
static inline BOOL proc_tbl_contains_atom(ProcessTbl p, LmnSAtom atom);
static inline BOOL proc_tbl_contains_mem(ProcessTbl p, LmnMembrane *mem);



/**
 * Inline Functions
 */


/* �ơ��֥��key���ɲá�put_atom,put_mem����Ѥ��롣 */
static inline void proc_tbl_put(ProcessTbl p, LmnWord key, LmnWord value) {
  p->n++;
#ifdef TIME_OPT
# ifdef DEBUG
  if (value == ULONG_MAX) lmn_fatal("cannot put ULONG_MAX");
# endif
  proc_tbl_expand(p, key);
  p->tbl[key] = value;
#else
  st_insert(p->tbl, (st_data_t)key, value);
#endif
}

/* �ơ��֥�˥��ȥ���ɲ� */
static inline void proc_tbl_put_atom(ProcessTbl p, LmnSAtom atom, LmnWord value) {
  proc_tbl_put(p, LMN_SATOM_ID(atom), value);
}

/* �ơ��֥������ɲ� */
static inline void proc_tbl_put_mem(ProcessTbl p, LmnMembrane *mem, LmnWord value) {
  proc_tbl_put(p, lmn_mem_id(mem), value);
}

/* �ơ��֥�˥ϥ��ѡ���󥯤��ɲ� */
static inline void proc_tbl_put_new_hlink(ProcessTbl p, HyperLink *hl, LmnWord value)
{
  proc_tbl_put(p, LMN_HL_ID(hl), value);
}

/* �ơ��֥��key���ɲä�, �����ͤ��֤�. ���Ǥ�p��¸�ߤ�������0���֤�.
 * �̾盧�δؿ��ǤϤʤ�put_new_atom, put_new_mem����Ѥ���. */
static inline int proc_tbl_put_new(ProcessTbl p, LmnWord key, LmnWord value) {
  p->n++;
#ifndef TIME_OPT
  return st_insert_safe(p->tbl, (st_data_t)key, value);
#else
#ifdef DEBUG
  if (value == ULONG_MAX) lmn_fatal("cannot put ULONG_MAX");
#endif
  proc_tbl_expand(p, key);
  if (p->tbl[key] != ULONG_MAX) return 0;
  p->tbl[key] = value;
  return 1;
#endif
}

/* �ơ��֥�˥��ȥ���ɲä��������ͤ��֤������Ǥ�Ʊ�����ȥब¸�ߤ�������0���֤� */
static inline int proc_tbl_put_new_atom(ProcessTbl p, LmnSAtom atom, LmnWord value) {
  return proc_tbl_put_new(p, LMN_SATOM_ID(atom), value);
}

/* �ơ��֥������ɲä��������ͤ��֤������Ǥ�Ʊ���줬¸�ߤ�������0���֤� */
static inline int proc_tbl_put_new_mem(ProcessTbl p, LmnMembrane *mem, LmnWord value) {
  return proc_tbl_put_new(p, lmn_mem_id(mem), value);
}

/* �ơ��֥뤫��key�Ȥ�����б������ͤ�������.
 * �̾盧�δֿ��ǤϤʤ�unput_atom, unput_mem����Ѥ���. */
static inline void proc_tbl_unput(ProcessTbl p, LmnWord key) {
  p->n--;
#ifdef TIME_OPT
  proc_tbl_expand(p, key);
  p->tbl[key] = ULONG_MAX;
#else
  st_delete(p->tbl, (st_data_t)key, NULL);
#endif
}

/* �ơ��֥뤫�饢�ȥ�Ȥ�����б������ͤ������� */
static inline void proc_tbl_unput_atom(ProcessTbl p, LmnSAtom atom) {
  proc_tbl_unput(p, LMN_SATOM_ID(atom));
}

/* �ơ��֥뤫����Ȥ�����б������ͤ������� */
static inline void proc_tbl_unput_mem(ProcessTbl p, LmnMembrane *mem) {
  proc_tbl_unput(p, lmn_mem_id(mem));
}

/* �ơ��֥��key���б������ͤ�value�����ꤷ, �����ͤ��֤�. key���ơ��֥��¸�ߤ��ʤ�����0���֤�.
 * �̾盧�δֿ��ǤϤʤ�get_by_atom, get_by_mem����Ѥ���./ */
static inline int proc_tbl_get(ProcessTbl p, LmnWord key, LmnWord *value) {
#ifdef TIME_OPT
  if (p->size > key && p->tbl[key] != ULONG_MAX) {
    if (value) *value = p->tbl[key];
    return 1;
  } else {
    return 0;
  }
#else
  return st_lookup(p->tbl, key, (st_data_t *)value);
#endif
}

/* �ơ��֥�Υ��ȥ�atom���б������ͤ�value�����ꤷ, �����ͤ��֤�.
 * �ơ��֥��atom��¸�ߤ��ʤ�����0���֤� */
static inline int proc_tbl_get_by_atom(ProcessTbl p, LmnSAtom atom, LmnWord *value) {
  return proc_tbl_get(p, LMN_SATOM_ID(atom), value);
}

/* �ơ��֥����mem���б������ͤ�value�����ꤷ, �����ͤ��֤�.
 * �ơ��֥��mem��¸�ߤ��ʤ�����0���֤� */
static inline int proc_tbl_get_by_mem(ProcessTbl p, LmnMembrane *mem, LmnWord *value) {
  return proc_tbl_get(p, lmn_mem_id(mem), value);
}

/* �ơ��֥�Υϥ��ѡ����hl���б������ͤ�value�����ꤷ, �����ͤ��֤�.
 * �ơ��֥��hl��¸�ߤ��ʤ�����0���֤� */
static inline int proc_tbl_get_by_hlink(ProcessTbl p, HyperLink *hl, LmnWord *value)
{
  return proc_tbl_get(p, LMN_HL_ID(hl), value);
}

static inline BOOL proc_tbl_contains(ProcessTbl p, LmnWord key) {
#ifdef TIME_OPT
  return key < p->size && p->tbl[key] != ULONG_MAX;
#else
  LmnWord t;
  return proc_tbl_get(p, key, &t);
#endif
}

/* �ơ��֥�˥��ȥ�atom���б������ͤ����ꤵ��Ƥ�����, �����ͤ��֤�. */
static inline BOOL proc_tbl_contains_atom(ProcessTbl p, LmnSAtom atom) {
  return proc_tbl_contains(p, LMN_SATOM_ID(atom));
}

/* �ơ��֥����mem���б������ͤ����ꤵ��Ƥ����硢�����ͤ��֤�. */
static inline BOOL proc_tbl_contains_mem(ProcessTbl p, LmnMembrane *mem) {
  return proc_tbl_contains(p, lmn_mem_id(mem));
}


/* --------------
 *  SimplyProcTbl
 *  �ץ���ID��key�ˤ���BYTE�������ơ��֥�
 */
struct SimplyProcTbl {
  unsigned long n, cap;
#ifdef TIME_OPT
  BYTE *tbl;
#else
  st_table_t tbl;
#endif
};

#define SPROC_TBL_INIT_V        (0xfU)
#define sproc_tbl_entry_num(P)  ((P)->n)

/**
 * Function Prototypes
 */


void sproc_tbl_init_with_size(SimplyProcTbl p, unsigned long size);
void sproc_tbl_init(SimplyProcTbl p);
void sproc_tbl_destroy(SimplyProcTbl p);

static inline void sproc_tbl_expand(SimplyProcTbl p, unsigned long n);
static inline void sproc_tbl_put(SimplyProcTbl p, LmnWord key, BYTE value);
static inline void sproc_tbl_put_atom(SimplyProcTbl p, LmnSAtom atom, BYTE value);
static inline void sproc_tbl_put_mem(SimplyProcTbl p, LmnMembrane *mem, BYTE value);
static inline void sproc_tbl_unput(SimplyProcTbl p, LmnWord key);
static inline void sproc_tbl_unput_atom(SimplyProcTbl p, LmnSAtom atom);
static inline void sproc_tbl_unput_mem(SimplyProcTbl p, LmnMembrane *mem);
static inline int  sproc_tbl_get(SimplyProcTbl p, LmnWord key, BYTE *value);
static inline int  sproc_tbl_get_by_atom(SimplyProcTbl p, LmnSAtom atom, BYTE *value);
static inline int  sproc_tbl_get_by_mem(SimplyProcTbl p, LmnMembrane *mem, BYTE *value);
static inline BOOL sproc_tbl_contains(SimplyProcTbl p, LmnWord key);
static inline BOOL sproc_tbl_contains_atom(SimplyProcTbl p, LmnSAtom atom);
static inline BOOL sproc_tbl_contains_mem(SimplyProcTbl p, LmnMembrane *mem);
static inline BOOL sproc_tbl_get_flag(SimplyProcTbl p, LmnWord key, BYTE flag);
static inline BOOL sproc_tbl_get_flag_by_atom(SimplyProcTbl p, LmnSAtom key, LmnWord flag);
static inline BOOL sproc_tbl_get_flag_by_mem(SimplyProcTbl p, LmnMembrane *key, LmnWord flag);
static inline void sproc_tbl_unset_flag(SimplyProcTbl p, LmnWord key, LmnWord flag);
static inline void sproc_tbl_set_flag(SimplyProcTbl p, LmnWord key, LmnWord flag);
static inline void sproc_tbl_set_atom_flag(SimplyProcTbl p, LmnSAtom key, LmnWord flag);
static inline void sproc_tbl_set_mem_flag(SimplyProcTbl p, LmnMembrane *key, LmnWord flag);
static inline void sproc_tbl_unset_atom_flag(SimplyProcTbl p, LmnSAtom key, LmnWord flag);
static inline void sproc_tbl_unset_mem_flag(SimplyProcTbl p, LmnMembrane *key, LmnWord flag);


/**
 * Inline Functions
 */

static inline void sproc_tbl_expand(SimplyProcTbl p, unsigned long n) {
  unsigned long org_size = p->cap;
  while (p->cap <= n) p->cap *= 2;
  p->tbl = LMN_REALLOC(BYTE, p->tbl, p->cap);
  memset(p->tbl + org_size,
         SPROC_TBL_INIT_V,
         sizeof(BYTE) * (p->cap - org_size));
}

/* �ơ��֥��key���ɲá�put_atom,put_mem����Ѥ��롣 */
static inline void sproc_tbl_put(SimplyProcTbl p, LmnWord key, BYTE value) {
#ifdef TIME_OPT
#ifdef DEBUG
  if (value == SPROC_TBL_INIT_V) lmn_fatal("i can't put this value");
#endif
  if (p->cap <= key) {
    sproc_tbl_expand(p, key);
  }

  if (p->tbl[key] == SPROC_TBL_INIT_V) {
    p->n++;
  }

  p->tbl[key] = value;
#else
  st_insert(p->tbl, (st_data_t)key, value);
#endif
}

static inline void sproc_tbl_put_atom(SimplyProcTbl p, LmnSAtom atom, BYTE value) {
  sproc_tbl_put(p, LMN_SATOM_ID(atom), value);
}

static inline void sproc_tbl_put_mem(SimplyProcTbl p, LmnMembrane *mem, BYTE value) {
  sproc_tbl_put(p, lmn_mem_id(mem), value);
}

static inline void sproc_tbl_unput(SimplyProcTbl p, LmnWord key) {
#ifdef TIME_OPT
  if (p->cap < key || p->tbl[key] == SPROC_TBL_INIT_V) return;
  p->n--;
  p->tbl[key] = SPROC_TBL_INIT_V;
#else
  st_delete(p->tbl, (st_data_t)key, NULL);
#endif
}

static inline void sproc_tbl_unput_atom(SimplyProcTbl p, LmnSAtom atom) {
  sproc_tbl_unput(p, LMN_SATOM_ID(atom));
}

static inline void sproc_tbl_unput_mem(SimplyProcTbl p, LmnMembrane *mem) {
  sproc_tbl_unput(p, lmn_mem_id(mem));
}

static inline int sproc_tbl_get(SimplyProcTbl p, LmnWord key, BYTE *value) {
#ifdef TIME_OPT
  if (p->cap > key && p->tbl[key] != SPROC_TBL_INIT_V) {
    if (value) *value = p->tbl[key];
    return 1;
  } else {
    return 0;
  }
#else
  return st_lookup(p->tbl, key, (st_data_t *)value);
#endif
}

static inline int sproc_tbl_get_by_atom(SimplyProcTbl p, LmnSAtom atom, BYTE *value) {
  return sproc_tbl_get(p, LMN_SATOM_ID(atom), value);
}

static inline int sproc_tbl_get_by_mem(SimplyProcTbl p, LmnMembrane *mem, BYTE *value) {
  return sproc_tbl_get(p, lmn_mem_id(mem), value);
}

static inline BOOL sproc_tbl_contains(SimplyProcTbl p, LmnWord key) {
#ifdef TIME_OPT
  return key < p->cap && p->tbl[key] != SPROC_TBL_INIT_V;
#else
  LmnWord t;
  return sproc_tbl_get(p, key, &t);
#endif
}

static inline BOOL sproc_tbl_contains_atom(SimplyProcTbl p, LmnSAtom atom) {
  return sproc_tbl_contains(p, LMN_SATOM_ID(atom));
}

static inline BOOL sproc_tbl_contains_mem(SimplyProcTbl p, LmnMembrane *mem) {
  return sproc_tbl_contains(p, lmn_mem_id(mem));
}

static inline BOOL sproc_tbl_get_flag(SimplyProcTbl p, LmnWord key, BYTE flag) {
#ifdef TIME_OPT
  if (p->cap > key && p->tbl[key] != SPROC_TBL_INIT_V) return p->tbl[key] & flag;
  else return 0;
#else
  LmnWord t;
  return st_lookup(p->tbl, key, &t) && (t & flag);
#endif
}

static inline BOOL sproc_tbl_get_flag_by_atom(SimplyProcTbl p, LmnSAtom key, LmnWord flag) {
  return sproc_tbl_get_flag(p, LMN_SATOM_ID(key), flag);
}

static inline BOOL sproc_tbl_get_flag_by_mem(SimplyProcTbl p, LmnMembrane *key, LmnWord flag) {
  return sproc_tbl_get_flag(p, lmn_mem_id(key), flag);
}

static inline void sproc_tbl_unset_flag(SimplyProcTbl p, LmnWord key, LmnWord flag) {
#ifdef TIME_OPT
  if (p->cap <= key) {
    sproc_tbl_expand(p, key);
  }
  if (p->tbl[key] != SPROC_TBL_INIT_V) {
    p->tbl[key] |= ~flag;
  } else {
    p->n++;
    p->tbl[key] = 0;
  }
#else
  st_data_t t;
  if (st_lookup(p->tbl, (st_data_t)key, &t)) {
    st_insert(p->tbl, (st_data_t)key, ((LmnWord)t) | flag);
  } else {
    st_insert(p->tbl, (st_data_t)key, flag);
  }
#endif
}

static inline void sproc_tbl_set_flag(SimplyProcTbl p, LmnWord key, LmnWord flag) {
#ifdef TIME_OPT
  if (p->cap <= key) {
    sproc_tbl_expand(p, key);
  }
  if (p->tbl[key] != SPROC_TBL_INIT_V) {
    p->tbl[key] |= flag;
  } else {
    p->n++;
    p->tbl[key] = flag;
  }
#else
  st_data_t t;
  if (st_lookup(p->tbl, (st_data_t)key, &t)) {
    st_insert(p->tbl, (st_data_t)key, ((LmnWord)t) | flag);
  } else {
    st_insert(p->tbl, (st_data_t)key, flag);
  }
#endif
}

static inline void sproc_tbl_set_atom_flag(SimplyProcTbl p, LmnSAtom key, LmnWord flag) {
  sproc_tbl_set_flag(p, LMN_SATOM_ID(key), flag);
}

static inline void sproc_tbl_set_mem_flag(SimplyProcTbl p, LmnMembrane *key, LmnWord flag) {
  sproc_tbl_set_flag(p, lmn_mem_id(key), flag);
}

static inline void sproc_tbl_unset_atom_flag(SimplyProcTbl p, LmnSAtom key, LmnWord flag) {
  sproc_tbl_unset_flag(p, LMN_SATOM_ID(key), flag);
}

static inline void sproc_tbl_unset_mem_flag(SimplyProcTbl p, LmnMembrane *key, LmnWord flag) {
  sproc_tbl_unset_flag(p, lmn_mem_id(key), flag);
}

/*----------------------------------------------------------------------
 * TraceLog
 * --------
 * - struct LogTracker
 * - struct TraceLog
 * - struct SimpleTraceLog
 *
 * VisitLog�β�����.
 * �ǡ�����¤�򥰥��Ʊ����Ƚ����ò�����������, VisitLog����Ӥ���������������.
 */


/* ---
 * Tracker����
 * ˬ��Ѥߥơ��֥�ξ��֤�Хå��ȥ�å������뤿��Υǡ�����¤
 */

struct LogTracker {
  struct Vector traced_ids, btp_idx;
};

/* ����Υȥ졼���ץ���������, ���򴬤��᤹. */
#define LogTracker_REVERT(Tracker, UnputFunc, Tbl)                             \
  do {                                                                         \
    unsigned long __bt_idx;                                                    \
    LMN_ASSERT(!vec_is_empty(&((Tracker)->btp_idx)));                          \
                                                                               \
    /* ���󥻥åȤ����ȥ졼�����Υ�����Ф� */                                     \
    __bt_idx = vec_pop(&((Tracker)->btp_idx));                                 \
    /* bt_idx�����idx���Ѥޤ줿�ǡ�������夫��unput���Ƥ��� */                    \
    while (vec_num(&((Tracker)->traced_ids)) > __bt_idx) {                     \
      LmnWord __key = vec_pop(&((Tracker)->traced_ids));                       \
      UnputFunc(Tbl, __key);                                                   \
    }                                                                          \
  } while (0)

/* ���ߤΥȥ졼���ץ��������l�˥�⤷�Ƥ��� */
#define LogTracker_PUSH(TR) (vec_push(&(TR)->btp_idx, vec_num(&(TR)->traced_ids)))
/* �Ǥ�Ƕ�˥�⤷���ȥ졼���ץ������ε�Ͽ�������� */
#define LogTracker_POP(TR)  (vec_pop(&(TR)->btp_idx))
/* �ץ���ID�����ID���ɲä��� */
#define LogTracker_TRACE(TR, ID) (vec_push(&(TR)->traced_ids, (ID)))

/* ----
 * TraceLog
 * �����Ʊ����Ƚ���ѥǡ�����¤����
 */

/** MEMO:
 *  ��TIME-OPT�Ǥ�������Ƥ��ʤ�
 */

/* ��{����ܥ륢�ȥ�, inside proxy���ȥ�, ��}���Ф���1�Ĥ����б�������ǡ�����¤
 * outside proxy���ȥ�ϴޤ�ʤ� */
struct TraceData { /* 64bit: 24Bytes (32bit: 16Bytes) */
  BYTE flag;                   /* �б������Ƥ���ǡ����μ���򼨤��ե饰 */
  unsigned int traversed_proc; /* ����б������Ƥ������, ���������ˬ�䤷��
                                   {����ܥ륢�ȥ�, inside proxy���ȥ�, ����}�����
                                  ��Ͽ���Ƥ���.
                                  ¾�Υǡ������б������Ƥ������0�Τޤ� */
  ProcessID owner_id;          /* �б������Ƥ���ǡ�����¤�ν�°���ID.
                                * �ץ���ID��1����Ϥޤ뤿��,
                                * ��°�줬�ʤ�(�㤨�Х����Х�롼�����)����, 0 */
  ProcessID matched;           /* �б������Ƥ���ץ����ȥޥå��������ץ�����ID.
                                * in-proxy���ȥ��BS encode����ˬ�����˿����ʤ�����,
                                * in-proxy���ȥ�ؤ��б��Ȥ��Ƥ�0�򥻥å� */
};

struct TraceLog {
  int cap, num;
  struct TraceData *tbl;
  struct LogTracker tracker;
};

typedef struct TraceLog *TraceLog;


#define TLOG_FLAG(V)               ((V).flag)
#define TLOG_NUM(V)                ((V).traversed_proc)
#define TLOG_OWNER(V)              ((V).owner_id)
#define TLOG_MATCHED(V)            ((V).matched)
#define TLOG_TRV_INC(V)            (TLOG_NUM(V)++)
#define TLOG_TRV_DEC(V)            (TLOG_NUM(V)--)
#define TLOG_SET_OWNER(V, N)       (TLOG_OWNER(V) = lmn_mem_id(N))
#define TLOG_SET_MATCHED(V, ID)    (TLOG_MATCHED(V) = (ID))

#define TLOG_DATA_CLEAR(V)                                                     \
    do {                                                                       \
      l->tbl[key].flag = TLOG_INIT_DATA;                                       \
      l->tbl[key].owner_id = 0;                                                \
      l->tbl[key].matched = 0;                                                 \
    } while (0)

#define TLOG_MATCHED_ID_NONE       (0U)

#define TLOG_INIT_DATA             (0x0U)
#define TLOG_TRAVERSED_ATOM        (0x1U)
#define TLOG_TRAVERSED_MEM         (0x2U)
#define TLOG_TRAVERSED_HLINK       (0x3U)
#define TLOG_TRAVERSED_OTHERS      (0xfU)

#define TLOG_SET_TRV_ATOM(F)       (F = TLOG_TRAVERSED_ATOM)
#define TLOG_SET_TRV_MEM(F)        (F = TLOG_TRAVERSED_MEM)
#define TLOG_SET_TRV_HLINK(F)      (F = TLOG_TRAVERSED_HLINK)
#define TLOG_SET_TRV_SOME(F)       (F = TLOG_TRAVERSED_OTHERS)
#define TLOG_IS_TRV(F)             (F != TLOG_INIT_DATA)
#define TLOG_UNSET_TRV(F)          (F = TLOG_INIT_DATA)

/**
 * Function ProtoTypes
 */

TraceLog tracelog_make(void);
void tracelog_free(TraceLog trc);
inline void tracelog_init(TraceLog trc);
inline void tracelog_init_with_size(TraceLog trc, unsigned long size);
inline void tracelog_destroy(TraceLog trc);

static inline BOOL tracelog_eq_traversed_proc_num(TraceLog      l,
                                                  LmnMembrane   *owner,
                                                  AtomListEntry *in_ent,
                                                  AtomListEntry *avoid);
static inline void tracelog_tbl_expand(TraceLog l, unsigned long new_size);
static inline int  tracelog_put(TraceLog l, LmnWord key, LmnWord matched_id,
                                LmnMembrane *owner);
static inline int  tracelog_put_atom(TraceLog l, LmnSAtom atom1, LmnWord atom2_id,
                                     LmnMembrane *owner1);
static inline int  tracelog_put_mem(TraceLog l, LmnMembrane *mem1, LmnWord mem2_id);
static inline int  tracelog_put_hlink(TraceLog l, HyperLink *hl1, LmnWord hl2_id);
static inline void tracelog_unput(TraceLog l, LmnWord key);
static inline BOOL tracelog_contains(TraceLog l, LmnWord key);
static inline BOOL tracelog_contains_atom(TraceLog l, LmnSAtom atom);
static inline BOOL tracelog_contains_mem(TraceLog l, LmnMembrane *mem);
static inline BOOL tracelog_contains_hlink(TraceLog l, HyperLink *hl) ;
static inline LmnWord tracelog_get_matched(TraceLog l, LmnWord key);
static inline LmnWord tracelog_get_atomMatched(TraceLog l, LmnSAtom atom);
static inline LmnWord tracelog_get_memMatched(TraceLog l, LmnMembrane *mem);
static inline LmnWord tracelog_get_hlinkMatched(TraceLog l, HyperLink *hl);
static inline void tracelog_backtrack(TraceLog l);
static inline void tracelog_set_btpoint(TraceLog l);
static inline void tracelog_continue_trace(TraceLog l);
static inline BYTE tracelog_get_matchedFlag(TraceLog l, LmnWord key);

/**
 * Inline Functions
 */

/* ��owner���оݤȤ���ˬ��Ѥߤˤ����ץ��� (����ܥ륢�ȥ� + ���� + inside proxies) �ο���
 * ��owner�Τ���Ȱ��פ��Ƥ��뤫�ݤ����֤� */
static inline BOOL tracelog_eq_traversed_proc_num(TraceLog      l,
                                                  LmnMembrane   *owner,
                                                  AtomListEntry *in_ent,
                                                  AtomListEntry *avoid)
{
  return
      TLOG_NUM(l->tbl[lmn_mem_id(owner)]) ==
          (lmn_mem_symb_atom_num(owner)
              + lmn_mem_child_mem_num(owner)
              + atomlist_get_entries_num(in_ent)
              - atomlist_get_entries_num(avoid));
}

/* �ȥ졼����l�Υơ��֥륵������new_size�ʾ���礭���˳�ĥ����. */
static inline void tracelog_tbl_expand(TraceLog l, unsigned long new_size)
{
  unsigned long org_size = l->cap;
  while (l->cap <= new_size) l->cap *= 2;
  l->tbl = LMN_REALLOC(struct TraceData, l->tbl, l->cap);
  memset(l->tbl + org_size, TLOG_INIT_DATA, sizeof(struct TraceData) * (l->cap - org_size));
}

/* ��l��Υ���key���֤ˤ���TraceLog���Ф���, ˬ���Ͽ����.
 * matched_id�ϥޥå������ץ�����ID, owner�Ͻ�°��.
 * ��°�줬NULL�Ǥʤ�����, ��°��ξ����Ͽ��, ��°��¦�Υץ���ˬ�䥫���󥿤��.
 * tracelog_put_atom, tracelog_put_mem����ƤӽФ��ؿ��Ǥ���, ľ��call���ʤ�����.
 * (�ƽФ������Ⱥ︺�Τ���˸����ؿ��Ȥ��Ƥ������) */
static inline int tracelog_put(TraceLog l, LmnWord key, LmnWord matched_id,
                               LmnMembrane *owner) {
  if (l->cap <= key) {
    tracelog_tbl_expand(l, key);
  } else if (TLOG_IS_TRV(TLOG_FLAG(l->tbl[key]))) {
    return 0;
  }

  l->num++;
  TLOG_SET_MATCHED(l->tbl[key], matched_id);

  if (owner) {
    TLOG_SET_OWNER(l->tbl[key], owner);
    LMN_ASSERT(l->cap > lmn_mem_id(owner));
    TLOG_TRV_INC(l->tbl[lmn_mem_id(owner)]);
  }

  LogTracker_TRACE(&l->tracker, key);

  return 1;
}

/* ��l��, ��°��owner1�Υ��ȥ�atom1�ؤ�ˬ���Ͽ����.
 * atom1�˥ޥå��������ȥ�Υץ���ID�⤷����ˬ���ֹ�atom2_id��ʻ���Ƶ�Ͽ����. */
static inline int tracelog_put_atom(TraceLog l, LmnSAtom atom1, LmnWord  atom2_id,
                                    LmnMembrane *owner1) {
  int ret = tracelog_put(l, LMN_SATOM_ID(atom1), atom2_id, owner1);
  TLOG_SET_TRV_ATOM(TLOG_FLAG(l->tbl[LMN_SATOM_ID(atom1)]));
  return ret;
}

/* ��l��, ��mem1�ؤ�ˬ���Ͽ����. (��°���mem1�Υ��Ф��黲�Ȥ��뤿������)
 * mem1�˥ޥå�������Υץ���ID�⤷����ˬ���ֹ�mem2_id��ʻ���Ƶ�Ͽ���� */
static inline int tracelog_put_mem(TraceLog l, LmnMembrane *mem1, LmnWord mem2_id) {
  int ret = tracelog_put(l, lmn_mem_id(mem1), mem2_id, lmn_mem_parent(mem1));
  TLOG_SET_TRV_MEM(TLOG_FLAG(l->tbl[lmn_mem_id(mem1)]));
  return ret;
}

/* ��l��, �ϥ��ѡ�����դΥ롼�ȥ��֥�������hl1�ؤ�ˬ���Ͽ����.
 * (�ϥ��ѡ�����չ�¤�ˤϽ�°��γ�ǰ���ʤ�, �쥪�֥������Ȥ���λ��Ȥ�Ǥ��ʤ�����,
 *  ��°����Ф�����ڤ���������)
 * hl1�˥ޥå������ϥ��ѥ�󥯥��֥�������ID�⤷����ˬ���ֹ�hl2_id��ʻ���Ƶ�Ͽ���� */
static inline int tracelog_put_hlink(TraceLog l, HyperLink *hl1, LmnWord hl2_id) {
  int ret = tracelog_put(l, LMN_HL_ID(hl1), hl2_id, NULL);
  TLOG_SET_TRV_HLINK(TLOG_FLAG(l->tbl[LMN_HL_ID(hl1)]));
  return ret;
}

static inline void tracelog_unput(TraceLog l, LmnWord key) {
  LMN_ASSERT (TLOG_IS_TRV(TLOG_FLAG(l->tbl[key]))); /* ˬ��ѤߤǤ�ʤ��ʤ��Τ�unput�����num���ͤ������ˤʤ����� */
  if (l->cap > key) {
    l->num--;
    TLOG_TRV_DEC(l->tbl[TLOG_OWNER(l->tbl[key])]);
    TLOG_DATA_CLEAR(l->tbl[key]);
  }
}

static inline BOOL tracelog_contains(TraceLog l, LmnWord key) {
  return (l->cap > key) && TLOG_IS_TRV(TLOG_FLAG(l->tbl[key]));
}

static inline BOOL tracelog_contains_atom(TraceLog l, LmnSAtom atom) {
  return tracelog_contains(l, LMN_SATOM_ID(atom));
}

static inline BOOL tracelog_contains_mem(TraceLog l, LmnMembrane *mem) {
  return tracelog_contains(l, lmn_mem_id(mem));
}

static inline BOOL tracelog_contains_hlink(TraceLog l, HyperLink *hl) {
  return tracelog_contains(l, LMN_HL_ID(hl));
}

static inline LmnWord tracelog_get_matched(TraceLog l, LmnWord key) {
  return TLOG_MATCHED(l->tbl[key]);
}

static inline LmnWord tracelog_get_atomMatched(TraceLog l, LmnSAtom atom) {
  return tracelog_get_matched(l, LMN_SATOM_ID(atom));
}

static inline LmnWord tracelog_get_memMatched(TraceLog l, LmnMembrane *mem) {
  return tracelog_get_matched(l, lmn_mem_id(mem));
}

static inline LmnWord tracelog_get_hlinkMatched(TraceLog l, HyperLink *hl) {
  return tracelog_get_matched(l, LMN_HL_ID(hl));
}

static inline BYTE tracelog_get_matchedFlag(TraceLog l, LmnWord key) {
  return TLOG_FLAG(l->tbl[key]);
}

static inline void tracelog_backtrack(TraceLog l) {
  LogTracker_REVERT(&l->tracker, tracelog_unput, l);
}

static inline void tracelog_set_btpoint(TraceLog l) {
  LogTracker_PUSH(&l->tracker);
}

static inline void tracelog_continue_trace(TraceLog l) {
  LogTracker_POP(&l->tracker);
}


/** ------
 *  SimpleTraceLog
 */

struct SimplyTraceLog {
  struct SimplyProcTbl tbl; /* Process ID��key, ˬ��Ѥߤ��ݤ��ο����ͤ�value�Ȥ����ơ��֥� */
  struct LogTracker tracker;
};

#define STRACE_TRUE   (!SPROC_TBL_INIT_V)

typedef struct SimplyTraceLog *SimplyLog;

/**
 * Function ProtoTypes
 */

void simplylog_init(SimplyLog trc);
inline void simplylog_init_with_size(SimplyLog trc, unsigned long size);
void simplylog_destroy(SimplyLog trc);

static inline void simplylog_put(SimplyLog l, LmnWord key);
static inline void simplylog_put_atom(SimplyLog l, LmnSAtom atom);
static inline void simplylog_put_mem(SimplyLog l, LmnMembrane *mem);
static inline BOOL simplylog_contains_atom(SimplyLog l, LmnSAtom atom);
static inline BOOL simplylog_contains_mem(SimplyLog l, LmnMembrane *mem);
static inline void simplylog_backtrack(SimplyLog l);
static inline void simplylog_set_btpoint(SimplyLog l);
static inline void simplylog_continue_trace(SimplyLog l);

/**
 * Inline Functions
 */
static inline void simplylog_put(SimplyLog l, LmnWord key)
{
  LogTracker_TRACE(&l->tracker, key);
  sproc_tbl_put(&l->tbl, key, STRACE_TRUE);
}

static inline void simplylog_put_atom(SimplyLog l, LmnSAtom atom) {
  simplylog_put(l, LMN_SATOM_ID(atom));
}

static inline void simplylog_put_mem(SimplyLog l, LmnMembrane *mem) {
  simplylog_put(l, lmn_mem_id(mem));
}

static inline BOOL simplylog_contains_atom(SimplyLog l, LmnSAtom atom) {
  return sproc_tbl_contains_atom(&l->tbl, atom);
}

static inline BOOL simplylog_contains_mem(SimplyLog l, LmnMembrane *mem) {
  return sproc_tbl_contains_mem(&l->tbl, mem);
}

static inline void simplylog_backtrack(SimplyLog l) {
  LogTracker_REVERT(&l->tracker, sproc_tbl_unput, &l->tbl);
}

static inline void simplylog_set_btpoint(SimplyLog l) {
  LogTracker_PUSH(&l->tracker);
}

static inline void simplylog_continue_trace(SimplyLog l) {
  LogTracker_POP(&l->tracker);
}


/*----------------------------------------------------------------------
 * Visit Log
 */

/* VisitLog - Ʊ����Ƚ��䡢ID�ʤɥХå��ȥ�å��򤷤ʤ��饰��դ�õ����������Ѥ���.
 * ���ȥ����Υ��ؤ��ɲû��ˤ��ɲý�˼�ưŪ���ֹ���ղä���.
 * �����å��ݥ���Ȥ�Ȥ����Ȥ�, �Хå��ȥ�å����˥���Хå��ȥ�å����ξ��֤˸����᤹���Ȥ��Ǥ���.
 */

/* VisitLog�˵�Ͽ���줿�ѹ��Υ��ʥåץ���å� */
struct Checkpoint {
  int n_data_atom;
  Vector elements;
};

/* ˬ��ѤߤΥ��ȥ����ε�Ͽ */
struct VisitLog {
  struct ProcessTbl tbl;         /* �ץ���ID��key�ˤ���ˬ��ɽ */
  int               ref_n,       /* �Х����󤫤��ɤ߽Ф����ץ����˺�ˬ�䤬ȯ���������Τ���λ����ֹ���������� */
                    element_num; /* ˬ�䤷���ץ������Υ����� */
  Vector            checkpoints; /* Checkpoint���֥������Ȥ����� */
};

typedef struct VisitLog    *VisitLog;
typedef struct Checkpoint  *Checkpoint;


/**
 * Function ProtoTypes
 */

void checkpoint_free(Checkpoint cp);

void visitlog_init_with_size(VisitLog p, unsigned long tbl_size);
void visitlog_destroy(VisitLog p);
void visitlog_set_checkpoint(VisitLog visitlog);
Checkpoint visitlog_pop_checkpoint(VisitLog visitlog);
void visitlog_revert_checkpoint(VisitLog visitlog);
void visitlog_commit_checkpoint(VisitLog visitlog);
void visitlog_push_checkpoint(VisitLog visitlog, Checkpoint cp);

static inline int  visitlog_put(VisitLog visitlog, LmnWord p);
static inline int  visitlog_put_atom(VisitLog visitlog, LmnSAtom atom);
static inline int  visitlog_put_mem(VisitLog visitlog, LmnMembrane *mem);
static inline int  visitlog_put_hlink(VisitLog visitlog, HyperLink *hl);
static inline void visitlog_put_data(VisitLog visitlog);
static inline int  visitlog_get_atom(VisitLog visitlog, LmnSAtom atom, LmnWord *value);
static inline int  visitlog_get_mem(VisitLog visitlog, LmnMembrane *mem, LmnWord *value);
static inline int  visitlog_get_hlink(VisitLog visitlog, HyperLink *hl, LmnWord *value);
static inline int  visitlog_element_num(VisitLog visitlog);


/**
 * Inline Functions
 */

/* ����p���ɲä�, �����ͤ��֤�. ���Ǥ�p��¸�ߤ�������0���֤�.
 * �̾盧�δؿ��ǤϤʤ�put_atom, put_mem����Ѥ���. */
static inline int visitlog_put(VisitLog visitlog, LmnWord p) {
  if (proc_tbl_put_new(&visitlog->tbl, p, visitlog->ref_n++)) {
    if (vec_num(&visitlog->checkpoints) > 0) {
      Checkpoint checkpoint = (Checkpoint)vec_last(&visitlog->checkpoints);
      vec_push(&checkpoint->elements, p);
    }
    visitlog->element_num++;
    return 1;
  } else {
    return 0;
  }
}

/* ���˥��ȥ���ɲä�, �����ͤ��֤�. ���Ǥ˥��ȥब¸�ߤ�������0���֤� */
static inline int visitlog_put_atom(VisitLog visitlog, LmnSAtom atom) {
  return visitlog_put(visitlog, LMN_SATOM_ID(atom));
}

/* ��������ɲä�, �����ͤ��֤�. ���Ǥ��줬¸�ߤ�������0���֤� */
static inline int visitlog_put_mem(VisitLog visitlog, LmnMembrane *mem) {
  return visitlog_put(visitlog, lmn_mem_id(mem));
}

/* ���˥ϥ��ѡ���󥯤��ɲä�, �����ͤ��֤�. ���Ǥ˥ϥ��ѡ���󥯤�¸�ߤ�������0���֤� */
static inline int visitlog_put_hlink(VisitLog visitlog, HyperLink *hl)
{
  return visitlog_put(visitlog, LMN_HL_ID(hl));
}

/* ���˥ǡ������ȥ���ɲä���.
 * �ʰ�����������̵�����Ȥ���ʬ����褦��, ñ��ˬ�䤷�����ȥ������뤿��˻��Ѥ���� */
static inline void visitlog_put_data(VisitLog visitlog) {
  if (vec_num(&visitlog->checkpoints) > 0) {
    struct Checkpoint *checkpoint = (struct Checkpoint *)vec_last(&visitlog->checkpoints);
    checkpoint->n_data_atom++;
  }
  visitlog->element_num++;
}

/* ���˵�Ͽ���줿���ȥ�atom���б������ͤ�value�����ꤷ, �����ͤ��֤�.
 * ����atom��¸�ߤ��ʤ�����, 0���֤�. */
static inline int visitlog_get_atom(VisitLog visitlog, LmnSAtom atom, LmnWord *value) {
  return proc_tbl_get_by_atom(&visitlog->tbl, atom, value);
}

/* ���˵�Ͽ���줿��mem���б������ͤ�value������, �����ͤ��֤�.
 * ����mem��¸�ߤ��ʤ�����, 0���֤�. */
static inline int visitlog_get_mem(VisitLog visitlog, LmnMembrane *mem, LmnWord *value) {
  return proc_tbl_get_by_mem(&visitlog->tbl, mem, value);
}

/* ���˵�Ͽ���줿hl���б������ͤ�value�����ꤷ, �����ͤ��֤�.
 * ����hl��¸�ߤ��ʤ�����, 0���֤�. */
static inline int visitlog_get_hlink(VisitLog visitlog, HyperLink *hl, LmnWord *value)
{
  return proc_tbl_get_by_hlink(&visitlog->tbl, hl, value);
}

/* visitlog�˵�Ͽ�������ǡ��졢���ȥ�ˤο����֤� */
static inline int visitlog_element_num(VisitLog visitlog) {
  return visitlog->element_num;
}


#endif
