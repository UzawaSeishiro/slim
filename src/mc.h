#ifndef MC_H
#define MC_H

#include "st.h"
#include "internal_hash.h"
#include "membrane.h"
#include "vector.h"

/**
 * task.c��interpret()���Ѥ�����ե饰�򽸤᤿���
 *
 * nd_exec: �����Ū�¹ԥե饰
 *   ������롼��Ŭ�Ѱ�����FALSE
 *   ������롼��Ŭ�Ѱʹߡ�TRUE
 * system_rule_committed: �ܥǥ��¹���ե饰
 *   �ܥǥ��¹���Τ�TRUE (i.e. �롼��Υܥǥ�����Ƭ��COMMIT̿��ν������ϻ���TRUE�ȤʤꡢPROCEED̿��ν�����λ����FALSE�ˤʤ�)
 *   ���դ˽и�����PROCEED��GROUP��λ��ɽ���ˤȱ��դ˽и�����PROCEED����̤��뤿��˻���
 * system_rule_proceeded: �����ƥ�롼��Ŭ�������ե饰
 *   �����ƥ�롼��Ŭ����������TRUE
 *   �����ƥ�롼��¹Ի���interpret()�����FALSE���֤����ͤȤʤäƤ��뤿�ᡢ�����ƥ�롼��Ŭ��������ɽ���ե饰�Ȥ�������ˤ�����Ѥ���
 * property_rule: �����롼��¹���ե饰
 *   �����롼��Ŭ����������TRUE���֤���Ū�ǻ���
 */
typedef struct McFlags {
  BOOL nd_exec;
  BOOL system_rule_committed;
  BOOL system_rule_proceeded;
  BOOL property_rule;
} McFlags;

typedef struct State State;

struct State {
  LmnMembrane *mem; /* �����Х�롼���� */
  int hash;         /* �ϥå����� */
  BOOL flags;       /* flags (unsigned char) */
  Vector successor; /* successor nodes */
};

LMN_EXTERN State *state_make(LmnMembrane *mem);
LMN_EXTERN inline void state_succ_init(State *s, int init_size);
LMN_EXTERN void state_free(State *s);

/* flag of the first DFS (nested DFS, on-stack state) */
#define FST_MASK (0x01U)
/* flag of the second DFS (nested DFS, visited state) */
#define SND_MASK (0x02U)
/* macros for nested DFS */
#define set_fst(S)    ((S)->flags |= FST_MASK)
#define unset_fst(S)  ((S)->flags &= (~FST_MASK))
#define is_fst(S)     ((S)->flags & FST_MASK)
#define set_snd(S)    ((S)->flags |= SND_MASK)
#define unset_snd(S)  ((S)->flags &= (~SND_MASK))
#define is_snd(S)     ((S)->flags & SND_MASK)

LMN_EXTERN int state_hash(LmnWord s);
LMN_EXTERN int state_cmp(HashKeyType s1, HashKeyType s2);

LMN_EXTERN inline void activate_ancestors(LmnMembrane *mem);

#endif
