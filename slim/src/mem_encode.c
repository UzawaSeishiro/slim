/*
 * mem_encode.c
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

/* 膜を一意のバイト列に変換する */

/* encode specification
 *
 * アトムや膜などの各要素をバイト列に書き込む際には、まず要素の種類を表す4ビットのタグを書き込み、
 * その次の位置から、それぞれの要素毎の値を書き込んでいく。
 *
 * 実装では、4ビットを一つの単位としてバイト列に値を書き込んでいく。
 *
 * atom
 *   tag: 0000
 *   functor:  (2 Byte)
 *   arguments
 *
 * membrane
 *   tag: 0001
 *   elements: (any length)
 *   end tag:   0010
 * named membrane
 *   tag: 0011
 *   name: 4 Byte
 *   elements
 *   end tag:   0010
 * atom ref
 *   tag: 0100
 *   ref id : 2 byte
 *   arg num: 1 byte
 * mem ref
 *   tag: 0101 
 * escape from membrane
 *   tag: 0110
 * from of traversal
 *   tag: 0001
 * int atom:
 *   tag: 1100
 * double atom:
 *   tag: 1101
*/

#include "mem_encode.h"
#include "error.h"
#include "functor.h"
#include "atom.h"
#include "st.h"
#include "util.h"
#ifdef PROFILE
#include "runtime_status.h"
#endif

#define TAG_BIT_SIZE 4
#define TAG_DATA_TYPE_BIT 2
#define TAG_IN_BYTE 2

/* Tags */
#define TAG_ATOM_START       0x0
#define TAG_MEM_START        0x1
#define TAG_MEM_END          0x2
#define TAG_NAMED_MEM_START  0x3
#define TAG_ATOM_REF         0x4
#define TAG_MEM_REF          0x5
#define TAG_ESCAPE_MEM       0x6
#define TAG_FROM             0x7
#define TAG_INT_DATA         0xc
#define TAG_DBL_DATA         0xd


#define ATOM_REF_SIZE 4
#define ATOM_REF_ARG_SIZE 2
#define MEM_REF_SIZE 4
#define FUNCTOR_SIZE (sizeof(LmnFunctor) * 2)
#define BS_INT_SIZE (SIZEOF_LONG * 2)
#define BS_MEM_NAME_SIZE (sizeof(lmn_interned_str) * 2)

/* エンコードの結果を書き込む、バイナリストリング */

struct BinStr {
  BYTE *v;
  /* バッファのサイズ（4ビット単位） */
  int size;
  /* 書き込み位置（4ビット単位） */
  int cur;
  /* バッファの位置を指し示す、BinStrPtrのベクタ */
  Vector *ptrs;
  /* ポインタ解放のために、追加されたポインタをすべて記録する */
  Vector *all_ptrs;
  /* 作業用 */
  Vector *ptrs2;
};

/* BinStrの特定の位置を指し示すポインタ。BinStrへの書き込みはBinStrPtr
   を介して行う。他のBinStrPtrの書き込みにより、現在のBinStrPtrが無効に
   なる場合があり、binstr_validが真を返すようになる。 */
struct BinStrPtr {
  struct BinStr *binstr;
  int pos; /* bit */
  BOOL valid;
};
typedef struct BinStrPtr *BinStrPtr;

static inline void bsptr_invalidate(BinStrPtr p);
static void binstr_invalidate_ptrs(struct BinStr *p, int start);
static inline void bsptr_free(struct BinStrPtr *p);

struct BinStr *binstr_make()
{
  struct BinStr *p = LMN_MALLOC(struct BinStr);
  p->size = 128 * TAG_IN_BYTE;
  p->v = LMN_NALLOC(BYTE, p->size / TAG_IN_BYTE);
  p->cur = 0;
  p->ptrs = vec_make(64);
  p->ptrs2 = vec_make(64);
  p->all_ptrs = vec_make(64);
  return p;
}

/* pが持つ、すべてのBinStrPtrを解放する。以後、このpへの書き込みや、
   BinStrPtrの作成は行えない。 */
static void binstr_free_ptrs(struct BinStr *p)
{
  int i;

  if (!p->all_ptrs) return;

  for (i = 0; i < vec_num(p->all_ptrs); i++) {
    bsptr_free((BinStrPtr)vec_get(p->all_ptrs, i));
  }
  vec_free(p->all_ptrs);
  vec_free(p->ptrs);
  vec_free(p->ptrs2);
  p->all_ptrs = NULL;
  p->ptrs = NULL;
  p->ptrs2 = NULL;
}

void binstr_free(BinStr p)
{
  LMN_FREE(p->v);
  binstr_free_ptrs(p);
  LMN_FREE(p);
}

/* TODO: instance size */
int binstr_byte_size(BinStr p)
{
  return p->size / TAG_IN_BYTE;
}

/* See http://isthe.com/chongo/tech/comp/fnv/ */
#if SIZEOF_LONG == 4
#define FNV_PRIME 16777619UL
#define FNV_BASIS 2166136261UL
#elif SIZEOF_LONG == 8
#define FNV_PRIME  1099511628211UL
#define FNV_BASIS 14695981039346656037UL
#endif

/* バイナリストリングのハッシュ値を返す */
unsigned long binstr_hash(const BinStr a)
{
  unsigned long hval = FNV_BASIS;
  int i = a->cur / TAG_IN_BYTE;

  /*
   * FNV-1a hash each octet in the buffer
   */
  while (--i >= 0) {
    /* xor the bottom with the current octet */
    hval ^= (unsigned int) a->v[i];

    /* multiply by the FNV magic prime mod 2^32 or 2^64 */
    hval *= FNV_PRIME;
  }
  return hval;
}

#define BS_SET(a, pos, v)                                                \
  (((pos) & 1) ?                                                      \
   ((a)[(pos)>>1] = ((a)[(pos)>>1] & 0x0f) | ((v) << TAG_BIT_SIZE)) :          \
   ((a)[(pos)>>1] = (v&0x0f) | ((a)[(pos)>>1] & 0xf0)))
#define BS_GET(a, pos)                                                \
  (((pos) & 1) ? ((a)[(pos)>>1] & 0xf0)>>TAG_BIT_SIZE :  (a)[(pos)>>1] & 0x0f)

/* bsの位置posにbの下位4ビットを書き込む。書き込みに成功した場合は真を
   返し、失敗した場合は偽を返す。*/
int binstr_set(struct BinStr *bs, BYTE b, int pos)
{
  if (bs->size <= pos) {
    bs->size *= 2;
    bs->v = LMN_REALLOC(BYTE, bs->v, bs->size / TAG_IN_BYTE);
  }

  if (bs->cur == pos) {
    BS_SET(bs->v, pos, b);
    bs->cur++;
    return 1;
  }
  else if (bs->cur > pos) {
    if (BS_GET(bs->v, pos) < b) return 0;
    else if (BS_GET(bs->v, pos) > b) {
      /* 現在、curが指す位置よりも前に書き込みを行ったため，
         curよりも後の位置を指すポインタをすべて無効にする */
      binstr_invalidate_ptrs(bs, pos + 1);
      bs->cur = pos + 1;
      BS_SET(bs->v, pos, b);
      return 1;
    }
    else { /* bs->v[pos] == b */
      return 1;
    }
  }
  else { /*(bs->cur < pos)*/
    lmn_fatal("unexpected");
    return 0;
  }
}

/* bsの位置posから1バイト読み込み，返す */
static inline BYTE binstr_get_byte(BinStr bs, int pos)
{
  return (BS_GET(bs->v, pos+1)<<4) | BS_GET(bs->v, pos);
}

static inline LmnFunctor binstr_get_functor(BinStr bs, int pos)
{
  if (sizeof(LmnFunctor) == 2) {
    long f = (binstr_get_byte(bs, pos+2)<<8 | binstr_get_byte(bs, pos));
    return (LmnFunctor)(FUNCTOR_MAX - f);
  } else {
    lmn_fatal("unexpected");
  }
}

static inline unsigned int binstr_get_ref_num(BinStr bs, int pos)
{
  if (ATOM_REF_SIZE == 4) {
    return binstr_get_byte(bs, pos+2)<<8 | binstr_get_byte(bs, pos);
  } else {
    lmn_fatal("unexpected");
  }
}

static inline unsigned int binstr_get_arg_ref(BinStr bs, int pos)
{
  if (ATOM_REF_ARG_SIZE == 2) {
    return binstr_get_byte(bs, pos);
  } else {
    lmn_fatal("unexpected");
  }
}

static inline long binstr_get_int(BinStr bs, int pos)
{
#if SIZEOF_LONG == 4
    return (long)(
      (binstr_get_byte(bs, pos+6)<<24) | (binstr_get_byte(bs, pos+4)<<16) |
      (binstr_get_byte(bs, pos+2)<<8) | (binstr_get_byte(bs, pos)));
#elif SIZEOF_LONG == 8
    return (long)(
      (binstr_get_byte(bs, pos+14)<<56) | (binstr_get_byte(bs, pos+12)<<48) |
      (binstr_get_byte(bs, pos+10)<<40) | (binstr_get_byte(bs, pos+8)<<32) |
      (binstr_get_byte(bs, pos+6)<<24) | (binstr_get_byte(bs, pos+4)<<16) |
      (binstr_get_byte(bs, pos+2)<<8) | (binstr_get_byte(bs, pos)));
#else
    #error "not supported"
#endif
}

static inline lmn_interned_str binstr_get_mem_name(BinStr bs, int pos)
{
  if (sizeof(lmn_interned_str) == SIZEOF_LONG) {
    return binstr_get_int(bs, pos);
  } else {
    lmn_fatal("unexpected");
  }
}

/* start以降を指すポインタをすべて無効にする */
static void binstr_invalidate_ptrs(struct BinStr *p, int start)
{
  int i;
  Vector *tmp;
  int len = vec_num(p->ptrs);
  /* EFFICIENCY */
  for (i = 0; i < len; i++) {
    BinStrPtr bsp = (BinStrPtr)vec_get(p->ptrs, i);
    if (bsp->pos >= start) bsptr_invalidate(bsp);
    else vec_push(p->ptrs2, (vec_data_t)bsp);
  }
  
  /* swap */
  tmp = p->ptrs;
  p->ptrs = p->ptrs2;
  p->ptrs2 = tmp;
  vec_clear(p->ptrs2);
}

/* バイナリストリングaとbの比較を行いaがbより、小さい、同じ、大きい場合
   に、それぞれ負の値、0、正の値を返す。*/
int binstr_comp(const BinStr a, const BinStr b)
{
  if (a->cur != b->cur) return a->cur - b->cur;
  else if (a->cur & 1) {
    int r = memcmp(a->v, b->v, a->cur / TAG_IN_BYTE);
    if (!r) return r;
    return (a->v[a->cur>>1] & 0x0f) - (b->v[b->cur>>1] & 0x0f);
  }
  else {
    return memcmp(a->v, b->v, a->cur / TAG_IN_BYTE);
  }
}

/* bsにポインタptrを追加する */
void binstr_add_ptr(const struct BinStr *bs, struct BinStrPtr* ptr)
{
  vec_push(bs->ptrs, (vec_data_t)ptr);
  vec_push(bs->all_ptrs, (vec_data_t)ptr);
}

void binstr_dump(const BinStr bs)
{
  int pos;

  pos = 0;
  while (pos < bs->cur) {
    unsigned int tag = BS_GET(bs->v, pos);
    pos++;

    switch (tag) {
    case TAG_ATOM_START:
      {
        LmnFunctor f = binstr_get_functor(bs, pos);
        pos += FUNCTOR_SIZE;
        printf("%s(", lmn_id_to_name(LMN_FUNCTOR_NAME_ID(f)));
      }
      break;
    case TAG_MEM_START:
      {
        printf("{");
      }
      break;
    case TAG_MEM_END:
      {
        printf("}");
      }
      break;
    case TAG_NAMED_MEM_START:
      {
        lmn_interned_str name;
        name = binstr_get_mem_name(bs, pos);
        pos += BS_MEM_NAME_SIZE;
        printf("%s{", lmn_id_to_name(name));
      }
      break;
    case TAG_ATOM_REF:
      {
        unsigned int ref, arg;
        ref = binstr_get_ref_num(bs, pos);
        pos += ATOM_REF_SIZE;
        arg =  binstr_get_arg_ref(bs, pos);
        pos += ATOM_REF_ARG_SIZE;
        printf("$%d_%d", ref, arg);
      }
      break;
    case TAG_MEM_REF:
      {
        unsigned int ref;
        ref = binstr_get_ref_num(bs, pos);
        pos += MEM_REF_SIZE;
        printf("#%d", ref);
      }
      break;
    case TAG_ESCAPE_MEM:
      {
        printf("!");
      }
      break;
    case TAG_FROM:
      {
        printf("_");
      }
      break;
    case TAG_INT_DATA:
      {
        long n;

        n = binstr_get_int(bs, pos);
        pos += BS_INT_SIZE;
        printf("%ld", n);
      }
      break;
    default:
      lmn_fatal("unexpected");
      break;
    }
  }
  printf("\n");
}

struct BinStrPtr *bsptr_make(struct BinStr *bs)
{
  struct BinStrPtr *p = LMN_MALLOC(struct BinStrPtr);
  p->binstr = bs;
  p->pos = 0;
  p->valid = TRUE;
  binstr_add_ptr(bs, p);
  return p;
}
                                    
static inline BinStrPtr bsptr_copy(BinStrPtr p)
{
  struct BinStrPtr *q = LMN_MALLOC(struct BinStrPtr);
  q->binstr = p->binstr;
  q->pos = p->pos;
  q->valid = p->valid;
  binstr_add_ptr(q->binstr, q);
  return q;
}
                                    
/* toをfromと同じ位置を指すようにする。fromとtoは同じバイナリすとリング
   を指すポインタの必要がある。 */
static inline void bsptr_copy_to(const BinStrPtr from, BinStrPtr to)
{
  if (to->binstr != from->binstr) lmn_fatal("unexpected");
  to->pos = from->pos;
  to->valid = from->valid;
}
                                    
static inline void bsptr_free(struct BinStrPtr *p)
{
  LMN_FREE(p);
}

static inline BOOL bsptr_valid(BinStrPtr p)
{
  return p->valid;
}

/* ポインタpが指すバイナリストリングに、vからサイズsize分だけ書き込む。
   書き込みに成功した場合は1を、失敗した場合は0を返す。*/
static inline int bsptr_push(struct BinStrPtr *p, const BYTE *v, int size)
{
  int i;
  int half_len = size>>1;
  if (!bsptr_valid(p)) return 0;
  
  for (i = 0; i < half_len; i++) {
    if (binstr_set(p->binstr, v[i] & 0x0f, p->pos)) p->pos++;
    else { bsptr_invalidate(p); return 0;}
    if (binstr_set(p->binstr, (v[i] & 0xf0)>>TAG_BIT_SIZE, p->pos)) p->pos++;
    else { bsptr_invalidate(p); return 0;}
  }
  if (size & 1) {
    if (binstr_set(p->binstr, v[size>>1] & 0x0f, p->pos)) p->pos++;
    else { bsptr_invalidate(p); return 0;}
  }    
  return 1;
}

static inline int bsptr_push1(struct BinStrPtr *p, const BYTE v)
{
  if (binstr_set(p->binstr, v & 0x0f, p->pos)) {
    p->pos++;
    return 1;
  }
  else {
    bsptr_invalidate(p);
    return 0;
  }
}

/* ポインタを無効にする */
static inline void bsptr_invalidate(BinStrPtr p)
{
  p->valid = FALSE;
}

static inline int bsptr_push_start_mem(BinStrPtr p, lmn_interned_str name)
{
  if (name == ANONYMOUS) {
    return bsptr_push1(p, TAG_MEM_START);
  } else {
    return
      bsptr_push1(p, TAG_NAMED_MEM_START) &&
      bsptr_push(p, (BYTE*)&name, sizeof(lmn_interned_str)*TAG_IN_BYTE);
  }
}

static inline int bsptr_push_end_mem(BinStrPtr p)
{
  return bsptr_push1(p, TAG_MEM_END);
}

static inline int bsptr_push_atom(BinStrPtr p, LmnSAtom a)
{
  /* ファンクタの最大値からファンクタの値を引いて、大小を反転させる */
  LmnFunctor f = (LmnFunctor)FUNCTOR_MAX - LMN_SATOM_GET_FUNCTOR(a);
  return
    bsptr_push1(p, TAG_ATOM_START) &&
    bsptr_push(p, (const BYTE*)&f, FUNCTOR_SIZE);
}

static inline int bsptr_push_data_atom(BinStrPtr p, LmnAtom atom, LmnLinkAttr attr)
{
  switch (attr) {
  case LMN_INT_ATTR:
    return
      bsptr_push1(p, TAG_INT_DATA) &&
      bsptr_push(p, (const BYTE*)&atom, sizeof(LmnWord)*TAG_IN_BYTE);
  case LMN_DBL_ATTR:
    return
      bsptr_push1(p, TAG_DBL_DATA) &&
      bsptr_push(p, (const BYTE*)(double*)atom, sizeof(double) * TAG_IN_BYTE);
  default:
    lmn_fatal("not implemented");
    return 0;
  }
}

static inline int bsptr_push_visited_atom(BinStrPtr p, int n, int arg)
{
  return
    bsptr_push1(p, TAG_ATOM_REF)           &&
    bsptr_push(p, (BYTE*)&n, ATOM_REF_SIZE) &&
    bsptr_push(p, (BYTE*)&n, ATOM_REF_ARG_SIZE);
}

static inline int bsptr_push_visited_mem(BinStrPtr p, int n)
{
  return
    bsptr_push1(p, TAG_MEM_REF)           &&
    bsptr_push(p, (BYTE*)&n, MEM_REF_SIZE);
}

static inline int bsptr_push_escape_mem(BinStrPtr p)
{
  return bsptr_push1(p, TAG_ESCAPE_MEM);
}

static inline int bsptr_push_from(BinStrPtr p)
{
  return bsptr_push1(p, TAG_FROM);
}

/* トラバーサルにおいて、すでに訪問済みのアトムや膜の記録を行う。ログは
 親のログへのポインタを持つ。ログ中に含まれるとは自分か親達のいずれかに
 含まれることであり、ログに含まれないとは自分にも親達にも含まれていない
 ということである */
struct VisitLog {
  struct VisitLog *pred;
  st_table_t visited;
  int n;
};

typedef struct VisitLog *VisitLog;

struct VisitLog *visitlog_make(struct VisitLog *pred)
{
  struct VisitLog *p = LMN_MALLOC(struct VisitLog);
  p->pred = pred;
  p->visited = NULL; 
  if (pred) p->n = pred->n;
  else p->n = 0;
  return p;
}

void visitlog_free(struct VisitLog *p)
{
  if (p->visited) st_free_table(p->visited);
  LMN_FREE(p);
}

/* toがfromと同じ内容を持つようにする。toはfromの親である必要がある */
void visitlog_copy_to(VisitLog from, VisitLog to)
{
  if (from == NULL || from == to) return;

  visitlog_copy_to(from->pred, to);
  if (from->visited) st_concat(to->visited, from->visited);
  if (to->n > from->n) lmn_fatal("unexpected");
  to->n = from->n;
}

void visitlog_put(VisitLog visited, LmnWord p)
{
  if (!visited->visited) {
    visited->visited = st_init_ptrtable();
  }
  st_insert(visited->visited, (st_data_t)p, visited->n++);
}

int visitlog_get(VisitLog visited, LmnWord p)
{
  st_data_t t;
  
  while (visited) {
    if (visited->visited && st_lookup(visited->visited, (st_data_t)p, &t))
      return (int)t;
    visited = visited->pred;
  }
  return -1;
}

BOOL visitlog_contains(VisitLog visited, LmnWord p)
{
  return visitlog_get(visited, p) >= 0;
}

/* prototypes */

static Vector *mem_atoms(LmnMembrane *mem);
static Vector *mem_functors(LmnMembrane *mem);
Vector *collect_smallest_mols(Vector *atoms,
                              BinStrPtr bsp,
                              VisitLog visited);

void write_mem_atoms(LmnMembrane *mem,
                     BinStrPtr bsp,
                     VisitLog visited);
void write_mols(Vector *atoms,
                BinStrPtr bsp,
                VisitLog visited);
void write_mem(LmnMembrane *mem,
               LmnAtom from_atom,
               LmnLinkAttr attr,
               int from,
               BinStrPtr bsp,
               VisitLog visited);
void write_mems(LmnMembrane *mem,
                BinStrPtr bsp,
                VisitLog visited);
void write_mol(LmnAtom atom,
               LmnLinkAttr attr,
               int from,
               BinStrPtr bsp,
               VisitLog visited);

BinStr encode_root_mem(LmnMembrane *mem)
{
  BinStr bs = binstr_make();
  BinStrPtr bsp = bsptr_make(bs);
  VisitLog visited = visitlog_make(NULL);
  
  write_mem(mem,
            0,
            -1,
            -1,
            bsp,
            visited);
  /* 最後に、ポインタの位置を修正する */
  bs->cur = bsp->pos;
  visitlog_free(visited);
  binstr_free_ptrs(bs);
  return bs;
}

void write_mem(LmnMembrane *mem,
               LmnAtom from_atom,
               LmnLinkAttr attr,
               int from,
               BinStrPtr bsp,
               VisitLog visited)
{
  int n_visited;
/*   st_data_t t; */
  
  if (!bsptr_valid(bsp)) return;

  if ((n_visited = visitlog_get(visited, (LmnWord)mem)) >= 0) {
    bsptr_push_visited_mem(bsp, n_visited);
    return;
  }

  visitlog_put(visited, (LmnWord)mem);
  
  bsptr_push_start_mem(bsp, LMN_MEM_NAME_ID(mem));

  if (!bsptr_valid(bsp)) return;

  if (from_atom != 0) {
    write_mol(from_atom, attr, from, bsp, visited);
  }

  write_mem_atoms(mem, bsp, visited);
  write_mems(mem, bsp, visited);

  bsptr_push_end_mem(bsp);
}

void write_mem_atoms(LmnMembrane *mem,
                     BinStrPtr bsp,
                     VisitLog visited)
{
  Vector *atoms;

  if (!bsptr_valid(bsp)) return;

  atoms = mem_atoms(mem);

  write_mols(atoms, bsp, visited);
  vec_free(atoms);
}

void write_mol(LmnAtom atom,
               LmnLinkAttr attr,
               int from,
               BinStrPtr bsp,
               VisitLog visited)
{
  int i_arg;
  int arity;
  int n_visited;
  LmnFunctor f;
  
  if (!bsptr_valid(bsp)) return;

  if (LMN_ATTR_IS_DATA(attr)) {
    bsptr_push_data_atom(bsp, atom, attr);
    return;
  }

  if ((n_visited = visitlog_get(visited, (LmnWord)atom)) >= 0) {
    bsptr_push_visited_atom(bsp, n_visited, from);
    return;
  }

  f = LMN_SATOM_GET_FUNCTOR(atom);
  
  if (f == LMN_OUT_PROXY_FUNCTOR) {
    LmnSAtom in = LMN_SATOM(LMN_SATOM_GET_LINK(atom, 0));
    LmnMembrane *in_mem = LMN_PROXY_GET_MEM(in);

    write_mem(in_mem, LMN_SATOM_GET_LINK(in, 1), LMN_SATOM_GET_ATTR(in, 1), LMN_ATTR_GET_VALUE(LMN_SATOM_GET_ATTR(in, 1)), bsp, visited);
    write_mol(LMN_SATOM_GET_LINK(in, 1),
              LMN_SATOM_GET_ATTR(in, 1),
              LMN_ATTR_GET_VALUE(LMN_SATOM_GET_ATTR(in, 1)),
              bsp,
              visited);
    return;
  }

  if (f == LMN_IN_PROXY_FUNCTOR) {
    LmnSAtom out = LMN_SATOM(LMN_SATOM_GET_LINK(atom, 0));

    bsptr_push_escape_mem(bsp);
    write_mol(LMN_SATOM_GET_LINK(out, 1),
              LMN_SATOM_GET_ATTR(out, 1),
              LMN_ATTR_GET_VALUE(LMN_SATOM_GET_ATTR(out, 1)),
              bsp,
              visited);
    return;
  }

  bsptr_push_atom(bsp, LMN_SATOM(atom));
  if (!bsptr_valid(bsp)) return;

  visitlog_put(visited, atom);

  arity = LMN_FUNCTOR_GET_LINK_NUM(f);
  for (i_arg = 0; i_arg < arity; i_arg++) {
    if (i_arg == from) {
      bsptr_push_from(bsp);
      continue;
    }
    
    write_mol(LMN_SATOM_GET_LINK(atom, i_arg),
              LMN_SATOM_GET_ATTR(atom, i_arg),
              LMN_ATTR_GET_VALUE(LMN_SATOM_GET_ATTR(atom, i_arg)),
              bsp,
              visited);
  }
}

/* atomsに含まれるアトムを起点とする未訪問分子を、バイナリストリングが
   最小となるように書き込む */
void write_mols(Vector *atoms,
                BinStrPtr bsp,
                VisitLog visited)
{
#define INIT_ATOMS_ARRAY_SIZE 1024

  int i, natom;
  BinStrPtr ary_bsp[INIT_ATOMS_ARRAY_SIZE], *v_bsp;
  VisitLog ary_visited[INIT_ATOMS_ARRAY_SIZE], *v_visited;
  BOOL first, first_func;

  if (!bsptr_valid(bsp)) return;

  if (vec_num(atoms) >=INIT_ATOMS_ARRAY_SIZE) {
    v_bsp = LMN_NALLOC(BinStrPtr, vec_num(atoms));
    v_visited = LMN_NALLOC(VisitLog, vec_num(atoms));
  } else {
    v_bsp = ary_bsp;
    v_visited = ary_visited;
  }
#undef INIT_ATOMS_ARRAY_SIZE 

  /* atoms中の未訪問のアトムを起点とする分子を、それぞれ試みる */
  natom = vec_num(atoms);
  first = TRUE;
  for (i = 0; i < natom; i++) {
    LmnFunctor f;
    LmnSAtom atom = LMN_SATOM(vec_get(atoms, i));

    f = LMN_SATOM_GET_FUNCTOR(atom);
    
    if (!atom ||
        (!first && f != first_func) ||  /* 最適化、最小のファンク以外は試す必要なし */
        visitlog_contains(visited, (LmnWord)atom)) {
      v_bsp[i] = NULL;
      v_visited[i] = NULL;
    } else {
      BinStrPtr new_bsptr = bsptr_copy(bsp);
      VisitLog new_visited = visitlog_make(visited);

      write_mol((LmnAtom)atom, LMN_ATTR_MAKE_LINK(0), -1, new_bsptr, new_visited);
      v_bsp[i] = new_bsptr;
      v_visited[i] = new_visited;

      if (!first) { first = FALSE; first_func = f; }
    } 
  }

  /* 書き込みに成功している分子に対して、まだ書き込んでいない分子を再帰
     的に書き込む */
  for (i = 0; i < natom; i++) {
    if (v_bsp[i] && bsptr_valid(v_bsp[i])) {
      vec_data_t t = vec_get(atoms, i);
      vec_set(atoms, i, 0);
      write_mols(atoms, v_bsp[i], v_visited[i]);
      vec_set(atoms, i, t);
    }
  }

  /* この時点で有効なポインタは、すべての分子を書き込み、かつ、最小のバ
     イナリストリングを書き込んでいるそこで、そのうち、一つを結果として採用する */
  for (i = 0; i < natom; i++) {
    if (v_bsp[i] && bsptr_valid(v_bsp[i])) {
      bsptr_copy_to(v_bsp[i], bsp);
      visitlog_copy_to(v_visited[i], visited); break;
    }
  }

  /* 後始末 */
  for (i = 0; i < natom; i++) {
    if (v_bsp[i]) {
      visitlog_free(v_visited[i]);
    }
  }    
}

/* write_atomsの膜バージョン。ここで書き込む計算する分子には、膜のみが
   含まれている */
void write_mems(LmnMembrane *mem,
                BinStrPtr bsp,
                VisitLog visited)
{
  LmnMembrane *m;
  int i;
  Vector *v_bsp;
  Vector *v_visited;

  if (!bsptr_valid(bsp)) return;
  
  v_bsp = vec_make(64);
  v_visited = vec_make(64);

  for (m = mem->child_head; m; m = m->next) {
    if (!visitlog_contains(visited, (LmnWord)m)) {
      BinStrPtr new_bsptr = bsptr_copy(bsp);
      VisitLog new_visited = visitlog_make(visited);

      write_mem(m, 0, -1, -1, new_bsptr, new_visited);
      vec_push(v_bsp, (vec_data_t)new_bsptr);
      vec_push(v_visited, (vec_data_t)new_visited);
    }
  }

  for (i = 0; i < vec_num(v_bsp); i++) {
    if (!bsptr_valid((BinStrPtr)vec_get(v_bsp, i))) continue;
    write_mems(mem, (BinStrPtr)vec_get(v_bsp, i), (VisitLog)vec_get(v_visited, i));
  }

  for (i = 0; i < vec_num(v_bsp); i++) {
    if (bsptr_valid((BinStrPtr)vec_get(v_bsp, i))) {
      bsptr_copy_to((BinStrPtr)vec_get(v_bsp, i), bsp);
      visitlog_copy_to((VisitLog)vec_get(v_visited, i), visited);
      break;
    }
  }

  for (i = 0; i < vec_num(v_bsp); i++) {
    visitlog_free((VisitLog)vec_get(v_visited, i));
  }    

  vec_free(v_bsp);
  vec_free(v_visited);
}

/* memから一意のバイナリストリングを計算する */
BinStr lmn_mem_encode(LmnMembrane *mem)
{
  BinStr bs;

#ifdef PROFILE
  status_start_mem_encode_calc();
#endif
  
  bs = encode_root_mem(mem);

#ifdef PROFILE
  status_finish_mem_encode_calc();
#endif
  return bs;
}

/* 膜にあるアトムのファンクタを降順で返す */
Vector *mem_functors(LmnMembrane *mem)
{
  Vector *v = vec_make(16);
  HashIterator iter;
  LmnFunctor f;
  
  for (iter = hashtbl_iterator(&mem->atomset);
       !hashtbliter_isend(&iter);
       hashtbliter_next(&iter)) {
    f = hashtbliter_entry(&iter)->key;
    if (!LMN_IS_PROXY_FUNCTOR(f)) {
      vec_push(v, f);
    }
  }

  vec_sort(v, comp_int_greater_f);
  return v;
}

static Vector *mem_atoms(LmnMembrane *mem)
{
  Vector *functors;
  Vector *atoms;
  unsigned i;
  AtomListEntry *ent;
  
  functors = mem_functors(mem);
  atoms = vec_make(128);
  for (i = 0; i < vec_num(functors); i++) {
    ent = lmn_mem_get_atomlist(mem, (LmnFunctor)vec_get(functors, i));
    LmnSAtom a;
    EACH_ATOM(a, ent, {
        vec_push(atoms, (vec_data_t)a);
    });
  }

  vec_free(functors);
  return atoms;
}
