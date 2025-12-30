# X68000 OpenAI Chat System

X68000からOpenAI APIを利用してチャットするためのシステムです。

## 概要

レトロコンピュータX68000とモダンなAI（OpenAI API）を接続し、シリアル通信経由でチャットを可能にします。

```
X68000 ──RS-232C──> Bridge (RPi/ESP32) ──HTTPS──> OpenAI API
```

## 機能

- **通信ライブラリ (libchat.a)**: 再利用可能なシリアル通信ライブラリ
- **CLIツール (chat.x)**: Human68kコマンドラインからのチャット
- **μEmacs統合**: エディタ内でのシームレスなAIアシスタント
- **日本語対応**: Shift_JIS ⇔ UTF-8 自動変換（ブリッジ側で処理）

## 開発環境

- **ホスト**: Windows + [xdev68k](https://github.com/yosshin4004/xdev68k)
- **ターゲット**: X68000 (Human68k)
- **ブリッジ**: Raspberry Pi または ESP32

## ドキュメント

仕様書は `.kiro/specs/x68k-openai-chat/` に配置されています：

- `requirements.md` - 要件定義（EARS記法）
- `design.md` - 技術設計
- `tasks.md` - 実装タスク

## 将来の拡張

256x256ピクセルモードを使用したゲーム風グラフィカルフロントエンド（英語専用）も計画されています。
同じ通信ライブラリ（libchat.a）を使用して実装予定です。

## ライセンス

This project is licensed under the MIT License.
See the LICENSE file for details.
