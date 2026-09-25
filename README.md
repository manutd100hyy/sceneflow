# SketchRoad

交通事故现场绘图（草图版、航拍版）。界面为简体中文，面向 Windows 11 与 Android 平板/手机，用 Qt Quick 做触控界面，现场图由 C++ 文档模型经 `QPainter` 绘制。同一套绘制代码输出屏幕和矢量 PDF。

几何单位是**米**，Y 轴向上。旧版 cocos 图符坐标是厘米，导入时已除以 100。对象类型用字符串（`roadline`、`symbol`、`trace`、`debris`、`dimension`、`text`、`crosswalk`、`guide`、`parking`、`compass`），不再使用旧的 `TufuKind` 整数。

## 运行

需要 Qt 5.14.2 或 5.15.x（不要用 Qt 6）。本仓库在 Qt 5.15.2 上编译通过。

```bash
mkdir build && cd build
qmake ../SketchRoad.pro
make -j$(nproc)
./SketchRoad
```

图符、道路模板和文书版式在 `data/`。程序按下面顺序找数据目录：环境变量 `SKETCHROAD_DATA`、可执行文件旁的 `data/`、上两级目录中的 `data/`，最后是编译时写入的路径。

案例默认写到应用数据目录（`QStandardPaths::AppDataLocation`）。可用 `SKETCHROAD_HOME` 指定根目录。每个案例一个文件夹：

```
cases/20260925T143000_0123456789ab/
  case.json
  thumb.png
  photos/
  aerial/
```

`case.json` 用 `QSaveFile` 原子写入，然后在事务里更新 SQLite 索引 `index.db`。索引含 FTS5，可从案例目录重建。

## 测试与离屏截图

```bash
mkdir build-tests && cd build-tests
qmake ../tests/tests.pro
make -j$(nproc)
QT_QPA_PLATFORM=offscreen ./tst_core
```

```bash
QT_QPA_PLATFORM=offscreen ./SketchRoad --capture /tmp/sketchroad-capture
```

`--capture` 会写出桌面、平板、手机尺寸的界面截图，以及 A3 现场图和各文书 PDF。

## Windows（Qt 5.14.2）

用 Qt 5.14.2 的 MinGW 或 MSVC 套件，不要换 Qt 6。

1. 安装 Qt 5.14.2，勾选 Qt Quick、Qt Quick Controls 2、Qt SVG。MinGW 套件选对应的 MinGW 编译器；MSVC 套件选已安装的 Visual Studio 2017 或 2019 生成工具。
2. 用该套件的 `qmake`：

```bat
mkdir build
cd build
qmake ..\SketchRoad.pro -spec win32-g++
mingw32-make
```

MSVC 把 `-spec` 换成 `win32-msvc`，然后用 `nmake` 或在 Qt Creator 里打开 `SketchRoad.pro` 构建。

3. 把 `data` 目录放到 `SketchRoad.exe` 旁边，或设置 `SKETCHROAD_DATA`。部署时用该套件的 `windeployqt SketchRoad.exe`。

## Android（Qt 5.14.2，qmake）

本机没有 Android SDK，仓库不附带 APK。清单在 `android/AndroidManifest.xml`，包名 `com.sketchroad.scene`，`minSdkVersion` 21。

1. 安装 Qt 5.14.2 的 Android 套件（armeabi-v7a 或 arm64-v8a）、Android SDK、NDK（与该 Qt 套件说明一致，5.14 常用 NDK r20/r21）和 JDK 8。
2. Qt Creator 打开 `SketchRoad.pro`，选择 Android 套件。工程已设置 `ANDROID_PACKAGE_SOURCE_DIR`。
3. 构建前把 `data/` 打进 APK。可在 `.pro` 里增加：

```qmake
android {
    data.files = $$PWD/data
    data.path = /assets/data
    INSTALLS += data
}
```

应用在 Android 上若找不到数据，设置资源复制逻辑或把 `SKETCHROAD_DATA` 指到解压后的目录。签名与打包用 Qt Creator 的 “Build APK”。

## 范围说明

只包含草图版和航拍版。航拍版在草图能力上增加照片底图、两点标定和叠绘导出。不做镜头畸变校正、AI 识别、注册授权、服务器上传、电子签名板、蓝牙测距，也不迁移旧 plist 案例。直角标注的比例化按两端直线距离移动对象。路口没有自动圆角衔接。详见 `docs/STATUS.md`。
