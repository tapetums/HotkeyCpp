【ソフト名】 HotkeyCpp
【著作権者】 Copyright (C) 2016 tapetums
【種　　別】 TTBase プラグイン
【動作環境】 Windows Vista/7/8/8.1/10 32/64-bit
【開発環境】 Visual Studio Community 2015


【概要】
簡易ホットキーランチャーです。
TTBase もしくは 互換プログラムのプラグインとして動作します。


【依存関係】
VC++14 の ランタイムが必要です。
お持ちでない場合は、Microsoft社 のページからダウンロードをお願い致します。

[Visual Studio 2015 Update 1 の Visual C++ 再頒布可能パッケージ]
(https://www.microsoft.com/ja-jp/download/details.aspx?id=49984)
（デッドリンクの際はご容赦下さい。）


【インストール】
本体となるプログラムによります。
TTBase の場合は、本体のインストールフォルダ以下の場所に配置してください。

本体が 32-bit版 の場合は x86\HotkeyCpp.dll を、
本体が 64-bit版 の場合は x64\HotkeyCpp.dll を それぞれお使いください。


【アンインストール】
DLLファイル 及び .iniファイルを削除してください。
常駐型のプラグインのため、本体を一旦終了させる必要があります。


【使用方法】
本体のコマンド一覧もしくはシステムメニューから「HotkeyCpp 設定」を選び、設定画面を表示します。
「追加」ボタンを押し、登録したいホットキー、修飾キー、コマンドなどを登録します。


【ホットキーの設定】
・hotkey
　ホットキーを登録します。エディットボックス内でキーを押すか、
　横の▲▼をクリックしてキーを選んで下さい。
　Enterなど、一部のキーは▲▼でしか選択できません。
　なお、(なし)や (未定義) など、動作しないキーもあります。

・command
　インストールされているプラグイン、もしくは任意のファイルやプログラムを登録できます。

・ID
　コマンドを登録した際に指定するコマンドIDです。横にコマンド名が表示されます。

・param
　コマンドではなく.exeファイルを指定した場合に、起動時にオプションとして指定する値です。


【ライセンス】
NYSL Version 0.9982 です。このプラグラムを使用したことによる一切の影響は関知しません。
http://www.kmonos.net/nysl/


【謝辞】
TTBase を生み出された K2 さん、peach を 公開されている U さん、TTBaseCpp の作者さんを始め、
数多くのプラグインを作られたそれぞれの作者さんたちに深い敬意と感謝を表します。
http://ttbase.sourceforge.jp/
http://seesaawiki.jp/ttbase/d/%A5%D7%A5%E9%A5%B0%A5%A4%A5%F3
http://white2.php.xdomain.jp/?page_id=27
http://ttbase.coresv.com/uploader/


【更新履歴】
2016.02.17 v0.2.0.0
 ・仮想キーの文字列が正しく表示されるよう修正

2016.02.16 v0.1.0.0
 ・初版作成


【文責】
tapetums
https://github.com/tapetums