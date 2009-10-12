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
#  ���L�Ƃ���,���[�h��__format_*�̎�,�s�̐擪��%�ł���s��, __echo_*�ɂȂ�. (__format�̎���__echo, __format_i�̎���__echo_i)

# case
#  �s����#hoge�̂悤�ɏ�L�̃��[�h�ɂȂ����̂��w�肷���,���Ԗ���INSTR_HOGE(���ׂđ啶���ɒu�������)�ɑ΂���R�[�h�������s��
#  #hoge X Y Z�̂悤�Ɉ������w�肷��
#  case���J����__format�ɂȂ�
#  case�J�n���玟��case�܂ł����̒��Ԗ��߂����s���邽�߂̃R�[�h�ł���
#  #__end�ɂ���Ď���case������case����邱�Ƃ��ł��� ���Ƀe���v���[�g�̍Ō�ɂ͕K������ __end���__echo���[�h�ɂȂ�

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

# functor argument
#  �t�@���N�^�����ʈ��� �f�[�^���ŏ���union�ɓǂݍ��� ����͏o�̓t�@�C���ɂ͕K�v�Ȃ��\��

# global id
#  �g�����X���[�g���ʂƂ���so�ɃR���p�C�������ꍇ,���[�J����id����O���[�o����id�ւ̕ϊ����K�v�ɂȂ�
#  �g�����X���[�^�̏o�̓R�[�h�ɂ����Ă݈̂Ӗ�������,����ȊO�ł͉������Ȃ��}�N����p�ӂ���
#  TR_GSID() trans global symbol id
#  TR_GFID() trans global functor id
#  TR_GRID() trans global ruleset id

# �����̂�����ꍇ��,translate.c��translate_instruction�֐����ɒ��ڏ���

# true:  generate translator
# false: generate interpreter
$translator_generate = true

def case_open(op, arg)
  # case���擪�̃R�[�h����
  # �ϐ��̐錾�ƒ��Ԗ��߂���̓ǂݍ���
  print "case INSTR_", op.upcase, ":{\n"
  for i in 0..arg.size-1
    if arg[i] == "$list"
      #print "  LmnInstrVar *targ", i, ";\n"
      #print "  int targ", i, "_num;\n"
    elsif arg[i] == "$functor"
      print "  LmnLinkAttr targ", i, "_attr;\n"
      print "  union LmnFunctorLiteral targ", i, ";\n"
    else
      print "  ", arg[i], " targ", i, ";\n"
    end
  end

  for i in 0..arg.size-1
    if arg[i] == "$list"
      warn "$list argument not implemented.\n"
      # ����̓}�N���ɒǂ��o��������������������Ȃ�
      #print "  READ_VAL(LmnInstrVar, instr, targ", i, "_num);\n"
      #print "  targ", i, " = malloc(sizeof(LmnInstrVar)*targ", i, "_num);\n"
      #print "  { int i; for(i=0; i<targ", i, "_num; ++i){ READ_VAL(LmnInstrVar, instr, targ", i, "[i]); } }\n"
      
      #if $translator_generate
        # �g�����X���[�^�̏ꍇ�͏o�͂�int targ1_num=5; int targ1[]={1,2,3,4,5}; ���܂߂�K�v������
        # ���̏o�͂͊֐��̓r���ł��o�Ă���+���O�����Ԃ�̂�{}�ň͂��K�v������ �K�����̒��Ԗ��߂ł��̃f�[�^���K�v�ɂȂ邱�Ƃ͂Ȃ��͂�
        # ���̏����������ł��邩�ʂ̂Ƃ���ł��邩�͍l����
        # �����Ńu���b�N���J���o�͂�����ƕ���o�͂��K�v���ۂ����o���Ă����K�v������
      #end
    elsif arg[i] == "$functor"
      # �������}�N���ɒǂ��o��������������������Ȃ�
      #print "  READ_VAL(LmnLinkAttr, instr, targ", i, "_attr);\n"
      #print "  switch(targ", i, "_attr){\n"
      #print "    case LMN_INT_ATTR: READ_VAL(long, instr, targ", i, ".long_data); break;\n"
      #print "    case LMN_DBL_ATTR: READ_VAL(double, instr, targ", i, ".double_data); break;\n"
      #print "    case LMN_STRING_ATTR: READ_VAL(lmn_interned_str, instr, targ", i, ".string_data); break;\n"
      #print "    default: READ_VAL(LmnFunctor, instr, targ", i, ".functor_data); break;\n"
      #print "  }\n"
      print "  READ_VAL_FUNC(instr, targ", i, ");\n"

      # �Ƃ肠�����̓g�����X���[�g��͑S�����ߍ��ޕ��j
      #if $translator_generate
        # �g�����X���[�^�̏ꍇ�͏o�͂�int targ1_attr=5; union LmnFunctorLiteral targ1; targ1.functor_data=8;������H
        #print "printf(\"  int targ", i, "_attr="
      #end
    else
      print "  READ_VAL(", arg[i], ", instr, targ", i, ");\n"
    end
  end

  if $translator_generate
    # �g�����X���[�g���ʂɂ͕ϐ����܂ޏꍇ�����邽��, ���񊇌ʂ��J���Ă���
    # �ϐ����܂ޏꍇ����($list�̏ꍇ����)�J���悤�ɂ���
    #print "  print_indent(indent); printf(\"{\\n\");\n"
  end
end

def case_close(arg)
  # ���X�g��ǂݍ���ł�����J������K�v������
  for i in 0..arg.size-1
    if arg[i] == "$list"
      print "  free(targ", i, ");\n"
    end
  end
  
  if $translator_generate
    # �g�����X���[�g���͏o�͓��̃J�b�R����Ď��̓ǂݍ��݈ʒu�����^�[��
    #print "  print_indent(indent); printf(\"}\\n\");\n"
    # $list�������ɂ���ꍇ��������
    print "  return instr;\n"
  else
    # �C���^�v���^�̓f�t�H���g�ł�switch�𔲂��邾��
    print "  break;\n"
  end

  print "}\n"
end

# # �g�����X���[�^�������ɏ�������printf�ŏo�͂��邽�߂̃v���O�������o�͂���
# # ��̈����̂Ƃ���ŏ������悤�Ɉꕔ�̎�̕ϐ��͒u�����s��Ȃ�����, n�Ԉ��������ꂩ�ۂ���m�邽��arg�������Ɏ��

def print_interp_format(line, arg)
  print "IF:: ", line, "\n"
end

def print_trans_format(line, arg)
  # \ -> \\ �����񃊃e�����ɂ�����\��\���������\\�Ə���
  # " -> \" �����񃊃e�����ɂ�����"��\���������\"�Ə���
  # % -> %% printf�̕�����ɂ�����%��\���������%%�Ə���
  # $$ : $  ���̃c�[�����͂ɂ�����$$�͂�����$���Ӗ���,$1��1�Ԗڂ̈�����\��
  # $X : X�Ԃ̈���(X�͈ꕶ���Ƃ������Ƃɂ��Ă���)
  
  # ruby�̎����͂�\\��\�ɂȂ�,gsub�̃t�H�[�}�b�g��͂ōX��\\��\�ɂȂ�
  line.gsub!("\\", "\\\\\\\\")
  line.gsub!("\"", "\\\"")
  line.gsub!("%", "%%")
  format_arg = []
  pos = 0
  while((pos=line.index("$", pos+1)) != nil)
    if line[pos+1] == "$"[0]
      # "$$"�Ȃ�1������"$"�ɂ��Ă��
      line[pos,1] = ""
    elsif line[pos+1]>="0"[0] && line[pos+1]<="9"[0]
      x = line[pos+1] - "0"[0]
      if arg[x] == "$list"
        # ���X�g�����Ȃ�, �ϊ���o�͂Ɋ܂܂��targ1�����̂܂܎Q��
        line[pos,2] = "targ" + x
      elsif arg[x] == "$functor"
        # �t�@���N�^�̏ꍇ, format�̒��� $1_long_data ������ꍇ, ����͑��l_long_data���Ӗ�����̂ł͂Ȃ�,
        # $1_long_data�S�̂�1�̑��l�ɂȂ��ė~���� �Ƃ������Ƃœ��ʂȈ������K�v
        long_data_name = "_long_data"
        double_data_name = "_double_data"
        string_data_name = "_string_data"
        functor_data_name = "_functor_data"
        attr_name = "_attr"
        if line[pos+2,long_data_name.size] == long_data_name
          format_arg << "targ"+x.to_s+".long_data"
          line[pos,long_data_name.size+2] = "%ld"
        elsif line[pos+2,double_data_name.size] == double_data_name
          format_arg << "targ"+x.to_s+".double_data"
          line[pos,double_data_name.size+2] = "%lf"
        elsif line[pos+2,string_data_name.size] == string_data_name
          format_arg << "targ"+x.to_s+".string_data"
          line[pos,string_data_name.size+2] = "%d"
        elsif line[pos+2,functor_data_name.size] == functor_data_name
          format_arg << "targ"+x.to_s+".functor_data"
          line[pos,functor_data_name.size+2] = "%d"
        elsif line[pos+2,attr_name.size] == attr_name
          format_arg << "targ"+x.to_s+"_attr"
          line[pos,attr_name.size+2] = "%d"
        else
          warn "unexpected functor type.\n"
          warn line[pos+2,long_data_name.size]
        end
      else
        # ���ʂ̈����Ȃ�, �ϊ����ɓǂݍ��񂾒l��˂����ނ��߂�%d�ɒu��
        format_arg << "targ"+x.to_s
        line[pos,2] = "%d"
      end
    elsif line[pos+1] == "s"[0]
      # "$s"�Ȃ�successcode
      format_arg << "successcode"
      line[pos,2] = "%s"
    elsif line[pos+1] == "f"[0]
      # "$f"�Ȃ�failcode
      format_arg << "failcode"
      line[pos,2] = "%s"
    end
  end
  # �ŏI�I�ɕ\������̂͂���
  print "  print_indent(indent); fprintf(OUT, \"", line, "\\n\""
  format_arg.each{|x| print ", ", x}
  print ");\n"
end

# -i�I�v�V���������Ă�����C���^�v���^����, ���Ă��Ȃ���΃g�����X���[�^����
if ARGV == 1 && ARG[0] == "-i" then $translator_generate = false end
is_case_opened = false # ��case�����J���Ă��邩�ǂ���
is_buffering_endl = false # ����s�̉��s�o�͂𗯕ۂ��Ă��邩�ǂ��� �ecase���̍Ō�̉��s�͏o�͂��Ȃ�
mode = 0 # ���̏o�̓��[�h
line = ""
arg = [] # ���J���Ă���case�̈���

while(line=gets())
  line.chop!

  if line == ""
    # ���ɉ��s���o�b�t�@����Ă����(=2�s�A����s)���s �����łȂ���Β��߂Ă���
    if is_buffering_endl
      print "\n"
    else
      is_buffering_endl = true
    end
  elsif line == "#__end" # __end����
    # ���P�[�X���J���Ă��������, ���߂Ă����s�͂����o�͂��Ȃ�, ���̌�__echo�ɂ���
    if is_case_opened
      is_case_opened = false
      case_close(arg)
    end
    is_buffering_endl = false
    mode = 1
  elsif line[0] == "#"[0] # �����̃R�}���h
    t = line[1,line.size-1].split(" ")
    a = ["__ignore", "__echo", "__echo_i", "__echo_t", "__format", "__format_i", "__format_t"].index(t[0])
    
    if a == nil # ���[�h�w��ł͂Ȃ��ꍇ=���Ԗ��ߑΉ���case���̊J�n
      # ����case�����J���Ă��������, case���J�����O�̉��s�͎̂Ă�
      if is_case_opened
        case_close(arg)
      end
      is_case_opened = true
      is_buffering_endl = false
      # �P�[�X���J��, ���[�h��__format��
      arg = t[1,t.size-1]
      case_open(t[0], arg)
      mode = 4
    else # ���[�h�w�肾�����ꍇ
      mode = a
    end
  else # �R�}���h�ł��Ȃ��ꍇ
    # ���܂��Ă����s������Ώo��
    if is_buffering_endl
      print "\n"
      is_buffering_endl = false
    end

    temp_mode = mode # �ꎞ�I�ȃ��[�h�ϐ�
    if line[0]=="%"[0] and temp_mode>=4 and temp_mode<=6 # �s����%��mode��format_*�Ȃ痪�L�Ȃ̂Ń��[�h�����炷
      line[0,1] = " "
      temp_mode = temp_mode - 3
    end
    
    case temp_mode
    when 0 # ignore
    when 1 # echo
      print line, "\n"
    when 2 # echo_i
      if not $translator_generate
        print line, "\n"
      end
    when 3 # echo_t
      if $translator_generate
        print line, "\n"
      end
    when 4 # format
      if $translator_generate
        # �g�����X���[�^�p�ɂ�$1��$f����%d�ɂ���printf�ŕ��ŏo�͂���C���[�W
        print_trans_format(line, arg)
      else
        # �C���^�v���^�p�ɂ�$1��$f����targ1��return�ɕϊ����ďo�͂���
        print_interp_format(line, arg)
      end
    when 5 # format_i
      if not $translator_generate
        print_interp_format(line, arg)
      end
    when 6 # format_t
      if $translator_generate
        print_trans_format(line, arg)
      end
    end
  end
end

# case���J�����ςȂ��ŏI�������ꍇ
# �{���͂�����__end�ŕ��Ă��̌�̊֐��̃J�b�R�����邱��
if is_case_opened
  case_close(arg)
end
