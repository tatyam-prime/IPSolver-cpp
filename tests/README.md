# テスト

`models/*.hpp` はソルバの検証・ベンチマーク用の定式化です。比較用の切り替えや探索統計はここに置き、`examples/*/*.cpp` は標準入力・標準出力を使う競プロの解法例として独立させています。

C++ のモデルテストは CMake / CTest で実行します。実際の解法例の入出力は Python の `test_*submissions.py`、`test_atcoder.py`、`test_abc326_g.py`、`test_cf008_c.py` で独立した全探索・DP などと照合します。提出用ソースを使うテストの前には `python3 tools/make_submissions.py` を実行してください。
