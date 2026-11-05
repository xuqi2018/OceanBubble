# OceanBubble（海洋消消泡）

## 一、游戏背景：
Qt Widgets 写的六边形蜂窝「三消合并」小游戏：圆心 + 3 圈蜂窝共 **37 格**，从右侧预览栏拖拽气泡放入棋盘，同数值 3 连消合并升级，连锁每多一跳得分翻倍，最高分用 `QSettings` 持久化。

---

## 二、环境要求

| 项 | 版本 / 路径（本机实测） |
|---|---|
| Qt | **5.15.2 MSVC2019 64-bit** → `D:\tool\QT 5.15.2\5.15.2\msvc2019_64` |
| 编译器 | Visual Studio 2022（工具集 v143，MSVC 19.37） |
| CMake | 3.28.5（最低 3.16） |
| Qt 模块 | `Core`、`Gui`、`Widgets`（无 QML / 无 Qt Creator / 无第三方库） |

> Qt 路径不同时，把下面命令里的 `CMAKE_PREFIX_PATH` 换成自己的 Qt 安装目录即可。
> 若使用 Qt 6，需要改 `CMakeLists.txt` 里的 `find_package(Qt5 ...)` 与 `Qt5::Widgets`。

---

## 三、构建（Visual Studio 生成器，推荐）

在工程根目录 `D:\xq\findWork\OceanBubble` 下执行：

### 3.1 配置（首次或修改 CMakeLists.txt 后执行）

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=D:/tool/QT 5.15.2/5.15.2/msvc2019_64"
```

### 3.2 编译

```bat
:: Debug
cmake --build build --config Debug
```

### 3.3 产物

| 配置 | 可执行文件 |
|---|---|
| Debug | `build\Debug\OceanBubble.exe` |

配置阶段还会生成 `build\OceanBubble.sln`，可用 VS 2022 直接打开该解决方案（它是 CMake 的生成物，不是手写工程）。

---

## 四、部署（生成可直接双击运行的目录）

Windows 下 Qt 程序需要 Qt 运行时 DLL。对产物执行一次 `windeployqt` 即可（已部署过则无需重复）。
注意 **必须把 exe 路径作为参数传给 windeployqt**，只写 `--debug` 不会部署任何东西。

**cmd.exe**

```bat
:: Debug（Debug 版依赖 Qt5Cored.dll 等调试库）
"D:\tool\QT 5.15.2\5.15.2\msvc2019_64\bin\windeployqt.exe" --debug "build\Debug\OceanBubble.exe"
```

**PowerShell**

PowerShell 不能直接以带空格的路径开头执行命令，路径必须加引号并用调用符 `&`：

```powershell
# Debug
& "D:\tool\QT 5.15.2\5.15.2\msvc2019_64\bin\windeployqt.exe" --debug "build\Debug\OceanBubble.exe"
```
---

## 五、运行命令
```bat
build\Debug\OceanBubble.exe
```

运行成功表现：出现标题为「海洋消消泡」的窗口，右侧预览栏可拖拽气泡到棋盘。

---

## 六、工程标识与配置存储

| 项 | 值 |
|---|---|
| 工程 / 可执行文件 | `OceanBubble` |
| 组织名 `OrganizationName` | `xuqi` |
| 应用名 `ApplicationName` | `OceanBubble` |
| 最高分存储位置 | 注册表 `HKEY_CURRENT_USER\Software\xuqi\OceanBubble` |
| 最高分键名 | `score/best` |

---

## 七、运行结果
![result](assets/image.png)
