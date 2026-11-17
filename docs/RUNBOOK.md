# 別の PC で計算する手順（computer use・接続フォルダ向け）

まとめのドキュメント:「196問題とリクレル数の研究：全体のまとめと引き継ぎ」
https://claude.ai/artifact/99sYx47NsWgTzkrHsrPvtE （ドキュメント ID: 42063666-dcc3-4db3-adbb-708a1f90284f）

## 0. Claude にどう動かしてもらうか
- **おすすめ: 接続フォルダのシェル。** 計算用 PC に Claude デスクトップアプリを入れてサインインし、その PC でチャットを開いてメッセージを送ると、チャットがその PC とつながる。コード一式を置いたフォルダを接続すれば、Claude はそのフォルダでコマンドを直接実行できる。
- **computer use（画面操作）は計算の実行に向かない。** ターミナルや IDE は「クリックのみ」の扱いで、文字を打てない。Finder での確認や、TeXShop で論文を開くといった画面の作業に使う。computer use はデスクトップアプリの設定で有効にする（初期状態はオフ）。

## 1. 最初の準備（1回だけ）
1. OS は macOS か Linux（Ubuntu など）がおすすめ。Windows は WSL2 の Ubuntu を使う（未検証）。
2. 必要なもの
   - C コンパイラと make: Mac は `xcode-select --install`、Ubuntu は `sudo apt install build-essential`
   - Python 3.10 以上と numpy: `python3 -m pip install numpy`（または `pip install -r requirements.txt`）
   - 論文の PDF を作るときだけ LaTeX（Mac は MacTeX）
3. `lychrel-self-similar-2026-11-16.zip` を置いて展開する（GitHub に置いた場合は `git clone https://github.com/dew-web/lychrel-self-similar`）。
4. 動作確認
   ```sh
   cd lychrel-self-similar
   make env      # OS・CPU・メモリ・AVX2・Python を表示
   make          # C のプログラムを bin/ に作る
   make p196     # AVX2 がある x86-64 の PC だけ（Apple Silicon の Mac では不可）
   make check    # 8項目の動作確認（10秒ほど）。p196 がない PC では 7項目 + SKIP
   ```

## 2. 計算の一覧
時間は 2コア・メモリ7GB のクラウド環境での目安。

| やること | コマンド | 時間 | 出力 |
| --- | --- | --- | --- |
| 動作確認 | `make check` | 10秒 | `work/check/` |
| 9進の証明を再確認 | `make verify-base9` | 数秒 | 画面 |
| 既知の族（2・4・11・26進）と9進の再発見 | `make validate` | 数分 | `work/validation/` |
| 繰り上がり込みの作用素の λ | `make carries` | 数秒 | `carries/results/op_output.txt` |
| 緩和の実験（1回ごと） | `python3 carries/relax_perstep.py --bases 12,24 --jobs 2` | 1基数 30秒〜2分 | `carries/results/perstep_B*.txt` |
| 緩和の実験（区間ごと、256回まで） | `python3 carries/relax_buckets.py --bases 10,16 --jobs 2` | 1基数 1〜2分 | `carries/results/buckets_B*.txt` |
| 半周期（ww 型）の探索 | `python3 scripts/halfsearch.py 9 10` | 1基数 6秒〜3分 | `work/half/half_progress.txt` |
| 10進の分類（2本の列の型） | `sh scripts/classify_base10.sh 2 15` | 長い（周期が上がると急に増える） | `work/base10/` |
| 196 の軌道 100万桁 | `./bin/p196 1000000` | 約4分（1コア） | 画面 |
| 波の検証1〜3 | `cd fourier && python3 wave.py` | 数秒 | 画面 |
| 波の検証4〜6 | `cd fourier && python3 verify2.py && python3 verify3.py` | 数分 | 画面 |
| 論文の PDF | `cd paper && lualatex base9_lychrel_ja.tex`（2回）、英語版は `pdflatex base9_lychrel.tex`（2回） | 1分 | `paper/*.pdf` |

## 3. 大きい PC 向けの計算
- **10進の未決着2パターン:** 両端オートマトンの状態数の上限を上げて作り直す。上限 2^N で約 48×2^N バイト使う（N=26 で約3GB、N=27 で約6GB、N=28 で約13GB）。
  ```sh
  cc -O2 -march=native -w -DECAP_LOG=27 -o bin/outauto_big src/outauto.c
  mkdir -p work/base10big && cd work/base10big
  ../../bin/outauto_big Aa 10 16 0 p0101010101010101 > P16.log
  ../../bin/outauto_big Aa 10 18 0 p010101001010101001 > P18.log
  ```
  両端の解（`outer_*.txt`）が出たら `../../bin/coreauto2 10 <ov> <cin> <outer ファイル>` で中心を判定する。
- **半周期探索を大きく回す:** `python3 scripts/halfsearch.py 6,7,14,24 12 --outauto bin/outauto --outer-timeout 600 --core-timeout 1800`
- **中心の状態数が足りないとき:** `cc -O2 -march=native -w -DECAP_LOG=26 -o bin/coreauto2_big src/coreauto2.c`（`--coreauto bin/coreauto2_big` で使う）

## 4. 長い計算の動かし方
```sh
mkdir -p logs
nohup python3 carries/relax_perstep.py --bases 12,24,48 --jobs 3 > logs/relax_$(date +%Y%m%d_%H%M).log 2>&1 &
tail -n 20 logs/relax_*.log        # 進み具合
ps aux | grep -E "relax|halfsearch|outauto|coreauto" | grep -v grep   # 動いているか
pkill -f relax_perstep.py          # 止める
```
- Mac は、スリープすると計算が止まる。`caffeinate -i` をコマンドの前に付ける。
- 並列の数は CPU のコア数まで（`make env` で確認）。
- 結果は `work/` と `carries/results/` にたまる。まとまったら、まとめのドキュメントの「数値データ」タブに追記する。

## 5. Claude への頼み方（例）
> まとめのドキュメントの「別のPCで計算する」タブに従って、接続したフォルダの lychrel-self-similar で `make check` を実行して。全部 PASS したら、12進と24進で緩和の実験を回して、2のべき乗のときのような跳ね上がりが出るか教えて。

Claude 向けの注意:
- コマンドは接続フォルダのシェルで実行する（computer use ではターミナルに入力できない）。
- 長い計算は `nohup … &` で裏に回し、ログを `tail` で確認する。
- 新しい結果はファイルに保存し、まとめのドキュメントにも追記する。
- 論文の出典に Wikipedia は使わない。
