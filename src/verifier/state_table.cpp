/*
 * state_table.cpp
 *
 *   Copyright (c) 2019, Ueda Laboratory LMNtal Group
 *                                         <lmntal@ueda.info.waseda.ac.jp>
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
 */

#include "state_table.hpp"

#include "state.h"
#include "state.hpp"

#define STATE_EQUAL(Tbl, Check, Stored)                                        \
  (state_hash(Check) == state_hash(Stored) &&                                  \
   ((Tbl)->type->compare)(Check, Stored))

/* 既に計算済のバイナリストリングbsを状態sに登録する.
 * statetable_{insert/add_direct}内の排他制御ブロック内で呼び出す. */
static void state_set_compress_for_table(State *s, LmnBinStr *bs) {
  if (!s->is_encoded() && bs) {
    s->state_set_binstr(bs);
  }
}

static inline int statetable_cmp_state_id_gr_f(const void *a_, const void *b_) {
  State *p1, *p2;

  p1 = *(State **)a_;
  p2 = *(State **)b_;

  if (p1 && p2) {
    return state_id(p1) - state_id(p2);
  } else if (p1 || p2) {
    return p1 ? state_id(p1) : (-1 * state_id(p2));
  } else {
    return 1;
  }
}

static inline void statetable_issue_state_id_f(State *s) {
  static unsigned long id = 1;
  state_set_format_id(s, id++);
}

/* 状態管理表stに登録されているcompress関数を用いて,
 * 状態sのバイナリストリングbsを計算して返す.
 * (--disable-compressの場合はdummy関数がNULLを返す.)
 * bsが計算済みの場合(NULLでない場合)は, 何もしない */
LmnBinStrRef StateTable::compress_state(State *s, LmnBinStrRef bs) {
  if (!bs) {
    /* canonical idにエンコードを行った場合,
     * bsは状態生成時に計算済みになっているため,
     * このブロックを実行することはない. */
    LMN_ASSERT(!s->is_encoded());
    bs = (*((this)->type->compress))(s);
  }
  return bs;
}

/** Global Vars
 */
struct statespace_type type_state_compress_z = {state_cmp_with_compress,
                                                state_calc_mem_dump_with_z};

struct statespace_type type_state_compress = {state_cmp_with_compress,
                                              state_calc_mem_dump};

struct statespace_type type_state_tree_compress = {
    state_cmp_with_tree, state_calc_mem_dump_with_tree};

struct statespace_type type_state_default = {state_cmp, state_calc_mem_dummy};

/* Table of prime numbers 2^n+a, 2<=n<=30. */
static unsigned long primes[] = {8192 + 27,       131072 + 29,
                                 1048576 + 7,     4194304 + 15,
                                 16777216 + 43,   67108864 + 15,
                                 268435456 + 3,   536870912 + 11,
                                 1073741824 + 85, 0};

static inline unsigned long table_new_size(unsigned long old_size) {
  unsigned long i, n;

  n = (unsigned long)(sizeof(primes) / sizeof(primes[0]));
  for (i = 0; i < n; i++) {
    if (primes[i] > old_size) {
      return primes[i];
    }
  }
  lmn_fatal("Size of StateSpace exceeded the limiting value");
  return 0; /* exception */
}

/* テーブルのサイズを1段階拡張する */
void StateTable::resize(unsigned long old_cap) {
#ifdef PROFILE
  if (lmn_env.profile_level >= 3) {
    profile_countup(PROFILE_COUNT__HASH_RESIZE_TRIAL);
  }
#endif
  if (this->cap() == old_cap) {
    slim::element::ewmutex mutex(slim::element::ewmutex::exclusive_enter,
                                 this->lock, env_my_thread_id());
    std::lock_guard<slim::element::ewmutex> lk(mutex);
    if (this->cap() == old_cap) {
      unsigned long i, new_cap, bucket;
      State *ptr, *next, **new_tbl;

      new_cap = table_new_size(old_cap);
      new_tbl = LMN_NALLOC(State *, new_cap);

      for (i = 0; i < new_cap; i++) {
        new_tbl[i] = NULL;
      }

      for (i = 0; i < old_cap; i++) {
        ptr = this->tbl[i];
        while (ptr) {
          next = ptr->next;
          bucket = state_hash(ptr) % new_cap;
          ptr->next = new_tbl[bucket];
          if (ptr->is_dummy() && ptr->is_expanded() && !ptr->is_encoded()) {
            /* オリジナルテーブルでdummy_stateが存在する状態にはバイト列は不要
             * (resize中に,
             * 展開済み状態へのデータアクセスは発生しないよう設計している) */
            ptr->free_binstr();
            if (ptr->s_is_d()) {
              ptr->s_unset_d();
            }
          }
          new_tbl[bucket] = ptr;
          ptr = next;
        }
      }

      LMN_FREE(this->tbl);
      this->tbl = new_tbl;
      this->cap_ = new_cap;
      this->cap_density_ = new_cap / this->thread_num;

#ifdef PROFILE
      if (lmn_env.profile_level >= 3) {
        profile_countup(PROFILE_COUNT__HASH_RESIZE_APPLY);
      }
#endif
    }
  }
}

void StateTable::set_lock(EWLock *lock) { this->lock = lock; }

bool StateTable::use_rehasher() const { return use_rehasher_; }

unsigned long StateTable::num_by_me() const {
  return this->num[env_my_thread_id()];
}

unsigned long StateTable::all_num() const {
  unsigned long ret = 0;
  unsigned int i;
  for (i = 0; i < this->thread_num; i++) {
    ret += this->num[i];
  }
  return ret;
}

void StateTable::num_increment() { num[env_my_thread_id()]++; }

void StateTable::num_dummy_increment() { num_dummy_[env_my_thread_id()]++; }

void StateTable::set_rehash_table(StateTable *rehash_tbl) {
  this->rehash_tbl_ = rehash_tbl;
  this->set_rehasher();
}

StateTable *StateTable::rehash_table() { return this->rehash_tbl_; }

unsigned long StateTable::space() const {
  return sizeof(struct StateTable) +
         (num ? thread_num * sizeof(unsigned long) : 0) +
         (num_dummy_ ? thread_num * sizeof(unsigned long) : 0) +
         (cap_ * sizeof(State *)) + lmn_ewlock_space(lock);
}

/* CAUTION: MT-Unsafe */
void StateTable::format_states() {
  qsort(this->tbl, this->cap(), sizeof(struct State *),
        statetable_cmp_state_id_gr_f);
  for (auto &ptr : *this)
    statetable_issue_state_id_f(ptr);
}

void StateTable::memid_rehash(unsigned long hash) {
  for (int i = 0; i < this->cap(); i++) {
    auto ptr = this->tbl[i];

    while (ptr) {
      State *next = ptr->next;
      if (state_hash(ptr) == hash) { /* statespace_mem_encode_f */
        this->memid_rehash(ptr);
      }
      ptr = next;
    }
  }
}

StateTable::StateTable(int thread_num, unsigned long size) {
  unsigned long i;
  this->thread_num = thread_num;

  if (lmn_env.enable_compress_mem) {
    if (lmn_env.z_compress) {
      this->type = &type_state_compress_z;
    } else if (lmn_env.tree_compress) {
      this->type = &type_state_tree_compress;
    } else {
      this->type = &type_state_compress;
    }
  } else {
    this->type = &type_state_default;
  }

  this->use_rehasher_ = FALSE;
  size = table_new_size(size);
  this->tbl = LMN_NALLOC(State *, size);
  this->cap_ = size;
  this->cap_density_ = size / thread_num;
  this->num = LMN_NALLOC(unsigned long, thread_num);
  this->num_dummy_ = LMN_NALLOC(unsigned long, thread_num);
  this->lock = (this->thread_num > 1)
                   ? ewlock_make(this->thread_num, DEFAULT_WLOCK_NUM)
                   : nullptr;
  this->rehash_tbl_ = NULL;

  memset(this->tbl, 0x00, size * (sizeof(State *)));

  for (i = 0; i < thread_num; i++) {
    this->num[i] = 0UL;
    this->num_dummy_[i] = 0UL;
  }
}

StateTable::~StateTable() {
  for (auto &ptr : *this)
    delete ptr;

  if (this->lock) {
    ewlock_free(this->lock);
  }
  delete (this->num_dummy_);
  delete (this->num);
  delete (this->tbl);
}

/* statetable_insert: 状態sが状態空間stに既出ならばその状態を,
 * 新規ならばs自身を返す.
 *
 * 以下, ハッシュ値が等しい状態を検出した場合の振舞いに関するメモ
 * - 膜のハッシュ関数(mhash)から膜のIDのハッシュ関数(memid_hash)へのrehash:
 * -- (用語) 膜のハッシュ関数によるテーブル   : オリジナルテーブル
 * -- (用語) 膜IDのハッシュ関数によるテーブル : memidテーブル
 *
 * - 状態をRehashするケース
 * -- その1
 *  サクセッサ状態の展開時, 展開元と展開先とでハッシュ値が衝突する場合
 *  展開先状態のメモリはまだハッシュ表に登録していないため,
 * 呼出しスレッドからの操作は全てMT-safe
 * -- その2
 *  既にハッシュ表(オリジナルテーブル)に登録してある状態をrehashできる条件は,
 *  >> 状態空間に対して全サクセッサの登録が完了した状態 << に対する場合(即ち,
 * 展開済み状態)のみ.
 *
 * - 前提/制約条件
 * -- テーブル単位で各状態は必ずユニークにする(即ち,
 * 状態展開時に選択する状態の重複は避ける)
 * --- 等価な状態が複数個, テーブル内に存在することは避ける.
 *      (理由1. 状態数不一致原因を特定するのが困難になる
 *       理由2. モデル検査器としての信頼性が曖昧になる
 *       理由3. 冗長な計算が発生する.)
 * -- 即ち,
 * オリジナルテーブルとmemidテーブルとの間で等価な状態が重複して存在することも避ける
 * -- バイト列のrehashにより, 許容できないメモリ使用量の増加を招いてはならない
 *   (Rehashによる単なる高速化ではなく, 時間と空間のトレードオフを考慮する)
 *
 * - 制約を満たすために
 * -- 1. rehash前後のstruct Stateオブジェクトにdummyフラグを立て,
 * ---
 * memidテーブル側のdummy状態(のメンバparent)からオリジナルテーブル側のdummy状態を刺す.
 * --- memidテーブル側のdummy状態は,
 * rehashしたバイト列データ(とそのハッシュ値)のみを保持する.
 *     (遷移先状態へのポインタや, 状態ラベルなどの情報を保持することはしない.)
 *
 *         [ original A ] <-----ptr----- [memid A]
 *         |- is_dummy:TRUE              |- is_dummy:TRUE
 *         |- is_memid:FALSE             |- is_memid:TRUE
 *         |- binstr                     |- mid_binstr
 *         |- mhash                      |- mid_hash (FNV)
 *         |- label                      |- ptr
 *         |- succ ptrs
 *         |- LTS parent
 *         |- et al..
 *
 *
 * -- 2. オリジナルテーブル側のdummy状態から,
 * MT-Safeなタイミングでrehash前のバイト列を破棄.
 * --- (Rehash前の状態は,
 * 任意のタイミングで異なるスレッドから探索によるメモリアクセスを受ける)
 * --- リスト走査中にdummy状態を発見した場合, 新たな処理を追加する.
 *
 * - statespace_insertに対する追加処理発生のタイミング
 * -- 状態(hash値A)を探索した際, dummy状態(hash値A)とハッシュ値が衝突した場合.
 *
 * - 追加処理の概要
 * -- 1. Search元の状態(ハッシュ値A)の一意なバイト列を計算
 * -- 2. 一意なバイト列への計算が完了したならば,
 *       そのバイト列とバイト列のハッシュ値(FNV)から,
 * memidテーブルをSearchを行う.
 *
 * - スレッドセーフなバイト列のRehashとRehash元バイト列の破棄
 * --
 * テーブルに登録済みの状態に対して任意のタイミングでメモリアクセスが発生する.
 *    任意のタイミングのメモリアクセスとは,
 *       A. 遷移先計算 (未展開状態のみ)
 *       B. 状態の等価性判定 (statespace_insert)
 * -- Rehashした状態(予めdummyフラグを立てておく)を,
 * まずmemidテーブルに登録してしまう.
 * -- memidテーブルへの登録が完了したならば,
 * Rehash元の状態にもdummyフラグを立てておく.
 * --
 * dummyフラグの立っているオリジナルテーブルの状態(バイト列)に対するメモリアクセスがなくなる.
 *    ただし, dummyフラグを立てている最中も,
 * バイト列へのメモリアクセスが発生し得る.
 * -- そこで, 全スレッドが同期を取るStateSpaceサイズ拡張処理の際に,
 * 冗長なバイト列を破棄する.
 */
State *StateTable::insert(State *ins, unsigned long *col) {
  State *ret;
  LmnBinStrRef compress;

  if (ins->is_binstr_user()) {
    /* 既に状態insがバイナリストリングを保持している場合 */
    compress = ins->state_binstr();
  } else {
    compress = NULL;
  }

  ret = NULL;
  slim::element::ewmutex outer_mutex(slim::element::ewmutex::enter, this->lock,
                                     env_my_thread_id());
  std::lock_guard<slim::element::ewmutex> lk(outer_mutex);
  State *str;
  unsigned long bucket, hash;

  hash = state_hash(ins);
  bucket = hash % this->cap();
  str = this->tbl[bucket];

  /* case: empty bucket */
  if (!str) {
    /* strがNULL --> 即ち未使用バケットの場合 */
    compress = this->compress_state(ins, compress);
    if (!this->tbl[bucket]) {
      slim::element::ewmutex mutex(slim::element::ewmutex::write, this->lock,
                                   bucket);
      std::lock_guard<slim::element::ewmutex> lk(mutex);
      if (!this->tbl[bucket]) {
        state_set_compress_for_table(ins, compress);
        this->num_increment();
        if (tcd_get_byte_length(&ins->tcd) == 0 && lmn_env.tree_compress) {
          TreeNodeID ref;
          tcd_set_byte_length(&ins->tcd, ins->state_binstr()->len);
          ref = lmn_bscomp_tree_encode(ins->state_binstr());
          tcd_set_root_ref(&ins->tcd, ref);
        }
        this->tbl[bucket] = ins;
        ret = ins;
      }
    }

    if (!ret) {
      /* 他のスレッドが先にバケットを埋めた場合, retがNULL
       * insert先strを再設定し, case: non-empty backetへ飛ぶ */
      str = this->tbl[bucket];
#ifdef PROFILE
      if (lmn_env.profile_level >= 3) {
        profile_countup(PROFILE_COUNT__HASH_FAIL_TO_INSERT);
      }
#endif
    }
  }

  /* case: non-empty bucket */
  while (!ret) {
#ifdef PROFILE
    if (lmn_env.profile_level >= 3) {
      profile_countup(PROFILE_COUNT__HASH_CONFLICT_ENTRY);
    }
#endif

    if (hash == state_hash(str)) {
      /* >>>>>>> ハッシュ値が等しい状態に対する処理ここから <<<<<<<<　*/
      if (lmn_env.hash_compaction) {
        if (str->is_dummy() && str->is_encoded()) {
          /* rehashテーブル側に登録されたデータ(オリジナル側のデータ:parentを返す)
           */
          ret = state_get_parent(str);
        } else {
          ret = str;
        }
        break;
      }

      if (this->use_rehasher() && str->is_dummy() && !str->is_encoded() &&
          lmn_env.tree_compress == FALSE) {
        /* A. オリジナルテーブルにおいて, dummy状態が比較対象
         * 　 --> memidテーブル側の探索へ切り替える.
         *    (オリジナルテーブルのdummy状態のバイト列は任意のタイミングで破棄されるため,
         *     直接dummy状態上のメモリを比較対象とするとスレッドセーフでなくなる)
         */

        if (ins->is_binstr_user()) {
          ins->free_binstr();
        } else if (compress) {
          lmn_binstr_free(compress);
        }
        ins->s_unset_d();
        ins->calc_mem_encode();
        /*compress = NULL;*/

#ifndef PROFILE
        ret = this->rehash_table()->insert(ins);
#else
        ret = this->rehash_table()->insert(ins, col);
#endif
        break;
      } else if (!STATE_EQUAL(this, ins, str)) {
        /** B. memidテーブルへのlookupの場合,
         *     もしくはオリジナルテーブルへのlookupで非dummy状態と等価な場合
         */
        if (str->is_dummy() && str->is_encoded()) {
          /* rehashテーブル側に登録されたデータ(オリジナル側のデータ:parentを返す)
           */
          ret = state_get_parent(str);
        } else {
          ret = str;
        }
        LMN_ASSERT(ret);
        break;
      } else if (str->is_encoded()) {
        /** C. memidテーブルへのlookupでハッシュ値が衝突した場合.
         * (同形成判定結果が偽) */
#ifdef PROFILE
        if (lmn_env.profile_level >= 3) {
          profile_countup(PROFILE_COUNT__HASH_CONFLICT_HASHV);
        }
#endif
      } else {
        /** D.
         * オリジナルテーブルへのlookupで非dummy状態とハッシュ値が衝突した場合.
         */
        LMN_ASSERT(!str->is_encoded());
#ifdef PROFILE
        (*col)++;
        if (lmn_env.profile_level >= 3) {
          profile_countup(PROFILE_COUNT__HASH_CONFLICT_HASHV);
        }
#endif
        if (this->use_rehasher() && lmn_env.tree_compress == FALSE) {
          if (state_get_parent(ins) == str) {
            /* 1step遷移した状態insの親状態とでハッシュ値が衝突している場合:
             *  + 状態insだけでなくその親状態もrehashする.
             *  + ただし, encodeした親状態をmemidテーブルへ突っ込んだ後,
             *    親状態の従来のバイト列の破棄は実施しない.
             *    これは,
             * 他のスレッドによる状態比較処理が本スレッドの処理と競合した際に,
             *    本スレッドが立てるdummyフラグを他のスレッドが見逃す恐れがあるため
             */
            this->memid_rehash(str);
            str->set_dummy();
          }

          /* 比較元をencode */
          if (ins->is_binstr_user()) {
            ins->free_binstr();
          } else if (compress) {
            lmn_binstr_free(compress);
          }
          ins->s_unset_d();
          ins->calc_mem_encode();

#ifndef PROFILE
          ret = this->rehash_table()->insert(ins);
#else
          ret = this->rehash_table()->insert(ins, col);
#endif

          LMN_ASSERT(ret);
          break;
        }
      }
      /* >>>>>>> ハッシュ値が等しい状態に対する処理ここまで <<<<<<<< */
    }

    /** エントリリストへの状態追加操作:
     * 逐次
     *   1. 各エントリ(状態)に対して, ハッシュ値と状態の比較を行う.
     *   2. 等価な状態を発見した場合, その状態へのアドレスを返す.
     *   3. リスト末尾へ到達した場合, バイト列が未計算ならば計算する.
     *   4. 計算したバイト列を状態に設定した後にリスト末尾に状態を追加する,
     *      (追加した状態のアドレスを返す.)
     * 並列処理への拡張
     *   4. (逐次版の手順4は実施しない)
     *      ロックを確保後,
     * 末尾エントリのnextアドレス(追加予定位置)がNULLであるか否かを検査
     *   5. NULLでない(他のスレッドが追加した)ならば, ロックを開放し,
     *      処理1(リスト走査)へcontinue.
     *   6. NULLならば,
     * 計算したバイト列を状態に設定してからリスト末尾に状態を追加する.
     *      (追加した状態のアドレスを返す.)
     */
    if (!str->next) { /* リスト末尾の場合 */
      compress = this->compress_state(ins, compress);
      slim::element::ewmutex mutex(slim::element::ewmutex::write, this->lock,
                                   bucket);
      if (!str->next) {
        std::lock_guard<slim::element::ewmutex> lk(mutex);
        if (!str->next) {
          this->num_increment();
          state_set_compress_for_table(ins, compress);
          if (tcd_get_byte_length(&ins->tcd) == 0 && lmn_env.tree_compress) {
            TreeNodeID ref;
            tcd_set_byte_length(&ins->tcd, ins->state_binstr()->len);
            ref = lmn_bscomp_tree_encode(ins->state_binstr());
            tcd_set_root_ref(&ins->tcd, ref);
          }
          str->next = ins;
          ret = ins;
        }
      }

#ifdef PROFILE
      if (!ret && lmn_env.profile_level >= 3) {
        profile_countup(PROFILE_COUNT__HASH_FAIL_TO_INSERT);
      }
#endif
    }

    str = str->next; /* リストを辿る */
  }

  if (ret != ins) {
    /* 別のスレッドの割込みで追加に失敗した場合, 計算したバイト列を破棄する.*/
    if (ins->is_binstr_user()) {
      ins->free_binstr();
    } else if (compress) {
      // lmn_binstr_free(compress);
    }
  }

  return ret;
}

/* 重複検査なしに状態sを状態表stに登録する */
void StateTable::add_direct(State *s) {
  slim::element::ewmutex outer_mutex(slim::element::ewmutex::enter, this->lock,
                                     env_my_thread_id());
  std::lock_guard<slim::element::ewmutex> lk(outer_mutex);

  LmnBinStrRef compress;
  State *ptr;
  unsigned long bucket;
  BOOL inserted;

  if (s->is_binstr_user()) {
    compress = s->state_binstr();
  } else {
    compress = NULL;
  }

  bucket = state_hash(s) % this->cap();
  ptr = this->tbl[bucket];
  inserted = FALSE;

  if (!ptr) { /* empty */
    compress = this->compress_state(s, compress);
    if (!this->tbl[bucket]) {
      slim::element::ewmutex mutex(slim::element::ewmutex::write, this->lock,
                                   bucket);
      std::lock_guard<slim::element::ewmutex> lk(mutex);
      if (!this->tbl[bucket]) {
        state_set_compress_for_table(s, compress);
        this->num_increment();
        if (tcd_get_byte_length(&s->tcd) == 0 && lmn_env.tree_compress) {
          TreeNodeID ref;
          tcd_set_byte_length(&s->tcd, s->state_binstr()->len);
          ref = lmn_bscomp_tree_encode(s->state_binstr());
          tcd_set_root_ref(&s->tcd, ref);
        }
        this->tbl[bucket] = s;
        inserted = TRUE;
      }
    }
    if (!inserted)
      ptr = this->tbl[bucket];
  }

  while (!inserted) {
    if (!ptr->next) { /* リスト末尾の場合 */
      compress = this->compress_state(s, compress);
      if (!ptr->next) {
        slim::element::ewmutex mutex(slim::element::ewmutex::write, this->lock,
                                     bucket);
        std::lock_guard<slim::element::ewmutex> lk(mutex);
        if (!ptr->next) {
          inserted = TRUE;
          this->num_increment();
          state_set_compress_for_table(s, compress);
          ptr->next = s;
        }
      }
    }
    ptr = ptr->next;
  }
}


/* **既に状態管理表stに追加済みの状態s**(およびsに対応する階層グラフ構造)と
 * 等価な状態を新たに生成し,
 * 生成した状態をテーブルstのrehash先テーブルへ追加する.
 * ここで新たに生成する状態は,
 * sに対応する階層グラフ構造対して一意なバイナリストリングを持つ. */
void StateTable::memid_rehash(State *s) {
  State *new_s;
  LmnMembraneRef m;

  new_s = new State();
  m = lmn_binstr_decode(s->state_binstr());
  new_s->state_name = s->state_name;
  new_s->state_set_binstr(lmn_mem_encode(m));
  new_s->hash = binstr_hash(new_s->state_binstr());

  new_s->set_encoded();
  new_s->set_expanded();
  new_s->set_dummy();

  this->rehash_table()->add_direct(
      new_s); /* dummy stateのparentをoriginalとして扱う */
  state_set_parent(new_s, s);
  this->rehash_table()->num_dummy_increment();

  lmn_mem_free_rec(m);
}
