/* This is a public domain general purpose hash table package written by Peter Moore @ UCB. */

/* @(#) st.h 5.1 89/12/14 */

/* http://sobjc.googlecode.com/svn/trunk/runtime/st.c
 �򸵤��ѹ�������
 st.h,st.c�ϥѥ֥�å��ɥᥤ��Υϥå���ơ��֥�饤�֥��ǡ�
 ruby�Ǥ�Ȥ��Ƥ��롣���ꥸ�ʥ��st.h,st.c�äƤɤ���������
 �Ǥ���Τ������� */

#ifndef ST_INCLUDED
#define ST_INCLUDED

#include <stddef.h>

typedef void *st_data_t;
typedef struct st_table st_table, *st_table_t;

struct st_hash_type {
  int (*compare)(); /* �оݤ�2�ĤΥ���ȥ꡼(st_table_entry)��Ʊ�����������°����ʤ�е��������Ǥʤ���п����֤��ؿ� */
  int (*hash)();    /* �ϥå���ؿ� */
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
  int num_bins; /* �ϥå���ɽ�Υ�����(����åȿ�) */
  int num_entries; /* �ϥå���ɽ��������ޤ줿���ǤθĿ�
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

st_table *st_init_table(struct st_hash_type *);
st_table *st_init_table_with_size(struct st_hash_type *, int);
st_table *st_init_numtable(void);
st_table *st_init_numtable_with_size(int);
st_table *st_init_strtable(void);
st_table *st_init_strtable_with_size(int);
st_table *st_init_ptrtable(void);
st_table *st_init_ptrtable_with_size(int);
int st_delete(st_table *, st_data_t *, st_data_t *);
int st_delete_safe(st_table *, st_data_t *, st_data_t *, st_data_t);
int st_insert(st_table *, st_data_t, st_data_t);
int st_lookup(st_table *, st_data_t, st_data_t *);
int st_foreach(st_table *, int(*)(ANYARGS), st_data_t);
void st_add_direct(st_table *, st_data_t, st_data_t);
void st_free_table(st_table *);
void st_cleanup_safe(st_table *, st_data_t);
st_table *st_copy(st_table *);
int st_strhash(const char *);
int st_numcmp(long, long);
int st_numhash(long);

#endif /* ST_INCLUDED */
