// ═══════════════════════════════════════════════════════════════
// 场景基类:每个数据结构对应一个场景(链表/栈/树/图/排序…)。
//
// 新增场景三步:继承 Scene → 实现 3 个虚函数 → main.cpp 里注册。
// 完整示例见 list_scene.h / list_scene.cpp。
// ═══════════════════════════════════════════════════════════════
#pragma once

namespace viz {

class Scene {
public:
    virtual ~Scene() = default;

    virtual const char* name() const = 0;  // Tab 标题
    virtual void update(double dt) = 0;    // 每帧逻辑(通常只调 player_.update(dt))
    virtual void draw() = 0;               // 每帧绘制(ImGui 控件 + 画布)
};

} // namespace viz
