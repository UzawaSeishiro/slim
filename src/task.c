/*
 * task.c
 *
 *   Copyright (c) 2008, Ueda Laboratory LMNtal Group <lang@ueda.info.waseda.ac.jp>
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
 * $Id: task.c,v 1.37 2008/10/21 11:31:58 riki Exp $
 */


#include "atom.h"
#include "dumper.h"
#include "instruction.h"
#include "lmntal.h"
#include "membrane.h"
#include "rule.h"
#include "task.h"
#include "vector.h"
#include "symbol.h"
#include "functor.h"
#include <stdio.h>
#include "st.h"
#include "mc.h"
#include "mhash.h"
#include <string.h>

LmnWord *wt, *wt_t; /* variable vector used in interpret */
LmnByte *at, *at_t; /* attribute vector */
unsigned int wt_size;

LmnMembrane *global_root;   /* for tracer */
unsigned int trace_num = 0; /* for tracer */

#define SWAP(T,X,Y)       do { T t=(X); (X)=(Y); (Y)=t;} while(0)
#define READ_VAL(T,I,X)      ((X)=*(T*)(I), I+=sizeof(T))

/*
  Java�ˤ������ϤǤϥ�󥯤ϥ�󥯥��֥������Ȥ�ɽ�����뤬��SLIM�Ǥ�
  ��󥯥��֥������ȤϤʤ�ľ�ܥ����Υ��ȥ�Ȱ����ֹ�ǥ�󥯤�ɽ����
  ������������SLIM�μ�����ˡ�Ǥϡ�a(L,L).  a(X,Y) :- b(X), b(Y). �ξ��
  ��2.�Τ褦�ˤʤꤦ�ޤ������ʤ�������ϡ�GETLINK��������󥯤��ͤ�
  INHERIT_LINK �ˤ���󥯤ΤĤʤ��ؤ��β����ǹ�������ʤ������������
  �ǡ�3.�Τ褦�ˤ���GETLINK �ǤΥ����μ�����ºݤˤ��ٱ䤵����褦
  �ˤ��������ư�����Ƥ�ͤ�����

  ���ߤ�3.����ˡ���äƤ��롣����Ū�ˤ� wt��at�˥����Υ��ȥ�Ȱ���
  �ֹ�ǤϤʤ���󥯸��Υ��ȥ�Ȱ����ֹ���ݻ����롣����ˡ���󥯸���
  �ݻ����Ƥ�����ˤ�wt����ͤβ���1�ӥåȤ�1�ˤ����������ݻ�����
  ������ȶ��̤򤷤Ƥ��롣�ɤ���ξ���Ʊ���ޥ����Ȥ�Ʊ�ͤΥ�����
  �ǥ����μ������Ф���褦�ˤ��Ƥ��롣

  1. ��󥯥��֥������Ȥ�������(Java�ˤ�������)

    0--+       0----b       0 +--b       +--b
    a  |  =>   a       =>   a |      =>  |
    1--+       1            1 +--b       +--b

  2. ��󥯥��֥������Ȥ��ʤ����(SLIM)
     A,B��GETLINK��������������ɽ���Ƥ���

                  (B�����ؤ�)    (A�����ؤ�)    (�����ʾ���)
      A                A                  A               A
      |                |             b--> |          b--> |
  +---0<---+       +---0              <---0
  |   a    |  =>   |   a      =>          a      =>
  +-->1----+       +-->1--->              1--->
      |                |<-- b             |<-- b          |<-- b
      B                B                  B               B


  3. ��󥯥��֥������Ȥ��ʤ���硢��󥯤ϥ�󥯸����ݻ�(SLIM)
      A,B��GETLINK��Ԥä��ݤΥ�󥯸��ξ������ġ��ºݤΥ�����
      ��󥯤ΤĤʤ��ؤ���Ԥ��ݤ����롣

             (B�Υ���������)                (A�Υ���������)  a������
                                 +---------+      +---------+       +---------+-+      +-----------+
                                 |         |      |         |       |         | |      |           |
    B                B           v     B   |      v     B   |       v     B   | |      v           |
    |            b-->|           b     |   |      b     |   |       b     |   | |      b           |
  +---0<--+       +---0<--+      | +---0   |      | +---0   |       | +---0   | |      |           |
  |   a   |  =>   |   a   |  =>  | |   a   |  =>  | |   a   |  =>   | |   a   | | =>   |           |
  +-->1---+       +-->1---+      +-+-->1---+      +-+-->1---+       | +-->1---+ |      |           |
        |              |                |                |<--b      +------|--->b      +---------->b
        A              A                A                A                 A
*/

/* �����Υ��ȥ������ */
#define LINKED_ATOM(LINKI) wt[LINKI]
/* �����Υ��ȥ�ΰ�����attribute������ */
#define LINKED_ATTR(LINKI) at[LINKI]

/* �쥹���å� */
struct Entity {
  LmnMembrane  *mem;
  struct Entity  *next;
};

struct MemStack {
  struct Entity  *head;
} memstack;

static BOOL interpret(LmnRuleInstr instr, LmnRuleInstr *next);

static void memstack_init()
{
  memstack.head = LMN_MALLOC(struct Entity);
  memstack.head->next = NULL;
  memstack.head->mem = NULL;
}

static int memstack_isempty()
{
  return memstack.head->next==NULL;
}

static void memstack_destroy()
{
  LMN_ASSERT(memstack_isempty());
  LMN_FREE(memstack.head);
}

void memstack_push(LmnMembrane *mem)
{
  struct Entity *ent = LMN_MALLOC(struct Entity);
  ent->mem = mem;
  ent->next = memstack.head->next;
  memstack.head->next = ent;
  mem->is_activated = TRUE;
}

static LmnMembrane* memstack_pop()
{
  struct Entity *ent = memstack.head->next;
  LmnMembrane *mem = ent->mem;
  memstack.head->next = ent->next;
  mem->is_activated = FALSE;
  LMN_FREE(ent);
  return mem;
}

static LmnMembrane* memstack_peek()
{
  return memstack.head->next->mem;
}

/* DEBUG: */
/* static void memstack_printall() */
/* { */
/*   struct Entity *ent; */
/*   for(ent = memstack.head; ent!=NULL; ent = ent->next){ */
/*     if(ent->mem!=NULL)printf("%d ", ent->mem->name); */
/*   } */
/*   printf("\n"); */
/* } */

/* MC �ǤΤ߻��Ѥδؿ����ǡ�����¤����� ��������  */
/*----------------------------------------------------------------------*/

/* prototypes */
LMN_EXTERN void nd_exec(void);
LMN_EXTERN void ltl_search1(void);

/* �����Ū�¹Ԥ��Ѥ���ե饰���� */
McFlags mc_flags;

/* �ե饰�������������(c.f. mc.h) */
static inline void init_mc_flags(void) {
  mc_flags.nd_exec = FALSE;
  mc_flags.system_rule_committed = FALSE;
  mc_flags.system_rule_proceeded = FALSE;
  mc_flags.property_rule = FALSE;
}

static struct st_hash_type type_statehash = {
  state_cmp,
  state_hash
};

/* ˬ��ѤߥΡ��ɤ���Ǽ����� */
static st_table *States;

/* õ���ѥ����å� */
static Vector Stack;

/* Ÿ�����줿�ҥΡ��ɤ���Ǽ����� */
static st_table *expanded;

/* for LTL */
LmnMembrane *seed = NULL; /* root of second DFS */

/* �����Х�롼����򥳥ԡ����� */
static inline SimpleHashtbl *copy_global_root(LmnMembrane *srcmem, LmnMembrane *dstmem) {
  unsigned int i;
  SimpleHashtbl *copymap = lmn_mem_copy_cells(dstmem, srcmem);
  for (i = 0; i < srcmem->rulesets.num; i++) {
    vec_push(&dstmem->rulesets, vec_get(&srcmem->rulesets, i));
  }
  return copymap;
}

/* LTL��ǥ븡���������Ū�¹Ի���mem�����mem�����Ĥ���������� */
static inline void activate(LmnMembrane *mem) {
  if (!mc_flags.property_rule) {
    activate_ancestors(mem);
  }
}

/**
 * �����(--nd �ޤ��� --nd_result)�¹Խ�λ���˾������ܥ���դ���Ϥ��롥
 * �ⳬ�ؿ�st_foreach(c.f. st.c)���ꤲ�ƻ��ѡ�
 */
static int print_state_transition_graph(st_data_t _k, st_data_t state_ptr, st_data_t _a) {
  unsigned int j = 0;
  State *tmp = (State *)state_ptr;

  fprintf(stdout, "%lu::", (long unsigned int)tmp); /* dump src state's ID */
  while (j < vec_num(&tmp->successor)) { /* dump dst state's IDs */
    fprintf(stdout, "%lu", vec_get(&tmp->successor, j++));
    if (j < vec_num(&tmp->successor)) {
      fprintf(stdout,",");
    }
  }
  fprintf(stdout, "::");
  lmn_dump_cell(tmp->mem); /* dump src state's global root membrane */
  return ST_CONTINUE;
}

/**
 * �����¹� or LTL��ǥ븡����λ���States���¸�ߤ����������򤹤٤�free����
 * �ⳬ�ؿ�st_foreach(c.f. st.c)���ꤲ�ƻ���
 */
static int kill_States_chains(st_data_t _k, st_data_t state_ptr, st_data_t rm_tbl_ptr) {
  State *tmp = (State *)state_ptr;
  HashSet *rm_tbl = (HashSet *)rm_tbl_ptr;

  if(hashset_contains(rm_tbl, (HashKeyType)tmp->mem)) {
    vec_destroy(&tmp->successor);
    LMN_FREE(tmp);
  }
  else {
    hashset_add(rm_tbl, (HashKeyType)tmp->mem);
    state_free(tmp);
  }
  return ST_CONTINUE;
}
/*----------------------------------------------------------------------*/
/* MC �ǤΤ߻��Ѥδؿ����ǡ�����¤����� �����ޤ� */

/* ��mem��ruleset�Υ롼���Ŭ�Ѥ��ߤ롣Ŭ�Ѥ������ä����TRUE���֤���
   ������ʤ��ä����ˤ�FALSE���֤��� */
static BOOL react_ruleset(LmnMembrane *mem, LmnRuleSet ruleset)
{
  int i;
  LmnRuleInstr dummy;
  int n;
  LmnRule rule;
  LmnTranslated translated;
  BYTE *inst_seq;

  wt[0] = (LmnWord)mem;
  n = lmn_ruleset_rule_num(ruleset);
  for (i = 0; i < n; i++) {
    rule = lmn_ruleset_get_rule(ruleset, i);
    translated = lmn_rule_get_translated(rule);
    inst_seq = lmn_rule_get_inst_seq(rule);

    /* �ޤ����ȥ�󥹥졼�ȺѤߤδؿ���¹Ԥ���
       ���줬�ʤ����̿�����interpret�Ǽ¹Ԥ��� */
    if ((translated &&  translated(mem)) ||
        (inst_seq && interpret(inst_seq, &dummy))) {
      if (lmn_env.trace) {
        fprintf(stdout, "(%s)\n\n", lmn_id_to_name(lmn_rule_get_name(rule)));
      }
      return TRUE;
    }
  }

  return FALSE;
}

/* ��󥯥��֥������Ȥ����� */
typedef struct LinkObj {
  LmnWord ap;
  LmnLinkAttr pos;
} LinkObj;

static LinkObj *LinkObj_make(LmnWord ap, LmnLinkAttr pos) {
  LinkObj* ret = LMN_MALLOC(LinkObj);
  ret->ap = ap;
  ret->pos = pos;
  return ret;
}

static int exec(LmnMembrane *mem)
{
  unsigned int i;
  int flag = FALSE;
/*     flag = react(mem, systemrulesets); */
  if (!flag) {

    for (i = 0; i < mem->rulesets.num; i++) {
      if (react_ruleset(mem, (LmnRuleSet)vec_get(&mem->rulesets, i))) {
        flag = TRUE;
        break;
      }
    }
  }

  return flag;
}

void lmn_run(LmnRuleSet start_ruleset)
{
  LmnMembrane *mem;

  /* Initialize for running */
  wt_size = 1024;
  wt = LMN_NALLOC(LmnWord, wt_size);
  wt_t = LMN_NALLOC(LmnWord, wt_size);
  at = LMN_NALLOC(LmnByte, wt_size);
  at_t = LMN_NALLOC(LmnByte, wt_size);

  /* make global root membrane */
  mem = lmn_mem_make();

  /* �̾�¹Ի� */
  if (!lmn_env.nd && !lmn_env.ltl) {
    memstack_init();
    memstack_push(mem);
  }
  /* LTL��ǥ븡���������Ū�¹Ի� */
  else {
    init_mc_flags();
    activate(mem);
  }

  /* for tracer */
  global_root = mem;

  /* initialize rule */
  react_ruleset(mem, start_ruleset);

  /* �̾�¹Ի� */
  if (!lmn_env.nd && !lmn_env.ltl) {
    while(!memstack_isempty()){
      LmnMembrane *mem = memstack_peek();
      LmnMembrane *m;
      if(!exec(mem)) {
        if (!react_ruleset(mem, system_ruleset)) {
          /* �롼�뤬����Ŭ�Ѥ���ʤ�����쥹���å�������Ƭ������� */
          m = memstack_pop(&memstack);
        }
      }
    }

    memstack_destroy();

    /* ����� */
    lmn_dump_cell(mem);
    lmn_mem_drop(mem);
    lmn_mem_free(mem);
    LMN_FREE(wt);
    LMN_FREE(wt_t);
    LMN_FREE(at);
    LMN_FREE(at_t);
    free_atom_memory_pools();
  }
  /* LTL��ǥ븡���������Ū�¹Ի� */
  else {
    lmn_mc_nd_run(mem);
  }
}

void lmn_mc_nd_run(LmnMembrane *mem) {
  State *initial_state;
  unsigned int i;

  for (i = 0; i < wt_size; i++) {
    wt[i] = at[i] = 0;
  }

  /**
   * initialize containers
   */
  States = st_init_table(&type_statehash);
  vec_init(&Stack, 2048);
  expanded = st_init_table(&type_statehash);

  /* ����ץ������������������֤����� */
  initial_state = state_make(mem);
  st_add_direct(States, (st_data_t)initial_state, (st_data_t)initial_state);
  vec_push(&Stack, (LmnWord)initial_state);

  /* ������롼�����̤��뤿�� */
  mc_flags.nd_exec = TRUE;

  /* �����Ū�¹� */
  if(lmn_env.nd) {
    nd_exec();

    /* �������ܥ���դ���Ϥ��� */
    if (!lmn_env.nd_result) {
      st_foreach(States, print_state_transition_graph, 0);
    }
  }
  /* LTL��ǥ븡�� */
  else {
    set_fst(initial_state);
    ltl_search1();
    printf("no cycles found\n");
  }

  fprintf(stdout, "# of States = %d\n", States->num_entries);

  /* finalize */
  {
    HashSet rm_tbl; /* LTL��ǥ븡���⡼�ɻ�����Ų������ɻߤ��뤿�� */
    hashset_init(&rm_tbl, 16);
    st_foreach(States, kill_States_chains, &rm_tbl);
    hashset_destroy(&rm_tbl);
  }

  st_free_table(States);
  vec_destroy(&Stack);
  st_free_table(expanded);
  LMN_FREE(wt);
  LMN_FREE(wt_t);
  LMN_FREE(at);
  LMN_FREE(at_t);
  free_atom_memory_pools();
}

/* Utility for reading data */

#define READ_DATA_ATOM(dest, attr)              \
  do {                                          \
    switch (attr) {                             \
    case LMN_INT_ATTR:                          \
       READ_VAL(int, instr, (dest));            \
       break;                                   \
     case LMN_DBL_ATTR:                         \
     {                                          \
        double *x;                              \
        x = LMN_MALLOC(double);                 \
        READ_VAL(double, instr, *x);            \
        (dest) = (LmnWord)x;                    \
        break;                                  \
     }                                          \
     default:                                   \
        lmn_fatal("Implementation error");      \
     }                                          \
   } while (0)

#define READ_CONST_DATA_ATOM(dest, attr)        \
  do {                                          \
    switch (attr) {                             \
    case LMN_INT_ATTR:                          \
       READ_VAL(int, instr, (dest));            \
       break;                                   \
     case LMN_DBL_ATTR:                         \
       (dest) = (LmnWord)instr;                 \
       instr += sizeof(double);                 \
       break;                                   \
     default:                                   \
        lmn_fatal("Implementation error");      \
     }                                          \
   } while (0)

#define READ_CMP_DATA_ATOM(attr, x, result)            \
       do {                                            \
          switch(attr) {                               \
          case LMN_INT_ATTR:                           \
            {                                          \
              int t;                                   \
              READ_VAL(int, instr, t);                 \
              (result) = ((int)(x) == t);              \
              break;                                   \
            }                                          \
          case LMN_DBL_ATTR:                           \
            {                                          \
              double t;                                \
              READ_VAL(double, instr, t);              \
              (result) = (*(double*)(x) == t);         \
              break;                                   \
            }                                          \
          default:                                     \
            lmn_fatal("Implementation error");         \
          }                                            \
          } while (0)

/* DEBUG: */
/* static void print_wt(void); */

/* mem != NULL �ʤ�� mem��UNIFY���ɲá������Ǥʤ����
   UNIFY����˽�°���ʤ� */
static HashSet *insertconnectors(LmnMembrane *mem, Vector *links)
{
  unsigned int i, j;
  HashSet *retset = hashset_make(8);
  /* EFFICIENCY: retset��Hash Set�Ǥ����̣��?���٥����Ǥ����ΤǤϡ�
     ���̿��ǥ��åȤ�Ȥ��褦�˽񤫤�Ƥ��� */

  for(i = 0; i < links->num; i++) {
    LmnWord linkid1 = vec_get(links, i);
    if (LMN_ATTR_IS_DATA(LINKED_ATTR(linkid1))) continue;
    for(j = i+1; j < links->num; j++) {
      LmnWord linkid2 = vec_get(links, j);
      if (LMN_ATTR_IS_DATA(LINKED_ATTR(linkid2))) continue;
      /* is buddy? */
      if (LINKED_ATOM(linkid2) == LMN_ATOM_GET_LINK(LINKED_ATOM(linkid1), LINKED_ATTR(linkid1)) &&
          LINKED_ATTR(linkid2) == LMN_ATOM_GET_ATTR(LINKED_ATOM(linkid1), LINKED_ATTR(linkid1))) {
        /* '='���ȥ��Ϥ��� */
        LmnAtomPtr eq;
        LmnAtomPtr a1, a2;
        LmnLinkAttr t1, t2;

        if (mem) eq = lmn_mem_newatom(mem, LMN_UNIFY_FUNCTOR);
        else {
          eq = lmn_new_atom(LMN_UNIFY_FUNCTOR);
        }

        /* ��󥯤���󥯤θ�����ľ�硢���餫��������μ����򤷤Ƥ��ʤ����
           �ʤ�ʤ�����󥯸���new_link���˽񤭴������Ƥ��ޤ���*/

        a1 = LMN_ATOM(LINKED_ATOM(linkid1));
        a2 = LMN_ATOM(LINKED_ATOM(linkid2));
        t1 = LINKED_ATTR(linkid1);
        t2 = LINKED_ATTR(linkid2);

        lmn_newlink_in_symbols(a1, t1, eq, 0);
        lmn_newlink_in_symbols(a2, t2, eq, 1);
        hashset_add(retset, (HashKeyType)eq);
      }
    }
  }

  return retset;
}

static BOOL interpret(LmnRuleInstr instr, LmnRuleInstr *next_instr)
{
/*   LmnRuleInstr start = instr; */
  LmnInstrOp op;
  while (TRUE) {
  LOOP:;
    READ_VAL(LmnInstrOp, instr, op);
/*     fprintf(stderr, "op: %d %d\n", op, (instr - start)); */
/*     lmn_dump_mem((LmnMembrane*)wt[0]); */
    switch (op) {
    case INSTR_SPEC:
    {
      LmnInstrVar s0, s1;

      READ_VAL(LmnInstrVar, instr, s0);
      READ_VAL(LmnInstrVar, instr, s1);

      /* extend vector if need */
      if (s1 > wt_size) {
        wt_size = s1;
        wt = LMN_REALLOC(LmnWord, wt, wt_size);
        at = LMN_REALLOC(LmnLinkAttr, at, wt_size);
      }
      break;
    }
    case INSTR_INSERTCONNECTORSINNULL:
    {
      LmnInstrVar seti, list_num;
      Vector links;
      unsigned int i;

      READ_VAL(LmnInstrVar, instr, seti);
      READ_VAL(LmnInstrVar, instr, list_num);

      vec_init(&links, list_num); /* TODO: vector_init�λ����ѹ���ȼ���ѹ����� */
      for (i = 0; i < list_num; i++) {
        LmnInstrVar t;
        READ_VAL(LmnInstrVar, instr, t);
        vec_push(&links, (LmnWord)t); /* TODO: vector_init�λ����ѹ���ȼ���ѹ�����(����ǥå�������������) */
      }

      wt[seti] = (LmnWord)insertconnectors(NULL, &links);
      if (lmn_env.nd || lmn_env.ltl) { at[seti] = 0; /* MC */ }
      vec_destroy(&links);

      /* EFFICIENCY: �����Τ���κƵ� */
      if(interpret(instr, next_instr)) {
        hashset_free((HashSet *)wt[seti]);
        return TRUE;
      }
      /**
       * MC -->
       * ��������Τ���FALSE���֤�
       */
      else if(mc_flags.nd_exec && !mc_flags.property_rule) {
        hashset_free((HashSet *)wt[seti]);
        return FALSE;
      }
      /**
       * <-- MC
       */
      else assert(0);
      break;
    }
    case INSTR_INSERTCONNECTORS:
    {
      LmnInstrVar seti, list_num, memi, enti;
      Vector links; /* src list */
      unsigned int i;

      READ_VAL(LmnInstrVar, instr, seti);
      READ_VAL(LmnInstrVar, instr, list_num);

      vec_init(&links, list_num);

      for (i = 0; i < list_num; i++) {
        READ_VAL(LmnInstrVar, instr, enti);
        vec_push(&links, (LmnWord)enti); /* TODO: vector_init�λ����ѹ���ȼ���ѹ����� */
      }

      READ_VAL(LmnInstrVar, instr, memi);
      wt[seti] = (LmnWord)insertconnectors((LmnMembrane *)wt[memi], &links);
      if (lmn_env.nd || lmn_env.ltl) { at[seti] = 0; /* MC */ }
      vec_destroy(&links);

      /* EFFICIENCY: �����Τ���κƵ� */
      if(interpret(instr, next_instr)) {
        hashset_free((HashSet *)wt[seti]);
        return TRUE;
      }
      /**
       * MC -->
       * ��������Τ���FALSE���֤�
       */
      else if(mc_flags.nd_exec && !mc_flags.property_rule) {
        hashset_free((HashSet *)wt[seti]);
        return FALSE;
      }
      /**
       * <-- MC
       */
      break;
    }
    case INSTR_JUMP:
    {
      /* EFFICIENCY: �����˺�������malloc���Ƥ���Τ������٤�
                     -O3 ��������������̿���JUMP���ޤޤ�ʤ�����
                     ����Ǥ�褤 */
      LmnInstrVar num, i, n;
      LmnJumpOffset offset;
      BOOL ret;
      LmnWord *wt_org = wt;
      LmnByte *at_org = at;
      LmnWord *wt2 = LMN_NALLOC(LmnWord, wt_size);
      LmnByte *at2 = LMN_NALLOC(LmnByte, wt_size);
      /**
       * MC -->
       */
      if (lmn_env.nd || lmn_env.ltl) {
        for(i = 0; i < wt_size; i++) {
          wt2[i] = at2[i] = 0;
        }
      }
      /**
       * <-- MC
       */
      unsigned int wt_size_org = wt_size;
      LmnRuleInstr next;

      READ_VAL(LmnJumpOffset, instr, offset);
      next = instr + offset;

      i = 0;
      /* atom */
      READ_VAL(LmnInstrVar, instr, num);
      for (; num--; i++) {
        READ_VAL(LmnInstrVar, instr, n);
        wt2[i] = wt[n];
        at2[i] = at[n];
      }
      /* mem */
      READ_VAL(LmnInstrVar, instr, num);
      for (; num--; i++) {
        READ_VAL(LmnInstrVar, instr, n);
        wt2[i] = wt[n];
        at2[i] = at[n];
      }
      /* vars */
      READ_VAL(LmnInstrVar, instr, num);
      for (; num--; i++) {
        READ_VAL(LmnInstrVar, instr, n);
        wt2[i] = wt[n];
        at2[i] = at[n];
      }

      instr = next;

      wt = wt2;
      at = at2;
      wt_size = wt_size_org;

      ret = interpret(instr, next_instr);
      LMN_FREE(wt);
      LMN_FREE(at);
      wt = wt_org;
      at = at_org;
      wt_size = wt_size_org;
      return ret;
    }
    case INSTR_RESETVARS:
    {
      LmnInstrVar num, i, n, t;

      i = 0;
      /* atom */
      READ_VAL(LmnInstrVar, instr, num);
      for (; num--; i++) {
        READ_VAL(LmnInstrVar, instr, n);
        wt_t[i] = wt[n];
        at_t[i] = at[n];
      }

      /* mem */
      READ_VAL(LmnInstrVar, instr, num);
      for (; num--; i++) {
        READ_VAL(LmnInstrVar, instr, n);
        wt_t[i] = wt[n];
        at_t[i] = at[n];
      }

      /* vars */
      READ_VAL(LmnInstrVar, instr, num);
      for (; num--; i++) {
        READ_VAL(LmnInstrVar, instr, n);
        wt_t[i] = wt[n];
        at_t[i] = at[n];
      }

      for (t=0; t<=i; t++) {
        wt[t] = wt_t[t];
        at[t] = at_t[t];
      }
      break;
    }
    case INSTR_COMMIT:
    {
      instr += sizeof(lmn_interned_str) + sizeof(LmnLineNum);

      /*
       * MC mode
       *
       * �����Х��ѿ�global_root�˳�Ǽ����Ƥ��륰���Х�롼����򥳥ԡ����ơ�
       * ���Υ��ԡ����Ф��ƥܥǥ�̿���Ŭ�Ѥ��롥
       * ���κݤ��ѿ�����ξ���⥳�ԡ����Τ�Τ��饳�ԡ���Τ�ΤؤȽ񤭴����롥
       *
       * �������롼��ξ��
       *   �����롼��Ŭ�ѷ�̤�global_root�˳�Ǽ�����
       * �������ƥ�롼��ξ��
       *   �����ƥ�롼��Ŭ�ѷ�̤򿷤��ʾ��֤Ȥ�����������
       *
       * CONTRACT: COMMIT̿�����ã�����롼��ϥޥå��󥰸������������Ƥ���
       */
      if (mc_flags.nd_exec) {
        unsigned int i;

        /* �����Х�롼����Υ��ԡ� */
        LmnMembrane *tmp_global_root = lmn_mem_make();
        SimpleHashtbl *copymap = copy_global_root(global_root, tmp_global_root);

        /* �ѿ����󤪤��°������Υ��ԡ� */
        LmnWord *wtcp = LMN_NALLOC(LmnWord, wt_size);
        LmnByte *atcp = LMN_NALLOC(LmnByte, wt_size);
        for(i = 0; i < wt_size; i++) {
          wtcp[i] = atcp[i] = 0;
        }

        /* copymap�ξ�������ѿ������񴹤��� */
        for (i = 0; i < wt_size; i++) {
          atcp[i] = at[i];
          if(LMN_INT_ATTR == at[i]) { /* int�Τߥݥ��󥿤Ǥʤ����� */
            wtcp[i] = wt[i];
          }
          else if(hashtbl_contains(copymap, wt[i])) {
            wtcp[i] = hashtbl_get_default(copymap, wt[i], 0);
          }
          else if(wt[i] == (LmnWord)global_root) { /* �����Х�롼���� */
            wtcp[i] = (LmnWord)tmp_global_root;
          }
        }
        hashtbl_free(copymap);

        /* �ѿ����󤪤��°������򥳥ԡ������촹���� */
        SWAP(LmnWord *, wtcp, wt);
        SWAP(LmnByte *, atcp, at);

        if (!mc_flags.property_rule) { /* �����ƥ�롼�� */
          /* ���դ˽и�����PROCEED�ȱ��դ˽и�����PROCEED����̤��뤿�� */
          mc_flags.system_rule_committed = TRUE;

          /* global_root���Ф��ƥܥǥ���Ŭ�Ѥ��� */
          interpret(instr, &instr);

          State *new_state = state_make(tmp_global_root);
          State *state_on_table;
          if (!st_lookup(expanded, (st_data_t)new_state, (st_data_t *)&state_on_table)) {
            st_insert(expanded, (st_data_t)new_state, (st_data_t)new_state);
          }
          else { /* ���˽и��������� */
            state_free(new_state);
          }
        }
        else { /* �����롼�� */
          global_root = tmp_global_root;
          interpret(instr, &instr);
        }

        /* �ѿ����󤪤��°������򸵤��᤹�ʤ���ʤ����⡩�� */
        SWAP(LmnWord *, wtcp, wt);
        SWAP(LmnByte *, atcp, at);

        LMN_FREE(wtcp);
        LMN_FREE(atcp);

        /*
         * �����롼�롧return TRUE
         * �����ƥ�롼�롧return FALSE
         */
        return mc_flags.property_rule;
      }
      /* MC */
      break;
    }
    case INSTR_FINDATOM:
    {
      LmnInstrVar atomi, memi;
      LmnLinkAttr attr;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnLinkAttr, instr, attr);

      if (LMN_ATTR_IS_DATA(attr)) {
        fprintf(stderr, "I can not find data atoms.\n");
        assert(FALSE);
      } else { /* symbol atom */
        LmnFunctor f;
        AtomListEntry *atomlist_ent;
        LmnAtomPtr atom;

        READ_VAL(LmnFunctor, instr, f);
        atomlist_ent = lmn_mem_get_atomlist((LmnMembrane*)wt[memi], f);
        if (atomlist_ent) {
          at[atomi] = LMN_ATTR_MAKE_LINK(0);
          for (atom = atomlist_head(atomlist_ent);
               atom != lmn_atomlist_end(atomlist_ent);
               atom = LMN_ATOM_GET_NEXT(atom)) {
            if(LMN_ATOM_GET_FUNCTOR(atom)==LMN_RESUME_FUNCTOR)
              continue;
            wt[atomi] = (LmnWord)atom;
            if (interpret(instr, &instr)) {
              *next_instr = instr;
              return TRUE;
            }
          }
        }
       return FALSE;
      }
      break;
    }
    case INSTR_FINDATOM2:
    {
      LmnInstrVar atomi, memi, findatomid;
      LmnLinkAttr attr;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, findatomid);
      READ_VAL(LmnLinkAttr, instr, attr);
      if (LMN_ATTR_IS_DATA(attr)) {
        fprintf(stderr, "I can not find data atoms.\n");
        assert(FALSE);
      } else { /* symbol atom */
        LmnFunctor f;
        AtomListEntry *atomlist_ent;
        LmnAtomPtr atom, record;

        READ_VAL(LmnFunctor, instr, f);
        atomlist_ent = lmn_mem_get_atomlist((LmnMembrane*)wt[memi], f);
        if (atomlist_ent) {
          at[atomi] = LMN_ATTR_MAKE_LINK(0);
          /*
           * TODO: (TOFIX)64bit�Ķ���warning���Ф�
           */
          record = (LmnAtomPtr)atomlist_get_record(atomlist_ent, findatomid);
          if(!record) {
            atom = atomlist_head(atomlist_ent);
            record = lmn_new_atom(LMN_RESUME_FUNCTOR);
            hashtbl_put(&atomlist_ent->record, findatomid, (HashKeyType)record);
            LMN_ATOM_SET_NEXT(atomlist_ent, record);
            LMN_ATOM_SET_PREV(record, atomlist_ent);
            LMN_ATOM_SET_NEXT(record, atom);
            LMN_ATOM_SET_PREV(atom, record);
          } else {
            atom = LMN_ATOM_GET_NEXT(record);
          }
#define DBG 0
#if DBG
          int count=0;
#endif
          for (;
               atom != lmn_atomlist_end(atomlist_ent);
               atom = LMN_ATOM_GET_NEXT(atom)) {
#if DBG
            count++;
#endif
            if(LMN_ATOM_GET_FUNCTOR(atom)==LMN_RESUME_FUNCTOR)
              continue;
            wt[atomi] = (LmnWord)atom;
            LMN_ATOM_SET_PREV(LMN_ATOM_GET_NEXT(record), LMN_ATOM_GET_PREV(record));
            LMN_ATOM_SET_NEXT(LMN_ATOM_GET_PREV(record), LMN_ATOM_GET_NEXT(record));
            LMN_ATOM_SET_NEXT(record, atom);
            LMN_ATOM_SET_PREV(record, LMN_ATOM_GET_PREV(atom));
            LMN_ATOM_SET_NEXT(LMN_ATOM_GET_PREV(atom), record);
            LMN_ATOM_SET_PREV(atom, record);

            if (interpret(instr, &instr)) {
              *next_instr = instr;
#if DBG
              printf("count=%d\n", count);
#endif
              return TRUE;
            }
          }
        }
        return FALSE;
      }
      break;
    }
    case INSTR_LOCKMEM:
    {
      LmnInstrVar mem, atom, memn;
      READ_VAL(LmnInstrVar, instr, mem);
      READ_VAL(LmnInstrVar, instr, atom);
      READ_VAL(lmn_interned_str, instr, memn);

      LMN_ASSERT(!LMN_ATTR_IS_DATA(at[atom]));
      LMN_ASSERT(LMN_IS_PROXY_FUNCTOR(LMN_ATOM_GET_FUNCTOR(LMN_ATOM(wt[atom]))));
      wt[mem] = (LmnWord)LMN_PROXY_GET_MEM(wt[atom]);
      if (lmn_env.nd || lmn_env.ltl) { at[mem] = 0; /* MC */ }
      LMN_ASSERT(((LmnMembrane *)wt[mem])->parent);
      break;
    }
    case INSTR_ANYMEM:
    {
      LmnInstrVar mem1, mem2, memt, memn; /* dst, parent, type, name */
      LmnMembrane* mp;

      READ_VAL(LmnInstrVar, instr, mem1);
      READ_VAL(LmnInstrVar, instr, mem2);
      READ_VAL(LmnInstrVar, instr, memt);
      READ_VAL(lmn_interned_str, instr, memn);

      mp = ((LmnMembrane*)wt[mem2])->child_head;
      while (mp) {
        wt[mem1] = (LmnWord)mp;
        if (lmn_env.nd || lmn_env.ltl) { at[mem1] = 0; /* MC */ }
        if (mp->name == memn && interpret(instr, &instr)) {
          *next_instr = instr;
          return TRUE;
        }
        mp = mp->next;
      }
      return FALSE;
      break;
    }
    case INSTR_NMEMS:
    {
      LmnInstrVar memi, nmems;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, nmems);

      if(!lmn_mem_nmems((LmnMembrane*)wt[memi], nmems)) {
        return FALSE;
      }
      break;
    }
    case INSTR_NORULES:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      if(((LmnMembrane *)wt[memi])->rulesets.num) return FALSE;
      break;
    }
    case INSTR_NEWATOM:
    {
      LmnInstrVar atomi, memi;
      LmnWord ap;
      LmnLinkAttr attr;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnLinkAttr, instr, attr);
      if (LMN_ATTR_IS_DATA(attr)) {
        READ_DATA_ATOM(ap, attr);
      } else { /* symbol atom */
        LmnFunctor f;

        READ_VAL(LmnFunctor, instr, f);
        ap = (LmnWord)lmn_new_atom(f);
      }
      lmn_mem_push_atom((LmnMembrane *)wt[memi], ap, attr);
      wt[atomi] = ap;
      at[atomi] = attr;
      break;
    }
    case INSTR_NATOMS:
    {
      LmnInstrVar memi, natoms;
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, natoms);
      if(!lmn_mem_natoms((LmnMembrane*)wt[memi], natoms)) {
        return FALSE;
      }
      break;
    }
    case INSTR_NATOMSINDIRECT:
    {
      LmnInstrVar memi, natomsi;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, natomsi);

      if(!lmn_mem_natoms((LmnMembrane*)wt[memi], wt[natomsi])) {
        return FALSE;
      }
      break;
    }
    case INSTR_ALLOCLINK:
    {
      LmnInstrVar link, atom, n;

      READ_VAL(LmnInstrVar, instr, link);
      READ_VAL(LmnInstrVar, instr, atom);
      READ_VAL(LmnInstrVar, instr, n);

      if (LMN_ATTR_IS_DATA(at[atom])) {
        wt[link] = wt[atom];
        at[link] = at[atom];
      } else { /* link to atom */
        wt[link] = (LmnWord)LMN_ATOM(wt[atom]);
        at[link] = LMN_ATTR_MAKE_LINK(n);
      }
      break;
    }
    case INSTR_UNIFYLINKS:
    {
      LmnInstrVar link1, link2, mem;

      READ_VAL(LmnInstrVar, instr, link1);
      READ_VAL(LmnInstrVar, instr, link2);
      READ_VAL(LmnInstrVar, instr, mem);

      if (LMN_ATTR_IS_DATA(LINKED_ATTR(link1))) {
        if (LMN_ATTR_IS_DATA(LINKED_ATTR(link2))) { /* 1, 2 are data */
          lmn_mem_link_data_atoms((LmnMembrane *)wt[mem], wt[link1], at[link1], LINKED_ATOM(link2), LINKED_ATTR(link2));
        }
        else { /* 1 is data */
          LMN_ATOM_SET_LINK(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(LINKED_ATTR(link2)), LINKED_ATOM(link1));
          LMN_ATOM_SET_ATTR(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(LINKED_ATTR(link2)), LINKED_ATTR(link1));
        }
      }
      else if (LMN_ATTR_IS_DATA(LINKED_ATTR(link2))) { /* 2 is data */
        LMN_ATOM_SET_LINK(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(LINKED_ATTR(link1)), LINKED_ATOM(link2));
        LMN_ATOM_SET_ATTR(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(LINKED_ATTR(link1)), LINKED_ATTR(link2));
      }
      else { /* 1, 2 are symbol atom */
        LMN_ATOM_SET_LINK(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(LINKED_ATTR(link1)), LINKED_ATOM(link2));
        LMN_ATOM_SET_LINK(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(LINKED_ATTR(link2)), LINKED_ATOM(link1));
        LMN_ATOM_SET_ATTR(LINKED_ATOM(link1), LMN_ATTR_GET_VALUE(LINKED_ATTR(link1)), LINKED_ATTR(link2));
        LMN_ATOM_SET_ATTR(LINKED_ATOM(link2), LMN_ATTR_GET_VALUE(LINKED_ATTR(link2)), LINKED_ATTR(link1));
      }
      break;
    }
    case INSTR_NEWLINK:
    {
      LmnInstrVar atom1, atom2, pos1, pos2, memi;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, pos1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, pos2);
      READ_VAL(LmnInstrVar, instr, memi);

      lmn_mem_newlink((LmnMembrane *)wt[memi],
                      wt[atom1],
                      at[atom1],
                      pos1,
                      wt[atom2],
                      at[atom2],
                      pos2);
      break;
    }
    case INSTR_RELINK:
    {
      LmnInstrVar atom1, atom2, pos1, pos2, memi;
      LmnAtomPtr ap;
      LmnByte attr;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, pos1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, pos2);
      READ_VAL(LmnInstrVar, instr, memi);

      ap = LMN_ATOM(LMN_ATOM_GET_LINK(wt[atom2], pos2));
      attr = LMN_ATOM_GET_ATTR(wt[atom2], pos2);

      if(LMN_ATTR_IS_DATA(at[atom1]) && LMN_ATTR_IS_DATA(attr)) {
        #ifdef DEBUG
        fprintf(stderr, "Two data atoms are connected each other.\n");
        #endif
      }
      else if (LMN_ATTR_IS_DATA(at[atom1])) {
        LMN_ATOM_SET_LINK(ap, attr, wt[atom1]);
        LMN_ATOM_SET_ATTR(ap, attr, at[atom1]);
      }
      else if (LMN_ATTR_IS_DATA(attr)) {
        LMN_ATOM_SET_LINK(LMN_ATOM(wt[atom1]), pos1, (LmnWord)ap);
        LMN_ATOM_SET_ATTR(LMN_ATOM(wt[atom1]), pos1, attr);
      }
      else {
        LMN_ATOM_SET_LINK(ap, attr, wt[atom1]);
        LMN_ATOM_SET_ATTR(ap, attr, pos1);
        LMN_ATOM_SET_LINK(LMN_ATOM(wt[atom1]), pos1, (LmnWord)ap);
        LMN_ATOM_SET_ATTR(LMN_ATOM(wt[atom1]), pos1, attr);
      }
      break;
    }
    case INSTR_INHERITLINK:
    {
      LmnInstrVar atomi, posi, linki, memi;
      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, posi);
      READ_VAL(LmnInstrVar, instr, linki);
      READ_VAL(LmnInstrVar, instr, memi);

      if(LMN_ATTR_IS_DATA(at[atomi]) && LMN_ATTR_IS_DATA(LINKED_ATTR(linki))) {
        #ifdef DEBUG
        fprintf(stderr, "Two data atoms are connected each other.\n");
        #endif
      }
      else if(LMN_ATTR_IS_DATA(at[atomi])) {
        LMN_ATOM_SET_LINK(LINKED_ATOM(linki), LINKED_ATTR(linki), wt[atomi]);
        LMN_ATOM_SET_ATTR(LINKED_ATOM(linki), LINKED_ATTR(linki), at[atomi]);
      }
      else if(LMN_ATTR_IS_DATA(LINKED_ATTR(linki))) {
        LMN_ATOM_SET_LINK(LMN_ATOM(wt[atomi]), posi, LINKED_ATOM(linki));
        LMN_ATOM_SET_ATTR(LMN_ATOM(wt[atomi]), posi, LINKED_ATTR(linki));
      }
      else {
        LMN_ATOM_SET_LINK(LMN_ATOM(wt[atomi]), posi, LINKED_ATOM(linki));
        LMN_ATOM_SET_ATTR(LMN_ATOM(wt[atomi]), posi, LINKED_ATTR(linki));
        LMN_ATOM_SET_LINK(LINKED_ATOM(linki), LINKED_ATTR(linki), wt[atomi]);
        LMN_ATOM_SET_ATTR(LINKED_ATOM(linki), LINKED_ATTR(linki), posi);
      }

      break;
    }
    case INSTR_GETLINK:
    {
      LmnInstrVar linki, atomi, posi;
      READ_VAL(LmnInstrVar, instr, linki);
      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, posi);

      /* �����μ����򤻤��˥�󥯸��ξ�����Ǽ���Ƥ�����
         ��󥯸�����Ǽ����Ƥ��뤳�Ȥ򼨤�����ǲ��̤ΥӥåȤ�Ω�Ƥ� */
      wt[linki] = LMN_ATOM_GET_LINK(wt[atomi], posi);
      at[linki] = LMN_ATOM_GET_ATTR(wt[atomi], posi);
      break;
    }
    case INSTR_UNIFY:
    {
      LmnInstrVar atom1, pos1, atom2, pos2, memi;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, pos1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, pos2);
      READ_VAL(LmnInstrVar, instr, memi);

      lmn_mem_unify_atom_args((LmnMembrane *)wt[memi],
                              LMN_ATOM(wt[atom1]),
                              pos1,
                              LMN_ATOM(wt[atom2]),
                              pos2);
      break;
    }
    case INSTR_PROCEED:
      /**
       * MC -->
       * mc_flags.system_rule_committed�ˤ�ä�
       * ���դ˽и�����PROCEED�ȱ��դ˽и�����PROCEED����̤���ɬ�פ�����
       */
      if(mc_flags.nd_exec && mc_flags.system_rule_committed) {
        mc_flags.system_rule_committed = FALSE;
        mc_flags.system_rule_proceeded = TRUE;
        return FALSE; /* ���θ����������뤿��˼��Ԥ��� */
      }
      /**
       * <-- MC
       */

      if (lmn_env.trace) { /* tracer */
        fprintf(stdout, "%d: ", trace_num++);
        lmn_dump_cell(global_root);
      }
      *next_instr = instr;
      return TRUE;
    case INSTR_STOP:
      *next_instr = instr;
      return FALSE;
    case INSTR_NOT:
    {
      LmnSubInstrSize subinstr_size;
      READ_VAL(LmnSubInstrSize, instr, subinstr_size);

      if (interpret(instr, &instr)) {
        return FALSE;
      }
      instr += subinstr_size;
      break;
    }
    case INSTR_ENQUEUEATOM:
    {
      LmnInstrVar atom;

      READ_VAL(LmnInstrVar, instr, atom);
      /* do nothing */
      break;
    }
    case INSTR_DEQUEUEATOM:
    {
      LmnInstrVar atom;

      READ_VAL(LmnInstrVar, instr, atom);
      break;
    }
    case INSTR_NEWMEM:
    {
      LmnInstrVar newmemi, parentmemi, memf;
      LmnMembrane *mp;

      READ_VAL(LmnInstrVar, instr, newmemi);
      READ_VAL(LmnInstrVar, instr, parentmemi);
      READ_VAL(LmnInstrVar, instr, memf);

      mp = lmn_mem_make(); /*lmn_new_mem(memf);*/
      lmn_mem_add_child_mem((LmnMembrane*)wt[parentmemi], mp);
      wt[newmemi] = (LmnWord)mp;
      if (lmn_env.nd || lmn_env.ltl) { /* MC */
        at[newmemi] = 0;
        activate(mp);
      } else { /* �̾�¹Ի� */
        memstack_push(mp);
      }
      break;
    }
    case INSTR_ALLOCMEM:
    {
      LmnInstrVar dstmemi;

      READ_VAL(LmnInstrVar, instr, dstmemi);

      wt[dstmemi] = (LmnWord)lmn_mem_make();
      break;
    }
    case INSTR_REMOVEATOM:
    {
      LmnInstrVar atomi, memi;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, memi);

      lmn_mem_remove_atom((LmnMembrane*)wt[memi], wt[atomi], at[atomi]);
      break;
    }
    case INSTR_FREEATOM:
    {
      LmnInstrVar atomi;

      READ_VAL(LmnInstrVar, instr, atomi);

      lmn_free_atom(wt[atomi], at[atomi]);
      break;
    }
    case INSTR_REMOVEMEM:
    {
      LmnInstrVar memi;
      LmnMembrane *mp;

      READ_VAL(LmnInstrVar, instr, memi);
      instr += sizeof(LmnInstrVar); /* ingnore parent */

      mp = (LmnMembrane*)wt[memi];
      LMN_ASSERT(mp->parent);
      if(mp->parent->child_head == mp) mp->parent->child_head = mp->next;
      if(mp->prev) mp->prev->next = mp->next;
      if(mp->next) mp->next->prev = mp->prev;
      mp->parent = NULL; /* removeproxies �Τ����ɬ�� */
      break;
    }
    case INSTR_FREEMEM:
    {
      LmnInstrVar memi;
      LmnMembrane *mp;

      READ_VAL(LmnInstrVar, instr, memi);

      mp = (LmnMembrane*)wt[memi];
      lmn_mem_free(mp);
      break;
    }
    case INSTR_ADDMEM:
    {
      LmnInstrVar dstmem, srcmem;

      READ_VAL(LmnInstrVar, instr, dstmem);
      READ_VAL(LmnInstrVar, instr, srcmem);

      LMN_ASSERT(!((LmnMembrane *)wt[srcmem])->parent);

      lmn_mem_add_child_mem((LmnMembrane *)wt[dstmem], (LmnMembrane *)wt[srcmem]);
      break;
    }
    case INSTR_ENQUEUEMEM:
    {
      LmnInstrVar memi;
      READ_VAL(LmnInstrVar, instr, memi);
      if (lmn_env.nd || lmn_env.ltl) {
        activate((LmnMembrane *)wt[memi]); /* MC */
      } else {
        memstack_push((LmnMembrane *)wt[memi]); /* �̾�¹Ի� */
      }
      break;
    }
    case INSTR_UNLOCKMEM:
    { /* ���⤷�ʤ� */
      LmnInstrVar memi;
      READ_VAL(LmnInstrVar, instr, memi);
      break;
    }
    case INSTR_LOADRULESET:
    {
      LmnInstrVar memi;
      LmnRulesetId id;
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnRulesetId, instr, id);

      lmn_mem_add_ruleset((LmnMembrane*)wt[memi], lmn_ruleset_from_id(id));
      break;
    }
    case INSTR_LOADMODULE:
    {
      LmnInstrVar memi;
      lmn_interned_str module_name_id;
      LmnRuleSet ruleset;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(lmn_interned_str, instr, module_name_id);

      if ((ruleset = lmn_get_module_ruleset(module_name_id))) {
        /* �ơ��֥���˥롼�륻�åȤ������� */
        lmn_mem_add_ruleset((LmnMembrane*)wt[memi], ruleset);
      }
      else {
        /* �ơ��֥���˥롼�륻�åȤ��ʤ���� */
        fprintf(stderr, "Undefined module %s\n", lmn_id_to_name(module_name_id));
      }
      break;
    }
    case INSTR_RECURSIVELOCK:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      /* do notiong */
      break;
    }
    case INSTR_RECURSIVEUNLOCK:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      /* do notiong */
      break;
    }
    case INSTR_DEREFATOM:
    {
      LmnInstrVar atom1, atom2, posi;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, posi);

      wt[atom1] = (LmnWord)LMN_ATOM(LMN_ATOM_GET_LINK(wt[atom2], posi));
      at[atom1] = LMN_ATOM_GET_ATTR(wt[atom2], posi);
      break;
    }
    case INSTR_DEREF:
    {
      LmnInstrVar atom1, atom2, pos1, pos2;
      LmnByte attr;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);
      READ_VAL(LmnInstrVar, instr, pos1);
      READ_VAL(LmnInstrVar, instr, pos2);

      attr = LMN_ATOM_GET_ATTR(wt[atom2], pos1);
      LMN_ASSERT(!LMN_ATTR_IS_DATA(at[atom2]));
      if (LMN_ATTR_IS_DATA(attr)) {
        if (pos2 != 0) return FALSE;
      }
      else {
        if (attr != pos2) return FALSE;
      }
      wt[atom1] = LMN_ATOM_GET_LINK(wt[atom2], pos1);
      at[atom1] = attr;
      break;
    }
    case INSTR_FUNC:
    {
      LmnInstrVar atomi;
      LmnFunctor f;
      LmnLinkAttr attr;
      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnLinkAttr, instr, attr);

      if (LMN_ATTR_IS_DATA(at[atomi]) == LMN_ATTR_IS_DATA(attr)) {
        if(LMN_ATTR_IS_DATA(at[atomi])) {
          BOOL eq;
          if(at[atomi] != attr) return FALSE; /* comp attr */
          READ_CMP_DATA_ATOM(attr, wt[atomi], eq);
          if (!eq) return FALSE;
        }
        else /* symbol atom */
          {
            READ_VAL(LmnFunctor, instr, f);
            if (LMN_ATOM_GET_FUNCTOR(LMN_ATOM(wt[atomi])) != f) {
              return FALSE;
            }
          }
      }
      else { /* LMN_ATTR_IS_DATA(at[atomi]) != LMN_ATTR_IS_DATA(attr) */
        return FALSE;
      }
      break;
    }
    case INSTR_NOTFUNC:
    {
      LmnInstrVar atomi;
      LmnFunctor f;
      LmnLinkAttr attr;
      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnLinkAttr, instr, attr);

      if (LMN_ATTR_IS_DATA(at[atomi]) == LMN_ATTR_IS_DATA(attr)) {
        if(LMN_ATTR_IS_DATA(at[atomi])) {
          if(at[atomi] == attr) {
            BOOL eq;
            READ_CMP_DATA_ATOM(attr, wt[atomi], eq);
            if (eq) return FALSE;
          }
        }
        else { /* symbol atom */
          READ_VAL(LmnFunctor, instr, f);
          if (LMN_ATOM_GET_FUNCTOR(LMN_ATOM(wt[atomi])) == f) return FALSE;
        }
      }
      break;
    }
    case INSTR_ISGROUND:
    {
      unsigned int i, atom_num;
      BOOL ret_flag = TRUE;
      LmnInstrVar funci, srclisti, avolisti;
      Vector *srcvec, *avovec; /* �ѿ��ֹ�Υꥹ�� */
      HashSet visited_atoms;
      Vector stack, visited_root;
      LinkObj *start;

      READ_VAL(LmnInstrVar, instr, funci);
      READ_VAL(LmnInstrVar, instr, srclisti);
      READ_VAL(LmnInstrVar, instr, avolisti);

      srcvec = (Vector*) wt[srclisti];
      avovec = (Vector*) wt[avolisti];

      vec_init(&stack, 16); /* �Ƶ������ѥ����å� */
      start = LinkObj_make((LmnWord)LINKED_ATOM(vec_get(srcvec, 0)), LINKED_ATTR(vec_get(srcvec, 0)));
      if(!LMN_ATTR_IS_DATA(start->pos)) { /* data atom ���Ѥޤʤ� */
        vec_push(&stack, (LmnWord)start);
      } else {
        LMN_FREE(start);
      }

      vec_init(&visited_root, 16);
      for(i = 0; i < srcvec->num; i++) {
        vec_push(&visited_root, FALSE);
      }
      vec_set(&visited_root, 0, TRUE);

      hashset_init(&visited_atoms, 256);

      atom_num = 0;

      while(stack.num != 0) { /* main loop: start */
        LinkObj *lo = (LinkObj *)vec_pop(&stack);

        if(hashset_contains(&visited_atoms, (HashKeyType)lo->ap)) {
          LMN_FREE(lo);
          continue;
        }

        /* �ФΥ�󥯤��ػߥ�󥯤Ǥʤ��� */
        for(i = 0; i < avovec->num; i++) {
          LmnAtomPtr avolink = (LmnAtomPtr)LINKED_ATOM(vec_get(avovec, i));
          LmnLinkAttr avoattr = LINKED_ATTR(vec_get(avovec, i));
          if((LmnAtomPtr)LMN_ATOM_GET_LINK(lo->ap, lo->pos) == avolink &&
              LMN_ATOM_GET_ATTR(lo->ap, lo->pos) == avoattr) {
            ret_flag = FALSE;
            break;
          }
        }
        if(!ret_flag) {
          LMN_FREE(lo);
          break;
        }

        /* ����ڤäƤ��ʤ��� */
        if(LMN_IS_PROXY_FUNCTOR(LMN_ATOM_GET_FUNCTOR(lo->ap))) {
          LMN_FREE(lo);
          ret_flag = FALSE;
          break;
        }

        /* ������ã������� */
        for(i = 0; i < visited_root.num; i++) {
          unsigned int index = vec_get(srcvec, i);
          if (lo->ap == (LmnWord)LMN_ATOM_GET_LINK((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))
              && lo->pos == LMN_ATOM_GET_ATTR((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))) {
            vec_set(&visited_root, i, TRUE);
            goto ISGROUND_CONT;
          }
        }

        atom_num++;
        hashset_add(&visited_atoms, (LmnWord)lo->ap);

        /* �Ҥ�Ÿ�� */
        for(i = 0; i < LMN_ATOM_GET_ARITY(lo->ap); i++) {
          LinkObj *next;
          if (i == lo->pos)
            continue;
          if(!LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(lo->ap, i))) { /* data atom ���Ѥޤʤ� */
            next = LinkObj_make((LmnWord)LMN_ATOM_GET_LINK(lo->ap, i), LMN_ATTR_GET_VALUE(LMN_ATOM_GET_ATTR(lo->ap, i)));
            vec_push(&stack, (LmnWord)next);
          }
        }

ISGROUND_CONT:
        LMN_FREE(lo);
      } /* main loop: end */

      for(i = 0; i < visited_root.num; i++) {
        if(!vec_get(&visited_root, i)) { /* ̤ˬ��κ������� */
          ret_flag=FALSE;
          break;
        }
      }

      hashset_destroy(&visited_atoms);
      vec_destroy(&stack);
      vec_destroy(&visited_root);
      if(!ret_flag) return FALSE;
      wt[funci] = (LmnWord)atom_num;
      at[funci] = LMN_INT_ATTR;
      break;
    }
    case INSTR_EQGROUND:
    case INSTR_NEQGROUND:
    {
      unsigned int i, j;
      BOOL ret_flag = TRUE;
      LmnInstrVar srci, dsti;
      Vector *srcv, *dstv; /* �ѿ��ֹ�Υꥹ�ȡ���Ӹ��������� */
      Vector stack1, stack2;
      SimpleHashtbl map; /* ��Ӹ�������� */
      LinkObj *start1, *start2;

      READ_VAL(LmnInstrVar, instr, srci);
      READ_VAL(LmnInstrVar, instr, dsti);

      srcv = (Vector *)wt[srci];
      dstv = (Vector *)wt[dsti];

      hashtbl_init(&map, 256);

      vec_init(&stack1, 16);
      vec_init(&stack2, 16);
      start1 = LinkObj_make((LmnWord)LINKED_ATOM(vec_get(srcv, 0)), LINKED_ATTR(vec_get(srcv, 0)));
      start2 = LinkObj_make((LmnWord)LINKED_ATOM(vec_get(dstv, 0)), LINKED_ATTR(vec_get(dstv, 0)));
      if (!LMN_ATTR_IS_DATA(start1->pos) && !LMN_ATTR_IS_DATA(start2->pos)) { /* �Ȥ�˥���ܥ륢�ȥ�ξ�� */
        vec_push(&stack1, (LmnWord)start1);
        vec_push(&stack2, (LmnWord)start2);
      }
      else { /* data atom ���Ѥޤʤ� */
        if(!lmn_eq_func(start1->ap, start1->pos, start2->ap, start2->pos)) ret_flag = FALSE;
        LMN_FREE(start1);
        LMN_FREE(start2);
      }

      while(stack1.num != 0) { /* main loop: start */
        LinkObj *l1 = (LinkObj *)vec_pop(&stack1);
        LinkObj *l2 = (LinkObj *)vec_pop(&stack2);
        BOOL contains1 = FALSE;
        BOOL contains2 = FALSE;

        for(i = 0; i < srcv->num; i++) {
          unsigned int index = vec_get(srcv, i);
          if (l1->ap == (LmnWord)LMN_ATOM_GET_LINK((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))
              && l1->pos == LMN_ATOM_GET_ATTR((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))) {
            contains1 = TRUE;
            break;
          }
        }
        for(j = 0; j < dstv->num; j++) {
          unsigned int index = vec_get(dstv, j);
          if (l2->ap == (LmnWord)LMN_ATOM_GET_LINK((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))
              && l2->pos == LMN_ATOM_GET_ATTR((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))) {
            contains2 = TRUE;
            break;
          }
        }
        if(i != j){ /* ���ΰ��֤��㤦 */
          LMN_FREE(l1); LMN_FREE(l2);
          ret_flag = FALSE;
          break;
        }
        if(contains1) { /* ������ã������� */
          LMN_FREE(l1); LMN_FREE(l2);
          continue;
        }

        if(l1->pos != l2->pos){ /* �������� */
          LMN_FREE(l1); LMN_FREE(l2);
          ret_flag = FALSE;
          break;
        }

        if(LMN_ATOM_GET_FUNCTOR(l1->ap) != LMN_ATOM_GET_FUNCTOR(l2->ap)){ /* �ե��󥯥����� */
          LMN_FREE(l1); LMN_FREE(l2);
          ret_flag = FALSE;
          break;
        }

        if(!hashtbl_contains(&map, l1->ap)) hashtbl_put(&map, l1->ap, l2->ap); /* ̤�� */
        else if(hashtbl_get(&map, l1->ap) != l2->ap) { /* ���Ф��԰��� */
          LMN_FREE(l1); LMN_FREE(l2);
          ret_flag = FALSE;
          break;
        }
        else continue; /* ���Фǰ��� */

        for(i = 0; i < LMN_ATOM_GET_ARITY(l1->ap); i++) {
          LinkObj *n1, *n2;
          if(i == l1->pos) continue;
          if (!LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(l1->ap, i)) && !LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(l1->ap, i))) {
            n1 = LinkObj_make((LmnWord)LMN_ATOM_GET_LINK(l1->ap, i), LMN_ATTR_GET_VALUE(LMN_ATOM_GET_ATTR(l1->ap, i)));
            n2 = LinkObj_make((LmnWord)LMN_ATOM_GET_LINK(l2->ap, i), LMN_ATTR_GET_VALUE(LMN_ATOM_GET_ATTR(l2->ap, i)));
            vec_push(&stack1, (LmnWord)n1);
            vec_push(&stack2, (LmnWord)n2);
          }
          else { /* data atom ���Ѥޤʤ� */
            if(!lmn_eq_func(LMN_ATOM_GET_LINK(l1->ap, i), LMN_ATOM_GET_ATTR(l1->ap, i),
                  LMN_ATOM_GET_LINK(l2->ap, i), LMN_ATOM_GET_ATTR(l2->ap, i))) {
              LMN_FREE(l1); LMN_FREE(l2);
              ret_flag = FALSE;
              goto EQGROUND_NEQGROUND_BREAK;
            }
          }
        }
        LMN_FREE(l1); LMN_FREE(l2);
      } /* main loop: end */

EQGROUND_NEQGROUND_BREAK:
      vec_destroy(&stack1);
      vec_destroy(&stack2);
      hashtbl_destroy(&map);
      if((!ret_flag && INSTR_EQGROUND == op) || (ret_flag && INSTR_NEQGROUND == op)) {
        return FALSE;
      }
      break;
    }
    case INSTR_COPYGROUND:
    {
      unsigned int i;
      LmnInstrVar dstlist, srclist, memi;
      Vector *srcvec, *dstlovec, *retvec; /* �ѿ��ֹ�Υꥹ�� */
      SimpleHashtbl *atommap = LMN_MALLOC(SimpleHashtbl);
      Vector stack;
      LinkObj *start;
      LmnAtomPtr cpatom;

      READ_VAL(LmnInstrVar, instr, dstlist);
      READ_VAL(LmnInstrVar, instr, srclist);
      READ_VAL(LmnInstrVar, instr, memi);

      srcvec = (Vector *)wt[srclist];

      vec_init(&stack, 16); /* �Ƶ������ѥ����å� */

      hashtbl_init(atommap, 256);
      /* atommap�ν�����ꡧ�롼����ǿƥ��ȥ�򻲾Ȥ���ɬ�פ����뤿�� */
      start = LinkObj_make((LmnWord)LINKED_ATOM(vec_get(srcvec, 0)),
                           (LmnLinkAttr)LINKED_ATTR(vec_get(srcvec, 0)));
      cpatom = (LmnAtomPtr)lmn_copy_atom(start->ap, start->pos);
      hashtbl_put(atommap, (HashKeyType)start->ap, (HashValueType)cpatom);
      if (!LMN_ATTR_IS_DATA(start->pos)) { /* data atom �Ǥʤ���� */
        mem_push_symbol_atom((LmnMembrane *)wt[memi], cpatom);
        for(i = 0; i < LMN_ATOM_GET_ARITY(cpatom); i++) {
          if(start->pos == i)
            continue;
          else {
            LmnLinkAttr attr = LMN_ATOM_GET_ATTR(start->ap, i);
            if (!LMN_ATTR_IS_DATA(attr)) {
              LinkObj *next = LinkObj_make((LmnWord)LMN_ATOM_GET_LINK(start->ap, i), LMN_ATOM_GET_ATTR(start->ap, i));
              vec_push(&stack, (LmnWord)next);
            }
            else { /* data atom �ϥ����å����Ѥޤʤ���¨���ԡ����� */
              LmnWord cpdata = lmn_copy_atom(LMN_ATOM_GET_LINK(start->ap, i), attr);
              lmn_mem_push_atom((LmnMembrane *)wt[memi], cpdata, attr);
              hashtbl_put(atommap, (HashKeyType)LMN_ATOM_GET_LINK(start->ap, i), (HashValueType)cpatom);
              LMN_ATOM_SET_LINK(cpatom, i, (LmnWord)cpdata);
              LMN_ATOM_SET_ATTR(cpatom, i, attr);
            }
          }
        }
      }
      else { /* data atom �ξ�� */
        lmn_mem_push_atom((LmnMembrane *)wt[memi], (LmnWord)cpatom, start->pos);
      }
      /* atommap�ν�����ꡧ�����ޤ� */

      while(stack.num != 0) { /* main loop: start */
        LinkObj *lo = (LinkObj *)vec_pop(&stack);

        /* ������ã������� */
        for(i = 0; i < srcvec->num; i++){
          unsigned int index = vec_get(srcvec, i);
          if (lo->ap == (LmnWord)LMN_ATOM_GET_LINK((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))
              && lo->pos == LMN_ATOM_GET_ATTR((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))) {
            goto COPYGROUND_CONT;
          }
        }

        if(!hashtbl_contains(atommap, (HashKeyType)lo->ap)) {
          /* ���ȥ�Υ��ԡ���������ƿƥ��ȥ�Υ��ԡ�����³���� */
          LmnAtomPtr cpbuddy = (LmnAtomPtr)hashtbl_get(atommap, (HashKeyType)(LmnAtomPtr)LMN_ATOM_GET_LINK(lo->ap, lo->pos));
          cpatom = (LmnAtomPtr)lmn_copy_atom(lo->ap, lo->pos);
          mem_push_symbol_atom((LmnMembrane *)wt[memi], (LmnAtomPtr)cpatom);

          hashtbl_put(atommap, (HashKeyType)lo->ap, (HashValueType)cpatom);
          LMN_ATOM_SET_LINK(cpbuddy, LMN_ATOM_GET_ATTR(lo->ap, lo->pos), (LmnWord)cpatom);
          LMN_ATOM_SET_ATTR(cpbuddy, LMN_ATOM_GET_ATTR(lo->ap, lo->pos), lo->pos);
          for(i = 0; i < LMN_ATOM_GET_ARITY(cpatom); i++) {
            if (!LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(lo->ap, i))) {
              LinkObj *next = LinkObj_make((LmnWord)LMN_ATOM_GET_LINK(lo->ap, i), LMN_ATOM_GET_ATTR(lo->ap, i));
              vec_push(&stack, (LmnWord)next);
            }
            else { /* data atom �ϥ����å����Ѥޤʤ���¨���ԡ����� */
              LmnAtomPtr cpdata = (LmnAtomPtr)lmn_copy_atom(LMN_ATOM_GET_LINK(lo->ap, i), LMN_ATOM_GET_ATTR(lo->ap, i));
              hashtbl_put(atommap, (HashKeyType)LMN_ATOM_GET_LINK(lo->ap, i), (HashValueType)cpatom);
              lmn_mem_push_atom((LmnMembrane *)wt[memi], (LmnWord)cpdata, LMN_ATOM_GET_ATTR(lo->ap, i));
              LMN_ATOM_SET_LINK(cpatom, i, (LmnWord)cpdata);
              LMN_ATOM_SET_ATTR(cpatom, i, LMN_ATOM_GET_ATTR(lo->ap, i));
            }
          }
        }
        else {
          /* ���ȥ�Υ��ԡ���ɽ����������ƿƥ��ȥ�Υ��ԡ�����³���� */
          LmnAtomPtr cpbuddy = (LmnAtomPtr)hashtbl_get(atommap, (HashKeyType)(LmnAtomPtr)LMN_ATOM_GET_LINK(lo->ap, lo->pos));
          cpatom = (LmnAtomPtr)hashtbl_get(atommap, (HashKeyType)lo->ap);
          LMN_ATOM_SET_LINK(cpbuddy, LMN_ATOM_GET_ATTR(lo->ap, lo->pos), (LmnWord)cpatom);
          LMN_ATOM_SET_ATTR(cpbuddy, LMN_ATOM_GET_ATTR(lo->ap, lo->pos), lo->pos);
        }

COPYGROUND_CONT:
        LMN_FREE(lo);
      } /* main loop: end */

      dstlovec = vec_make(srcvec->num);
      for(i = 0; i < srcvec->num; i++) {
        LmnAtomPtr src_ap = (LmnAtomPtr)LINKED_ATOM(vec_get(srcvec, i));
        LmnByte src_pos = (LmnLinkAttr)LINKED_ATTR(vec_get(srcvec, i));
        LinkObj *new = LinkObj_make((LmnWord)hashtbl_get(atommap, (HashKeyType)src_ap), src_pos);
        vec_push(dstlovec, (LmnWord)new);
      }

      /* �֤��ͤκ��� */
      retvec = vec_make(2);
      vec_push(retvec, (LmnWord)dstlovec);
      vec_push(retvec, (LmnWord)atommap);
      wt[dstlist] = (LmnWord)retvec;
      at[dstlist] = (LmnByte)LIST_AND_MAP;

      LMN_FREE(start);
      vec_destroy(&stack);

      /* �����Τ���κƵ� */
      interpret(instr, next_instr);

      while(dstlovec->num) {
        LMN_FREE(vec_get(dstlovec, dstlovec->num-1));
        dstlovec->num--;
      }
      vec_free(dstlovec);
      vec_free(retvec);
      /**
       * MC -->
       */
      if (mc_flags.nd_exec) {
        return mc_flags.property_rule;
      }
      /**
       * <-- MC
       */
      return TRUE; /* COPYGROUND�ϥܥǥ��˽и����� */

      break;
    }
    case INSTR_REMOVEGROUND:
    case INSTR_FREEGROUND:
    {
      unsigned int i;
      LmnInstrVar listi, memi;
      Vector *srcvec; /* �ѿ��ֹ�Υꥹ�� */
      HashSet visited_atoms;
      Vector stack;
      LinkObj *start;

      READ_VAL(LmnInstrVar, instr, listi);
      if (INSTR_REMOVEGROUND == op) {
        READ_VAL(LmnInstrVar, instr, memi);
      }

      srcvec = (Vector *)wt[listi];

      vec_init(&stack, 16); /* �Ƶ������ѥ����å� */
      start = LinkObj_make((LmnWord)LINKED_ATOM(vec_get(srcvec, 0)), LINKED_ATTR(vec_get(srcvec, 0)));
      if(!LMN_ATTR_IS_DATA(start->pos)) {
        vec_push(&stack, (LmnWord)start);
      }
      else { /* data atom ���Ѥޤʤ� */
        switch (op) {
          case INSTR_REMOVEGROUND:
            lmn_mem_remove_atom((LmnMembrane*)wt[memi], start->ap, start->pos);
            LMN_FREE(start);
            break;
          case INSTR_FREEGROUND:
            /* data atom����³���줿symbol atom��free�����Ȱ���free����� */
            LMN_FREE(start);
            break;
        }
      }

      hashset_init(&visited_atoms, 256);

      while(stack.num != 0) { /* main loop: start */
        LinkObj *lo = (LinkObj *)vec_pop(&stack);

        if(hashset_contains(&visited_atoms, (HashKeyType)lo->ap))
          continue;

        hashset_add(&visited_atoms, (LmnWord)lo->ap);
        for(i = 0; i < srcvec->num; i++) { /* ������ã������ */
          unsigned int index = vec_get(srcvec, i);
          if (lo->ap == (LmnWord)LMN_ATOM_GET_LINK((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))
              && lo->pos == LMN_ATOM_GET_ATTR((LmnAtomPtr)LINKED_ATOM(index), LINKED_ATTR(index))) {
            goto REMOVE_FREE_GROUND_CONT;
          }
        }

        for(i = 0; i < LMN_ATOM_GET_ARITY(lo->ap); i++) {
          LinkObj *next;
          if(i == lo->pos)
            continue;
          if(!LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(lo->ap, i))) {
            next = LinkObj_make((LmnWord)LMN_ATOM_GET_LINK(lo->ap, i), LMN_ATTR_GET_VALUE(LMN_ATOM_GET_ATTR(lo->ap, i)));
            vec_push(&stack, (LmnWord)next);
          }
          else { /* data atom ���Ѥޤʤ� */
            switch (op) {
              case INSTR_REMOVEGROUND:
                lmn_mem_remove_atom((LmnMembrane*)wt[memi], (LmnWord)LMN_ATOM_GET_LINK(lo->ap, i), LMN_ATOM_GET_ATTR(lo->ap, i));
                break;
              case INSTR_FREEGROUND:
                /* data atom����³���줿symbol atom��free�����Ȱ���free����� */
                break;
            }
          }
        }

        switch (op) {
          case INSTR_REMOVEGROUND:
            lmn_mem_remove_atom((LmnMembrane*)wt[memi], lo->ap, lo->pos);
            break;
          case INSTR_FREEGROUND:
            lmn_free_atom(lo->ap, lo->pos);
            break;
        }

REMOVE_FREE_GROUND_CONT:
        LMN_FREE(lo);
      } /* main loop: end */

      vec_destroy(&stack);
      hashset_destroy(&visited_atoms);
      break;
    }
    case INSTR_ISUNARY:
    {
      LmnInstrVar atomi;
      READ_VAL(LmnInstrVar, instr, atomi);

      if(!LMN_ATTR_IS_DATA(at[atomi]) &&
         LMN_ATOM_GET_ARITY((LmnAtomPtr)wt[atomi]) != 1) return FALSE;
      break;
    }
    case INSTR_ISINT:
    {
      LmnInstrVar atomi;
      READ_VAL(LmnInstrVar, instr, atomi);

      if (at[atomi] != LMN_INT_ATTR)
        return FALSE;
      break;
    }
    case INSTR_ISFLOAT:
    {
      LmnInstrVar atomi;
      READ_VAL(LmnInstrVar, instr, atomi);

      if(at[atomi] != LMN_DBL_ATTR)
        return FALSE;
      break;
    }
    case INSTR_ISINTFUNC:
    {
      LmnInstrVar funci;
      READ_VAL(LmnInstrVar, instr, funci);

      if(at[funci] != LMN_INT_ATTR)
        return FALSE;
      break;
    }
    case INSTR_ISFLOATFUNC:
    {
      LmnInstrVar funci;
      READ_VAL(LmnInstrVar, instr, funci);

      if(at[funci] != LMN_DBL_ATTR)
        return FALSE;
      break;
    }
    case INSTR_COPYATOM:
    {
      LmnInstrVar atom1, memi, atom2;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, atom2);

      at[atom1] = at[atom2];
      wt[atom1] = lmn_copy_atom(wt[atom2], at[atom2]);
      lmn_mem_push_atom((LmnMembrane *)wt[memi], wt[atom1], at[atom1]);
      break;
    }
    case INSTR_EQATOM:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      /* �ǡ������ȥ�ϣ������ʤΤ�,����̿�᤬�Ф�����Ǥ�
         �ǤϾ��FALSE�ΤϤ� */
      if (LMN_ATTR_IS_DATA(at[atom1]) ||
          LMN_ATTR_IS_DATA(at[atom2]) ||
          LMN_ATOM(wt[atom1]) != LMN_ATOM(wt[atom2]))
        return FALSE;
      break;
    }
    case INSTR_NEQATOM:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if (!(LMN_ATTR_IS_DATA(at[atom1]) ||
            LMN_ATTR_IS_DATA(at[atom2]) ||
            LMN_ATOM(wt[atom1]) != LMN_ATOM(wt[atom2])))
        return FALSE;
      break;
    }
    case INSTR_EQMEM:
    {
      LmnInstrVar mem1, mem2;

      READ_VAL(LmnInstrVar, instr, mem1);
      READ_VAL(LmnInstrVar, instr, mem2);
      if (wt[mem1] != wt[mem2]) return FALSE;
      break;
    }
    case INSTR_NEQMEM:
    {
      LmnInstrVar mem1, mem2;
      READ_VAL(LmnInstrVar, instr, mem1);
      READ_VAL(LmnInstrVar, instr, mem2);

      if(wt[mem1] == wt[mem2]) return FALSE;
      break;
    }
    case INSTR_STABLE:
    {
      LmnInstrVar memi;
      READ_VAL(LmnInstrVar, instr, memi);

      if (((LmnMembrane *)wt[memi])->is_activated)
      {
        return FALSE;
      }

      break;
    }
    case INSTR_NEWLIST:
    {
      LmnInstrVar listi;
      Vector *listvec = vec_make(16);
      READ_VAL(LmnInstrVar, instr, listi);
      wt[listi] = (LmnWord)listvec;
      if (lmn_env.nd || lmn_env.ltl) { at[listi] = 0; /* MC */ }
      /* �����Τ���κƵ� */
      if(interpret(instr, next_instr)) {
        vec_free(listvec);
        return TRUE;
      }
      else {
        vec_free(listvec);
        return FALSE;
      }
      break;
    }
    case INSTR_ADDTOLIST:
    {
      LmnInstrVar listi, linki;
      READ_VAL(LmnInstrVar, instr, listi);
      READ_VAL(LmnInstrVar, instr, linki);
      vec_push((Vector *)wt[listi], linki);
      break;
    }
    case INSTR_GETFROMLIST:
    {
      LmnInstrVar dsti, listi, posi;
      READ_VAL(LmnInstrVar, instr, dsti);
      READ_VAL(LmnInstrVar, instr, listi);
      READ_VAL(LmnInstrVar, instr, posi);

      switch (at[listi]) {
        case LIST_AND_MAP:
          wt[dsti] = vec_get((Vector *)wt[listi], (unsigned int)posi);
          if (posi == 0)
            at[dsti] = LINK_LIST;
          else if (posi == 1)
            at[dsti] = MAP;
          else
            assert(0);
          break;
        case LINK_LIST: /* LinkObj��free����ΤϤ����� */
        {
          LinkObj *lo = (LinkObj *)vec_get((Vector *)wt[listi], (unsigned int)posi);
          wt[dsti] = (LmnWord)lo->ap;
          at[dsti] = lo->pos;
          break;
        }
      }
      break;
    }
    case INSTR_IADD:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] + (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_ISUB:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] - (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IMUL:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] * (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IDIV:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] / (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_INEG:
    {
      LmnInstrVar dstatom, atomi;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atomi);
      wt[dstatom] = (LmnWord)(-(int)wt[atomi]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IMOD:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] % (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_INOT:
    {
      LmnInstrVar dstatom, atomi;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atomi);
      wt[dstatom] = (LmnWord)~(int)atomi;
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IAND:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] & (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IOR:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] | (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IXOR:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)((int)wt[atom1] ^ (int)wt[atom2]);
      at[dstatom] = LMN_INT_ATTR;
      break;
    }
    case INSTR_ILT:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!((int)wt[atom1] < (int)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_ILE:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!((int)wt[atom1] <= (int)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_IGT:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!((int)wt[atom1] > (int)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_IGE:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!((int)wt[atom1] >= (int)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_IEQ:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!((int)wt[atom1] == (int)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_INE:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!((int)wt[atom1] != (int)wt[atom2])) return FALSE;
      break;
    }
    case  INSTR_FADD:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)LMN_MALLOC(double);
      *(double *)wt[dstatom] = *(double *)wt[atom1] + *(double *)wt[atom2];
      at[dstatom] = LMN_DBL_ATTR;
      break;
    }
    case  INSTR_FSUB:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)LMN_MALLOC(double);
      *(double *)wt[dstatom] = *(double *)wt[atom1] - *(double *)wt[atom2];
      at[dstatom] = LMN_DBL_ATTR;
      break;
    }
    case  INSTR_FMUL:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)LMN_MALLOC(double);
      *(double *)wt[dstatom] = *(double *)wt[atom1] * *(double *)wt[atom2];
      at[dstatom] = LMN_DBL_ATTR;
      break;
    }
    case  INSTR_FDIV:
    {
      LmnInstrVar dstatom, atom1, atom2;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      wt[dstatom] = (LmnWord)LMN_MALLOC(double);
      *(double *)wt[dstatom] = *(double *)wt[atom1] / *(double *)wt[atom2];
      at[dstatom] = LMN_DBL_ATTR;
      break;
    }
    case  INSTR_FNEG:
    {
      LmnInstrVar dstatom, atomi;
      READ_VAL(LmnInstrVar, instr, dstatom);
      READ_VAL(LmnInstrVar, instr, atomi);

      wt[dstatom] = (LmnWord)LMN_MALLOC(double);
      *(double *)wt[dstatom] = -*(double *)wt[atomi];
      at[dstatom] = LMN_DBL_ATTR;
      break;
    }
    case INSTR_FLT:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!(*(double*)wt[atom1] < *(double*)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_FLE:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!(*(double*)wt[atom1] <= *(double*)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_FGT:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!(*(double*)wt[atom1] > *(double*)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_FGE:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!(*(double*)wt[atom1] >= *(double*)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_FEQ:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!(*(double*)wt[atom1] == *(double*)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_FNE:
    {
      LmnInstrVar atom1, atom2;
      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if(!(*(double*)wt[atom1] != *(double*)wt[atom2])) return FALSE;
      break;
    }
    case INSTR_ALLOCATOM:
    {
      LmnInstrVar atomi;
      LmnAtomPtr ap;
      LmnLinkAttr attr;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnLinkAttr, instr, attr);
      at[atomi] = attr;
      if (LMN_ATTR_IS_DATA(attr)) {
        READ_DATA_ATOM(wt[atomi], attr);
      } else { /* symbol atom */
        LmnFunctor f;
        fprintf(stderr, "symbol atom can't be created in GUARD\n");
        exit(EXIT_FAILURE);
        READ_VAL(LmnFunctor, instr, f);
        wt[atomi] = (LmnWord)ap;
      }
      break;
    }
    case INSTR_ALLOCATOMINDIRECT:
    {
      LmnInstrVar atomi;
      LmnFunctor funci;

      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnInstrVar, instr, funci);

      if (LMN_ATTR_IS_DATA(at[funci])) {
        wt[atomi] = lmn_copy_data_atom(wt[funci], at[funci]);
        at[atomi] = at[funci];
      } else { /* symbol atom */
        fprintf(stderr, "symbol atom can't be created in GUARD\n");
        exit(EXIT_FAILURE);
      }
      break;
    }
    case INSTR_SAMEFUNC:
    {
      LmnInstrVar atom1, atom2;

      READ_VAL(LmnInstrVar, instr, atom1);
      READ_VAL(LmnInstrVar, instr, atom2);

      if (!lmn_eq_func(wt[atom1], at[atom1], wt[atom2], at[atom2]))
        return FALSE;
      break;
    }
    case INSTR_GETFUNC:
    {
      LmnInstrVar funci, atomi;

      READ_VAL(LmnFunctor, instr, funci);
      READ_VAL(LmnInstrVar, instr, atomi);

      if(LMN_ATTR_IS_DATA(at[atomi])) {
        /* ����������ե��󥯥��ϥ�����̿����ǰ��Ū�˻Ȥ�������ʤΤ�
           double �ϥݥ��󥿤Υ��ԡ��ǽ�ʬ�ʤϤ� */
        wt[funci]=wt[atomi];
      }
      else {
        wt[funci] = (LmnWord)LMN_ATOM_GET_FUNCTOR(wt[atomi]);
      }
      at[funci] = at[atomi];

      break;
    }
    case INSTR_PRINTINSTR:
    {
      char c;

      while (TRUE) {
        READ_VAL(char, instr, c);
        if (!c) break;
        fprintf(stderr, "%c", c);
      }
      goto LOOP;
    }
    case INSTR_SETMEMNAME:
    {
      LmnInstrVar memi;
      lmn_interned_str name;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(lmn_interned_str, instr, name);
      ((LmnMembrane *)wt[memi])->name = name;
      break;
    }
    case INSTR_COPYRULES:
    {
      LmnInstrVar destmemi, srcmemi;
      unsigned int i;
      struct Vector *v;

      READ_VAL(LmnInstrVar, instr, destmemi);
      READ_VAL(LmnInstrVar, instr, srcmemi);
      v = &((LmnMembrane *)wt[srcmemi])->rulesets;
      for (i = 0; i< v->num; i++) {
        lmn_mem_add_ruleset((LmnMembrane *)wt[destmemi], (LmnRuleSet)vec_get(v, i));
      }
      break;
    }
    case INSTR_REMOVEPROXIES:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      lmn_mem_remove_proxies((LmnMembrane *)wt[memi]);
      break;
    }
    case INSTR_INSERTPROXIES:
    {
      LmnInstrVar parentmemi, childmemi;

      READ_VAL(LmnInstrVar, instr, parentmemi);
      READ_VAL(LmnInstrVar, instr, childmemi);
      lmn_mem_insert_proxies((LmnMembrane *)wt[parentmemi], (LmnMembrane *)wt[childmemi]);
      break;
    }
    case INSTR_DELETECONNECTORS:
    {
      LmnInstrVar srcset, srcmap;
      HashSet *delset;
      SimpleHashtbl *delmap;
      HashSetIterator it;
      READ_VAL(LmnInstrVar, instr, srcset);
      READ_VAL(LmnInstrVar, instr, srcmap);

      delset = (HashSet *)wt[srcset];
      delmap = (SimpleHashtbl *)wt[srcmap];

      for(it = hashset_iterator(delset); !hashsetiter_isend(&it); hashsetiter_next(&it)) {
        LmnAtomPtr orig = (LmnAtomPtr)hashsetiter_entry(&it);
        LmnAtomPtr copy = (LmnAtomPtr)hashtbl_get(delmap, (HashKeyType)orig);
        lmn_mem_unify_symbol_atom_args(copy, 0, copy, 1);
        /* mem ���ʤ��Τǻ����ʤ�ľ�ܥ��ȥ�ꥹ�Ȥ�Ĥʤ��Ѥ���
           UNIFY���ȥ��natom�˴ޤޤ�ʤ��Τ������ */
        LMN_ATOM_SET_PREV(LMN_ATOM_GET_NEXT(copy), LMN_ATOM_GET_PREV(copy));
        LMN_ATOM_SET_NEXT(LMN_ATOM_GET_PREV(copy), LMN_ATOM_GET_NEXT(copy));

        lmn_delete_atom(orig);
      }

      hashtbl_free(delmap);
      break;
    }
    case INSTR_REMOVETOPLEVELPROXIES:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      lmn_mem_remove_toplevel_proxies((LmnMembrane *)wt[memi]);
      break;
    }
    case INSTR_DEREFFUNC:
    {
      LmnInstrVar funci, atomi, pos;
      LmnLinkAttr attr;

      READ_VAL(LmnInstrVar, instr, funci);
      READ_VAL(LmnInstrVar, instr, atomi);
      READ_VAL(LmnLinkAttr, instr, pos);

      attr = LMN_ATOM_GET_ATTR(LMN_ATOM(wt[atomi]), pos);
      if (LMN_ATTR_IS_DATA(attr)) {
        wt[funci] = LMN_ATOM_GET_LINK(LMN_ATOM(wt[atomi]), pos);
      }
      else { /* symbol atom */
        wt[funci] = LMN_ATOM_GET_FUNCTOR(LMN_ATOM_GET_LINK(LMN_ATOM(wt[atomi]), pos));
      }
      at[funci] = attr;
      break;
    }
    case INSTR_LOADFUNC:
    {
      LmnInstrVar funci;
      LmnLinkAttr attr;

      READ_VAL(LmnFunctor, instr, funci);
      READ_VAL(LmnLinkAttr, instr, attr);
      at[funci] = attr;
      if(LMN_ATTR_IS_DATA(attr)) {
        READ_CONST_DATA_ATOM(wt[funci], attr);
      }
      else {
        LmnFunctor f;

        READ_VAL(LmnFunctor, instr, f);
        wt[funci] = (LmnWord)f;
      }
      break;
    }
    case INSTR_ADDATOM:
    {
      LmnInstrVar memi, atomi;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, atomi);
      lmn_mem_push_atom((LmnMembrane *)wt[memi], wt[atomi], at[atomi]);
      break;
    }
    case INSTR_MOVECELLS:
    {
      LmnInstrVar destmemi, srcmemi;

      READ_VAL(LmnInstrVar, instr, destmemi);
      READ_VAL(LmnInstrVar, instr, srcmemi);
      LMN_ASSERT(wt[destmemi] != wt[srcmemi]);
      lmn_mem_move_cells((LmnMembrane *)wt[destmemi], (LmnMembrane *)wt[srcmemi]);
      break;
    }
    case INSTR_REMOVETEMPORARYPROXIES:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      lmn_mem_remove_temporary_proxies((LmnMembrane *)wt[memi]);
      break;
    }
    case INSTR_NFREELINKS:
    {
      LmnInstrVar memi, count;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, count);

      if (!lmn_mem_nfreelinks((LmnMembrane *)wt[memi], count)) return FALSE;
      break;
    }
    case INSTR_COPYCELLS:
    {
      LmnInstrVar mapi, destmemi, srcmemi;

      READ_VAL(LmnInstrVar, instr, mapi);
      READ_VAL(LmnInstrVar, instr, destmemi);
      READ_VAL(LmnInstrVar, instr, srcmemi);
      wt[mapi] = (LmnWord)lmn_mem_copy_cells((LmnMembrane *)wt[destmemi],
                                             (LmnMembrane *)wt[srcmemi]);
      break;
    }
    case INSTR_LOOKUPLINK:
    {
      LmnInstrVar destlinki, tbli, srclinki;

      READ_VAL(LmnInstrVar, instr, destlinki);
      READ_VAL(LmnInstrVar, instr, tbli);
      READ_VAL(LmnInstrVar, instr, srclinki);

      at[destlinki] = LINKED_ATTR(srclinki);
      if (LMN_ATTR_IS_DATA(LINKED_ATTR(srclinki))) {
        wt[destlinki] = LINKED_ATOM(srclinki);
      }
      else { /* symbol atom */
        SimpleHashtbl *ht = (SimpleHashtbl *)wt[tbli];
        wt[destlinki] = hashtbl_get(ht, LINKED_ATOM(srclinki));
      }
      break;
    }
    case INSTR_CLEARRULES:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      vec_clear(&((LmnMembrane *)wt[memi])->rulesets);
      break;
    }
    case INSTR_DROPMEM:
    {
      LmnInstrVar memi;

      READ_VAL(LmnInstrVar, instr, memi);
      lmn_mem_drop((LmnMembrane *)wt[memi]);
      break;
    }
    case INSTR_TESTMEM:
    {
      LmnInstrVar memi, atomi;

      READ_VAL(LmnInstrVar, instr, memi);
      READ_VAL(LmnInstrVar, instr, atomi);
      LMN_ASSERT(!LMN_ATTR_IS_DATA(at[atomi]));
      LMN_ASSERT(LMN_IS_PROXY_FUNCTOR(LMN_ATOM_GET_FUNCTOR(wt[atomi])));

      if (LMN_PROXY_GET_MEM(wt[atomi]) != (LmnMembrane *)wt[memi]) return FALSE;
      break;
    }
    case INSTR_IADDFUNC:
    {
      LmnInstrVar desti, i0, i1;

      READ_VAL(LmnInstrVar, instr, desti);
      READ_VAL(LmnInstrVar, instr, i0);
      READ_VAL(LmnInstrVar, instr, i1);
      LMN_ASSERT(at[i0] == LMN_INT_ATTR);
      LMN_ASSERT(at[i1] == LMN_INT_ATTR);
      wt[desti] = wt[i0] + wt[i1];
      at[desti] = LMN_INT_ATTR;
      break;
    }
    case INSTR_ISUBFUNC:
    {
      LmnInstrVar desti, i0, i1;

      READ_VAL(LmnInstrVar, instr, desti);
      READ_VAL(LmnInstrVar, instr, i0);
      READ_VAL(LmnInstrVar, instr, i1);
      LMN_ASSERT(at[i0] == LMN_INT_ATTR);
      LMN_ASSERT(at[i1] == LMN_INT_ATTR);
      wt[desti] = wt[i0] - wt[i1];
      at[desti] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IMULFUNC:
    {
      LmnInstrVar desti, i0, i1;

      READ_VAL(LmnInstrVar, instr, desti);
      READ_VAL(LmnInstrVar, instr, i0);
      READ_VAL(LmnInstrVar, instr, i1);
      LMN_ASSERT(at[i0] == LMN_INT_ATTR);
      LMN_ASSERT(at[i1] == LMN_INT_ATTR);
      wt[desti] = wt[i0] * wt[i1];
      at[desti] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IDIVFUNC:
    {
      LmnInstrVar desti, i0, i1;

      READ_VAL(LmnInstrVar, instr, desti);
      READ_VAL(LmnInstrVar, instr, i0);
      READ_VAL(LmnInstrVar, instr, i1);
      LMN_ASSERT(at[i0] == LMN_INT_ATTR);
      LMN_ASSERT(at[i1] == LMN_INT_ATTR);
      wt[desti] = wt[i0] / wt[i1];
      at[desti] = LMN_INT_ATTR;
      break;
    }
    case INSTR_IMODFUNC:
    {
      LmnInstrVar desti, i0, i1;

      READ_VAL(LmnInstrVar, instr, desti);
      READ_VAL(LmnInstrVar, instr, i0);
      READ_VAL(LmnInstrVar, instr, i1);
      LMN_ASSERT(at[i0] == LMN_INT_ATTR);
      LMN_ASSERT(at[i1] == LMN_INT_ATTR);
      wt[desti] = wt[i0] % wt[i1];
      at[desti] = LMN_INT_ATTR;
      break;
    }
    case INSTR_GROUP:
    {
      LmnSubInstrSize subinstr_size;
      READ_VAL(LmnSubInstrSize, instr, subinstr_size);

      if (!interpret(instr, &instr)) return FALSE;
      instr += subinstr_size;
      break;
    }
    case INSTR_BRANCH:
    {
      LmnSubInstrSize subinstr_size;
      READ_VAL(LmnSubInstrSize, instr, subinstr_size);

      if (interpret(instr, &instr)) return TRUE;
      instr += subinstr_size;
      break;
    }
    default:
      fprintf(stderr, "interpret: Unknown operation %d\n", op);
      exit(1);
    }
/*     lmn_dump_mem((LmnMembrane *)wt[0]); */
/*     print_wt(); */

    #ifdef DEBUG
/*     print_wt(); */
    #endif
  }
}

/* DEBUG: */
/* static void print_wt(void) */
/* { */
/*   unsigned int i; */
/*   unsigned int end = 16; */

/*   fprintf(stderr, " wt: ["); */
/*   for (i = 0; i < end; i++) { */
/*     if (i>0) fprintf(stderr, ", "); */
/*     fprintf(stderr, "%lu", wt[i]); */
/*   } */
/*   fprintf(stderr, "]"); */
/*   fprintf(stderr, "\n"); */
/*   fprintf(stderr, " at: ["); */
/*   for (i = 0; i < end; i++) { */
/*     if (i>0) fprintf(stderr, ", "); */
/*     fprintf(stderr, "%u", at[i]); */
/*   } */
/*   fprintf(stderr, "]"); */
/*   fprintf(stderr, "\n"); */
/* } */

/* MC�������Ū�¹ԤΥᥤ��롼���� �������� */
/*----------------------------------------------------------------------*/

/* ��������롼�륻�åȤ�Ŭ�Ѥ��� */
/* must_be_activated_right_under�Ϥ�����ľ���ˤ����ƥ롼�뤬Ŭ�����졢
 * �����줬stable�Ǥ����ǽ�����ʤ��ʤä�����TRUE����������
 * ����ǥ롼�뤬ȿ��������⤢�뤿�ᡢFALSE�Ǥ��äƤ�ľ���ˤ����줬stable�Ǥ���Ȥϸ����ʤ�
 * ����ͤ򤽤Τޤ޸���ȥ����ƥ�롼�륻�åȤ�ȿ����ƨ������Ȥ��ʤ�
 */
static BOOL react_all_rulesets(LmnMembrane *cur_mem, BOOL *must_be_activated_right_under) {
  unsigned int i;
  struct Vector rulesets = cur_mem->rulesets; /* ����Υ롼�륻�åȤν��� */
  BOOL temp_must_be_activated = TRUE;

  mc_flags.system_rule_proceeded = FALSE;
  for (i = 0; i < rulesets.num; i++) {
    LmnRuleSet ruleset = (LmnRuleSet)vec_get(&rulesets, i); /* �롼�륻�å� */
    react_ruleset(cur_mem, ruleset); /* return FALSE */
  }
  if (!mc_flags.system_rule_proceeded) { /* �̾�Υ롼�륻�åȤ�Ŭ�ѤǤ��ʤ��ä���� */
    /* �����ƥ�롼�륻�åȤ�Ŭ�� */
    if (!react_ruleset(cur_mem, system_ruleset)) {
      /* ����Υ롼�륻�å�Ŭ�Ѹ������ʥ����ƥ�롼�륻�åȴޤ�ơ����Ƽ��Ԥ������ */
      temp_must_be_activated = FALSE;
    }
  }

  if(temp_must_be_activated){
    *must_be_activated_right_under = TRUE;
  }
  return mc_flags.system_rule_proceeded;
}

/*
 * ���֤�Ÿ������
 * �����Ȥ���Ϳ����줿��ȡ�������˽�°�������ƤΥ����ƥ�������Ф���
 * �롼��Ŭ�Ѹ�����Ԥ�
 * ���줫��롼��Ŭ�Ѥ�ԤäƤ���
 */
/* must_be_activated�Ϥ����줬stable�Ǥ����ǽ�����ʤ��ʤä��Ȥ���TRUE����������
 * ����ȼ��ȤΥ롼��Ŭ���ƥ��Ȥ���λ�����Ȥ�FALSE�ʤ餳�����stable�Ǥ���
 */
static BOOL expand_inner(LmnMembrane *cur_mem, BOOL *must_be_activated) {
  BOOL ret_flag = FALSE;
  for (; cur_mem; cur_mem = cur_mem->next) {
    BOOL temp_must_be_activated = FALSE;
    
    if (expand_inner(cur_mem->child_head, &temp_must_be_activated)) { /* ��ɽ������Ф��ƺƵ����� */
      ret_flag = TRUE;
    }

    if (cur_mem->is_activated && react_all_rulesets(cur_mem, &temp_must_be_activated)) {
      ret_flag = TRUE;
    }

    if(temp_must_be_activated){
      *must_be_activated = TRUE;
    }else{
      cur_mem->is_activated = FALSE;
    }
  }
  return ret_flag;
}

/* ���󥿡��ե�������ݻ����뤿��Υ�åѡ� */
static BOOL expand(LmnMembrane *cur_mem) {
  BOOL dummy = FALSE;
  return expand_inner(cur_mem, &dummy);
}

/* ����ĺ���Ǥ���ʤ��TRUE */
static inline int is_accepting(LmnMembrane *mem) {
  /* ��̾��"accept"��ޤ� */
  return NULL != strstr(LMN_MEM_NAME(mem), "accept");
}
/* TRUE���֤�ʤ��never�������ؤ���ã���̣����(i.e. ȿ��θ���) */
static inline int is_end_state(LmnMembrane *mem) {
  /* ��̾��"end"��ޤ� */
  return NULL != strstr(LMN_MEM_NAME(mem), "end");
}

static inline void violate() {
  unsigned int i;
  fprintf(stdout, "cycle(or error) found:\n");
  /* ��̽��� */
  for (i = 0; i < vec_num(&Stack); i++) {
    State *tmp_s = (State *)vec_get(&Stack, i);
    if (is_snd(tmp_s)) printf("*");
    printf("%d:\t", i);
    lmn_dump_mem(((State *)vec_get(&Stack, i))->mem);
  }
  fprintf(stdout, "\n");

  if (!lmn_env.ltl_all) { /* ��ĥ��顼�����Ĥ��ä��齪λ���� */
    exit(0);
  }
}

/**
 * �������(s)����ľ�����ܲ�ǽ�ʾ��֤򤹤٤�push���뤿��δؿ�
 * �ⳬ�ؿ�st_foreach(c.f. st.c)���ꤲ�ƻ��Ѥ��뤳�Ȥǡ��ơ��֥����s�μ��ξ��֤򼡡��ȼ��Ф��Ƥ���
 *
 * successor_state��(st_table *)expanded����������줿s�μ��ξ���
 */
static int gen_successor_states(st_data_t _k, st_data_t successor_state, void *current_state) {
  State *s = (State *)current_state;
  State *ss = (State *)successor_state;

  vec_push(&s->successor, (LmnWord)ss);

  /* s->successor�˥���ȥ꡼��push�����顤���Υ���ȥ꡼��free���� */
  return ST_DELETE;
}

/**
 * 2nd DFS�η�̼���ĺ�������Ĥ��ä��ݤˡ�States��Τ��٤ƤΥ���ȥ꡼(=����)��Ω�äƤ���
 * ��2nd DFS�¹Ի��˥����å���������ĺ���Ǥ��ץե饰�������롥
 * �ⳬ�ؿ�st_foreach(c.f. st.c)���ꤲ�ƻ��ѡ�
 */
static int unset_snd_all(st_data_t _k, st_data_t s, st_data_t _a) {
  unset_snd((State *)s);
  return ST_CONTINUE;
}

/*
 *  2�ʳ��ܤ�DFS
 *  ltl_search1()��ȯ����������ĺ���䡢�ܥ᥽�åɼ¹���˿�����ȯ����������ĺ���ν����
 *  ����Ŭ���ʼ���ĺ��������ã��ǽ�Ǥ��뤫�ɤ�����Ƚ�ꤹ�롣
 *  ���ʤ������ĺ���������ĺ���ػ����ϩ(������������)��¸�ߤ��ǧ���뤳�Ȥǡ�
 *  Ϳ����줿LTL������­����¹Է�ϩ��¸�ߤ��뤫�ݤ���Ƚ�ꤹ�롣
 */
static void ltl_search2() {
  unsigned int i, j;
  State *s = (State *)vec_peek(&Stack);

#ifdef DEBUG
  fprintf(stdout, "\n----- enter function: ltl_search2() -----\n");
  fprintf(stdout, "seed=");
  lmn_dump_mem(seed);
  fprintf(stdout, "\n");

  fprintf(stdout, "stack:\n");
  for(i = vec_num(&Stack) - 1; i >= 0; i--) {
    State *tmp_s = (State *)vec_get(&Stack, i);
    fprintf(stdout, "%d\n", is_snd(tmp_s));
    fprintf(stdout, "%d: (fst=%d,snd=%d):\t", i, is_fst(tmp_s) ? 1 : 0, is_snd(tmp_s) ? 1 : 0);
    lmn_dump_mem(tmp_s->mem);
  }
  fprintf(stdout, "\n");
#endif

  for(i = 0; i < vec_num(&s->successor); i++) { /* for each (s,l,s') */
    State *ss = (State *)vec_get(&s->successor, i);
    LmnMembrane *ssmem = ss->mem;

    BOOL on_stack = FALSE;

    for (j = 0; j < Stack.num; j++) {
      State *tmp_state = (State *)vec_get(&Stack, j);
      /* ������pointer��Ӥ����Ǥ������� */
      if (/*lmn_mem_equals((HashKeyType)tmp_state->mem, (HashKeyType)ssmem)*/
      tmp_state->mem==ssmem && is_fst(tmp_state)) {
        on_stack = TRUE;
      }
    }
    if (on_stack || lmn_mem_equals(seed, ssmem)) {
      violate();

#ifdef DEBUG
      fprintf(stdout, "+++++ return function: ltl_search2() +++++\n\n");
#endif
      return;
    }

    /* second DFS ��̤ˬ�䢪�Ƶ� */
    if(!is_snd(ss)) {
      set_snd(ss);
      ltl_search2();
    }
  }
}

/* nested(�ޤ���double)DFS�ˤ�����1�ʳ��ܤμ¹� */
void ltl_search1() {
  unsigned int i, j;
  State *s = (State *)vec_peek(&Stack);

#ifdef DEBUG
  fprintf(stdout, "\n----- enter function: ltl_search1() -----\n");
  fprintf(stdout, "seed=");
  lmn_dump_mem(seed);
  fprintf(stdout, "\n");

  fprintf(stdout, "stack:\n");
  for(i = vec_num(&Stack) - 1; i >= 0; i--) {
    State *tmp_s = (State *)vec_get(&Stack, i);
    fprintf(stdout, "%d: (fst=%d,snd=%d):\t", i, is_fst(tmp_s) ? 1 : 0, is_snd(tmp_s) ? 1 : 0);
    lmn_dump_mem(tmp_s->mem);
  }
  fprintf(stdout, "\n");
#endif

  if (is_end_state(s->mem->child_head)) {
    violate();
    unset_fst((State *)vec_pop(&Stack));
#ifdef DEBUG
    fprintf(stdout, "+++++ return function: ltl_search1() +++++\n\n");
#endif
    return;
  }

  /*
   * ����Ÿ��
   */
  for (i = 0; i < s->mem->rulesets.num; i++) {
    LmnRuleSet ruleset = (LmnRuleSet)vec_get(&s->mem->rulesets, i); /* �����롼�� */
    LmnRule rule;
    BYTE *inst_seq;
    LmnRuleInstr dummy;

    for (j = 0; j < lmn_ruleset_rule_num(ruleset); j++) {
      rule = lmn_ruleset_get_rule(ruleset, j);
      inst_seq = lmn_rule_get_inst_seq(rule);

      global_root = s->mem; /* ���ԡ����Ȥʤ륰���Х�롼���� */
      wt[0] = (LmnWord)s->mem; /* �����Х�롼���줬�����롼����ݻ����� */

      mc_flags.property_rule = TRUE;

      /**
       * �����롼��Ŭ��
       * �����Х��ѿ�global_root�������롼��Ŭ�ѷ�̤���Ǽ�����
       */
      if (interpret(inst_seq, &dummy)) {
        mc_flags.property_rule = FALSE;

        /**
         * global_root���ؤ�����Ф��ƥ����ƥ�롼��Ŭ�Ѹ�����Ԥ�
         * �����ƥ�롼��Ŭ�Ѥ�global_root���ؤ���Υ��ԡ����Ф��ƹԤ�
         */
        if (!expand(global_root->child_head)) { /* stutter extension */
          /* �����Х�롼����Υ��ԡ� */
          State *newstate;
          LmnMembrane *newmem = lmn_mem_make();
          SimpleHashtbl *copymap = copy_global_root(global_root, newmem);
          hashtbl_free(copymap);

          newstate = state_make(newmem);
          st_insert(expanded, newstate, (st_data_t)newstate);
#ifdef DEBUG
          fprintf(stdout, "stutter:\t");
          lmn_dump_mem(newmem);
#endif
        }
        lmn_mem_drop(global_root);
        lmn_mem_free(global_root);
      }
    }
  }

  if (expanded->num_entries > 0) {
    /* expanded�����Ƥ�State->succeccor����¸�������θ�
     * expanded��Υ���ȥ꡼�򤹤٤�free���� */
    state_succ_init(s, expanded->num_entries);
    st_foreach(expanded, gen_successor_states, s);

    for(j = 0; j < vec_num(&s->successor); j++) { /* for each (s,l,s') */
      State *ss = (State *)vec_get(&s->successor, j);
      State *ss_on_table;

      if (!st_lookup(States, (st_data_t)ss, (st_data_t *)&ss_on_table)) {
        st_add_direct(States, ss, (st_data_t)ss);
        /* push ��set �򣱤Ĥδؿ��ˤ��� */
        vec_push(&Stack, (LmnWord)ss);
        set_fst(ss);
        ltl_search1();
      }
      else { /* contains */
        vec_set(&s->successor, j, (LmnWord)ss_on_table);
        if(s->mem != ss->mem) {
          state_free(ss);
        }
        else { /* ���ʥ롼�פξ�����ϲ������ʤ� */
          vec_destroy(&ss->successor);
          LMN_FREE(ss);
        }
      }
    }

    /* entering second DFS */
    if (is_accepting(s->mem->child_head)) {
      seed = s->mem;

      set_snd(s);
      vec_push(&Stack, (LmnWord)s);
      ltl_search2();
      vec_pop(&Stack);

      /* reset hashset */
#ifdef DEBUG
      fprintf(stdout, "reset States\n\n");
#endif
      st_foreach(States, unset_snd_all, 0);
      seed = NULL;
    }
  }

#ifdef DEBUG
  fprintf(stdout, "+++++ return function: ltl_search1() +++++\n\n");
#endif

  unset_fst((State *)vec_pop(&Stack));
}

/**
 * �����(--nd �ޤ��� --nd_result)�¹Ի��ˡ�����current_state�μ��ξ��֤��������뤿��δؿ���
 * �ơ��֥�expanded����������줿���֤���¸�Ǥ���(����States���¸�ߤ���)���ݤ��˴�Ť���
 * ���ֶ��֤��ĥ���뤫�ݤ�Ƚ�Ǥ��롥�ⳬ�ؿ�st_foreach(c.f. st.c)���ꤲ�ƻ��ѡ�
 */
static int expand_states_and_stack(st_data_t k, st_data_t successor_state, void *current_state) {
  if (k != 0 && successor_state != 0) {
    State *s  = (State *)current_state;
    State *ss = (State *)successor_state;
    State *ss_on_table;

    if (!st_lookup(States, (st_data_t)ss, (st_data_t *)&ss_on_table)) {
      /* expanded�����Ƥ�State->successor����¸����ʿ����� */
      if (!lmn_env.nd_result) {
        vec_push(&s->successor, (LmnWord)ss);
      }

      st_add_direct(States, ss, (st_data_t)ss); /* ���ֶ��֤��ɲ� */
      vec_push(&Stack, (LmnWord)ss); /* �����å����ɲ� */
    } else {
      /* expanded�����Ƥ�State->successor����¸����ʹ�ή�� */
      if (!lmn_env.nd_result) {
        vec_push(&s->successor, (LmnWord)ss_on_table);
      }
      state_free(ss); /* free dupulicate state */
    }
    return ST_DELETE; /* Stack(�ޤ���s->successor)�˥���ȥ꡼�����Ƥ���¸�����餳�Υ���ȥ꡼��free���Ƥ��� */
  } else {
    /* ����bin�ˤϥ���ȥ꡼(st_table_entry)��¸�ߤ��ʤ��ΤǼ���bin������å����� */
    return ST_CONTINUE;
  }
}

/*
 * nondeterministic execution
 * ����ͥ������¹Է�ϩ���������
 */
void nd_exec() {

  while (vec_num(&Stack) != 0) {
    State *s = (State *)vec_peek(&Stack); /* Ÿ���� */
    if (!s->flags) { /* ���֤�̤Ÿ���Ǥ����� */
      global_root = s->mem; /* �����Х�롼���������ѿ�����Ͽ���� */
      s->flags = TRUE; /* Ÿ���Ѥߥե饰 */

      expand(s->mem); /* Ÿ�����expanded�˳�Ǽ���� */

      /* dump: execution result */
      if (lmn_env.nd_result && expanded->num_entries == 0) {
        unsigned int i, j;
        fprintf(stdout, "execution result:\n");
        for (i = 0, j = 0; i < vec_num(&Stack); i++) {
          State *dump_s = (State *)vec_get(&Stack, i);
          if (dump_s->flags) {
            fprintf(stdout, "%d(%10lu):\t", j++, (LmnWord)dump_s);
            lmn_dump_cell(dump_s->mem);
          }
        }
        fprintf(stdout, "\n");
      }

      /* expanded�����Ƥ�State->successor����¸���� */
      if (!lmn_env.nd_result && expanded->num_entries != 0) {
        state_succ_init(s, expanded->num_entries);
      }

      st_foreach(expanded, expand_states_and_stack, s);
    }
    else { /* s->toggle == TRUE */
      vec_pop(&Stack); /* ���֤�Ÿ���ѤߤǤ����� */
    }

    /* �����ʳ���expanded�϶��Ǥ���ɬ�פ����� */
    assert(expanded->num_entries == 0);
  }
}
/*----------------------------------------------------------------------*/
/* MC�������Ū�¹ԤΥᥤ��롼���� �����ޤ� */
