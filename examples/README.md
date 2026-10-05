# 整数計画による競プロの解法例

各サイトの README に、変数・制約・前処理・解の復元方法を説明しています。

| サイト | 解説 | 主な定式化 |
| --- | --- | --- |
| AtCoder | [8問の解法](atcoder/README.md) | クリーク、閉包、反鎖、条件付き等式、部分巡回除去 |
| Codeforces | [5問の解法](codeforces/README.md) | マッチング、集合被覆、同じ属性の統合、素因数 packing、厳密消去 |
| QOJ | [4問の解法](qoj/README.md) | 充填パターン、独立集合、等式の自由度への縮約、隣接数とオイラー路 |

編集するモデルは `examples/<サイト>/<問題番号>.cpp` です。提出用ソースは、リポジトリのルートで次のコマンドを実行すると `build/submissions/<サイト>/<問題番号>.cpp` に生成されます。

```sh
python3 tools/make_submissions.py
```

生成したファイルはソルバを埋め込んでおり、その全体を C++17 以上で提出できます。コメント・空白・改行はそのまま保ちます。ソルバやモデルを更新した場合は再生成してください。

モデルを直接コンパイルする場合は、ヘッダの検索先に `include` を指定します。

```sh
c++ -std=c++17 -O2 -Iinclude examples/atcoder/abc002_d.cpp -o /tmp/abc002_d
```
