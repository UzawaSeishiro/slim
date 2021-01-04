## --with-sccオプションについて

全状態空間についてのSCCグラフを生成します

非決定実行する必要があるので--ndオプションが必須です

「slim --nd --with-scc (その他オプション) (LMNtalプログラム)」として呼び出します(まずは簡単のためその他オプションがない前提で実装を進める)

状態空間生成ののち、その状態空間についてのSCCグラフを計算し、データ構造へのポインタを返します



## 目指しているデザインについて(2021/01/04現在)

「簡単にHomeプロパティが検証できる」と言いたいので、状態空間と同じように「LaViTから呼び出して、LaViTに返して、描画する」ことを想定します

このSCCグラフは状態空間に対する付加的な情報なので、「非決定実行をしたらまず状態空間を表示する」という不変条件は守ります(つまりSCCグラフよりも状態空間の方を優先)

状態空間と一緒にデータ構造へのポインタをLaViTに返し、ユーザの要求があればSCCグラフを表示するようにします

状態空間ボタン・SCCボタンをLaViTの状態空間画面に追加し、(後述のSCC内部グラフと違い)状態空間およびSCCグラフへはどこからでもすぐに遷移できるようにします



## SCC内部グラフについて

SCC内部グラフは状態空間のうち、そのSCCに所属する状態ノードおよび入力ノード・出力ノードで構成されます

各状態ノードを新たに用意するのはムダなので(状態ノードのデータ構造は大きい)、状態空間における各状態ノードへの参照とします

入力矢印、出力矢印が2つのSCC内部グラフ間でダブっていること、およびそれを入力ノード・出力ノードで終端していることを除けば状態空間のサブセットになっています

SCCグラフにおいてSCC1を表すノードは「SCC1内部グラフ」に対する参照になっており、ダブルクリックでSCC1内部グラフを表示します

SCCは基本的に前のSCCからの入力矢印、次のSCCへの出力矢印をもっているので、これらはそのSCC内部グラフを参照する入力ノード・出力ノードという特殊なノードを用意して終端します

検索結果を状態空間だけでなくSCCグラフからも見られるようにします(これは後)

それ以外の操作はすべてこれまでどおり、状態空間に対する操作として受け付けます

(まずは簡単のために膜やハイパーリンクのないFlat LMNtalであることを前提として実装を進める)