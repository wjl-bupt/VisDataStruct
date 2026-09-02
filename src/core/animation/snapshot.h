// ═══════════════════════════════════════════════════════════════
// 快照:一帧画面所需的全部信息。
// 数据结构本身不感知动画;算法通过 Recorder 把"每一步长什么样"
// 记录成快照序列,Player 再在相邻快照之间插值播放。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <vector>

namespace viz {

struct Color {
    float r, g, b, a = 1.0f;
    static Color make(float r, float g, float b, float a = 1.0f) {
        return Color{r, g, b, a};
    }
};

// 场景通用配色(各场景可自行扩充)
namespace Palette {
inline const Color NodeFill   = Color::make(0.23f, 0.27f, 0.32f); // 普通节点
inline const Color NodeBorder = Color::make(0.60f, 0.66f, 0.72f); // 节点边框
inline const Color NodeText   = Color::make(0.95f, 0.96f, 0.98f); // 节点文字
inline const Color Edge       = Color::make(0.55f, 0.58f, 0.62f); // 普通边/指针
inline const Color Active     = Color::make(1.00f, 0.62f, 0.18f); // 当前访问/比较(橙)
inline const Color Insert     = Color::make(0.91f, 0.33f, 0.33f); // 新插入(红)
inline const Color Success    = Color::make(0.36f, 0.78f, 0.42f); // 完成(绿)
} // namespace Palette

struct Snapshot {
    struct Node {
        int id;              // 节点稳定身份:播放器靠 id 做帧间匹配,必须跨帧稳定
        float x, y;          // 布局坐标(场景坐标系,绘制时再加画布偏移)
        std::string label;   // 节点文字(值/键)
        Color color;         // 节点填充色
    };
    struct Edge {
        int from, to;        // 两端节点 id
        std::string weight;  // 权重等附加文字,可为空
        Color color;
    };
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::string desc;        // 本步骤说明,播放时显示
};

// 颜色线性插值(Player 用)
inline Color lerpColor(const Color& a, const Color& b, float t) {
    return Color{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t,
    };
}

} // namespace viz
