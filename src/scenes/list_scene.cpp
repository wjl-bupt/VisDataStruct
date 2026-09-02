// ═══════════════════════════════════════════════════════════════
// 链表场景【示例实现】:完整的"录制 → 插值播放"流程。
//
// recordInsert() 里的插入逻辑是演示用的最简单实现;
// 你复习链表时,把 core/ds/linked_list.h 和这里的插入逻辑
// 换成自己的即可,录制/播放框架不用动。
// ═══════════════════════════════════════════════════════════════
#include "scenes/list_scene.h"

#define IMGUI_DEFINE_MATH_OPERATORS   // 启用 ImVec2 的 +-*/ 运算符(必须早于 imgui.h)
#include "imgui.h"
#include "imgui_internal.h"           // ImRect 在这里

#include <cmath>
#include <string>
#include <unordered_map>

namespace viz {

namespace {

constexpr float kRowY    = 320.0f;  // 链表节点行 y
constexpr float kFloatY  = 190.0f;  // 新节点悬浮行 y
constexpr float kX0      = 120.0f;  // 首个节点中心 x
constexpr float kSpacing = 110.0f;  // 节点间距
constexpr float kBoxW    = 64.0f;   // 节点框宽
constexpr float kBoxH    = 40.0f;   // 节点框高

// Color → ImU32(ImGui 绘制用)
ImU32 toImU32(Color c) {
    return IM_COL32(static_cast<int>(c.r * 255), static_cast<int>(c.g * 255),
                    static_cast<int>(c.b * 255), static_cast<int>(c.a * 255));
}

} // namespace

ListScene::ListScene() { resetDemo(); }

void ListScene::update(double dt) {
    player_.setSpeed(speed_);
    player_.update(dt);
}

float ListScene::nodeX(int index) const { return kX0 + index * kSpacing; }

int ListScene::idOf(const ds::ListNode* n) {
    // 节点的稳定 id:同一节点(按指针)在所有帧里 id 不变,
    // 播放器才能正确地对它做帧间插值(滑动/变色/淡入淡出)。
    auto it = idMap_.find(n);
    if (it != idMap_.end()) return it->second;
    int id = nextId_++;
    idMap_[n] = id;
    return id;
}

void ListScene::rebuildWorking() {
    // 每步从零重建工作帧:节点/边状态永远与真实链表一致(全部默认配色),
    // 之后由算法按需 markNode 高亮本步关心的节点。
    rec_.resetWorking();
    int i = 0;
    for (ds::ListNode* n = list_.head; n; n = n->next, ++i) {
        int id = idOf(n);
        rec_.setNode(id, nodeX(i), kRowY, std::to_string(n->data), Palette::NodeFill);
        if (n->next)
            rec_.setEdge(id, idOf(n->next), "", Palette::Edge);
    }
}

void ListScene::resetDemo() {
    list_.clear();
    idMap_.clear();
    nextId_ = 0;
    for (int v : {2, 4, 6, 8}) list_.pushBack(v);

    rec_.clear();
    rebuildWorking();
    rec_.commit("初始链表 [2, 4, 6, 8]");
    player_.setFrames(rec_.frames());
}

void ListScene::recordInsert(int val) {
    rec_.clear();
    rebuildWorking();
    rec_.commit("准备插入 " + std::to_string(val));

    // 1) 创建新节点,悬浮在链表上方(红色)
    ds::ListNode* fresh = new ds::ListNode{val, nullptr};
    int freshId = idOf(fresh);
    rec_.setNode(freshId, nodeX(0), kFloatY, std::to_string(val), Palette::Insert);
    rec_.commit("创建新节点 " + std::to_string(val) + ",开始定位插入位置");

    // 2) 遍历找插入位置:新节点跟着指针移动,当前比较节点高亮(橙色)
    ds::ListNode* prev = nullptr;
    ds::ListNode* cur = list_.head;
    int idx = 0;
    while (cur && cur->data < val) {
        rebuildWorking();
        rec_.setNode(freshId, nodeX(idx), kFloatY, std::to_string(val), Palette::Insert);
        rec_.markNode(idOf(cur), Palette::Active);
        rec_.commit(std::to_string(cur->data) + " < " + std::to_string(val) + ",指针后移");
        prev = cur;
        cur = cur->next;
        ++idx;
    }

    // 3) 找到位置
    rebuildWorking();
    rec_.setNode(freshId, nodeX(idx), kFloatY, std::to_string(val), Palette::Insert);
    if (cur) {
        rec_.markNode(idOf(cur), Palette::Active);
        std::string between = prev ? std::to_string(prev->data) + " 与 " + std::to_string(cur->data)
                                   : std::string("表头");
        rec_.commit(std::to_string(cur->data) + " >= " + std::to_string(val) + ",插在 " +
                    between + " 之间");
    } else {
        std::string where = prev ? std::to_string(prev->data) : std::string("空表");
        rec_.commit("已到表尾,插在 " + where + " 之后");
    }

    // 4) 执行插入(你的手写逻辑)。rebuild 后新节点坐标落到链表行,
    //    播放器对相邻帧插值,自然产生"滑入 + 指针重连"的动画
    fresh->next = cur;
    if (prev) prev->next = fresh;
    else list_.head = fresh;
    rebuildWorking();
    rec_.commit("插入节点 " + std::to_string(val) + ",重连指针");

    // 5) 完成:新节点标绿
    rebuildWorking();
    rec_.markNode(freshId, Palette::Success);
    rec_.commit("插入完成");

    player_.setFrames(rec_.frames());   // 交给播放器回放
}

void ListScene::draw() {
    // ── 控制面板 ──
    ImGui::SetNextItemWidth(80);
    ImGui::InputInt("要插入的值", &inputVal_);
    ImGui::SameLine();
    if (ImGui::Button("插入")) recordInsert(inputVal_);
    ImGui::SameLine();
    if (ImGui::Button("重置")) resetDemo();
    ImGui::SameLine();
    if (ImGui::Button(player_.playing() ? "暂停" : "播放")) player_.toggle();
    ImGui::SameLine();
    if (ImGui::Button("单步前进")) player_.stepForward();
    ImGui::SameLine();
    if (ImGui::Button("单步后退")) player_.stepBack();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("速度", &speed_, 0.25f, 4.0f, "%.2fx");

    ImGui::ProgressBar(static_cast<float>(player_.progress()));
    ImGui::Text("帧 %d / %d", player_.frameIndex() + 1, player_.frameCount());

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.30f, 1.0f), "步骤: %s",
                       player_.currentDesc().c_str());
    ImGui::Separator();

    // ── 画布:把插值快照画出来 ──
    ImGui::BeginChild("canvas", ImVec2(0, 0), false);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    Snapshot frame = player_.interpolated();

    // id → 屏幕坐标(节点中心)
    std::unordered_map<int, ImVec2> pos;
    for (const auto& n : frame.nodes)
        pos[n.id] = origin + ImVec2(n.x, n.y);

    // 先画边/指针(在节点下层)
    for (const auto& e : frame.edges) {
        ImVec2 a = pos[e.from] + ImVec2(kBoxW * 0.5f, 0);
        ImVec2 b = pos[e.to] - ImVec2(kBoxW * 0.5f, 0);
        ImU32 col = toImU32(e.color);
        dl->AddLine(a, b, col, 2.0f);
        // 箭头
        ImVec2 dir = b - a;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 1.0f) {
            dir = dir / len;
            ImVec2 perp(-dir.y, dir.x);
            ImVec2 base = b - dir * 12.0f;
            dl->AddTriangleFilled(b, base + perp * 5.0f, base - perp * 5.0f, col);
        }
        if (!e.weight.empty())
            dl->AddText((a + b) * 0.5f, col, e.weight.c_str());
    }
    // 再画节点
    for (const auto& n : frame.nodes) {
        ImVec2 p = pos[n.id];
        ImRect box(p - ImVec2(kBoxW * 0.5f, kBoxH * 0.5f),
                   p + ImVec2(kBoxW * 0.5f, kBoxH * 0.5f));
        dl->AddRectFilled(box.Min, box.Max, toImU32(n.color), 6.0f);
        dl->AddRect(box.Min, box.Max, toImU32(Palette::NodeBorder), 6.0f, 0, 1.5f);
        ImVec2 ts = ImGui::CalcTextSize(n.label.c_str());
        dl->AddText(ImVec2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f),
                    toImU32(Palette::NodeText), n.label.c_str());
    }
    ImGui::EndChild();
}

} // namespace viz
