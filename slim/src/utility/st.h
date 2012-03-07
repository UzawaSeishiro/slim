/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

/** http://sobjc.googlecode.com/svn/trunk/runtime/st.c�򸵤��ѹ�������
 *  st.h,st.c�ϥѥ֥�å��ɥᥤ��Υϥå���ơ��֥�饤�֥��ǡ�ruby�Ǥ�Ȥ��Ƥ��롣
 *  ���ꥸ�ʥ��st.h,st.c�äƤɤ���������Ǥ���Τ�������
 */

#ifndef ST_INCLUDED
#define ST_INCLUDED

#include "lmntal.h"
#include "lmntal_thread.h"
#include <stddef.h>

typedef unsigned long st_data_t;
typedef struct st_table *st_table_t;

struct st_hash_type {
  int (*compare)(); /* �оݤ�2�ĤΥ���ȥ꡼(st_table_entry)��Ʊ�����������°����ʤ�е��������Ǥʤ���п����֤��ؿ� */
  long (*hash)();    /* �ϥå���ؿ� */
};

/* num_bins = 5, num_entries = 3 �ʤ� struct st_table_entry **bins ����
 * ("��"�ϥݥ���, NULL�ϥݥ��󥿤λؤ������褬¸�ߤ��ʤ����Ȥ�ɽ��)
 *
 *  bins[0]��(st_table_entry)��(st_table_entry)
 *  bins[1]�� NULL
 *  bins[2]��(st_table_entry)
 *  bins[3]�� NULL
 *  bins[4]�� NULL
 */
struct st_table {
  struct st_hash_type *type;
  unsigned long num_bins; /* �ϥå���ɽ�Υ�����(����åȿ�) */
  unsigned long num_entries; /* �ϥå���ɽ��������ޤ줿���ǤθĿ�
                      (�ƥ���åȤ�Ʊ��Υϥå����ͤ�������Ǥ��Ǽ����(Linked)�ꥹ�ȹ�¤(struct st_table_entry *)�������
                       Ʊ���ꥹ�����������ޤ줿�����Ǥ��̸Ĥ˥�����Ȥ���) */
  struct st_table_entry **bins; /* ��������ˡ�˴�Ť��ϥå���ɽ���� */
};

#define st_is_member(table,key) st_lookup(table,key,(st_data_t *)0)

enum st_retval {
  ST_CONTINUE, ST_STOP, ST_DELETE, ST_CHECK
};

#ifndef _
# define _(args) args
#endif
#ifndef ANYARGS
# ifdef __cplusplus
#   define ANYARGS ...
# else
#   define ANYARGS
# endif
#endif

static inline unsigned long st_num(st_table_t table) {
  return table->num_entries;
}

static inline unsigned long st_cap(st_table_t table) {
  return table->num_bins;
}

st_table_t st_init_table(struct st_hash_type *t);
st_table_t st_init_table_with_size(struct st_hash_type *t, int size);
st_table_t st_init_statetable(void);
st_table_t st_init_statetable_with_size(int size);
st_table_t st_init_numtable(void);
st_table_t st_init_numtable_with_size(int size);
st_table_t st_init_strtable(void);
st_table_t st_init_strtable_with_size(int size);
st_table_t st_init_ptrtable(void);
st_table_t st_init_ptrtable_with_size(int size);
int st_delete(register st_table_t tbl, register st_data_t key, st_data_t *value);
int st_delete_safe(register st_table_t tbl, st_data_t *key, st_data_t *value, st_data_t never);
int st_insert(st_table_t tbl, st_data_t key, st_data_t value);
int st_insert_safe(st_table_t tbl, st_data_t key, st_data_t value);
int st_lookup(st_table_t tbl, st_data_t key, st_data_t *value);
int st_lookup_with_col(st_table_t tbl, st_data_t key, st_data_t *value, long *n_col);
int st_contains(st_table_t tbl, st_data_t key);
int st_foreach(st_table_t tbl, int(*func)( ANYARGS), st_data_t arg);
void st_add_direct(st_table_t tbl, st_data_t key, st_data_t value);
unsigned long st_table_space(st_table_t tbl);
void st_free_table(st_table_t tbl);
void st_cleanup_safe(st_table_t tbl, st_data_t never);
void st_clear(st_table_t tbl);
st_table_t st_copy(st_table_t tbl);
void st_print(st_table_t tbl);
void st_get_entries_key(st_table_t tbl, Vector *vec);
void st_get_entries_value(st_table_t tbl, Vector *vec);
int st_equals(st_table_t tbl1, st_table_t tbl2);
long st_strhash(const char *str);
int st_numcmp(long num1, long num2);
long st_numhash(long num);
long st_statehash(LmnWord state);
int st_foreach_hash(st_table_t table, st_data_t hash, int(*func)( ANYARGS), st_data_t arg);

/* tbl1��tbl2�Τ��٤ƤΥ���ȥ���ɲä��롣tbl1��tbl2��Ʊ����������ĥ�
   ��ȥ꤬¸�ߤ������ư��ϡʤȤꤢ������̤����� */
void st_concat(st_table_t tbl1, const st_table_t tbl2);

unsigned long st_table_space(st_table_t table);



#endif /* ST_INCLUDED */
