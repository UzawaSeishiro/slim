/*
 * membrane.c
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
 *    3. Neither the name of the Ueda Laboratory LMNtal Groupy LMNtal
 *       Group nor the names of its contributors may be used to
 *       endorse or promote products derived from this software
 *       without specific prior written permission.
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
 * $Id: membrane.c,v 1.34 2008/10/16 18:12:27 sasaki Exp $
 */

#include "membrane.h"
#include "atom.h"
#include "rule.h"
#include "dumper.h" /* for debug */
#include "functor.h"
#include <ctype.h>

/* �롼�륻�åȤ�����ɲä��� */
void lmn_mem_add_ruleset(LmnMembrane *mem, LmnRuleSet ruleset)
{
  Vector *v = &mem->rulesets;
  int i, n=vec_num(v);

  if (ruleset==NULL) LMN_ASSERT(FALSE);
  /* ��ʣ����������õ���򤷤Ƥ��� */
  for (i=0; i<n&&(vec_get(v, i)!=(LmnWord)ruleset); i++) ;
  if (i==n) {
    vec_push(&mem->rulesets, (LmnWord)ruleset);
  }
}

/*----------------------------------------------------------------------
 * Atom Set
 */

/* static BOOL atom_list_is_empty(AtomSetEntry *entry) */
/* { */
/*   return entry->head == (LmnAtomPtr)entry; */
/* } */

/* ���ȥ�ꥹ�Ȥ���ˤ���. */
#define EMPTY_ATOMLIST(X)                       \
  do {                                          \
    LMN_ATOM_SET_PREV((X), (X));                \
    LMN_ATOM_SET_NEXT((X), (X));                \
  } while (0)

/* ���������ȥ�ꥹ�Ȥ��� */
static AtomListEntry *make_atomlist()
{
  AtomListEntry *as = LMN_MALLOC(struct AtomListEntry);
  hashtbl_init(&as->record, 4);
  EMPTY_ATOMLIST(as);

  return as;
}

/* ���ȥ�ꥹ�Ȥβ������� */
static void free_atomlist(AtomListEntry *as)
{
  /* lmn_mem_move_cells�ǥ��ȥ�ꥹ�Ȥκ����Ѥ�ԤäƤ��ƥݥ��󥿤�NLL
     �ˤʤ��礬����Τǡ�������Ԥ�ɬ�פ����롣*/
  if (as) {
    hashtbl_destroy(&as->record);
    LMN_FREE(as);
  }
}

void mem_push_symbol_atom(LmnMembrane *mem, LmnAtomPtr atom)
{
  AtomListEntry *as;
  LmnFunctor f = LMN_ATOM_GET_FUNCTOR(atom);

  as = (AtomListEntry *)hashtbl_get_default(&mem->atomset, f, 0);
  if (!as) { /* ������˽��ƥ��ȥ�atom��PUSH���줿��� */
    as = make_atomlist();
    hashtbl_put(&mem->atomset, (HashKeyType)f, (HashValueType)as);
  }

  if (LMN_IS_PROXY_FUNCTOR(f)) {
    LMN_PROXY_SET_MEM(atom, (LmnWord)mem);
  }
  else if (f != LMN_UNIFY_FUNCTOR) {
    /* symbol atom except proxy and unify */
    mem->atom_num++;
  }

  LMN_ATOM_SET_NEXT(atom, as);
  LMN_ATOM_SET_PREV(atom, as->tail);
  LMN_ATOM_SET_NEXT(as->tail, atom);
  as->tail = (LmnWord)atom;
}

void lmn_mem_push_atom(LmnMembrane *mem, LmnWord atom, LmnLinkAttr attr)
{
  if (LMN_ATTR_IS_DATA(attr)) {
    mem->atom_num++;
  }
  else { /* symbol atom */
    mem_push_symbol_atom(mem, LMN_ATOM(atom));
  }
}

/* append e2 to e1 */
static inline void append_atomlist(AtomListEntry *e1, AtomListEntry *e2)
{
  if (atomlist_head(e2) != lmn_atomlist_end(e2)) {/* true if e2 is not empty */
    LMN_ATOM_SET_NEXT(e1->tail, e2->head);
    LMN_ATOM_SET_PREV(e2->head, e1->tail);
    LMN_ATOM_SET_NEXT(e2->tail, e1);
    e1->tail = e2->tail;
  }
  EMPTY_ATOMLIST(e2);
}

static inline void mem_remove_symbol_atom(LmnMembrane *mem, LmnAtomPtr atom)
{
  LmnFunctor f = LMN_ATOM_GET_FUNCTOR(atom);

  LMN_ATOM_SET_PREV(LMN_ATOM_GET_NEXT(atom), LMN_ATOM_GET_PREV(atom));
  LMN_ATOM_SET_NEXT(LMN_ATOM_GET_PREV(atom), LMN_ATOM_GET_NEXT(atom));

  if (LMN_IS_PROXY_FUNCTOR(f)) {
    LMN_PROXY_SET_MEM(atom,(LmnWord)NULL);
  }
  else if (f != LMN_UNIFY_FUNCTOR) {
    mem->atom_num--;
  }
}

void lmn_mem_remove_atom(LmnMembrane *mem, LmnWord atom, LmnLinkAttr attr)
{
  if (LMN_ATTR_IS_DATA(attr)) {
    mem->atom_num--;
  }
  else {
    mem_remove_symbol_atom(mem, LMN_ATOM(atom));
  }
}

/*----------------------------------------------------------------------
 * Membrane
 */

LmnMembrane *lmn_mem_make(void)
{
  LmnMembrane *mem = LMN_MALLOC(LmnMembrane);

  memset(mem, 0, sizeof(LmnMembrane)); /* set all data to 0 */
  vec_init(&mem->rulesets, 1);
  hashtbl_init(&mem->atomset, 16); /* EFFICIENCY: ����������Ϥ����Ĥ�Ŭ���� */
  return mem;
}

/* ����Υץ����Ȼ�����˴����� */
void lmn_mem_drop(LmnMembrane *mem)
{
  HashIterator iter;
  LmnMembrane *m, *n;

  /* drop and free child mems */
  m = mem->child_head;
  while (m) {
    n = m;
    m = m->next;
    lmn_mem_drop(n);
    lmn_mem_free(n);
  }
  mem->child_head = NULL;

  /* free all atoms */
  for (iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    AtomListEntry *ent = (AtomListEntry *)hashtbliter_entry(&iter)->data;
    LmnAtomPtr a = LMN_ATOM(ent->head), b;
    while (a != lmn_atomlist_end(ent)) {
      b = a;
      a = LMN_ATOM_GET_NEXT(a);
      free_symbol_atom_with_buddy_data(b);
    }
    EMPTY_ATOMLIST(ent);
  }
  mem->atom_num = 0;
}

/* ��mem�β�����Ԥ� */
void lmn_mem_free(LmnMembrane *mem)
{
  HashIterator iter;

  LMN_ASSERT(mem->atom_num == 0);
  /* free all atomlists  */
  for (iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    free_atomlist((AtomListEntry*)hashtbliter_entry(&iter)->data);
  }

  hashtbl_destroy(&mem->atomset);
  vec_destroy(&mem->rulesets);

  LMN_FREE(mem);
}

/* add newmem to parent child membranes */
void lmn_mem_add_child_mem(LmnMembrane *parentmem, LmnMembrane *newmem)
{
  newmem->prev = NULL;
  newmem->next = parentmem->child_head;
  newmem->parent = parentmem;
  LMN_ASSERT(parentmem);
  if(parentmem->child_head) parentmem->child_head->prev = newmem;
  parentmem->child_head = newmem;
}

/* return NULL when atomlist don't exists. */
AtomListEntry* lmn_mem_get_atomlist(LmnMembrane *mem, LmnFunctor f)
{
  return (AtomListEntry*)hashtbl_get_default(&mem->atomset, f, 0);
}

/* return NULL when atomlist don't exists. */
LmnAtomPtr* atomlist_get_record(AtomListEntry *atomlist, int findatomid)
{
  return (LmnAtomPtr*)hashtbl_get_default(&atomlist->record, findatomid, 0);
}

/* make atom which functor is f, and push atom into mem */
LmnAtomPtr lmn_mem_newatom(LmnMembrane *mem, LmnFunctor f)
{
  LmnAtomPtr atom = lmn_new_atom(f);
  mem_push_symbol_atom(mem, atom);
  return atom;
}

BOOL lmn_mem_natoms(LmnMembrane *mem, unsigned int count)
{
  return mem->atom_num == count;
}

BOOL lmn_mem_nmems(LmnMembrane *mem, unsigned int count)
{
	unsigned int i;
	LmnMembrane *mp = mem->child_head;
	for(i = 0; mp && i <= count; mp = mp->next, i++);
  return i == count;
}

/* return TRUE if # of freelinks in mem is equal to count */
/* EFFICIENCY: �ꥹ�Ȥ򤿤ɤäƿ�������Ƥ���Τ�O(n)��
   count������ۤ��礭���ʤ�ʤ��������Ϥʤ��� */
BOOL lmn_mem_nfreelinks(LmnMembrane *mem, unsigned int count)
{
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset,
                                                          LMN_IN_PROXY_FUNCTOR,
                                                          0);
  unsigned int n;
  LmnAtomPtr atom;

  if (!ent) return count == 0;
  for (atom = atomlist_head(ent), n = 0;
       atom != lmn_atomlist_end(ent) && n<=count;
       atom = LMN_ATOM_GET_NEXT(atom), n++) {}
  return count == n;
}

void lmn_mem_link_data_atoms(LmnMembrane *mem,
                             LmnWord d0,
                             LmnLinkAttr attr0,
                             LmnWord d1,
                             LmnLinkAttr attr1)
{
  LmnAtomPtr ap = lmn_new_atom(LMN_UNIFY_FUNCTOR);

  LMN_ATOM_SET_LINK(ap, 0, d0);
  LMN_ATOM_SET_LINK(ap, 1, d1);
  LMN_ATOM_SET_ATTR(ap, 0, attr0);
  LMN_ATOM_SET_ATTR(ap, 1, attr1);
  mem_push_symbol_atom(mem, ap);
}

/* atom1, atom2�򥷥�ܥ륢�ȥ�˸��ꤷ�� unify link */
void lmn_mem_unify_symbol_atom_args(LmnAtomPtr atom1,
                                    int pos1,
                                    LmnAtomPtr atom2,
                                    int pos2)
{
  LmnWord ap1 = LMN_ATOM_GET_LINK(atom1, pos1);
  LmnLinkAttr attr1 = LMN_ATOM_GET_ATTR(atom1, pos1);
  LmnWord ap2 = LMN_ATOM_GET_LINK(atom2, pos2);
  LmnLinkAttr attr2 = LMN_ATOM_GET_ATTR(atom2, pos2);

  LMN_ATOM_SET_LINK(ap2, attr2, (LmnWord)ap1);
  LMN_ATOM_SET_ATTR(ap2, attr2, attr1);
  LMN_ATOM_SET_LINK(ap1, attr1, (LmnWord)ap2);
  LMN_ATOM_SET_ATTR(ap1, attr1, attr2);
}

/* atom1, atom2�ϥ���ܥ륢�ȥ�ΤϤ� */
void lmn_mem_unify_atom_args(LmnMembrane *mem,
                   LmnAtomPtr atom1,
                   int pos1,
                   LmnAtomPtr atom2,
                   int pos2)
{
  LmnWord ap1 = LMN_ATOM_GET_LINK(atom1, pos1);
  LmnLinkAttr attr1 = LMN_ATOM_GET_ATTR(atom1, pos1);
  LmnWord ap2 = LMN_ATOM_GET_LINK(atom2, pos2);
  LmnLinkAttr attr2 = LMN_ATOM_GET_ATTR(atom2, pos2);

  if(LMN_ATTR_IS_DATA(attr1) && LMN_ATTR_IS_DATA(attr2)) {
    lmn_mem_link_data_atoms(mem, ap1, attr1, ap2, attr2);
  }
  else if (LMN_ATTR_IS_DATA(attr1)) {
    LMN_ATOM_SET_LINK(ap2, attr2, (LmnWord)ap1);
    LMN_ATOM_SET_ATTR(ap2, attr2, attr1);
  }
  else if (LMN_ATTR_IS_DATA(attr2)) {
    LMN_ATOM_SET_LINK(ap1, attr1, (LmnWord)ap2);
    LMN_ATOM_SET_ATTR(ap1, attr1, attr2);
  }
  else {
    LMN_ATOM_SET_LINK(ap2, attr2, (LmnWord)ap1);
    LMN_ATOM_SET_ATTR(ap2, attr2, attr1);
    LMN_ATOM_SET_LINK(ap1, attr1, (LmnWord)ap2);
    LMN_ATOM_SET_ATTR(ap1, attr1, attr2);
  }
}

/* ����ܥ륢�ȥ�˸��ꤷ��newlink */
void lmn_newlink_in_symbols(LmnAtomPtr atom0,
                            int pos0,
                            LmnAtomPtr atom1,
                            int pos1)
{
  LMN_ATOM_SET_LINK(atom0, pos0, (LmnWord)atom1);
  LMN_ATOM_SET_LINK(atom1, pos1, (LmnWord)atom0);
  LMN_ATOM_SET_ATTR(atom0, pos0, pos1);
  LMN_ATOM_SET_ATTR(atom1, pos1, pos0);
}

/* ����ܥ륢�ȥ�atom0��, ����ܥ�or�ǡ������ȥ� atom1 �δ֤˥�󥯤�ĥ��
   ���Υ����ɤ���ʣ���Ƹ��줿�Τ�,�ؿ���ʬ�䤷�� */
static inline void newlink_symbol_and_something(LmnAtomPtr atom0,
                                                int pos,
                                                LmnWord atom1,
                                                LmnLinkAttr attr)
{
  LMN_ATOM_SET_LINK(atom0, pos, atom1);
  LMN_ATOM_SET_ATTR(atom0, pos, attr);
  if (!LMN_ATTR_IS_DATA(attr)) {
    LMN_ATOM_SET_LINK(LMN_ATOM(atom1), LMN_ATTR_GET_VALUE(attr), (LmnWord)atom0);
    LMN_ATOM_SET_ATTR(LMN_ATOM(atom1), LMN_ATTR_GET_VALUE(attr), LMN_ATTR_MAKE_LINK(pos));
  }
}

void lmn_mem_newlink(LmnMembrane *mem,
                     LmnWord atom0,
                     LmnLinkAttr attr0,
                     int pos0,
                     LmnWord atom1,
                     LmnLinkAttr attr1,
                     int pos1)
{
  if (LMN_ATTR_IS_DATA(attr0)) {
    if (LMN_ATTR_IS_DATA(attr1)) { /* both data */
      LMN_ASSERT(pos0 == 0 && pos1 == 0);
      lmn_mem_link_data_atoms(mem, atom0, pos0, atom1, pos1);
    }
    else { /* atom0 data, atom1 symbol */
      LMN_ATOM_SET_LINK(LMN_ATOM(atom1), pos1, atom0);
      LMN_ATOM_SET_ATTR(LMN_ATOM(atom1), pos1, attr0);
    }
  }
  else if (LMN_ATTR_IS_DATA(attr1)) { /* atom0 symbol, atom1 data */
    LMN_ATOM_SET_LINK(LMN_ATOM(atom0), pos0, atom1);
    LMN_ATOM_SET_ATTR(LMN_ATOM(atom0), pos0, attr1);
  }
  else { /* both symbol */
    lmn_newlink_in_symbols(LMN_ATOM(atom0), pos0, LMN_ATOM(atom1), pos1);
  }
}

void lmn_mem_relink_atom_args(LmnMembrane *mem,
                              LmnWord atom0,
                              LmnLinkAttr attr0,
                              int pos0,
                              LmnWord atom1,
                              LmnLinkAttr attr1,
                              int pos1)
{
  /* TODO: relink�Ǥ�atom0,atom1���ǡ����ˤʤ뤳�ȤϤʤ��Ϥ�
           ���Τ��Ȥ��ǧ���� */
  LMN_ASSERT(!LMN_ATTR_IS_DATA(attr0) &&
             !LMN_ATTR_IS_DATA(attr1));

  newlink_symbol_and_something(LMN_ATOM(atom0),
                               pos0,
                               LMN_ATOM_GET_LINK(LMN_ATOM(atom1), pos1),
                               LMN_ATOM_GET_ATTR(LMN_ATOM(atom1), pos1));
}

void lmn_mem_move_cells(LmnMembrane *destmem, LmnMembrane *srcmem)
{
  /* move atoms */
  {
    HashIterator iter;

    for (iter = hashtbl_iterator(&srcmem->atomset);
         !hashtbliter_isend(&iter);
         hashtbliter_next(&iter)) {
      LmnFunctor f = (LmnFunctor)hashtbliter_entry(&iter)->key;
      AtomListEntry *srcent = (AtomListEntry *)hashtbliter_entry(&iter)->data;
      AtomListEntry *destent = lmn_mem_get_atomlist(destmem, f);

      if (LMN_IS_PROXY_FUNCTOR(f)) {
        LmnAtomPtr a;
        for (a = atomlist_head(srcent);
             a != lmn_atomlist_end(srcent);
             a = LMN_ATOM_GET_NEXT(a)) {
          LMN_PROXY_SET_MEM(a, (LmnWord)destmem);
        }
      }

      if (destent) { /* Ʊ���ե��󥯥��Υ��ȥ�ꥹ�Ȥ������� */
        append_atomlist(destent, srcent);
      }
      else {
        /* ���ȥ�ꥹ�Ȥ�����Ѥ���dest�˰ܤ� */
        hashtbliter_entry(&iter)->data = 0; /* free����ʤ��褦�� NULL �ˤ��� */
        hashtbl_put(&destmem->atomset, (HashKeyType)f, (HashValueType)srcent);
      }
    }
    destmem->atom_num += srcmem->atom_num;
    srcmem->atom_num = 0;
  }

  /* move membranes */
  {
    LmnMembrane *m, *next;

    for (m = srcmem->child_head; m; m = next) {
      next = m->next;
      lmn_mem_add_child_mem(destmem, m);
    }
  }
}

#define REMOVE            1
#define STATE(ATOM)        (LMN_ATOM_GET_ATTR((ATOM), 2))
#define SET_STATE(ATOM,S)  (LMN_ATOM_SET_ATTR((ATOM), 2, (S)))

static inline void alter_functor(LmnMembrane *mem, LmnAtomPtr atom, LmnFunctor f)
{
  mem_remove_symbol_atom(mem, atom);
  LMN_ATOM_SET_FUNCTOR(atom, f);
  mem_push_symbol_atom(mem, atom);
}

/* cf. Java������ */
/*
 * TODO:
 * �ȤƤ����Ψ�ʤΤǡ�������REMOVE������Ȥä��������᤹��
 * HashSet��Ȥ��褦�ˤ���
 */
void lmn_mem_remove_proxies(LmnMembrane *mem)
{
  unsigned int i;
  Vector remove_list, change_list;
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset,
      LMN_OUT_PROXY_FUNCTOR,
      0);

  vec_init(&remove_list, 16);
  vec_init(&change_list, 16);

  if (ent) {
    LmnAtomPtr opxy;

    for (opxy = atomlist_head(ent);
        opxy != lmn_atomlist_end(ent);
        opxy = LMN_ATOM_GET_NEXT(opxy)) {
      LmnAtomPtr a0 = LMN_ATOM(LMN_ATOM_GET_LINK(opxy, 0));
      if (LMN_PROXY_GET_MEM(a0)->parent != mem && /* opxy�Υ���褬����Ǥʤ���� */
          !LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(opxy, 1))) {
        LmnAtomPtr a1 = LMN_ATOM(LMN_ATOM_GET_LINK(opxy, 1));
        LmnFunctor f1 = LMN_ATOM_GET_FUNCTOR(a1);
        if (f1 == LMN_IN_PROXY_FUNCTOR) { /* (1) */
          lmn_mem_unify_atom_args(mem, opxy, 0, a1, 0);
          vec_push(&remove_list, (LmnWord)opxy);
          vec_push(&remove_list, (LmnWord)a1);
        }
        else {
          if (f1 == LMN_OUT_PROXY_FUNCTOR &&
              LMN_PROXY_GET_MEM(LMN_ATOM_GET_LINK(a1, 0))->parent != mem) { /* (3) */
            if (!vec_contains(&remove_list, (LmnWord)opxy)) {
              lmn_mem_unify_atom_args(mem, opxy, 0, a1, 0);
              vec_push(&remove_list, (LmnWord)opxy);
              vec_push(&remove_list, (LmnWord)a1);
            }
          } else { /* (2) */
            vec_push(&change_list, (LmnWord)opxy);
          }
        }
      }
    }
  }

  for (i = 0; i < vec_num(&remove_list); i++) {
    mem_remove_symbol_atom(mem, LMN_ATOM(vec_get(&remove_list, i)));
    lmn_delete_atom(LMN_ATOM(vec_get(&remove_list, i)));
  }
  vec_destroy(&remove_list);


  /* add inside proxy to change list */
  ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset,
      LMN_IN_PROXY_FUNCTOR,
      0);
  if (ent) {
    LmnAtomPtr a;
    /* clear mem attribute */
    for (a = atomlist_head(ent);
        a != lmn_atomlist_end(ent);
        a = LMN_ATOM_GET_NEXT(a)) {
      vec_push(&change_list, (LmnWord)a);
    }
  }

  { /* change to star proxy */
    for (i = 0; i < change_list.num; i++) {
      alter_functor(mem, LMN_ATOM(vec_get(&change_list, i)), LMN_STAR_PROXY_FUNCTOR);
    }
  }
  vec_destroy(&change_list);
}

/* cf. Java������ */
/*
 * TODO:
 * �ȤƤ����Ψ�ʤΤǡ�������REMOVE������Ȥä��������᤹��
 * HashSet��Ȥ��褦�ˤ���
 */
void lmn_mem_insert_proxies(LmnMembrane *mem, LmnMembrane *child_mem)
{
  unsigned int i;
  Vector remove_list, change_list;
  LmnAtomPtr star, oldstar;
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&child_mem->atomset,
      LMN_STAR_PROXY_FUNCTOR,
      0);

  if (!ent) return;

  vec_init(&remove_list, 16);
  vec_init(&change_list, 16); /* inside proxy �ˤ��륢�ȥ� */

  for (star = atomlist_head(ent);
      star != lmn_atomlist_end(ent);
      star = LMN_ATOM_GET_NEXT(star)) {
    oldstar = LMN_ATOM(LMN_ATOM_GET_LINK(star, 0));
    if (LMN_PROXY_GET_MEM(oldstar) == child_mem) { /* (1) */
      if (!vec_contains(&remove_list, (LmnWord)star)) {
        lmn_mem_unify_atom_args(child_mem, star, 1, oldstar, 1);
        vec_push(&remove_list, (LmnWord)star);
        vec_push(&remove_list, (LmnWord)oldstar);
      }
    }
    else {
      vec_push(&change_list, (LmnWord)star);

      if (LMN_PROXY_GET_MEM(oldstar) == mem) { /* (2) */
        alter_functor(mem, oldstar, LMN_OUT_PROXY_FUNCTOR);
        lmn_newlink_in_symbols(star, 0, oldstar, 0);
      } else { /* (3) */
        LmnAtomPtr outside = lmn_mem_newatom(mem, LMN_OUT_PROXY_FUNCTOR);
        LmnAtomPtr newstar = lmn_mem_newatom(mem, LMN_STAR_PROXY_FUNCTOR);
        lmn_newlink_in_symbols(outside, 1, newstar, 1);
        lmn_mem_relink_atom_args(mem,
            (LmnWord)newstar,
            LMN_ATTR_MAKE_LINK(0),
            0,
            (LmnWord)star,
            LMN_ATTR_MAKE_LINK(0),
            0);
        lmn_newlink_in_symbols(star, 0, outside, 0);
      }
    }
  }

  {
    for (i = 0; i < vec_num(&change_list); i++) {
      alter_functor(child_mem, LMN_ATOM(vec_get(&change_list, i)), LMN_IN_PROXY_FUNCTOR);
    }
  }
  vec_destroy(&change_list);

  for (i = 0; i < vec_num(&remove_list); i++) {
    mem_remove_symbol_atom(mem, LMN_ATOM(vec_get(&remove_list, i)));
    lmn_delete_atom(LMN_ATOM(vec_get(&remove_list, i)));
  }
  vec_destroy(&remove_list);
}

/* cf. Java������ */
/*
 * TODO:
 * �ȤƤ����Ψ�ʤΤǡ�������REMOVE������Ȥä��������᤹��
 * HashSet��Ȥ��褦�ˤ���
 */
void lmn_mem_remove_temporary_proxies(LmnMembrane *mem)
{
  unsigned int i;
  Vector remove_list;
  LmnAtomPtr star, outside;
  AtomListEntry *ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset,
      LMN_STAR_PROXY_FUNCTOR,
      0);

  if (!ent) return;

  vec_init(&remove_list, 16);

  for (star = atomlist_head(ent);
      star != lmn_atomlist_end(ent);
      star = LMN_ATOM_GET_NEXT(star)) {
    outside = LMN_ATOM(LMN_ATOM_GET_LINK(star, 0));
    if (!vec_contains(&remove_list, (LmnWord)star)) {
      lmn_mem_unify_atom_args(mem, star, 1, outside, 1);
      vec_push(&remove_list, (LmnWord)star);
      vec_push(&remove_list, (LmnWord)outside);
    }
  }
  for (i = 0; i < remove_list.num; i++) {
    mem_remove_symbol_atom(mem, LMN_ATOM(vec_get(&remove_list, i)));
    lmn_delete_atom(LMN_ATOM(vec_get(&remove_list, i)));
  }

  vec_destroy(&remove_list);
}

/* cf. Java������ */
/*
 * TODO:
 * �ȤƤ����Ψ�ʤΤǡ�������REMOVE������Ȥä��������᤹��
 * HashSet��Ȥ��褦�ˤ���
 */
void lmn_mem_remove_toplevel_proxies(LmnMembrane *mem)
{
  Vector remove_list;
  AtomListEntry *ent;
  LmnAtomPtr outside;
  unsigned int i;

  ent = (AtomListEntry *)hashtbl_get_default(&mem->atomset,
      LMN_OUT_PROXY_FUNCTOR,
      0);
  if (!ent) return;

  vec_init(&remove_list, 16);

  for (outside = atomlist_head(ent);
      outside != lmn_atomlist_end(ent);
      outside = LMN_ATOM_GET_NEXT(outside)) {
    LmnAtomPtr a0;
    a0 = LMN_ATOM(LMN_ATOM_GET_LINK(outside, 0));
    if (LMN_PROXY_GET_MEM(a0) &&
        LMN_PROXY_GET_MEM(a0)->parent != mem) {
      if (!LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(outside, 1))) {
        LmnAtomPtr a1 = LMN_ATOM(LMN_ATOM_GET_LINK(outside, 1));
        if (LMN_ATOM_GET_FUNCTOR(a1) == LMN_OUT_PROXY_FUNCTOR) {
          LmnAtomPtr a10 = LMN_ATOM(LMN_ATOM_GET_LINK(a1, 0));
          if (LMN_PROXY_GET_MEM(a10) &&
              LMN_PROXY_GET_MEM(a10)->parent != mem) {
            if (!vec_contains(&remove_list, (LmnWord)outside)) {
              lmn_mem_unify_atom_args(mem, outside, 0, a1, 0);
              vec_push(&remove_list, (LmnWord)outside);
              vec_push(&remove_list, (LmnWord)a1);
            }
          }
        }
      }
    }
  }

  for (i = 0; i < remove_list.num; i++) {
    mem_remove_symbol_atom(mem, LMN_ATOM(vec_get(&remove_list, i)));
    lmn_delete_atom(LMN_ATOM(vec_get(&remove_list, i)));
  }
  vec_destroy(&remove_list);
}

/* mem -> atoms ��hashtbl��old atom -> newatom��hashtbl��Ĥ�����Ǥ���.
   �����ɬ����˥��ԡ����롣������,������ȡ���Ĥ�hashtbl���礭���ʤäƤ��ޤ�
   ���꤬����
   071204 oldatom->newatom��hashtbl��Ĥ����礷��
*/
SimpleHashtbl *lmn_mem_copy_cells(LmnMembrane *destmem, LmnMembrane *srcmem)
{
  SimpleHashtbl *atoms;
  LmnMembrane *m;
  HashIterator iter;
  unsigned int i;

  atoms = hashtbl_make(srcmem->atom_num * 2);

  /* copy child mems */
  for (m = srcmem->child_head; m; m = m->next) {
    LmnMembrane *new_mem = lmn_mem_make();
    SimpleHashtbl *child_mem_atoms = lmn_mem_copy_cells(new_mem, m);
    lmn_mem_add_child_mem(destmem, new_mem);

    hashtbl_put(atoms, (HashKeyType)m, (HashValueType)new_mem);
    for (iter = hashtbl_iterator(child_mem_atoms); !hashtbliter_isend(&iter); hashtbliter_next(&iter)) {
      hashtbl_put(atoms, hashtbliter_entry(&iter)->key, hashtbliter_entry(&iter)->data);
    }
    hashtbl_free(child_mem_atoms);
    /* copy name */
    new_mem->name = m->name;
    /* copy rulesets */
    for (i = 0; i < m->rulesets.num; i++) {
      vec_push(&new_mem->rulesets, vec_get(&m->rulesets, i));
    }
  }

  /* copy atoms */
  for (iter = hashtbl_iterator(&srcmem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    AtomListEntry *ent = (AtomListEntry *)hashtbliter_entry(&iter)->data;
    LmnAtomPtr srcatom;

    LMN_ASSERT(ent);

    for (srcatom = atomlist_head(ent);
         srcatom != lmn_atomlist_end(ent);
         srcatom = LMN_ATOM_GET_NEXT(srcatom)) {
      LmnFunctor f;
      LmnAtomPtr newatom;
      unsigned int start, end;

      if (hashtbl_contains(atoms, (HashKeyType)srcatom)) continue;
      f = LMN_ATOM_GET_FUNCTOR(srcatom);
      newatom = lmn_mem_newatom(destmem, f);
      hashtbl_put(atoms, (HashKeyType)srcatom, (HashValueType)newatom);
      start = 0;
      end = LMN_ATOM_GET_ARITY(srcatom);

      if (LMN_IS_PROXY_FUNCTOR(f)) {
        start = 1, end = 2;
        LMN_PROXY_SET_MEM(newatom, (LmnWord)destmem);
        if (f == LMN_OUT_PROXY_FUNCTOR) {
          LmnAtomPtr srcinside = LMN_ATOM(LMN_ATOM_GET_LINK(srcatom, 0));
          LmnAtomPtr newinside = LMN_ATOM(hashtbl_get(atoms, (HashKeyType)srcinside));
          /* ɬ������ˤĤʤ��äƤ���Ϥ� */
          LMN_ASSERT(LMN_ATOM_GET_FUNCTOR(srcinside) == LMN_IN_PROXY_FUNCTOR &&
              LMN_PROXY_GET_MEM(srcinside)->parent == LMN_PROXY_GET_MEM(srcatom));
          lmn_newlink_in_symbols(newatom, 0, newinside, 0);
        }
      }

      /* ��������³ */
      for (i = start; i < end; i++) {
        LmnLinkAttr attr = LMN_ATOM_GET_ATTR(srcatom, i);
        if(!(LMN_INT_ATTR == attr) && hashtbl_contains(atoms, LMN_ATOM_GET_LINK(srcatom, i))) {
          newlink_symbol_and_something(newatom, i, hashtbl_get(atoms, LMN_ATOM_GET_LINK(srcatom, i)), attr);
        } else { /* LMN_INT_ATTR == attr || !hashtbl_contains() */
          LmnWord newargatom = lmn_copy_atom(LMN_ATOM_GET_LINK(srcatom, i), attr);
          newlink_symbol_and_something(newatom, i, newargatom, attr);
          if (!(LMN_INT_ATTR == attr)) {
            /* TODO: �ǡ������ȥ�ΰ������������� */
            /*hashtbl_put(atoms, LMN_ATOM_GET_LINK(srcatom, i), newargatom);*/
          }
        }
      }
    }
  }
  destmem->atom_num = srcmem->atom_num;

  /* copy activated flag */
  destmem->is_activated = srcmem->is_activated; /* MC */

  return atoms;
}

/* to get # of descendant membranes */
inline unsigned int lmn_mem_count_descendants(LmnMembrane *mem) {
  unsigned int n = 0;
  LmnMembrane *m_child;

  for (m_child = mem->child_head; m_child; m_child = m_child->next) {
    n += 1 + lmn_mem_count_descendants(m_child);
  }
  return n;
}

/* to get # of child membranes */
inline unsigned int lmn_mem_count_children(LmnMembrane *mem) {
  unsigned int n = 0;
  LmnMembrane *m_child;

  for (m_child = mem->child_head; m_child; m_child = m_child->next) {
    ++n;
  }
  return n;
}

/* ���Ʊ����Ƚ�� �������� */
/*----------------------------------------------------------------------*/
typedef struct AtomVecData {
  LmnFunctor fid;
  Vector *atom_ptrs;
} atomvec_data;

/* ��¤��'atomvec_data'�˴ط�������꡼���ΰ��������� */
static void free_atomvec_data(Vector *vec) {
  unsigned int i;

  for (i = 0; i < vec_num(vec); ++i) {
    if (vec_get(vec, i)) {
      vec_free(((atomvec_data *)vec_get(vec, i))->atom_ptrs);
      LMN_FREE((atomvec_data *)vec_get(vec, i));
    }
  }
  vec_free(vec);
}

/* ����ľ���Τ��٤ƤΥ��ȥ�(��¹����Υ��ȥ�ϴޤޤʤ�)�ˤĤ��ơ��ޤ��ե��󥯥����Ȥ˥��롼��ʬ����Ԥ���
 * ���롼��ʬ���ˤϹ�¤��'atomvec_data'���Ѥ��롣��¤��'atomvec_data'����������륢�ȥ�Υե��󥯥���ID(= 0,1,2,...)�ȡ�
 * ���Υե��󥯥�����������ȥ�Υ��ɥ쥹���������٥�����'atom_ptrs'�Ȥ����Ȥ��ƻ��ġ�
 * ����ľ���Τ��٤ƤΥ��ȥ�Υ��ɥ쥹�򡢤��줾���б����빽¤��'atomvec_data'�����������
 * ���ι�¤�Τ��ܥ᥽�åɤ�����ͤȤʤ�٥����������ǤȤ��Ƴ�Ǽ���Ƥ�����
 *
 * ��Ʊ����Ƚ��Υ��르�ꥺ��Ǥϡ��־����ɤΥ��ȥ�פ���ޥå��󥰤򳫻ϤǤ���褦�ˤ��뤳�Ȥ�
 * ��ɸ�Ȥ��Ƥ��뤿�ᡢ������¤�Τ����������٥������ˤĤ��ơ���¤����Υ��ȥ������¿����פ˥����Ȥ��Ƥ��ɬ�פ����롣
 * �����ǡ�¿����פȤ�����ͳ�ϡ���Υ��ƥåפ��ܥ٥�������(���Ū��)POP���ʤ��饢�ȥ�Υ��ɥ쥹�������Ф����Ȥ�
 * �ʤ뤿��Ǥ��롣(�����ɤΥ��ȥफ����Ф���뤳�Ȥˤʤ뤳�Ȥ���դ���) */
static Vector *lmn_mem_mk_matching_vec(LmnMembrane *mem) {
  Vector *vec, *v_tmp;
  HashIterator atom_iter;
  LmnFunctor f;
  LmnAtomPtr a;
  AtomListEntry *ent;
  unsigned int anum_max; /* �����¸�ߤ��륢�ȥ��ե��󥯥���˥��롼�ײ������ݤΡ�������礭���κ����� */
  int i, j;

  vec = vec_make(1);
  memset(vec->tbl, 0, sizeof(atomvec_data *) * vec->cap);
  anum_max = 0;

  for (atom_iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&atom_iter);
       hashtbliter_next(&atom_iter)) {
    f = hashtbliter_entry(&atom_iter)->key;
    ent = (AtomListEntry *)hashtbliter_entry(&atom_iter)->data;

    atomvec_data *ad = LMN_MALLOC(atomvec_data);
    ad->fid = f;
    ad->atom_ptrs = vec_make(1);
    vec_push(vec, (LmnWord)ad);

    /* ����ľ���Υ��ȥ���⡢�ե��󥯥���f�Ǥ����ΤΥ��ɥ쥹��٥�����atom_ptrs����������롣
     * ��ǥ����Ȥ���ط��ǡ��Ǥ�¿���Υ��ȥ�Υ��ɥ쥹��������빽¤��(atomvec_data)��Υ��ȥ������Ƥ��롣 */
    for (a = atomlist_head(ent);
         a != lmn_atomlist_end(ent);
         a = LMN_ATOM_GET_NEXT(a)) {
      vec_push(((atomvec_data *)vec_peek(vec))->atom_ptrs, (LmnWord)a);
      if (vec_num(((atomvec_data *)vec_peek(vec))->atom_ptrs) > anum_max) {
        anum_max = vec_num(((atomvec_data *)vec_peek(vec))->atom_ptrs);
      }
    }
    /* �ե��󥯥�f����ĥ��ȥब�������1�Ĥ�¸�ߤ��ʤ���硢���Υե��󥯥��Τ���˳䤤�����꡼���ΰ��������롣
     * ������դ�ȥ���꡼����������Τ����!! */
    if (vec_peek(vec) && vec_is_empty(((atomvec_data *)vec_peek(vec))->atom_ptrs)) {
      vec_free(((atomvec_data *)vec_peek(vec))->atom_ptrs);
      LMN_FREE((atomvec_data *)vec_pop(vec));
    }
  }

  /* sort */
  if (anum_max > 0) {
    v_tmp = vec_make(vec_num(vec));

    for (i = 1; i <= anum_max; ++i) {
      for (j = 0; j < vec_num(vec); ++j) {
        if (vec_num(((atomvec_data *)vec_get(vec, j))->atom_ptrs) == i) {
          vec_push(v_tmp, vec_get(vec, j));
        }
      }
    }
    vec_clear(vec);
    /* ��¤����Υ��ȥ������¿����פ˥����� */
    while (!vec_is_empty(v_tmp)) {
      vec_push(vec, vec_pop(v_tmp));
    }
    vec_free(v_tmp);
  }

  return vec;
}

/* �������ľ���Τ��٤Ƥλ���ؤΥݥ��󥿤��ݻ�����٥�����vec��
 * ��¹�����¿����˥����Ȥ��롣�����Ȥ��줿vec������Ǥϸ�Υ��ƥåפ�
 * POP����뤿�ᡢ��¹����ξ��ʤ����줫���˥ޥå��󥰤��оݤȤʤ뤳�Ȥˤʤ롣 */
static void lmn_mem_mk_sorted_children(Vector *vec) {
  unsigned int num_descendants_max;
  int i, n;
  Vector *v_mems_tmp;

  assert(vec_num(vec));

  for (i = 0, num_descendants_max = 0; i < vec_num(vec); ++i) {
    if ((n = lmn_mem_count_descendants((LmnMembrane *)vec_get(vec, i)))
          > num_descendants_max) {
      num_descendants_max = n;
    }
  }
  v_mems_tmp = vec_make(vec_num(vec));
  for (n = 0; n <= num_descendants_max; ++n) {
    for (i = 0; i < vec_num(vec); ++i) {
      if (n == lmn_mem_count_descendants((LmnMembrane *)vec_get(vec, i))) {
        vec_push(v_mems_tmp, vec_get(vec, i));
      }
    }
  }
  vec_clear(vec);
  while (!vec_is_empty(v_mems_tmp)) {
    vec_push(vec, vec_pop(v_mems_tmp));
  }
  vec_free(v_mems_tmp);
}

/* ���ȥ�a1��a2�����Ȥ���ʬ��(= ���륢�ȥफ���󥯤ˤ�ä�ľ��é�뤳�ȤΤǤ���ץ����ν���)
 * �ι�¤���ߤ��˰��פ��뤫�ݤ���Ƚ�ꤹ�롣Ʊ����Ƚ���Ԥ���Ǥ��濴Ū����ô�äƤ��롣
 * ξʬ�ҹ�¤�������˰��פ�����ϡ���������̲ᤷ�������ȥ�(i.e. ʬ����������ȥ�)�Υ��ɥ쥹��
 * ���ѤΥ٥�����(v_log1��v_log2)����¸����롣
 *
 * �ʤ�����5������ 'current_depth' �ϡ�Ʊ����Ƚ���оݤȤʤäƤ�����ο���ˤޤ�
 * �������ڤ֤Τ��ɤ�����Τ�Ρ�������Υץ����������оݤ��ܤ�ݤ� current_depth ��1���ä���
 * �դ˿�����Υץ����˰ܤ�ݤ�1�������롣Ʊ����Ƚ���оݤ���ο�����0�ˤʤäƤ��뤿�ᡢ
 * 0̤���ο�����¸�ߤ���ץ������������ʤ��褦�ˤ��롣 */
static BOOL lmn_mem_trace_links(LmnAtomPtr a1, LmnAtomPtr a2, Vector *v_log1, Vector *v_log2, int current_depth) {
  LmnAtomPtr l1, l2;
  LmnLinkAttr attr1, attr2;
  unsigned int arity;
  int i;
  BOOL ret_next_step; /* a1, a2��ľ����³����Ƥ��륢�ȥ���Ф��ƺƵ�Ū���ܥ᥽�åɤ�Ŭ�Ѥ��뤿��˻��� */
  int next_depth;

  vec_push(v_log1, (LmnWord)a1);
  vec_push(v_log2, (LmnWord)a2);

  /* a1��a2�Υե��󥯥������פ��뤳�Ȥ��ǧ(�԰��פξ���̵���˵����֤�) */
  if (LMN_ATOM_GET_FUNCTOR(a1) != LMN_ATOM_GET_FUNCTOR(a2)) {
    return FALSE;
  }

  arity = LMN_ATOM_GET_ARITY(a1);
  if (LMN_IS_PROXY_FUNCTOR(LMN_ATOM_GET_FUNCTOR(a1))) {
    /* �ץ������ȥ����3�����Ͻ�°��ؤΥݥ��󥿤ʤΤǤ���ʬ����� */
    --arity;
  }
  for (i = 0; i < arity; ++i) {
    attr1 = LMN_ATOM_GET_ATTR(a1, i);
    attr2 = LMN_ATOM_GET_ATTR(a2, i);

    if (!LMN_ATTR_IS_DATA(attr1) && !LMN_ATTR_IS_DATA(attr2)) {
      /* ���ȥ�a1��a2����i��󥯤���³�褬���˥���ܥ�(or �ץ���)���ȥ�Υ�����
       * (���ξ��Ϥޤ���ξ���ȥ����i��󥯤���³�襢�ȥ�ˤ����벿���ܤΥ�󥯤���������Τ����ǧ����
       * ���줬���פ��뤳�Ȥ��ǧ����(�԰��פξ��ϵ����֤�)��³���ơ���³�襷��ܥ�(or �ץ���)���ȥ��
       * �����������줬�����¸�ߤ��뤫�ɤ���������å�������ˤޤ�¸�ߤ��ʤ������Υ��ȥ�Ǥ�����ϡ�
       * ������Ф��ƺƵ�Ū���ܥ᥽�åɤ�Ŭ�Ѥ���a1�����a2�����Ȥ���ʬ�����ΤΥޥå��󥰤�Ԥ�) */
      if (attr1 != attr2) {
          /* {c(X,Y), c(X,Y)} vs. {c(X,Y), c(Y,X)}
            * ����Τ褦�ˡ�2���ȥ�֤Υ�󥯤���³������ۤʤäƤ������FALSE���֤� */
        return FALSE;
      }
      l1 = (LmnAtomPtr)LMN_ATOM_GET_LINK(a1, i);
      l2 = (LmnAtomPtr)LMN_ATOM_GET_LINK(a2, i);

      if ((vec_contains(v_log1, (LmnWord)l1) && !vec_contains(v_log2, (LmnWord)l2))
        || (!vec_contains(v_log1, (LmnWord)l1) && vec_contains(v_log2, (LmnWord)l2))) {
         /* ��������ˤ����Ƥϡ�����ޤǤΥȥ졼�����̲�ѤߤΥ��ȥ�˴ԤäƤ��� (i.e. ʬ����˴ľ��ι�¤(= ��ϩ)��¸�ߤ���)
          * ��ΤΡ��⤦��������Ǥ�Ʊ�ͤ���ϩ����ǧ�Ǥ�������¤���԰��פ�ǧ���줿����˵����֤� */
        return FALSE;
      } else if (vec_contains(v_log1, (LmnWord)l1) && vec_contains(v_log2, (LmnWord)l2)) {
         /* ��1��2����б�����ʬ�Ҥ�������ϩ������������ϡ���(i+1)��󥯤���³��Υ����å��˰ܤ� */
        continue;
      }

      if (LMN_ATOM_GET_FUNCTOR(l1) == LMN_IN_PROXY_FUNCTOR) {
        next_depth = current_depth + 1;
      } else if (LMN_ATOM_GET_FUNCTOR(l1) == LMN_OUT_PROXY_FUNCTOR) {
        next_depth = current_depth - 1;
      } else {
        next_depth = current_depth;
      }
      if (next_depth < 0) {
        /* ���������оݥ��ȥब���⤽���Ʊ����Ƚ���оݤȤʤäƤ�����ο���ʾ�γ��ؤ�
         * ��°���Ƥ��뤿�ᡢ�����̵�뤷�Ƽ��Υ��ȥ�������оݤ�����ľ�� */
        continue;
      }

      ret_next_step = lmn_mem_trace_links(l1, l2, v_log1, v_log2, next_depth);

      if (!ret_next_step) {
        return FALSE;
      }
    } else if (LMN_ATTR_IS_DATA(attr1) && LMN_ATTR_IS_DATA(attr2)) {
      /* ���ȥ�a1��a2����i��󥯤���³�褬���˥ǡ������ȥ�Υ�����
       * (���ξ�����³��ǡ������ȥ���ͤ���Ӥ����ߤ�����������м��Υ��(i.e. ��(i+1)���)����³�襢�ȥ�
       * ����Ӻ�Ȥ˰ܤꡢ�������ʤ����ϵ����֤�) */
      if (LMN_ATOM_GET_LINK(a1, i) != LMN_ATOM_GET_LINK(a2, i)) {
        return FALSE;
      }
    } else {
      /* ���ȥ�a1��a2����i��󥯤���³�襢�ȥ�μ��ब�ߤ��˰��פ��ʤ�������
       * (���ξ���̵���˵����֤�) */
      return FALSE;
    }
  }

  return TRUE;
}

static BOOL lmn_mem_equals_rec(LmnMembrane *mem1, LmnMembrane *mem2, int current_depth) {
  Vector *atomvec_mem1, *atomvec_mem2; /* atomvec_memX (X = 1,2)�ϡ���memX ľ���Υ��ȥ�ξ�����ݻ�����Vector��
                                              * ����Υ��ȥ��ե��󥯥�����������������ɤΥ��ȥफ��ޥå��󥰤򳫻ϤǤ���褦�ˤ�����Ū�ǻ��Ѥ��롣 */
  BOOL is_the_same_functor;
  int i, j;

  /* Step1. ξ��λ�¹��θĿ���ξ����Υ��ȥ�θĿ�����̾���ߤ������������Ȥ��ǧ */
  if (lmn_mem_count_descendants(mem1) != lmn_mem_count_descendants(mem2)
      || mem1->atom_num != mem2->atom_num
      || mem1->name != mem2->name) return FALSE;

  {
    atomvec_mem1 = lmn_mem_mk_matching_vec(mem1);
    atomvec_mem2 = lmn_mem_mk_matching_vec(mem2);

    /* Step2. ξ����Υ��ȥ�μ�������ߤ������������Ȥ��ǧ */
    if (vec_num(atomvec_mem1) != vec_num(atomvec_mem2)) {
      free_atomvec_data(atomvec_mem1); free_atomvec_data(atomvec_mem2);
      return FALSE;
    }
    /* Step3. ξ����˴ޤޤ�륢�ȥ�Υե��󥯥�����ӸĿ����ߤ������������Ȥ��ǧ */
    for (i = 0; i < vec_num(atomvec_mem1); ++i) {
      is_the_same_functor = FALSE;
      for (j = 0; j < vec_num(atomvec_mem2); ++j) {
        if (((atomvec_data *)vec_get(atomvec_mem1, i))->fid
             == ((atomvec_data *)vec_get(atomvec_mem2, j))->fid) {
          is_the_same_functor = TRUE;
          break;
        }
      }
      if (!is_the_same_functor ||
           (is_the_same_functor &&
                 vec_num(((atomvec_data *)vec_get(atomvec_mem1, i))->atom_ptrs)
              != vec_num(((atomvec_data *)vec_get(atomvec_mem2, j))->atom_ptrs)
           ))
      {
        free_atomvec_data(atomvec_mem1); free_atomvec_data(atomvec_mem2);
        return FALSE;
      }
    }
  }
  /* �����ʳ���ξ��ϸߤ������������λ�¹��������ξ����Υ��ȥ�Υե��󥯥��μ���
   * ����Ӥ��θĿ��������˰��פ��뤳�Ȥ���ǧ����Ƥ��롣
   * (i.e. ��̤���Ʊ���Ǥʤ�(��)�פˤʤ�ʤ�С�����ˤ������󥯤���³�ط� or ��¹�줬�ۤʤäƤ��뤳�Ȥ��̣����)
   * �ʹߡ������ɤΥ��ȥफ���˺������Ƥ��������ȥ�����Ȥ��������μ¹Ԥ˰ܤäƤ����� */
  {
    LmnAtomPtr a1, a2;
    BOOL matched;
    BOOL has_atoms; /* ���줬���ʤ��Ȥ�1�ĤΥ���ܥ�(or �ץ���)���ȥ����Ĥ��Ȥ�ɽ���ե饰 */
    BOOL has_descendants; /* ���줬��¹�����Ĥ��Ȥ�ɽ���ե饰 */
    Vector *v_log1, *v_log2; /* ��������̲ᤷ�����ȥ�Υ����������Vector */
    Vector *v_atoms_not_checked1, *v_atoms_not_checked2; /* Ʊ������Ƚ���Ƚ�꤬�Ѥ�Ǥ��ʤ����ȥ�ν�����������Vector (�ƥ��ȥ�ؤΥݥ��󥿤���¸) */
    Vector *v_mems_children1, *v_mems_children2; /* ����ľ���λ�����������Vector (����Vector�����ˤʤ�ޤǻ�������Ȥ���������³��) */
    LmnMembrane *m;
    int n;
    unsigned int length;

    /* init */
    {
      /* mem1, mem2ľ���λ��������������θĿ������������Ȥ��ǧ */
      {
        length = lmn_mem_count_children(mem1);
        if (length) {
          has_descendants = TRUE;
        } else {
          has_descendants = FALSE;
        }

        if (has_descendants) {
          v_mems_children1 = vec_make(length);
          memset(v_mems_children1->tbl, 0, sizeof(LmnMembrane *) * v_mems_children1->cap);
          v_mems_children2 = vec_make(length);
          memset(v_mems_children2->tbl, 0, sizeof(LmnMembrane *) * v_mems_children2->cap);

          for (m = mem1->child_head; m; m = m->next) {
            vec_push(v_mems_children1, (LmnWord)m);
          }
          for (m = mem2->child_head; m; m = m->next) {
            vec_push(v_mems_children2, (LmnWord)m);
          }
          /* ����ľ���λ�������ߤ��˰��פ��ʤ�����ľ���˵����֤� */
          if (vec_num(v_mems_children1) != vec_num(v_mems_children2)) {
            free_atomvec_data(atomvec_mem1); free_atomvec_data(atomvec_mem2);
            vec_free(v_mems_children1); vec_free(v_mems_children2);

            return FALSE;
          }
        }
      }
      /* �ʹߡ�̤�����������ѥ��ȥ���������vector�ν���� */
      length = mem1->atom_num;
      assert(length == mem2->atom_num);
      if (length) {
        has_atoms = TRUE;
      } else {
        has_atoms = FALSE;
      }

      if (has_atoms) {
        v_atoms_not_checked1 = vec_make(length);
        memset(v_atoms_not_checked1->tbl, 0, sizeof(LmnAtomPtr) * v_atoms_not_checked1->cap);
        v_atoms_not_checked2 = vec_make(length);
        memset(v_atoms_not_checked2->tbl, 0, sizeof(LmnAtomPtr) * v_atoms_not_checked2->cap);

        assert(vec_num(atomvec_mem1) == vec_num(atomvec_mem2));

        /* �٥�����atomvec_mem{1,2}�ˤ�¿���ɤΥ��ȥफ�������ꤳ�ޤ�Ƥ��뤿�ᡢ
         * �٥�����v_atoms_not_checked{1,2}�ˤ�¿���ɤΥ��ȥ�Υݥ��󥿤�����
         * ���ꤳ�ޤ�Ƥ������Ȥˤʤ롣�椨�ˡ�v_atoms_not_checked{1,2}��POP���Ƥ���
         * ���ȤǾ����ɤΥ��ȥफ���˼��Ф��Ƥ������Ȥ��Ǥ���褦�ˤʤ롣 */
        for (i = 0; i < vec_num(atomvec_mem1); ++i) {
          for (n = 0; n < vec_num(((atomvec_data *)vec_get(atomvec_mem1, i))->atom_ptrs); ++n) {
            vec_push(v_atoms_not_checked1,
                     vec_get(((atomvec_data *)vec_get(atomvec_mem1, i))->atom_ptrs, n));
            vec_push(v_atoms_not_checked2,
                     vec_get(((atomvec_data *)vec_get(atomvec_mem2, i))->atom_ptrs, n));
          }
        }
        v_log1 = vec_make(length);
        v_log2 = vec_make(length);
      }
    }

    /* �ʹ�atomvec_mem1/2�ϻ��Ѥ��ʤ����ᤳ���ʳ��ǥ��꡼��������Ƥ��� */
    free_atomvec_data(atomvec_mem1); free_atomvec_data(atomvec_mem2);

    /* Step4. ���ȥ�����Ȥ������� (has_atoms�����ξ��Τ߹Ԥ��Ф褤) */
    if (has_atoms) {
      matched = FALSE;

      assert(vec_num(v_atoms_not_checked1) == vec_num(v_atoms_not_checked2));

      while (!vec_is_empty(v_atoms_not_checked1)) {
        /* ��1�⤫��1�ĥ��ȥ����Ф�����򺬤Ȥ��롣
         * ��1��Υ��ȥ���⡢(�ե��󥯥���)�����ɤΤ�Τ����˺��������Ƥ������Ȥ���դ��衣 */
        a1 = (LmnAtomPtr)vec_pop(v_atoms_not_checked1);

        /* fprintf(stdout, "fid(a1):%u\n", (unsigned int)LMN_ATOM_GET_FUNCTOR(a1)); */

        for (i = vec_num(v_atoms_not_checked2); i > 0 && !matched; --i) {
          a2 = (LmnAtomPtr)vec_get(v_atoms_not_checked2, i-1); /* ��2�⤫�麬a1���б����륢�ȥ�θ������� (��: �����μ�����vec_pop�ϻ����Բ�!!) */
          vec_clear(v_log1);
          memset(v_log1->tbl, 0, sizeof(LmnAtomPtr) * v_log1->cap);
          vec_clear(v_log2);
          memset(v_log2->tbl, 0, sizeof(LmnAtomPtr) * v_log2->cap);

          /* a2��������a1���б����륢�ȥ�Ǥ��뤫�ݤ���ºݤ˥���չ�¤��ȥ졼�����Ƴ�ǧ���롣
           * a2��a1�Ȥ�1:1���б�������˸¤ä� matched �˿����֤ꡢ
           * v_log{1,2}��ˤ�a{1,2}�����Ȥ���ʬ����������ȥ�Υ��ɥ쥹���������Ȥ��Ƶ�Ͽ����롣 */
          matched = lmn_mem_trace_links(a1, a2, v_log1, v_log2, current_depth);
          if (matched) {
           /* fprintf(stdout, "fid(a2):%u\n", (unsigned int)LMN_ATOM_GET_FUNCTOR(a2)); */

            /* ξ�����¸�ߤ��뤢��ʬ��Ʊ�ΤΥޥå��󥰤������������ˤ��������롣
             * ��2���̤�ޥå��󥰤Υ��ȥ��������Ƥ����٥�����(v_atoms_not_checked2)
             * ���麬a1���б����륢�ȥ�a2�����롣 */
            assert(vec_num(v_log1) == vec_num(v_log2));
            for (n = 0; n < vec_num(v_atoms_not_checked2); ++n) {
              if ((LmnAtomPtr)vec_get(v_atoms_not_checked2, n) == a2) {
                vec_pop_n(v_atoms_not_checked2, n);
                break;
              }
            }
            assert(vec_num(v_atoms_not_checked1) == vec_num(v_atoms_not_checked2));

            /* �����¸�ߤ��뤹�٤ƤΥ��ȥ��̤�����å����ȥ�Υꥹ�Ȥ���POP���� */
            {
              for (n = 0; n < vec_num(v_log1); ++n) {
                for (i = 0; i < vec_num(v_atoms_not_checked1); ++i) {
                  if ((LmnAtomPtr)vec_get(v_log1, n) == (LmnAtomPtr)vec_get(v_atoms_not_checked1, i)) {
                    vec_pop_n(v_atoms_not_checked1, i);
                    break;
                    }
                  }
                }
              for (n = 0; n < vec_num(v_log2); ++n) {
                for (i = 0; i < vec_num(v_atoms_not_checked2); ++i) {
                  if ((LmnAtomPtr)vec_get(v_log2, n) == (LmnAtomPtr)vec_get(v_atoms_not_checked2, i)) {
                    vec_pop_n(v_atoms_not_checked2, i);
                    break;
                   }
                 }
               }
            }
            assert(vec_num(v_atoms_not_checked1) == vec_num(v_atoms_not_checked2));
          }
        }
        if (!matched) {
          /* ��1��ˤ�����a1�򺬤Ȥ���ʬ�Ҥ��б�����ʬ�Ҥ���2���¸�ߤ��ʤ����ˤ��������� (���ξ���̵���˵����֤�) */
          vec_free(v_log1); vec_free(v_log2);
          vec_free(v_atoms_not_checked1); vec_free(v_atoms_not_checked2);
          if (has_descendants) {
            vec_free(v_mems_children1); vec_free(v_mems_children2);
          }
          return FALSE;
        }
        /* ��1ľ���Υ��ȥ����ˤޤ�̤�����å��Τ�Τ��ޤޤ�Ƥ�����ϡ�
         * �ޥå��󥰤��³���뤿��˥ե饰matched�򵶤˥��åȤ��Ƥ��� */
        if (!vec_is_empty(v_atoms_not_checked1)) {
          matched = FALSE;
        }
      }
    }
    /* �����ʳ���������Ρ֥��ȥ�����Ȥ��������פϤ��٤ƴ�λ����
     * ξ��ľ���Υ���չ�¤�ϴ����˰��פ��뤳�Ȥ��ݾڤ���Ƥ��롣
     * ��������ϡ�ξ�����¸�ߤ��뤹�٤Ƥλ���ˤĤ��ơ����ι�¤�����פ��뤫�ɤ����ˤĤ���Ĵ�٤Ƥ�����
     * �ʹߡ�mem1��λ����1�ĸ��ꤷ��mem2ľ���λ��줫���б������Τ����ꤹ���Ȥ�
     * �ܤäƤ���������̤����Ȥʤ륱�����ˤ��������®�٤���夵���뤿�ᡢ
     * ��¹��������ʤ����줫��ͥ��Ū�˸��ꤹ�����ˤ��롣 */
    if (has_descendants) { /* ���줬¸�ߤ�����Τ߰ʹߤν�����Ԥ� */
      LmnMembrane *cm1, *cm2;

      /* ��¹�����¿�����v_mems_children1, v_mems_children2�򥽡��� */
      {
        assert(vec_num(v_mems_children1) == vec_num(v_mems_children2));
        lmn_mem_mk_sorted_children(v_mems_children1);
        lmn_mem_mk_sorted_children(v_mems_children2);
      }

      /* Step5. ��������Ȥ������� */
      matched = FALSE;
      while (!vec_is_empty(v_mems_children1)) {
        cm1 = (LmnMembrane *)vec_pop(v_mems_children1);

        /* fprintf(stderr, "\t-- start to test a descendant membrane --\n\t\t# of descendants of mem(%u): %u\n", (unsigned int)cm1, lmn_mem_count_descendants(cm1)); */

        for (i = vec_num(v_mems_children2); i > 0 && !matched; --i) {
          cm2 = (LmnMembrane *)vec_get(v_mems_children2, i-1);
          matched = lmn_mem_equals_rec(cm1, cm2, current_depth + 1);
          if (matched) {
            /* cm1��Ʊ������(=cm2)��v_mems_children2��˸��Ĥ��ä����ˤ��������롣
             * v_mems_children2����cm2��������� */
            for (n = 0; n < vec_num(v_mems_children2); ++n) {
              if (cm2 == (LmnMembrane *)vec_get(v_mems_children2, n)) {
                vec_pop_n(v_mems_children2, n);
                break;
              }
            }
          }
        }
        if (!matched) {
          /* cm1��Ʊ�����줬v_mems_children2���¸�ߤ��ʤ���� */
          if (has_atoms) {
            vec_free(v_log1); vec_free(v_log2);
            vec_free(v_atoms_not_checked1); vec_free(v_atoms_not_checked2);
          }
          vec_free(v_mems_children1); vec_free(v_mems_children2);
          return FALSE;
        }
        if (!vec_is_empty(v_mems_children1)) {
          matched = FALSE;
        }
      }
    }
    /* mem1, mem2��λ�¹���ޤह�٤ƤΥץ�����Ʊ����Ƚ������� */
    if (has_atoms) {
      vec_free(v_log1); vec_free(v_log2);
      vec_free(v_atoms_not_checked1); vec_free(v_atoms_not_checked2);
    }
    if (has_descendants) {
      vec_free(v_mems_children1); vec_free(v_mems_children2);
    }
  }

  return TRUE;
}

BOOL lmn_mem_equals(LmnMembrane *mem1, LmnMembrane *mem2) {
  return lmn_mem_equals_rec(mem1, mem2, 0);
}
/*----------------------------------------------------------------------*/
/* ���Ʊ����Ƚ�� �����ޤ� */
