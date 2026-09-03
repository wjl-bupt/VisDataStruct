# 数据结构可视化复习项目 (C++)

用 **C++ 手写全部数据结构与算法**,通过 **Dear ImGui + SDL2** 把它们**动态可视化**出来——链表的插入、树的旋转、图的遍历、排序的比较交换,每一步都有动画和文字说明,可播放/暂停/单步/调速。

## 需求与方案的对应

| 你的要求 | 方案 |
|---|---|
| [1] C++ 实现 | 纯 C++17 项目,无第二门语言 |
| [2] 动态可视化 | 快照 + 插值播放器:算法的每一步录制成快照,播放器在相邻快照间插值,产生连续动画 |
| [3] 算法全部手写 | 数据结构(`core/ds/`)和布局算法(`layout/`)全部留给你实现;框架只提供录制与渲染 |

## 快速开始

依赖(Ubuntu / Deepin):

```bash
sudo apt install cmake g++ fonts-noto-cjk
```

构建与运行:

```bash
bash scripts/build_sdl2.sh      # 一次性:本地静态编译 SDL 2.30(约 2~3 分钟)
cmake -B build
cmake --build build -j
./build/ds_vis
```

依赖说明:

- **SDL2 为什么本地编译**:imgui 1.91+ 的 SDL2 渲染后端要求 SDL ≥ 2.0.17,而 Ubuntu 20.04 系统包只有 2.0.10。`scripts/build_sdl2.sh` 从 libsdl.org 下载 SDL 2.30.9 静态编译到 `third_party/sdl2/`,最终静态链接进二进制——不动系统库,运行时零依赖。
- **imgui** 由 CMake FetchContent 自动拉取(固定 v1.91.9b)。国内网络默认走 Gitee 镜像,如需换源:
  `cmake -B build -DIMGUI_GIT_URL=https://github.com/ocornut/imgui.git`

## 远程浏览器可视化(SSH 连服务器开发用)

无法在服务器本机看桌面时,用这套方案:**程序画在容器里的虚拟显示器上,你通过浏览器观看**。链路:

```
程序 ──DISPLAY=:1──▶ Xvfb(虚拟显示器) ──▶ x11vnc ──▶ websockify/noVNC ──▶ 你的浏览器
```

### 一次性环境(容器内,已配好则跳过)

容器已装好 `xvfb`、`x11vnc`、`novnc`。若在新环境重做:

```bash
sudo apt install xvfb x11vnc novnc
```

### 日常使用:一条命令

```bash
bash scripts/run.sh               # 启动/恢复环境并拉起程序(幂等,重复执行安全)
```

- 活着的组件不会动,只补缺失的;`ds_vis` 以脱离终端的方式启动,**断开 SSH 不受影响**
- 重新编译后想看新版本:`bash scripts/run.sh --restart`(只重启程序,服务栈不动)
- 自定义 VNC 密码:`bash scripts/run.sh --restart 我的密码`

脚本末尾有健康检查,缺什么会直接提示去看哪个日志。

### 手动启停(一般用不到)

```bash
bash scripts/start_remote_vis.sh  # 只启动服务栈(不拉起程序)
bash scripts/stop_remote_vis.sh   # 完整停止:程序 + 服务栈
```

### 本地电脑访问(关键:SSH 端口转发)

websockify 监听在**容器**的 6080 端口,而你的 SSH 登录在**服务器**上——中间隔着 docker 网络,转发目标有两种情况,先试 ①,不通再试 ②:

```bash
# ① 若容器启动时做了端口映射(docker run -p 6080:6080),转发服务器本机:
ssh -L 6080:localhost:6080 用户名@服务器地址

# ② 若没做映射,直接转发到容器 IP(容器与服务器同在 docker0 网桥,服务器可达):
#    容器 IP 用 hostname -I 查看(如 172.17.0.12,注意重启后会变)
ssh -L 6080:172.17.0.12:6080 用户名@服务器地址
```

保持这个 SSH 连接不断。然后**本地浏览器**打开:

```
http://localhost:6080/vnc.html
```

输入 VNC 密码(`ds2026`),就能看到一个 1280×800 的虚拟桌面。如果用 Mac 内置 VNC Viewer,也可直接转 `5901` 端口(同样按上面两种情况选目标):`ssh -L 5901:localhost:5901 ...`。

### 运行程序

程序由 `run.sh` 自动拉起(已脱离终端);若要手动在前台跑(日志直出):

```bash
DISPLAY=:1 ./build/ds_vis
```

窗口会出现在浏览器的虚拟桌面上。动画卡的话调小 noVNC 的画质或本地 `ssh -L` 加 `-C` 开启压缩。

### 关闭服务

```bash
bash scripts/stop_remote_vis.sh
```

> 说明:这套方案比 X11 转发(`ssh -X`)在跨网络时更流畅——X11 转发是每帧全量走网络,而 VNC/noVNC 只传压缩后的画面差分。代价是需要在容器里跑一组后台服务,已封装成上面的脚本。


## 动画架构:快照 + 插值播放

这是整个项目的核心设计,一句话:**算法负责录制,渲染层负责播放。**

```
你的算法(纯数据结构代码,零绘图污染)
        │  每一步调用 Recorder 记录
        ▼
Recorder ──▶ vector<Snapshot>   完整的步骤时间线
        │
        ▼
Player:在 Snapshot[i] 与 Snapshot[i+1] 之间做位置/颜色插值
        │
        ▼
ImGui 每帧绘制插值画面  →  动态动画
```

### 三个核心类(框架已实现,直接使用)

| 类 | 职责 | 位置 |
|---|---|---|
| `Snapshot` | 一帧画面的全部信息:节点(id/坐标/文字/颜色)、边、本步说明 | `src/core/animation/snapshot.h` |
| `Recorder` | 录制器:`setNode` / `setEdge` / `markNode` 修改工作帧,`commit(说明)` 压入时间线 | `src/core/animation/recorder.h` |
| `Player` | 播放器:插值、播放/暂停/单步前进/后退、调速、进度 | `src/core/animation/player.h` |

### 关键设计点

- **节点 `id` 是稳定身份**——播放器靠它匹配相邻帧的节点,决定谁移动、谁变色、谁消失。id 必须跨帧稳定(见链表示例场景里 `idOf()` 的做法)。
- **每步"全量重建 + 局部高亮"**——先把结构当前状态按默认配色重建进工作帧,再 `markNode` 标出本步关心的节点,最后 `commit`。这个模式简单、不易出错,建议沿用。
- **交互操作 = 瞬时跑算法 + 回放动画**——点"插入"后算法一次跑完、录下几十帧,再由播放器动画回放,不需要把算法写成可挂起的状态机。

### 最小示例(链表插入)

```cpp
void ListScene::recordInsert(int val) {
    rec_.clear();
    rebuildWorking();                                    // 帧1:当前链表
    rec_.commit("准备插入 " + std::to_string(val));

    // ... 遍历找位置,每步:rebuildWorking() → markNode(当前节点, 高亮色) → commit("xx < xx,后移")

    // 真正执行插入(你的手写逻辑)
    fresh->next = cur;
    prev->next = fresh;
    rebuildWorking();                                    // 新节点已在链表中 → 坐标变化
    rec_.commit("插入节点,重连指针");                    // 播放器自动把"滑入"过程动画化

    player_.setFrames(rec_.frames());                    // 交给播放器回放
}
```

完整写法见 `src/scenes/list_scene.cpp`(骨架自带的参考实现)。

## 目录结构

```
.
├── CMakeLists.txt
├── third_party/sdl2/           # 构建产物:本地静态 SDL(build_sdl2.sh 生成,可删可重建)
├── scripts/
│   ├── build_sdl2.sh           # 一次性:本地静态编译 SDL 2.30
│   ├── start_remote_vis.sh     # 启动 Xvfb+x11vnc+noVNC 远程浏览器服务
│   └── stop_remote_vis.sh      # 停止上述服务
└── src/
    ├── main.cpp                 # SDL2 窗口 + ImGui 渲染循环 + 场景 Tab 切换
    ├── core/
    │   ├── animation/           # ★ 动画框架(已完成,直接用)
    │   │   ├── snapshot.h
    │   │   ├── recorder.h
    │   │   └── player.h
    │   └── ds/                  # ★ 你的战场:手写数据结构(纯算法,不含绘图)
    │       └── linked_list.h    #   【示例】演示用,复习时替换成你自己的实现
    ├── layout/                  # ★ 你的战场:手写布局算法
    │   ├── tree_layout.h        #   【示例】中序列号层次布局,复习时重写
    │   └── graph_layout.h       #   【待实现】力导向布局
    └── scenes/                  # 每个数据结构一个场景(录制 + 绘制)
        ├── scene.h              # 场景基类
        ├── list_scene.h/.cpp    # 【示例】链表场景:插入动画,完整演示流程
        └── ...
```

## 如何新增一个场景

以"二叉树"为例,五步:

1. **写数据结构** — 在 `core/ds/` 建 `binary_tree.h`,实现你的树(节点、插入、删除、遍历……)。**纯算法,禁止出现任何绘图/动画代码**。
2. **写布局** — 在 `layout/` 实现 `tree_layout.h`:递归层次布局,算出每个节点的 `(x, y)`。
3. **建场景** — 在 `scenes/` 建 `tree_scene.h/.cpp`,继承 `Scene`,实现 `name()` / `update()` / `draw()`。
4. **写录制逻辑** — 每个操作(插入/删除/旋转/遍历)里:布局全部节点 → `setNode`/`setEdge` 写进 Recorder → `markNode` 高亮本步节点 → `commit("步骤说明")`,最后 `player_.setFrames(rec_.frames())`。
5. **注册** — 在 `main.cpp` 的 `scenes` 列表里 `emplace_back` 你的场景,自动出现在 Tab 栏。

对照 `list_scene.cpp` 抄一遍流程即可。

> **框架细节(已处理,勿破坏)**:`main.cpp` 已把 Tab 栏与场景绘制包在全屏主窗口(`ImGui::Begin("##main", ...)`)里。场景的 `draw()` 内直接画控件 + `BeginChild` 画布即可,**不要**再自己 `Begin` 顶层窗口——ImGui 会改用自动收缩的 "Debug" 回退窗口,画布 `BeginChild(ImVec2(0,0))` 拿到 0 高度,节点全部被裁掉不显示。

## 路线图

| 模块 | 可视化方案 | 状态 |
|---|---|---|
| 链表(单/双/循环) | 节点框 + 指针箭头,插入/删除动画 | ✅ 已完成(插入动画已端到端验证) |
| 顺序表 | 数组格子 + 下标,插入/删除的搬移动画 | 待实现 |
| 栈 / 队列 | 槽位,进出动画 | 待实现 |
| 二叉树 / BST / AVL | 递归层次布局,遍历路径高亮,**旋转动画** | ✅ 建树/遍历/深度/宽度/翻转/LCA 已接入用户 BinaryTree;BST 插入为演示实现;AVL 待实现 |
| 堆 | 树视图 + 数组视图联动,上浮/下沉交换 | 待实现 |
| 图 | 力导向布局,DFS/BFS 染色,Dijkstra 松弛边 | 待实现 |
| 排序(冒泡~基数) | 柱状图,比较/交换高亮 | ✅ 已接入 7 种:冒泡/改进冒泡/选择/插入/归并(区间带)/快排(显式栈)/计数(负数支持);希尔/堆/基数/桶待实现 |
| 哈希表 | 桶 + 拉链,冲突动画 | 待实现 |
| KMP | 主串/模式串双指针,失配回退 | 待实现 |

## 约定

- **算法全部手写**:`core/ds/`(数据结构)与 `layout/`(布局算法)是复习的主战场;`core/animation/`、`main.cpp` 是给你用的框架,可随意修改。
- **示例代码可替换**:`linked_list.h` 和 `list_scene.cpp` 是演示可视化框架的最小实现,复习对应内容时请替换成你自己的实现。
- **C++17**(本机 g++ 9.4,不支持 C++20 协程,也无必要)。
- 中文显示依赖系统中文字体(Noto CJK / 文泉驿);找不到字体时界面中文会显示为方块,此时安装 `fonts-noto-cjk` 即可。
