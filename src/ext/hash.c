#include <stdio.h>
#include "../vm/dumper.h"
#include "../element/lmnstring.h"
#include "hash.h"
#include "../vm/special_atom.h"
#include "../verifier/mem_encode.h"
#include "../verifier/mc_worker.h"

static int hash_atom_type;

static int state_map_atom_type;

static BYTE mc_flag = 0x10U;

#define LMN_HASH_DATA(obj) (LMN_HASH(obj)->tbl)

/*
 * Internal Constructor
 */

static LmnHashRef lmn_make_hash(LmnMembraneRef mem)
{
  LmnHashRef h = LMN_MALLOC(struct LmnHash);
  LMN_SP_ATOM_SET_TYPE(h, hash_atom_type);
  h->tbl = st_init_table(&type_mem_hash);
  return h;
}

static LmnStateMapRef lmn_make_state_map(LmnMembraneRef mem)
{
  LmnStateMapRef s = LMN_MALLOC(struct LmnStateMap);
  LMN_SP_ATOM_SET_TYPE(s, state_map_atom_type);
  s->states = statespace_make(NULL, NULL);
  return s;
}

void lmn_hash_free(LmnHashRef hash, LmnMembraneRef mem)
{
  st_free_table(LMN_HASH_DATA(hash));
  LMN_FREE(hash);
}

void lmn_state_map_free(LmnStateMapRef state_map, LmnMembraneRef mem)
{
  statespace_free(LMN_STATE_MAP(state_map)->states);
  LMN_FREE(state_map);
}

/*----------------------------------------------------------------------
 * Callbacks
 */

/* 
 *生成
 *
 * -a0: ハッシュテーブル
 */
void cb_hash_init(LmnReactCxtRef rc,
                  LmnMembraneRef mem,
                  LmnAtom a0, LmnLinkAttr t0)
{
  LmnHashRef atom = lmn_make_hash(mem);
  LmnLinkAttr attr = LMN_SP_ATOM_ATTR;
  LMN_SP_ATOM_SET_TYPE(atom, hash_atom_type);
  lmn_mem_push_atom(mem, LMN_ATOM(atom), attr);
  lmn_mem_newlink(mem,
                  a0, t0, LMN_ATTR_GET_VALUE(t0),
                  LMN_ATOM(atom), attr, 0);
}

void cb_state_map_init(LmnReactCxtRef rc,
                       LmnMembraneRef mem,
                       LmnAtom a0, LmnLinkAttr t0)
{
  LmnStateMapRef atom = lmn_make_state_map(mem);
  LmnLinkAttr attr = LMN_SP_ATOM_ATTR;
  LMN_SP_ATOM_SET_TYPE(atom, state_map_atom_type);
  lmn_mem_push_atom(mem, LMN_ATOM(atom), attr);
  lmn_mem_newlink(mem,
                  a0, t0, LMN_ATTR_GET_VALUE(t0),
                  LMN_ATOM(atom), attr, 0);
}

/* 
 * 解放
 *
 * +a0: ハッシュテーブル
 */
void cb_hash_free(LmnReactCxtRef rc,
                  LmnMembraneRef mem,
                  LmnAtom a0, LmnLinkAttr t0)
{
  lmn_hash_free(LMN_HASH(a0), mem);
  lmn_mem_remove_data_atom(mem, a0, t0);
}

void cb_state_map_free(LmnReactCxtRef rc,
                       LmnMembraneRef mem,
                       LmnAtom a0, LmnLinkAttr t0)
{
  lmn_state_map_free(LMN_STATE_MAP(a0), mem);
  lmn_mem_remove_data_atom(mem, a0, t0);
}

/* 
 *要素取得
 *
 * +a0: ハッシュテーブル
 * +a1: 膜
 * -a2: 判定
 * -a3: 膜
 * -a4: 新ハッシュテーブル
 */
void cb_hash_get(LmnReactCxtRef rc,
                 LmnMembraneRef mem,
                 LmnAtom a0, LmnLinkAttr t0,
                 LmnAtom a1, LmnLinkAttr t1,
                 LmnAtom a2, LmnLinkAttr t2,
                 LmnAtom a3, LmnLinkAttr t3,
                 LmnAtom a4, LmnLinkAttr t4)
{
  st_data_t entry;
  LmnSAtom result;
  LmnMembraneRef m = LMN_PROXY_GET_MEM(LMN_SATOM_GET_LINK(a1, 0));
  int res = st_lookup(LMN_HASH_DATA(a0), (st_data_t)m, &entry);
  if(res){
    result = lmn_mem_newatom(mem, lmn_functor_intern(ANONYMOUS, lmn_intern("success"), 1));}
  else{
    result = lmn_mem_newatom(mem, lmn_functor_intern(ANONYMOUS, lmn_intern("fail"), 1));}
  lmn_dump_cell_stdout((LmnMembraneRef )entry);
  lmn_mem_newlink(mem,
                  LMN_ATOM(result), LMN_ATTR_MAKE_LINK(0), 0,
                  a2, t2, LMN_ATTR_GET_VALUE(t2));

  lmn_mem_newlink(mem,
                  a1, t1, LMN_ATTR_GET_VALUE(t1),
                  a3, t3, LMN_ATTR_GET_VALUE(t3));

  lmn_mem_newlink(mem,
                   a0, t0, LMN_ATTR_GET_VALUE(t0),
                   a4, t4, LMN_ATTR_GET_VALUE(t4));
}

/* 
 * 要素挿入
 * 
 * +a0: ハッシュテーブル
 * +a1: 膜
 * -a2: 膜
 * -a3: 新ハッシュテーブル
 */
void cb_hash_put(LmnReactCxtRef rc,
                 LmnMembraneRef mem,
                 LmnAtom a0, LmnLinkAttr t0,
                 LmnAtom a1, LmnLinkAttr t1,
                 LmnAtom a2, LmnLinkAttr t2,
                 LmnAtom a3, LmnLinkAttr t3)
{
  LmnMembraneRef m = LMN_PROXY_GET_MEM(LMN_SATOM_GET_LINK(a1, 0));
  LmnMembraneRef key = lmn_mem_copy_ex(m);
  st_insert(LMN_HASH_DATA(a0), (st_data_t)key, (st_data_t)key);

  lmn_mem_newlink(mem,
                  a1, t1, LMN_ATTR_GET_VALUE(t1),
                  a2, t2, LMN_ATTR_GET_VALUE(t2));

  lmn_mem_newlink(mem,
                  a0, t0, LMN_ATTR_GET_VALUE(t0),
                  a3, t3, LMN_ATTR_GET_VALUE(t3));
}

/*
 * 要素挿入
 *
 * +a0: ハッシュテーブル
 * +a1: 膜(key)
 * +a2: 膜(val)
 * -a3: ハッシュテーブル
 */
void cb_map_put(LmnReactCxtRef rc,
                LmnMembraneRef mem,
                LmnAtom a0, LmnLinkAttr t0,
                LmnAtom a1, LmnLinkAttr t1,
                LmnAtom a2, LmnLinkAttr t2,
                LmnAtom a3, LmnLinkAttr t3)
{
  LmnMembraneRef key = LMN_PROXY_GET_MEM(LMN_SATOM_GET_LINK(a1, 0));
  LmnMembraneRef val = LMN_PROXY_GET_MEM(LMN_SATOM_GET_LINK(a2, 0));
  st_insert(LMN_HASH_DATA(a0), (st_data_t)key, (st_data_t)val);

  lmn_mem_newlink(mem,
                  a0, t0, LMN_ATTR_GET_VALUE(t0),
                  a3, t3, LMN_ATTR_GET_VALUE(t3));

  lmn_mem_remove_mem(mem, key);
  lmn_mem_remove_mem(mem, val);
}




/*
 * 要素取得
 * +a0: ハッシュテーブル
 * +a1: 膜(key)
 * -a2: 判定
 * -a3: 膜(key)
 * -a4: ハッシュテーブル
 */

void cb_map_get(LmnReactCxtRef rc,
                LmnMembraneRef mem,
                LmnAtom a0, LmnLinkAttr t0,
                LmnAtom a1, LmnLinkAttr t1,
                LmnAtom a2, LmnLinkAttr t2,
                LmnAtom a3, LmnLinkAttr t3,
                LmnAtom a4, LmnLinkAttr t4)
{
  st_data_t entry;
  LmnSAtom result;
  LmnMembraneRef key = LMN_PROXY_GET_MEM(LMN_SATOM_GET_LINK(a1, 0));
  int res = st_lookup(LMN_HASH_DATA(a0), (st_data_t)key, &entry);

  if(res){
    result = lmn_mem_newatom(mem, lmn_functor_intern(ANONYMOUS, lmn_intern("some"), 2));
    LmnMembraneRef val = (LmnMembraneRef )entry;
    AtomListEntryRef ent;
    LmnFunctor f;
    LmnSAtom in;
    LmnSAtom out = lmn_mem_newatom(mem, LMN_OUT_PROXY_FUNCTOR);
    EACH_ATOMLIST_WITH_FUNC(val, ent, f, ({
          LmnSAtom satom;
          EACH_ATOM(satom, ent, ({
                if(f==LMN_IN_PROXY_FUNCTOR){
                  in = satom;
                }
              }))
            }));
    lmn_newlink_in_symbols(out, 0, in, 0);
    lmn_newlink_in_symbols(out, 1, result, 1);
  }else{
    result = lmn_mem_newatom(mem, lmn_functor_intern(ANONYMOUS, lmn_intern("none"), 1));
  }
  lmn_mem_newlink(mem,
                  LMN_ATOM(result), LMN_ATTR_MAKE_LINK(0), 0,
                  a2, t2, LMN_ATTR_GET_VALUE(t2));

  lmn_mem_newlink(mem,
                  a1, t1, LMN_ATTR_GET_VALUE(t1),
                  a3, t3, LMN_ATTR_GET_VALUE(t3));

  lmn_mem_newlink(mem,
                   a0, t0, LMN_ATTR_GET_VALUE(t0),
                   a4, t4, LMN_ATTR_GET_VALUE(t4));
}

/*
 * ID->状態
 * +a0 Map
 * +a1 ID
 * -a2 状態
 * -a3 Map
 */
void cb_state_map_state_find(LmnReactCxt *rc,
			     LmnMembrane *mem,
			     LmnAtom a0, LmnLinkAttr t0,
			     LmnAtom a1, LmnLinkAttr t1,
			     LmnAtom a2, LmnLinkAttr t2,
			     LmnAtom a3, LmnLinkAttr t3)
{
  st_table_t i_tbl=LMN_STATE_MAP(a0)->id_tbl;
  State *s=(State *)a1;
  st_data_t entry;
  int res=st_lookup(i_tbl, (st_data_t)s, &entry);
  LmnSAtom result;
  if(res){
    LmnMembrane *val=LMN_MALLOC(LmnMembrane);
    val=lmn_mem_copy((LmnMembrane *)entry);
    AtomListEntry *ent;
    LmnFunctor f;
    LmnSAtom in;
    LmnSAtom out = lmn_mem_newatom(mem, LMN_OUT_PROXY_FUNCTOR);
    EACH_ATOMLIST_WITH_FUNC(val, ent, f, ({
	  LmnSAtom satom;
	  EACH_ATOM(satom, ent, ({
		if(f==LMN_IN_PROXY_FUNCTOR){
		  in=satom;
		}
	      }))
	    }));
    lmn_newlink_in_symbols(out, 0, in, 0);
    lmn_mem_newlink(mem,
		    a2, t2, LMN_ATTR_GET_VALUE(t2),
		    out, LMN_ATTR_MAKE_LINK(1),1);
  }else{
    result=lmn_mem_newatom(mem, lmn_functor_intern(ANONYMOUS, lmn_intern("none"), 1));
    lmn_mem_newlink(mem,
		    LMN_ATOM(result), LMN_ATTR_MAKE_LINK(0), 0,
		    a2, t2, LMN_ATTR_GET_VALUE(t2));
  }
  lmn_mem_newlink(mem,
		  a0, t0, LMN_ATTR_GET_VALUE(t1),
		  a3, t3, LMN_ATTR_GET_VALUE(t3));
}

/*
 * 状態->ID
 * +a0 Map
 * +a1 状態
 * -a2 ID
 * -a3 Map
 */
void cb_state_map_id_find(LmnReactCxtRef rc,
                          LmnMembraneRef mem,
                          LmnAtom a0, LmnLinkAttr t0,
                          LmnAtom a1, LmnLinkAttr t1,
                          LmnAtom a2, LmnLinkAttr t2,
                          LmnAtom a3, LmnLinkAttr t3)
{
  LmnMembraneRef key = LMN_PROXY_GET_MEM(LMN_SATOM_GET_LINK(a1, 0));
  AtomListEntryRef ent;
  LmnFunctor f;
  LmnSAtom in = LMN_SATOM_GET_LINK(a1, 0);
  LmnSAtom out = a1;
  LmnSAtom plus = LMN_SATOM_GET_LINK(in, 1);
  LmnBinStrRef s;
  LmnLinkAttr in_attr = LMN_SATOM_GET_ATTR(a1, 0);
  StateSpaceRef ss = LMN_STATE_MAP(a0)->states;
  volatile StateTable *dst_st = statespace_tbl(ss);


  lmn_mem_delete_atom(key, in, in_attr);

  LmnSAtom at = lmn_mem_newatom(key, lmn_functor_intern(ANONYMOUS, lmn_intern("@"), 1));
  lmn_newlink_in_symbols(plus, 0, at, 0);

  State *new_s = state_make(key, NULL, mc_use_canonical(mc_flag));

  State *succ = statespace_insert(ss, new_s);

  if(succ == new_s){
    /* new state */
    state_id_issue(succ);
  }
  lmn_mem_push_atom(mem, succ, LMN_INT_ATTR);
  lmn_mem_newlink(mem,
                  a2, t2, LMN_ATTR_GET_VALUE(t2),
                  (int)succ, LMN_INT_ATTR, 0);

  lmn_mem_newlink(mem,
                  a0, t0, LMN_ATTR_GET_VALUE(t0),
                  a3, t3, LMN_ATTR_GET_VALUE(t3));

  lmn_mem_delete_atom(mem, a1, t1);

  lmn_mem_remove_mem(mem, key);
}

/*----------------------------------------------------------------------
 * Initialization
 */

void *sp_cb_hash_copy(void *data)
{
  /* LmnHashRef h = LMN_MALLOC(struct LmnHash); */
  /* LMN_SP_ATOM_SET_TYPE(h, hash_atom_type); */
  /* h->tbl = st_copy(LMN_HASH_DATA(LMN_HASH(data))); */
  return data;
}

void sp_cb_hash_free(void *data)
{
  /* lmn_hash_free(LMN_HASH(data), NULL); */
}

void sp_cb_hash_eq(void *_p1, void *_p2)
{
}

void sp_cb_hash_dump(void *hash, LmnPortRef port)
{
  port_put_raw_s(port, "<hash>");
}

void sp_cp_hash_is_ground(void *data)
{
}

void *sp_cb_state_map_copy(void *data)
{
  return data;
}

void sp_cb_state_map_free(void *data)
{
}

void sp_cb_state_map_eq(void *_p1, void *_p2)
{
}

void sp_cb_state_map_dump(void *state_map, LmnPortRef port)
{
  port_put_raw_s(port, "<state_map>");
}

void sp_cb_state_map_is_ground(void *data)
{
}

void init_hash(void)
{
  hash_atom_type = lmn_sp_atom_register("hash",
                                        sp_cb_hash_copy,
                                        sp_cb_hash_free,
                                        sp_cb_hash_eq,
                                        sp_cb_hash_dump,
                                        sp_cp_hash_is_ground);
  state_map_atom_type = lmn_sp_atom_register("state_map",
                                         sp_cb_state_map_copy,
                                         sp_cb_state_map_free,
                                         sp_cb_state_map_eq,
                                         sp_cb_state_map_dump,
                                         sp_cb_state_map_is_ground);
  lmn_register_c_fun("cb_hash_init", (void *)cb_hash_init, 1);
  lmn_register_c_fun("cb_hash_get", (void *)cb_hash_get, 5);
  lmn_register_c_fun("cb_hash_put", (void *)cb_hash_put, 4);
  lmn_register_c_fun("cb_hash_free", (void *)cb_hash_free, 1);
  lmn_register_c_fun("cb_map_put", (void *)cb_map_put, 4);
  lmn_register_c_fun("cb_map_get", (void *)cb_map_get, 5);
  lmn_register_c_fun("cb_state_map_init", (void *)cb_state_map_init, 1);
  lmn_register_c_fun("cb_state_map_free", (void *)cb_state_map_free, 1);
  lmn_register_c_fun("cb_state_map_id_find", (void *)cb_state_map_id_find, 4);
  lmn_register_c_fun("cb_state_map_state_find", (void *)cb_state_map_state_find, 4);
}
