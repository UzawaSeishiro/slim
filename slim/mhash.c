/**
 * mhash.c
 *
 * cf. devel/sample/khiroto/mhash.lmn
 * �쳬�ؤ������ˤ��ɤ�ʤ����������ǤȰۤʤ�
 * ����ͳ�ϥ����Ȥ������ڡ������ȡ�
 *
 * ����ͤˤ�äƤϥ����С��ե��������Ƥ��ޤ���
 * �׻���̤��׻�����˰��äƤ��ޤ����Ȥ�����Τ�
 * ��դ��뤳��
 *
 * TODO: �ǡ������ȥ����ư����������ʸ����ˤΥϥå�����
 */

#include "mhash.h"
#include "internal_hash.h"
#include "atom.h"
#include "membrane.h"
#include <limits.h>
#include "functor.h"

/* tag */
#define ATOM_OBJ  0
#define MEM_OBJ   1

/* �þ褪��Ӿ軻��Ԥ��ݤξ�;�黻���Ѥ������ */
#define ADD_MOD_FACTOR  (INT_MAX/100)
#define MUL_MOD_FACTOR  (INT_MAX/100000)

/* ����ܥ륢�ȥ�Υϥå����� */
static inline unsigned int atom_hash(LmnAtomPtr a) {
  return LMN_FUNCTOR_NAME_ID(LMN_ATOM_GET_FUNCTOR(a));
}
/* �ǡ������ȥ�Υϥå����� */
static inline int data_hash(LmnAtomPtr a, LmnArity n) {
  int ret;
  switch(LMN_ATOM_GET_ATTR(a, n)) {
    case LMN_INT_ATTR:
      ret = LMN_ATOM_GET_LINK(a,n);
      break;
    case LMN_DBL_ATTR:
      /* TODO: ̤���� */
      LMN_ASSERT(FALSE);
      break;
    default:
      LMN_ASSERT(FALSE);
      break;
  }
  return ret;
}

/* �� -> �ϥå����� */
static SimpleHashtbl mem2h;

static int mhash_internal(LmnMembrane *mem)
{
  /*
   * Ŭ���ʽ����
   *   add: �û��δ�����
   *   mul: �軻�δ�����
   */
  long long int add = 3412;
  long long int mul = 3412;
  /* ����ѿ� */
  LmnAtomPtr a = NULL;
  LmnMembrane *m = NULL;
  HashEntry *obj = NULL;
  /* ������� */
  SimpleHashtbl objs0;  /* ���Ƥλ��줪��ӻҥ��ȥ� */
  SimpleHashtbl objs1;  /* ̤���� */
  HashSet objs2;        /* ������ */
  hashtbl_init(&objs0, 128);
  hashtbl_init(&objs1, 128);
  hashset_init(&objs2, 128);
  HashIterator i0;
  HashIterator i1;
  HashSetIterator i2;

  HashIterator atomit;

  /*
   * objs0 <- atom
   */
  for (atomit = hashtbl_iterator(&mem->atomset);
      !hashtbliter_isend(&atomit);
      hashtbliter_next(&atomit)) {
    AtomListEntry *ent = (AtomListEntry *)hashtbliter_entry(&atomit)->data;
    if (atomlist_head(ent) != lmn_atomlist_end(ent) &&
        LMN_IS_PROXY_FUNCTOR(LMN_ATOM_GET_FUNCTOR(atomlist_head(ent)))) {
      continue; /* skip proxies */
    }
    for (a = atomlist_head(ent);
        a != lmn_atomlist_end(ent);
        a = LMN_ATOM_GET_NEXT(a)) {
      hashtbl_put(&objs0, (HashKeyType)a, (HashValueType)ATOM_OBJ);
    }
  }
  /* objs0 <- mem */
  for(m = mem->child_head; m; m = m->next) {
    hashtbl_put(&objs0, (HashKeyType)m, (HashValueType)MEM_OBJ);
    /* ����Ϥ��餫����׻����Ƥ��� */
    hashtbl_put(&mem2h, (HashKeyType)m, (HashValueType)mhash_internal(m));
  }

  while (hashtbl_num(&objs0) > 0) {
    /* ����� */
    long long int mol;           /* ʬ�ҤΥϥå����� */
    long long int tmp = 0;       /* ���ܷ׻�ñ�̤Υϥå����� */
    long long int mol_add = 0;   /* ���ܷ׻�ñ�̤βû������� */
    long long int mol_mul = 41;  /* ���ܷ׻�ñ�̤ξ軻������ */

    hashtbl_clear(&objs1);
    hashset_clear(&objs2);
    i0 = hashtbl_iterator(&objs0);
    obj = hashtbliter_entry(&i0);

    hashtbl_put(&objs1, (HashKeyType)obj->key, (HashValueType)obj->data);
    hashtbl_delete(&objs0, (HashKeyType)obj->key);

    /* ʬ�ҤΥϥå����ͤη׻� */
    while (hashtbl_num(&objs1) > 0) {
      HashKeyType objptr;
      HashValueType objtag;

      i1 = hashtbl_iterator(&objs1);
      obj = hashtbliter_entry(&i1);
      objptr = obj->key;
      objtag = obj->data;

      hashset_add(&objs2, (HashKeyType)obj->key);
      hashtbl_delete(&objs1, (HashKeyType)obj->key);

      /* DEBUG */
      assert(!hashtbl_contains(&objs1,objptr));

      if (ATOM_OBJ == objtag) { /* ���ȥ� */
        int i;

        a = (LmnAtomPtr)objptr;
        tmp = atom_hash(a);

        for (i = 0; i < LMN_ATOM_GET_ARITY(a); i++) { /* ����� */
          if (LMN_IS_PROXY_FUNCTOR(LMN_ATOM_GET_FUNCTOR(a)) && i == 2) continue; /* �� */
          LmnWord link = LMN_ATOM_GET_LINK(a, i);
          LmnLinkAttr attr = LMN_ATOM_GET_ATTR(a, i);
          tmp *= 31; /* Ŭ������ */
          assert(tmp>=0);

          if (LMN_ATTR_IS_DATA(attr)) { /* �ǡ������ȥ� */
            tmp += data_hash(a, i);
            assert(tmp>=0);
          }
          else if (LMN_IN_PROXY_FUNCTOR == LMN_ATOM_GET_FUNCTOR((LmnAtomPtr)link)) {
            tmp += (atom_hash((LmnAtomPtr)link) * ((unsigned int)attr + 1));
            assert(tmp>=0);
          }
          else if (LMN_OUT_PROXY_FUNCTOR == LMN_ATOM_GET_FUNCTOR((LmnAtomPtr)link)) {
            unsigned int t = 0;
            LmnAtomPtr in; /* inside proxy */

            m = LMN_PROXY_GET_MEM(LMN_ATOM_GET_LINK((LmnAtomPtr)link, 0)); /* ���� */
            if(!hashset_contains(&objs2, (HashKeyType)m)) {
              hashtbl_put(&objs1, (HashKeyType)m, MEM_OBJ);
            }
            /* ���Ӥ���󥯤ν��� */
            while (!LMN_ATTR_IS_DATA(attr) &&
            LMN_OUT_PROXY_FUNCTOR == LMN_ATOM_GET_FUNCTOR((LmnAtomPtr)link)) {
              in = (LmnAtomPtr)LMN_ATOM_GET_LINK((LmnAtomPtr)link, 0);
              link = LMN_ATOM_GET_LINK(in, 1);
              attr = LMN_ATOM_GET_ATTR(in, 1);
              m = LMN_PROXY_GET_MEM((LmnAtomPtr)in);
              assert(hashtbl_get_default(&mem2h, (HashKeyType)m, 0));
              t += hashtbl_get_default(&mem2h, (HashKeyType)m, 0);
              t *= 13; /* Ŭ������ */
              assert(t>0);
            }
            if (LMN_ATTR_IS_DATA(attr)) { /* ��³�褬�ǡ������ȥ� */
              t += data_hash((LmnAtomPtr)in, 1);
            }
            else { /* ��³�褬����ܥ륢�ȥ� */
              t += atom_hash((LmnAtomPtr)link);
              t *= (unsigned int)(attr + 1);
              assert(t>0);
            }
          }
          else { /* ����ܥ륢�ȥ� */
            if(!hashset_contains(&objs2, (HashKeyType)link)) {
              hashtbl_put(&objs1, (HashKeyType)link, ATOM_OBJ);
            }
            tmp += (atom_hash((LmnAtomPtr)link) * ((unsigned int)attr + 1));
            assert(tmp>=0);
          }
        }
      }
      else if (MEM_OBJ == objtag) { /* �� */
        LmnWord link;
        LmnLinkAttr attr;
        LmnAtomPtr in;
        AtomListEntry *insides;
        unsigned int myhash; /* ������Υϥå����� */

        m = (LmnMembrane *)objptr;
        assert(hashtbl_contains(&mem2h, (HashKeyType)m));
        myhash = (unsigned int)hashtbl_get_default(&mem2h, (HashKeyType)m, 0);
        tmp = myhash;
        /*
         * TODO:
         * a(X1). {{'+'(X1)}}
         * �Τ褦�ʹ�¤�ǹ����Ǥ�Ʊ���Х�
         * ��mem2h�ˤʤ����get���褦�Ȥ����
         * ���Ф�Τǡ�������ˤ��ɤ�ʤ��褦�ˤ���
         */
//        insides = lmn_mem_get_atomlist(m, LMN_IN_PROXY_FUNCTOR);
//        if (insides) {
//          for (a = atomlist_head(insides);
//              a != lmn_atomlist_end(insides);
//              a = LMN_ATOM_GET_NEXT(a)) {
//            unsigned int s = 0, t = 0;
//
//            /* ��γ�¦�򤿤ɤ� */
//            LmnAtomPtr out = (LmnAtomPtr)LMN_ATOM_GET_LINK(a, 0);
//            if(LMN_OUT_PROXY_FUNCTOR == LMN_ATOM_GET_ATTR(out, 1)) { /* ����褬��ξ�� */
//              LmnMembrane *mm = LMN_PROXY_GET_MEM(out);
//              if (!hashset_contains(&objs2, (HashKeyType)mm)) {
//                hashtbl_put(&objs1, (HashKeyType)mm, (HashValueType)MEM_OBJ);
//              }
//            }
//            else { /* ����褬���ȥ�ξ�� */
//              if (LMN_ATTR_IS_DATA(LMN_ATOM_GET_ATTR(out, 1))) { /* �ǡ������ȥ� */
//                tmp += data_hash(out, 1);
//              }
//              else { /* ����ܥ륢�ȥ� */
//                LmnAtomPtr aa = (LmnAtomPtr)LMN_ATOM_GET_LINK(out, 1);
//                if (!hashset_contains(&objs2, (HashKeyType)aa)) {
//                  hashtbl_put(&objs1, (HashKeyType)aa, (HashValueType)ATOM_OBJ);
//                }
//              }
//            }
//            in = out;
//            link = LMN_ATOM_GET_LINK(out, 1);
//            attr = LMN_ATOM_GET_ATTR(out, 1);
//            /* ���Ӥ���󥯤ν��� */
//            while (!LMN_ATTR_IS_DATA(attr) &&
//                LMN_OUT_PROXY_FUNCTOR == LMN_ATOM_GET_FUNCTOR((LmnAtomPtr)link)) {
//              in = (LmnAtomPtr)LMN_ATOM_GET_LINK((LmnAtomPtr)link, 0);
//              link = LMN_ATOM_GET_LINK(in, 1);
//              attr = LMN_ATOM_GET_ATTR(in, 1);
//lmn_dump_cell(LMN_PROXY_GET_MEM(in));
//              assert(hashtbl_get_default(&mem2h, (HashKeyType)LMN_PROXY_GET_MEM(in), 0));
//              s += (unsigned int)hashtbl_get_default(&mem2h, (HashKeyType)LMN_PROXY_GET_MEM(in), 0);
//              s *= 13;
//              assert(s>0);
//            }
//            if (LMN_ATTR_IS_DATA(attr)) { /* �ǡ������ȥ� */
//              s += data_hash(in, 1);
//            }
//            else { /* ����ܥ륢�ȥ� */
//              s += atom_hash((LmnAtomPtr)link);
//              s *= (unsigned int)(attr + 1);
//              assert(s>0);
//            }
//
//            /* �����¦�򤿤ɤ� */
//            in = a;
//            link = LMN_ATOM_GET_LINK(a, 1);
//            attr = LMN_ATOM_GET_ATTR(a, 1);
//            /* ���Ӥ���󥯤ν��� */
//            while (!LMN_ATTR_IS_DATA(attr) &&
//                LMN_OUT_PROXY_FUNCTOR == LMN_ATOM_GET_FUNCTOR((LmnAtomPtr)link)) {
//              in = (LmnAtomPtr)LMN_ATOM_GET_LINK((LmnAtomPtr)link, 0);
//              link = LMN_ATOM_GET_LINK(in, 1);
//              attr = LMN_ATOM_GET_ATTR(in, 1);
//lmn_dump_cell(LMN_PROXY_GET_MEM(in));
//              assert(hashtbl_get_default(&mem2h, (HashKeyType)LMN_PROXY_GET_MEM(in), 0));
//              t += (int)hashtbl_get_default(&mem2h, (HashKeyType)LMN_PROXY_GET_MEM(in), 0);
//              t *= 13;
//              assert(t>0);
//            }
//            if (LMN_ATTR_IS_DATA(attr)) { /* �ǡ������ȥ� */
//              assert(LMN_IN_PROXY_FUNCTOR == LMN_ATOM_GET_FUNCTOR((LmnAtomPtr)a));
//              t *= data_hash(in, 1);
//              assert(t>=0);
//            }
//            else { /* ����ܥ륢�ȥ� */
//              t *= atom_hash((LmnAtomPtr)link);
//              assert(t>=0);
//              t *= (unsigned int)(attr + 1);
//              assert(t>=0);
//            }
//            tmp += myhash ^ t * s;
//            assert(tmp>0);
//          }
//        }
      }
      else {
        assert(0);
      }
      mol_add += tmp;
      mol_add %= ADD_MOD_FACTOR;
      mol_mul *= (tmp % MUL_MOD_FACTOR);
      assert(mol_mul>=0);
      mol_mul %= (MUL_MOD_FACTOR / 10);
    }

    for (i2 = hashset_iterator(&objs2);
        !hashsetiter_isend(&i2);
        hashsetiter_next(&i2)) {
      /*
       * a(X1). {{'+'(X1)}}
       * �Τ褦�ʤȤ����쳬�غǾ�̤ˤ�����
       * �֥��ȥ�->��פȤ���������ȡ���->���ȥ�פȤ����������
       * ��̤���äƤ��ޤäƤ�������ʲ���ifʸ���ɲ�
       * �����ԤǤϻ��줬���󡤸�ԤǤϣ����������Ƥ�����
       * �ᥤ��롼�פǥ��ȥ�->��ν�ǽ�������ʤɡ�¾���н���ˡ����
       */
      if(hashtbl_get(&objs0, hashsetiter_entry(&i2)) != MEM_OBJ) {
        hashtbl_delete(&objs0, hashsetiter_entry(&i2));
      }
    }

    mol = mol_add ^ mol_mul;
    add += mol;
    assert(add>0);
    add %= (unsigned int)ADD_MOD_FACTOR;
    mul *= mol;
    assert(mul>=0);
    mul %= (unsigned int)MUL_MOD_FACTOR;
  }

  hashtbl_destroy(&objs0);
  hashtbl_destroy(&objs1);
  hashset_destroy(&objs2);
  assert(INT_MIN <= mul^add && mul^add <= INT_MAX);
  return (int)(mul ^ add) + mem->name; /* ��̾��ȿ�Ǥ����� */
}

int mhash(LmnWord mem)
{
  int ret = 1;
/*   hashtbl_init(&mem2h, 32); */
/*   ret =  mhash_internal((LmnMembrane *)mem); */
/*   hashtbl_destroy(&mem2h); */
  return ret;
}

