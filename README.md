# PROMASTER ONE

## リファレンスベース・マスタリングVSTプラグイン

**PROMASTER ONE**は、リファレンストラックを解析し、あなたのミックスを理想のサウンドに近づけるためのインテリジェントなマスタリングVSTプラグインです。

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20|%20macOS-lightgrey)
![Format](https://img.shields.io/badge/format-VST3-green)

---

## 特徴

### 🎯 ワンクリック・マスタリング
「Adjust」ボタンを押すだけで、マルチバンドコンプレッサーとイコライザーが自動的に働き、リファレンストラックのサウンドに近づけます。

### 📊 リアルタイム周波数解析
8バンドのスペクトラムアナライザーが、現在のミックスとリファレンスの差を視覚的に表示します。

### 💡 インテリジェント・サジェスチョン
AIがミックスを分析し、改善すべきポイントを日本語と英語で提案します：
- ベースの不足/過剰
- 中域のバランス
- 高域の明るさ
- 全体の音圧

### 💾 プリセット管理
リファレンストラックのプロファイルをプリセットとして保存し、いつでも呼び出せます。

### ⚡ 低CPU負荷設計
最適化されたDSPアルゴリズムにより、プロジェクト全体のパフォーマンスに影響を与えません。

---

## システム要件

### Windows
- Windows 10/11 (64-bit)
- VST3対応DAW（Cakewalk Sonar, Cubase, Studio One等）
- 4GB RAM以上推奨
- SSE2対応CPU

### macOS
- macOS 10.13以降
- VST3対応DAW
- Apple Silicon (M1/M2) またはIntel Mac

---

## インストール

### Windows
```bash
# ビルド
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# インストール
cmake --install . --config Release
```

プラグインは `C:\Program Files\Common Files\VST3\PromasterOne.vst3` にインストールされます。

### macOS
```bash
mkdir build && cd build
cmake .. -G Xcode
cmake --build . --config Release
cmake --install . --config Release
```

---

## 使い方

### 1. リファレンストラックの読み込み

1. プラグインウィンドウにオーディオファイルをドラッグ＆ドロップ
2. または「Load Reference...」ボタンからファイルを選択
3. 対応フォーマット: WAV, MP3, AIFF, FLAC, OGG

### 2. スペクトラム解析

プラグインは自動的に以下を解析します：

| バンド | 周波数帯域 | 役割 |
|--------|-----------|------|
| Sub Bass | 20-60Hz | 超低域の土台 |
| Bass | 60-250Hz | キックやベースの基音 |
| Low Mid | 250-500Hz | 楽器の厚み |
| Mid | 500-1kHz | ボーカル・楽器の中心 |
| Upper Mid | 1-2kHz | 明瞭度・存在感 |
| Presence | 2-6kHz | アタック・鮮明さ |
| Brilliance | 6-12kHz | 輝き・空気感 |
| Air | 12-20kHz | 超高域の開放感 |

### 3. サジェスチョンの確認

画面下部に表示されるサジェスチョンを確認します：

```
⚠ 低音がリファレンスと比較して不足しています
  Bass is lacking compared to reference
  重要度: [▓▓▓▓▓▓▓░░░] 高
```

### 4. Adjustボタン

大きな「ADJUST」ボタンをクリックすると、以下の処理が自動適用されます：

1. **マルチバンドイコライザー** - 各帯域のレベルをリファレンスに合わせて調整
2. **マルチバンドコンプレッサー** - ダイナミクスを最適化
3. **ラウドネス調整** - 全体の音圧を目標値に

### 5. プリセットの保存

気に入った設定はプリセットとして保存できます：

1. 「+ Save」ボタンをクリック
2. プリセット名を入力
3. `.pmo`ファイルとして保存

---

## 付属プリセット

| プリセット名 | ジャンル | 特徴 |
|-------------|---------|------|
| Pop Master 2024 | ポップス | パンチのある低音、明るい高音 |
| Rock Energy | ロック | アグレッシブな中域、パワフルなドラム |
| Jazz Warmth | ジャズ | 暖かみのあるサウンド、自然なダイナミクス |
| Electronic Punch | エレクトロニック | 強力な低音、シャープな高音 |

---

## 技術仕様

### DSP処理

```
入力信号
    │
    ▼
┌─────────────┐
│ FFT解析     │ ← 4096ポイント、75%オーバーラップ
└─────────────┘
    │
    ▼
┌─────────────┐
│ 8バンド     │ ← Linkwitz-Riley 4次クロスオーバー
│ 分割        │
└─────────────┘
    │
    ├──▶ [Band 0] ─▶ EQ ─▶ Comp ─┐
    ├──▶ [Band 1] ─▶ EQ ─▶ Comp ─┤
    ├──▶ [Band 2] ─▶ EQ ─▶ Comp ─┤
    ...                          │
    └──▶ [Band 7] ─▶ EQ ─▶ Comp ─┤
                                 │
                                 ▼
                         ┌─────────────┐
                         │ バンド合成   │
                         └─────────────┘
                                 │
                                 ▼
                         ┌─────────────┐
                         │ ラウドネス   │
                         │ 調整        │
                         └─────────────┘
                                 │
                                 ▼
                            出力信号
```

### パラメータ

| パラメータ | 範囲 | デフォルト |
|-----------|------|----------|
| Adjust Amount | 0-100% | 0% |
| Input Gain | -12dB〜+12dB | 0dB |
| Output Gain | -12dB〜+12dB | 0dB |
| Bypass | On/Off | Off |

### CPU最適化

- SIMD命令（SSE2/AVX2）による並列処理
- 効率的なFFT実装
- スマートなバッファ管理
- 不要な計算のスキップ

---

## トラブルシューティング

### プラグインがDAWに表示されない
1. VST3パスが正しく設定されているか確認
2. DAWのプラグインスキャンを再実行
3. 64bit版のDAWを使用しているか確認

### 音が歪む
1. 入力ゲインを下げる
2. Adjust Amountを控えめに設定
3. リファレンストラックの音圧が高すぎないか確認

### CPUが高負荷
1. バッファサイズを大きくする（512サンプル以上推奨）
2. サンプルレートを確認（44.1kHz/48kHz推奨）

---

## ライセンス

MIT License

Copyright (c) 2024 Promaster Audio

---

## サポート

- GitHub Issues: バグ報告・機能リクエスト
- Email: support@promaster.audio

---

## 更新履歴

### v1.0.0 (2024-01-15)
- 初回リリース
- 8バンドスペクトラムアナライザー
- ワンクリックAdjust機能
- プリセット管理システム
- 日本語/英語サジェスチョン
