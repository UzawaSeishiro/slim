#include "mc.h"
#include <string.h>

/**
 * ���󥹥ȥ饯��
 */
State *state_make(LmnMembrane *mem) {
  State *new = LMN_MALLOC(State);
  new->mem = mem;
  new->flags = 0x00U;
  /* successor�ε�����򥼥��ꥢ���� */
  memset(&new->successor, 0x00U, sizeof(Vector));
  /* �ϥå����ͤϤ��餫����׻����Ƥ��� */
  new->hash = mhash(new->mem);
  return new;
}

/**
 * ������s�κǽ��n�Х��Ȥ�����Ǥ��뤳�Ȥ��ǧ����
 */
static inline BOOL mem_is_zero(const void *s, size_t n) {
  const unsigned char *p = (const unsigned char *)s;
  while (n-- > 0 ) {
    if(*p != 0x00U) return FALSE;
    p++;
  }
  return TRUE;
}

inline void state_succ_init(State *s, int init_size) {
  vec_init(&s->successor, init_size);
}

/**
 * �ǥ��ȥ饯��
 */
void state_free(State *s) {
  lmn_mem_drop(s->mem);
  lmn_mem_free(s->mem);
  if (!mem_is_zero(&s->successor, sizeof(Vector))) {
    vec_destroy(&s->successor);
  }
  LMN_FREE(s);
}

inline int state_hash(LmnWord s) {
  return ((State *)s)->hash;
}

/**
 * �����Ȥ��Ƥ�������줿State�����������ɤ�����Ƚ�ꤹ��
 * �ϥå����ͤ�����������Ʊ��Ƚ���Ԥ�
 */
static int state_equals(HashKeyType s1, HashKeyType s2) {
  State *ss1 = (State *)s1;
  State *ss2 = (State *)s2;
  int h1 = state_hash((LmnWord)ss1);
  int h2 = state_hash((LmnWord)ss2);

  if (h1 != h2) return FALSE;
  return lmn_mem_equals(ss1->mem, ss2->mem);
}

/**
 * Ϳ����줿2�Ĥξ��֤��ߤ��˰ۤʤäƤ���п��򡢵դ����������ϵ����֤�
 */
int state_cmp(HashKeyType s1, HashKeyType s2) {
  return !state_equals(s1, s2);
}

/**
 * �쥹���å���������
 * ���Ȥ�ޤ᤿���Ƥ�������򵯤���
 */
inline void activate_ancestors(LmnMembrane *mem) {
  LmnMembrane *cur;
  for (cur=mem; cur; cur=cur->parent) {
    cur->is_activated = TRUE;
  }
}
