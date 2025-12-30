# ブリッジサーバ インストールガイド

Raspberry PiでX68000 OpenAI Chat Bridgeをセットアップする手順です。

## 必要なもの

- Raspberry Pi（3B以降推奨）
- USB-シリアル変換アダプタ（FTDI推奨）
- OpenAI APIキー
- Python 3.9以降

## 1. 準備

### Python環境の構築（pyenv使用）

```bash
# pyenvのインストール
curl https://pyenv.run | bash

# .bashrcに追加
echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc
echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc
echo 'eval "$(pyenv init -)"' >> ~/.bashrc
source ~/.bashrc

# Python 3.9のインストール
pyenv install 3.9.18
pyenv global 3.9.18
```

### リポジトリのクローン

```bash
cd ~
git clone https://github.com/your-repo/x68k-openai-chat.git
cd x68k-openai-chat/bridge
```

### 依存パッケージのインストール

```bash
pip install pyserial openai pyyaml
```

## 2. 設定

### config.yamlの編集

```bash
cp config.yaml config.yaml.backup
nano config.yaml
```

シリアルポートを環境に合わせて設定します：

```yaml
serial:
  port: /dev/ttyUSB0   # USB-シリアルアダプタのポート
  baud_rate: 9600

openai:
  api_key: ${OPENAI_API_KEY}
  model: gpt-4o-mini
  max_tokens: 1024
```

### シリアルポートの確認

```bash
# 接続されているUSBシリアルデバイスを確認
ls -la /dev/ttyUSB*

# dialoutグループに追加（再ログイン必要）
sudo usermod -a -G dialout $USER
```

## 3. 動作確認

### 手動起動でテスト

```bash
# 環境変数にAPIキーを設定
export OPENAI_API_KEY="sk-your-api-key-here"

# ブリッジを起動
python bridge.py
```

X68000側から `CHAT "Hello"` を実行して応答があることを確認します。

## 4. サービス化（自動起動）

### APIキー用の環境変数ファイルを作成

```bash
# 環境変数ファイルを作成
echo "OPENAI_API_KEY=sk-your-actual-key-here" > ~/.x68k-chat-env
chmod 600 ~/.x68k-chat-env
```

### サービスファイルの設定

```bash
# サービスファイルをコピー
sudo cp x68k-chat-bridge.service /etc/systemd/system/

# サービスファイルを編集
sudo nano /etc/systemd/system/x68k-chat-bridge.service
```

以下のように編集します：

```ini
[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/x68k-openai-chat/bridge
EnvironmentFile=/home/pi/.x68k-chat-env
ExecStart=/home/pi/.pyenv/versions/3.9.18/bin/python /home/pi/x68k-openai-chat/bridge/bridge.py
Restart=on-failure
RestartSec=5
```

### サービスの有効化と起動

```bash
# systemdの再読み込み
sudo systemctl daemon-reload

# 自動起動を有効化
sudo systemctl enable x68k-chat-bridge

# サービスを起動
sudo systemctl start x68k-chat-bridge
```

### 動作確認

```bash
# ステータス確認
sudo systemctl status x68k-chat-bridge

# ログをリアルタイム表示
journalctl -u x68k-chat-bridge -f

# サービスの停止
sudo systemctl stop x68k-chat-bridge

# サービスの再起動
sudo systemctl restart x68k-chat-bridge
```

## 5. トラブルシューティング

### シリアルポートが開けない

```bash
# 権限を確認
ls -la /dev/ttyUSB0

# dialoutグループに所属しているか確認
groups

# 一時的に権限を変更（テスト用）
sudo chmod 666 /dev/ttyUSB0
```

### APIキーエラー

```bash
# 環境変数が設定されているか確認
echo $OPENAI_API_KEY

# 環境変数ファイルの内容を確認
cat ~/.x68k-chat-env
```

### サービスが起動しない

```bash
# 詳細なエラーログを確認
journalctl -u x68k-chat-bridge -n 50

# Pythonパスを確認
which python
/home/pi/.pyenv/versions/3.9.18/bin/python --version
```

## 6. エコーサーバでのテスト

OpenAI APIを使わずにシリアル通信をテストする場合：

```bash
python echo_server.py /dev/ttyUSB0
```

X68000側から送信したメッセージがそのまま返ってきます。
