# 
#  translate_generator.rb - 
# 
#    Copyright (c) 2008, Ueda Laboratory LMNtal Group <lmntal@ueda.info.waseda.ac.jp>
#    All rights reserved.
# 
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the following conditions are
#    met:
# 
#     1. Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
# 
#     2. Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
# 
#     3. Neither the name of the Ueda Laboratory LMNtal Group nor the
#        names of its contributors may be used to endorse or promote
#        products derived from this software without specific prior
#        written permission.
# 
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
#  $Id: translate_generator.rb,v 1.5 2008/09/19 05:18:17 riki Exp $


# �킴�킴���ɂ���C���^�v���^�������̂��|���̂�,���ʂ̓g�����X���[�^�����ɂ̂ݎg�p����
# ���̏ꍇ, translate_generated.c���o�͂�, translate_instruction_generated�֐����`����

# mode
#  �s����#__echo�̂悤�Ɏw�肷���,���̃��[�h�w��܂ł͂��̏o�͕����ɂȂ�
#  0: __ignore ��������, �ŏ��Ƀ��[�h���w�肳���܂ł�__ignore�ɂȂ��Ă���
#  1: __echo ������Ă��邱�Ƃ����̂܂܃g�����X���[�^/�C���^�v���^�̃R�[�h�ɏo��
#  2: __echo_i �C���^�v���^�������͂��̂܂܏o��,�g�����X���[�^�������͖�������
#  3: __echo_t �g�����X���[�^�������́`
#  4: __format �C���^�v���^�������͏�����Ă��邱�Ƃ��C���^�v���^�̃R�[�h�ɏo�͂���, $$1�Ȃǂ̃}�N��������lmn���s���ɒu����������
#              �g�����X���[�^�������͏�����Ă��邱�Ƃ��g�����X���[�g���ʂ�C�R�[�h�ɏo�͂���, �}�N���̒u���̓g�����X���[�g���s���ɍs����
#  5: __format_i �C���^�v���^�������́`
#  6: __format_t �g�����X���[�^�������́`

# case
#  �s����#hoge�̂悤�ɏ�L�̃��[�h�ɂȂ����̂��w�肷���,���Ԗ���INSTR_HOGE(���ׂđ啶���ɒu�������)�ɑ΂���R�[�h�������s��
#  #hoge X Y Z�̂悤�Ɉ������w�肷��
#  case�J�n���玟��case�܂ł����̒��Ԗ��߂����s���邽�߂̃R�[�h�ł���

# argument
#  �������w�肳�ꂽ�ꍇ,���̈������i�[����ϐ��Ɠǂݍ��ނ��߂̃R�[�h�����������
#  �����ꂽ�ʂ�̌^�ŕϐ��錾���s���� #hoge fuga �Ə�����Ă����, fuga�Ƃ����^�����ϐ����p�ӂ���,fuga�T�C�Y���������Ԗ��ߗ񂩂�Ǎ��݂��s��
#  �����̓X�y�[�X�ōs��
#  __format���[�h�ł�,$0,$1,$2�̌`�ŕϐ��ɃA�N�Z�X�ł���(�ȒP�̂���$9�܂�)
#  �C���^�v���^���s��,�g�����X���[�^�ϊ����ɂ�����, �ϐ���targ0,targ1,...�̖��O�Ŋm�ۂ���邽��,__echo����͂��̂悤�ɃA�N�Z�X�ł���
#  �g�����X���[�^�ϊ����ʂł�,$0,$1�̕����ɑ��l�����ߍ��܂��

# macro
#  ��̈����A�N�Z�X�ȊO��,__format���[�h�Ŏg�p�ł���}�N��������
#  $s successcode.���������Ƃ����s���ׂ��R�[�h.proceed�ł����g��Ȃ�����
#  $f failcode.���s�����Ƃ����s���ׂ��R�[�h.���̃}�b�`���O�ɍs������,���[�����̂����s�����肷��
#( $r recursive.�ċA�Ăяo��? �܂��l���Ă��Ȃ� )
#  �܂�,�}�N���ł͂Ȃ��P��$���R�[�h�Ɏg���ꍇ��$$�Ə��� ab$1cd��"abtarg1cd"�ƂȂ�, ab$$1cd�� "ab$1cd"�ƂȂ�

# list argument
#  [1,2,3,4]�̂悤�ȉϒ��������X�g�������Ƃ��閽�߂���������,�����ǂݍ��ޏꍇ
#  #hogehoge $list�Ə������ƂœK���ȓǍ��݂��s��("$list"�S�̂ŗ\��)
#  $0[0]�̂悤�ɗv�f�ɃA�N�Z�X���� $0_num�ŗv�f���ɃA�N�Z�X�ł��� (�����̓���͒ʏ�̕ϐ��Ƃ͈قȂ�)
#  ���̏ꍇ�C���^�v���^�ɑ΂��Ă�,  int arg0_num=�ǂݍ���;int *arg0=malloc(arg0_num);for(...(�S���ǂݍ���))  �𐶐�����.
#  �g�����X���[�^�ɑ΂��Ă�,  int arg0_num=�ǂݍ���;int *arg0=malloc(arg0_num);for(...(�S���ǂݍ���))  �𐶐���,
#  �X�Ƀg�����X���[�^�o�͌��ʂ�  int arg0_num=�萔;int arg0[] = {...};  ���o�͂����悤�ȃR�[�h�������s��.
#  ��O�I�Ƀg�����X���[�g�o�͌��ʂɂ����Ă�arg0�̖��O���c���Ă���̂Œ���

# global id
#  �g�����X���[�g���ʂƂ���so�ɃR���p�C�������ꍇ,���[�J����id����O���[�o����id�ւ̕ϊ����K�v�ɂȂ�
#  �g�����X���[�^�̏o�̓R�[�h�ɂ����Ă݈̂Ӗ�������,����ȊO�ł͉������Ȃ��}�N����p�ӂ���
#  TR_GSID() trans global symbol id
#  TR_GFID() trans global functor id
#  TR_GRID() trans global ruleset id

# �����̂�����ꍇ��,translate.c��translate_instruction�֐����ɒ��ڏ���

# true:  generate translator
# false: generate interpreter
$translateor_generate = true

def case_open(op, arg)
  # case���擪�̃R�[�h����
  # �ϐ��̐錾�ƒ��Ԗ��߂���̓ǂݍ���
  # load.c: load_arg�ɑ������鏈���œǂݍ���
  print "case INSTR_", op.upcase, ":{\n"
  for i in 0..arg.size-1
    if arg[i] != "$list"
      print "  ", arg[i], " targ", i, ";\n"
    else
      print "  int *targ", i, ";\n"
      print "  int targ", i, "_num;\n"
    end
  end
  for i in 0..arg.size-1
    if arg[i] != "$list"
      print "  READ_VAL(", arg[i], ", instr, targ", i, ");\n"
    else
      print "  READ_VAL(LmnJumpOffset, instr, targ", i, "_num);\n"
      print "  targ", i, " = malloc(sizeof(int)*targ", i, "_num);\n"
      print "  { int i; for(i=0; i<targ", i, "_num; ++i){ READ_VAL(int, instr, targ", i, "[i]); } }\n"
    end
  end
end

def format_print(line)
  # \ -> \\
  # " -> \"
  # % -> %%
  # $$ : $
  # $X : X�Ԃ̈���(X�͈ꕶ���Ƃ������Ƃɂ��Ă���)
  line.gsub!("\\", "\\\\")
  line.gsub!("\"", "\\\"")
  line.gsub!("%", "%%")
  format_arg = []
  pos = 0
  while((pos=line.index("$", pos+1)) != nil)
    if line[pos+1] == "$"[0]
      line[pos,1] = ""
    elsif line[pos+1]>="0"[0] && line[pos+1]<="9"[0]
      format_arg << line[pos+1] - "0"[0]
      line[pos,2] = "%d"
    end
  end
  print "\tindent();\n"
  print "\tprintf(\"", line, "\\n\""
  format_arg.each{|x| print ", arg", x}
  print ");\n"
end

# -i�I�v�V���������Ă�����C���^�v���^����, ���Ă��Ȃ���΃g�����X���[�^����
if ARGV == 1 && ARG[0] == "-i" then $translator_generate = false end
first_of_case = true
mode = 0
argument = []
line = ""
while(line=gets())
  line.chop!
  if line == ""
    # skip 
  elsif line == "#__end"
    if not first_of_case
      first_of_case = true
      print "}\n"
    end
    mode = 1
  elsif line[0] == "#"[0]
    t = line[1,line.size-1].split(" ")
    a = ["__ignore", "__echo", "__echo_i", "__echo_t", "__format", "__format_i", "__format_t"].index(t[0])
    if a == nil
      if first_of_case
        first_of_case = false
      else
        print "}\n"
      end
      case_open(t[0], t[1,t.size-1])
      mode = 4
    else
      mode = a
    end
  else
    case mode
    when 0 #ignore
    when 1 #echo
      print line, "\n"
    when 2 #echo_i
    when 3 #echo_t
    when 4 #format
      format_print(line)
    when 5 #format_i
    when 6 #format_t
    end
  end
end

if not first_of_case
  print "}\n"
end
