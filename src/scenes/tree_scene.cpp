// ═══════════════════════════════════════════════════════════════
// 二叉树场景【示例实现】:BST 插入 + 三种遍历的录制流程。
// 核心模式与 list_scene 相同:rebuild 全量重建 → markNode 高亮 → commit。
// 树形布局每帧重算,节点移动/子树平移由播放器插值自动动画化。
// ═══════════════════════════════════════════════════════════════
#define IMGUI_DEFINE_MATH_OPERATORS
#include "scenes/tree_scene.h"

#include "imgui.h"
#include "imgui_internal.h"   // ImRect

#include <cmath>
#include <string>

namespace viz {

namespace {

constexpr float kBoxW   = 56.0f;   // 节点框宽
constexpr float kBoxH   = 36.0f;   // 节点框高
constexpr float kFloatY = 42.0f;   // 新节点悬浮行 y

ImU32 toImU32(Color c) {
    return IM_COL32(static_cast<int>(c.r * 255), static_cast<int>(c.g * 255),
                    static_cast<int>(c.b * 255), static_cast<int>(c.a * 255));
}

} // namespace

void TreeScene::update(double dt) {
    player_.setSpeed(speed_);
    player_.update(dt);
}

int TreeScene::idOf(const ds::TreeNode* n) {
    // 与 list_scene 相同:按指针给节点稳定 id,播放器据此做帧间插值
    auto it = idMap_.find(n);
    if (it != idMap_.end()) return it->second;
    int id = nextId_++;
    idMap_[n] = id;
    return id;
}

void TreeScene::destroyTree(ds::TreeNode* n) {
    if (!n) return;
    destroyTree(n->lchild);
    destroyTree(n->rchild);
    delete n;
}

void TreeScene::layoutAndRebuild() {
    pos_ = layout_.compute(root_);
    rec_.resetWorking();
    // 先序遍历写节点与父子边(顺序无关紧要,只是全量重建)
    // 用显式栈避免递归里夹带录制逻辑
    std::vector<const ds::TreeNode*> st;
    if (root_) st.push_back(root_);
    while (!st.empty()) {
        const ds::TreeNode* n = st.back(); st.pop_back();
        Vec2 p = pos_[n];
        rec_.setNode(idOf(n), p.x, p.y, std::to_string(n->val), Palette::NodeFill);
        if (n->lchild) { rec_.setEdge(idOf(n), idOf(n->lchild), "", Palette::Edge); st.push_back(n->lchild); }
        if (n->rchild) { rec_.setEdge(idOf(n), idOf(n->rchild), "", Palette::Edge); st.push_back(n->rchild); }
    }
}

void TreeScene::drawFloating(int id, const ds::TreeNode* anchor, const std::string& label) {
    // 新节点悬浮在"当前比较节点"上方,随搜索路径移动
    float x = anchor ? pos_[anchor].x : 600.0f;
    rec_.setNode(id, x, kFloatY, label, Palette::Insert);
}

void TreeScene::resetDemo() {
    destroyTree(root_);
    root_ = nullptr;
    idMap_.clear();
    nextId_ = 0;
    for (int v : {5, 3, 8, 1, 4, 7, 9}) {          // 与插入动画一致的 BST 逐个插入
        ds::TreeNode** slot = &root_;
        while (*slot) slot = (v < (*slot)->val) ? &(*slot)->lchild : &(*slot)->rchild;
        *slot = new ds::TreeNode(v);
    }
    layoutAndRebuild();
    rec_.commit("示例 BST [5,3,8,1,4,7,9]");
    player_.setFrames(rec_.frames());
}

void TreeScene::recordInsert(int val) {
    rec_.clear();
    layoutAndRebuild();
    rec_.commit("准备插入 " + std::to_string(val));

    ds::TreeNode* fresh = new ds::TreeNode(val);
    const int freshId = idOf(fresh);

    // 1) 沿搜索路径下行:当前节点高亮橙色,新节点悬浮跟随
    ds::TreeNode* cur = root_;
    ds::TreeNode* parent = nullptr;
    bool duplicate = false;
    while (cur) {
        layoutAndRebuild();
        drawFloating(freshId, cur, std::to_string(val));
        rec_.markNode(idOf(cur), Palette::Active);
        if (val < cur->val) {
            rec_.commit(std::to_string(val) + " < " + std::to_string(cur->val) + ",走左子树");
            parent = cur; cur = cur->lchild;
        } else if (val > cur->val) {
            rec_.commit(std::to_string(val) + " > " + std::to_string(cur->val) + ",走右子树");
            parent = cur; cur = cur->rchild;
        } else {
            rec_.commit("值 " + std::to_string(val) + " 已存在,不插入");
            duplicate = true;
            break;
        }
    }

    if (duplicate) {
        idMap_.erase(fresh);                        // 未使用,回收 id 映射
        delete fresh;
        player_.setFrames(rec_.frames());
        return;
    }

    // 2) 挂接新节点(你的手写逻辑;此处为演示用的最小 BST 插入)
    if (!parent) root_ = fresh;
    else if (val < parent->val) parent->lchild = fresh;
    else parent->rchild = fresh;

    // 3) 悬浮帧:新节点悬在插入位置上方,父节点高亮(空树直接落根,无此帧)
    if (parent) {
        layoutAndRebuild();
        drawFloating(freshId, parent, std::to_string(val));
        rec_.markNode(idOf(parent), Palette::Active);
        rec_.commit(std::string("找到空位,挂在 ") + std::to_string(parent->val) +
                    (val < parent->val ? " 的左孩子" : " 的右孩子"));
    }

    // 4) 落位:新节点已入树,布局重算 → 播放器自动生成"滑入 + 子树让位"动画
    layoutAndRebuild();
    rec_.markNode(freshId, Palette::Insert);
    rec_.commit("插入节点 " + std::to_string(val) + ",重连指针");

    // 5) 完成
    layoutAndRebuild();
    rec_.markNode(freshId, Palette::Success);
    rec_.commit("插入完成");

    player_.setFrames(rec_.frames());
}

void TreeScene::recordTraversal(int order) {
    static const char* kNames[] = {"先序", "中序", "后序"};
    rec_.clear();
    layoutAndRebuild();
    rec_.commit(std::string("开始") + kNames[order] + "遍历(高亮顺序即访问顺序)");

    std::vector<int> seq;                           // 已访问序列,写进步骤说明
    std::vector<const ds::TreeNode*> visited;       // 已访问节点,染绿

    // 递归遍历,在"访问"时刻录帧。注意:算法本身仍是纯遍历,
    // 只是访问动作换成了录制(结构只读,不改树)。
    // order: 0 先序 / 1 中序 / 2 后序
    // 用 lambda 递归(C++17 直接 auto self)
    auto visit = [&](const ds::TreeNode* n) {
        layoutAndRebuild();
        for (const auto* v : visited) rec_.markNode(idOf(v), Palette::Success);
        rec_.markNode(idOf(n), Palette::Active);
        seq.push_back(n->val);
        std::string s = "访问 " + std::to_string(n->val) + ",输出: [";
        for (size_t i = 0; i < seq.size(); ++i)
            s += std::to_string(seq[i]) + (i + 1 < seq.size() ? ", " : "");
        s += "]";
        rec_.commit(s);
        visited.push_back(n);
    };
    auto walk = [&](auto&& self, const ds::TreeNode* n) -> void {
        if (!n) return;
        if (order == 0) visit(n);
        self(self, n->lchild);
        if (order == 1) visit(n);
        self(self, n->rchild);
        if (order == 2) visit(n);
    };
    walk(walk, root_);

    layoutAndRebuild();
    for (const auto* v : visited) rec_.markNode(idOf(v), Palette::Success);
    std::string s = std::string(kNames[order]) + "遍历完成: [";
    for (size_t i = 0; i < seq.size(); ++i)
        s += std::to_string(seq[i]) + (i + 1 < seq.size() ? ", " : "");
    s += "]";
    rec_.commit(s);

    player_.setFrames(rec_.frames());
}

void TreeScene::draw() {
    // ── 控制面板 ──
    ImGui::SetNextItemWidth(80);
    ImGui::InputInt("##insval", &inputVal_);
    ImGui::SameLine();
    if (ImGui::Button("插入")) recordInsert(inputVal_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::Combo("##order", &travOrder_, "先序\0中序\0后序\0");
    ImGui::SameLine();
    if (ImGui::Button("遍历")) recordTraversal(travOrder_);
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

    // ── 画布 ──
    ImGui::BeginChild("canvas", ImVec2(0, 0), false);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    Snapshot frame = player_.interpolated();

    std::unordered_map<int, ImVec2> pos;
    for (const auto& n : frame.nodes)
        pos[n.id] = origin + ImVec2(n.x, n.y);

    // 先画边(父子连线,在节点下层)
    for (const auto& e : frame.edges) {
        auto a = pos.find(e.from), b = pos.find(e.to);
        if (a == pos.end() || b == pos.end()) continue;
        dl->AddLine(a->second, b->second, toImU32(e.color), 2.0f);
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
