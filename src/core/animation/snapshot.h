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

// 场景通用配色(卡通浅色主题;描边/阴影由 draw 侧用 Darken 派生)
namespace Palette {
inline const Color NodeFill   = Color::make(0.66f, 0.85f, 0.94f); // 普通节点(粉彩蓝)
inline const Color NodeText   = Color::make(0.20f, 0.24f, 0.33f); // 节点文字(深蓝灰)
inline const Color Edge       = Color::make(0.71f, 0.60f, 0.47f); // 边/指针(暖棕)
inline const Color Active     = Color::make(1.00f, 0.81f, 0.36f); // 当前访问/比较(粉彩黄)
inline const Color Insert     = Color::make(1.00f, 0.57f, 0.53f); // 新插入(粉彩红)
inline const Color Success    = Color::make(0.55f, 0.87f, 0.62f); // 完成(粉彩绿)
} // namespace Palette

// 同色系加深:卡通贴纸风的描边与硬阴影 = 填充色的深色版(k 取 0~1)
inline Color Darken(const Color& c, float k) {
    return Color::make(c.r * k, c.g * k, c.b * k, c.a);
}

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
    struct Band {
        int x0, x1;          // 覆盖的槽位区间(场景坐标系换算,如排序的下标区间)
        std::string label;   // 区间标注文字
        Color color;         // 半透明底色
    };
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<Band> bands; // 区间带(排序场景展示拆分/归并范围)
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
