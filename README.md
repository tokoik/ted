# TelExistence Display System (TED)

遠隔地の視覚的環境を観測者の周囲に再現する実験システム

    Copyright (c) 2016 Kohe Tokoi. All Rights Reserved.
    
    Permission is hereby granted, free of charge,  to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction,  including without limitation the rights
    to use, copy,  modify, merge,  publish, distribute,  sublicense,  and/or sell
    copies or substantial portions of the Software.
    
    The above  copyright notice  and this permission notice  shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE  IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
    IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO THE WARRANTIES  OF MERCHANTABILITY,
    FITNESS  FOR  A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN  NO EVENT  SHALL
    KOHE TOKOI  BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER IN
    AN ACTION  OF CONTRACT,  TORT  OR  OTHERWISE,  ARISING  FROM,  OUT OF  OR  IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## 概要

このシステムは遠隔地の情景や空間的情報,
および光源環境を取得し,
観測者の周囲にあるように再現する実験のためのシステムです.
観測者があたかも遠隔地にいるように知覚させることのほか,
取得した情報の一部を観測者側のものに置き換えることによって,
遠隔地のものが観測者の手元にあるように見せることも目的にしています.
ただし, あくまで実験を目的としたものであり,
特定の目的を果たすアプリケーションとして完成したものではありません.


### 応用

本システムをもとに [UZUME Project](http://kazusa.net/uzume/)
における「遠隔代理科学者」,
すなわちロボットの視覚への応用を目指した実験システムが開発されています.
つまり, 想定している遠隔地は, とりあえず「月」です.

1. 床井浩平, 大山英明, 河野功. "遠隔地のロボットと視覚を共有する AR 型 HMD システム."
宇宙科学技術連合講演会講演集 60 (2016): 5p.
2. 大山英明, et al. "月探査のためのテレイグジスタンスロボット操縦システム."
宇宙科学技術連合講演会講演集 60 (2016): 4p.
3. 河野功, et al. "月火星縦孔地下空洞探査 (UZUME) システムの研究."
宇宙科学技術連合講演会講演集 60 (2016): 6p.


### TED という名前の由来について

TED という名前は, 作者が初めてオープンソースとして公開した
(当時はオープンソースという言葉はありませんでしたが)
テキストエディタ [TED](http://www.vector.co.jp/soft/dos/writing/se001452.html)
に由来しています.
作者の本業は CG プログラミングですが,
作者はこのプログラムの開発によりプログラマとしてのスタート地点に立てたと思っています.
作者は初心に帰ってこのシステムの開発に取り組む決意を込めて,
本システムを TED と名付けました.


## 機能

* 本システムを 2 台の PC で動作させ, 一方の HMD (作業者 HMD) の視界及びヘッドトラッキング情報を, ネットワークを介して, もう一方の HMD (操縦者 HMD) と共有することができる.
* 作業者及び操縦者の HMD には, ともに Oculus Rift DK1/DK2 あるいは CV1 が使用できる. また, HMD 以外にも通常の平面ディスプレイ (単眼視) のほか, サイドバイサイド, トップアンドボトム, および時分割多重 (フレームシーケンシャル) 方式の3Dディスプレイへの表示も可能である.
* 環境映像の取得には，全方位 (全天球) カメラの RICOH THETA S のほか, 魚眼カメラの KODAK SP360 4K, 魚眼レンズ (フジノンFE185C046HA-1) を取り付けた USB カメラ (センテックSTC-MCE132U3V), 一般の Web カメラ, および広角ステレオカメラ Ovrvision Pro が使用できる.
* 映像の取得に魚眼カメラや全方位カメラを用いた場合は, それを平面投影像に展開し, 環境映像から操縦者の視野の映像を切り出す機能をもつ. また, パンチルトカメラの映像を合成して全天球画像として用いる機能も持っている (が, キャリブレーションが大変な上, カメラの移動に並進が含まれていると上手くいかないのでやめようかと思っている).
* 広角ステレオカメラ Ovrvision Pro や 2 台の Web カメラにより取得した立体視映像を表示する機能をもつほか, HMD あるいはパンチルトヘッドに装着した 2 台の全天球カメラ (RICOH THETA S) の映像を安定化する (映像からカメラの方向変化を除去して並進のみにする) 機能を持つ.
* 表示映像に Alias OBJ 形式の三次元形状データを半透明で重畳表示する機能をもつ. これにはワールド座標に固定するもの, 装着する HMD の動きに追従するもの, もう一方の HMD に追従するものを別々に指定できる. ワールド座標に固定したものは静止物体として環境中に配置される. この配置はマウスやゲームパッドを用いて調整することができる. 装着する HMD に追従するものには, 装着者の視線のマーカや, 後述する Leap Motion で制御する「手」のモデルを指定する. もう一方の HMD に追従するものには，操縦者の視線や手の動きを作業者に伝えるマーカや「手」のモデル, あるいは作業者の視線を操縦者にフィードバックするためのマーカを指定する.
* Alias OBJ 形式の三次元形状データはパーツ間の階層構造や骨格などのロボットの表現に必要となる機能を持たないため, JSON (JavaScript Object Notation) による独自形式のシーングラフ (将来的には URDF を採用すべきだと考えている) でシーンの記述を行う. 階層化されたパーツ間の相対的な位置関係はシーングラフ内に記述できるほか, 共有メモリ (名前付きファイルマッピングオブジェクト) 上に置いた 4 行 4 列の変換行列を指定することもできる. これにより, パーツ間の位置関係を外部プログラムから制御することができる. このシーングラフの書式は config.pdf に示している.
* 操縦者の手の動きをモーションコントローラの Leap Motion で取得し, それを剛体変換の行列として前述の共有メモリに格納する. これにより, 任意の形状をL eap Motion で制御する「手」のモデルとして使用することができる.


## 必要システム

システムは Visual Studio 2017 の C++, x64 で開発しています.
CMake などは使っていません.
ツールキットとして [GLFW](http://www.glfw.org/) を使用し,
補助プログラムとして作者が授業の宿題用に使っている[フレームワーク](http://www.wakayama-u.ac.jp/~tokoi/lecture/gg/html/)
を使用しています.
ゲームエンジンなどは使用していません
(将来的に移行する計画はあります).

* OpenGL 3.2 以降
	+ GLFW 3.2.1 をプロジェクトに組み込んでいます.
		- 「プロジェクトのプロパティ」の「構成プロパティ」の「C/C++」の「コード生成」の「ランタイムライブラリ」を, Release ビルドでは「マルチスレッド (/MT)」, Debug ビルドでは「マルチスレッド デバッグ (/MTd)」にしています.
* Oculus Rift DK2 (0.8) または CV1 (1.18)
	+ ソースは両対応で, 使用する SDK のバージョンで自動判別しています.
	+ OculusSDK の LibOVR フォルダが C:\LibOVR にあることを想定しています.
		- OculusSDK には Debug ビルドのライブラリが含まれていないので, このプログラムを Debug ビルドする場合は, Samples.sln を使って LibOVR を Debug ビルドしてください. その際,「プロジェクトのプロパティ」の「構成プロパティ」の「C/C++」の「コード生成」の「ランタイムライブラリ」を「マルチスレッド デバッグ (/MTd)」にしておく必要があります.
* Ovrvision Pro
	+ SDK はプロジェクトに組み込んでいます.
	+ Ovrvision Pro は OpenCL を使うので, これも組み込んでいます.
	  - Optimus に対応した PC では Intel 版の OpenCL が使われてしまい, うまく動かないかもしれません. その場合は Intel 版の OpenCL を無効にするようレジストリをいじる必要があります.
* Leap Motion
	+ LeapSDK 3.2 が C:\LeapSDK に置かれていることを想定しています.
* OpenCV
	+ OpenCV 3.2 のバイナリが C:\OpenCV に置かれていることを想定しています.
	+ [libjpeg-turbo](https://www.libjpeg-turbo.org/) を使わないとキャプチャがだいぶ遅くなります.
	+ 開発には CUDA 8 による cudacodec を有効にしたものを使っていますが, 現状はなくてもいいかも知れません.
* USB カメラ
	+ 通常の USB カメラ, RICOH THETA S, Kodak PIXPRO 360 4K, 正距円筒図法 (RICOH THETA V など)

これらのほか, 設定ファイルの読み書きに [picojson](https://github.com/kazuho/picojson)
を使用しています. ありがとうございます.


## ドキュメント

* config.pdf
  + 設定ファイル (config.json) の書き方
* scenegraph.pdf
  + シーン記述ファイルの書き方


## 謝辞

本研究は, 現在, 平成29年度科学研究時助成事業 (基盤研究 (C), 課題番号:17K00271) の助成により実施しています.
