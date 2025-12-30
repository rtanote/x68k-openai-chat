# X68000 インストールガイド

X68000側でOpenAI Chat Clientをセットアップする手順です。

## 必要なもの

- X68000本体（実機またはXM6等のエミュレータ）
- Human68k
- RS-232Cケーブル（クロス）※実機の場合
- ブリッジサーバが稼働していること

## 1. ファイルの配置

### CHAT.Xの配置

`CHAT.X` を任意のディレクトリにコピーします。
PATHが通っている場所（例：`A:\BIN\`）に置くと便利です。

```
A:\BIN\CHAT.X
```

## 2. シリアルポートの設定

### SWITCH.Xでの設定（推奨）

`SWITCH.X` を使用してRS-232Cの設定を行います：

```
A>SWITCH
```

RS-232Cの設定項目：
- **ボーレート**: 9600（または4800）
- **データビット**: 8
- **パリティ**: なし
- **ストップビット**: 1
- **フロー制御**: なし

設定後、X68000を再起動して設定を反映させます。

### CONFIG.SYSでの設定（代替）

`CONFIG.SYS` に以下を追加する方法もあります：

```
RSDRV.SYS
```

## 3. エミュレータ使用時の設定（com0com）

XM6等のエミュレータを使用する場合、仮想シリアルポート（com0com）を使ってブリッジと接続します。

### com0comのインストール

1. [com0com](https://sourceforge.net/projects/com0com/) をダウンロード
2. インストーラを実行（署名なしドライバの警告が出る場合あり）
3. セットアップユーティリティで仮想ポートペアを作成

### 仮想ポートペアの作成

com0comセットアップユーティリティを起動：

```
C:\Program Files (x86)\com0com\setupc.exe
```

ポートペアを作成（例：COM6 ⇔ COM7）：

```
> install PortName=COM6 PortName=COM7
```

または、GUIの `setupg.exe` を使用：
1. 「Add Pair」をクリック
2. 左側を `COM6`、右側を `COM7` に設定
3. 「Apply」をクリック

### 接続構成

```
XM6 (COM6) ←→ com0com ←→ Bridge (COM7)
```

- **XM6側**: RS-232CポートをCOM6に設定
- **ブリッジ側**: config.yamlでCOM7を指定

### XM6の設定

1. XM6を起動
2. メニュー「ツール」→「オプション」→「RS-232C」
3. 「COMポート」を `COM6` に設定
4. 「速度」を `9600` に設定
5. 「OK」をクリック

### ブリッジ側の設定（Windows）

`bridge/config.yaml`:

```yaml
serial:
  port: COM7
  baud_rate: 9600
```

ブリッジの起動：

```cmd
cd bridge
set OPENAI_API_KEY=sk-your-api-key-here
python bridge.py
```

### エミュレータでの動作確認

1. ブリッジを起動（COM7で待機）
2. XM6を起動（COM6に接続）
3. XM6上でSWITCH.Xを使いRS-232Cを9600bps/8N1に設定
4. `CHAT "Hello"` を実行

## 4. 動作確認

### 基本的なテスト

```
A>CHAT "Hello"
```

ブリッジサーバが応答を返してきます。

### バージョン確認

```
A>CHAT -v
CHAT.X version 1.0.0
```

### ヘルプ表示

```
A>CHAT -h
X68000 OpenAI Chat Client

Usage: chat [options] [message]

Options:
  -i, --interactive  Enter interactive mode
  -b, --baud <rate>  Set baud rate and init serial (4800/9600/19200/38400)
                     Without -b, uses SWITCH.X settings
  -t, --timeout <s>  Set timeout in seconds (default: 60)
  -v, --version      Show version
  -h, --help         Show this help

Examples:
  chat "What is X68000?"
  chat -i
  chat -b 38400 "Hello"
```

## 5. 使用方法

### 単発メッセージ

```
A>CHAT "X68000について教えてください"
```

### インタラクティブモード

対話形式でチャットします。`exit` または `quit` で終了。

```
A>CHAT -i
X68000 Chat - Interactive Mode
Type 'exit' or 'quit' to end.

> こんにちは
...
こんにちは！何かお手伝いできることはありますか？

> X68000の歴史を教えてください
...
X68000は1987年にシャープが発売した16ビットパーソナルコンピュータです...

> exit
Goodbye!
```

### タイムアウトの変更

応答に時間がかかる場合、タイムアウトを延長できます（デフォルト: 60秒）：

```
A>CHAT -t 120 "長い文章を生成してください"
```

### ボーレートの明示指定

SWITCH.Xの設定を使わず、明示的にボーレートを指定する場合：

```
A>CHAT -b 9600 "Hello"
```

**注意**: `-b` オプションを使用すると SET232C が呼び出されます。
通常はSWITCH.Xの設定（`-b` なし）で動作させることを推奨します。

## 6. μEmacs統合

μEmacsからAIを利用する場合は、`chat.rc` を設定します。

### 設定

`chat.rc` をμEmacsのディレクトリにコピーし、`emacs.rc` に以下を追加：

```
execute-file "chat.rc"
```

※chat.rcの呼び出しとスクリプトは各自で調整してください。

### キーバインド

- **ESC a**: AIに質問（ミニバッファから入力）
- **ESC A**: 選択範囲をAIに送信して置換
- **Ctrl+C Ctrl+S**: ESC a と同じ

## 7. トラブルシューティング

### Timeout waiting for response

- ブリッジサーバが起動しているか確認
- シリアルケーブルの接続を確認
- ボーレートがブリッジ側と一致しているか確認
- タイムアウトを延長して再試行: `CHAT -t 120 "Hello"`

### 文字化けする

- SWITCH.Xでデータビットが8になっているか確認
- パリティが「なし」になっているか確認

### CHAT.X実行後にシリアル通信ができなくなる

`-b` オプションを使用せず、SWITCH.Xの設定のみで動作させてください：

```
A>CHAT "Hello"        ← 推奨（-b なし）
A>CHAT -b 9600 "Hello" ← SET232Cが呼ばれる
```

### 接続テスト（COPY CON AUX:）

CHAT.Xを使わずにシリアル通信をテストする方法：

```
A>COPY CON AUX:
Hello
^D
```

（Ctrl+Dで送信）

ブリッジ側のエコーサーバが動いていれば、応答が返ります：

```
A>COPY AUX: CON:
Echo: Hello
^C
```

## 8. ビルド方法（開発者向け）

xdev68k環境でビルドする場合：

```bash
# Windows (xdev68k)
cd src
make

# 出力
# cli/CHAT.X - CLIツール
# libchat/libchat.a - ライブラリ
```
