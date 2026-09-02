// ═══════════════════════════════════════════════════════════════
// 二叉树场景【示例实现】:数值列表建树 + BST 单值插入 + 三种遍历。
// 核心模式与 list_scene 相同:rebuild 全量重建 → markNode 高亮 → commit。
// 列表建树与单值插入共用 recordInsertFrames():一次插入 = 一段帧序列。
// 树形布局每帧重算,节点移动/子树平移由播放器插值自动动画化。
// ═══════════════════════════════════════════════════════════════
#define IMGUI_DEFINE_MATH_OPERATORS
#include "scenes/tree_scene.h"

#include "imgui.h"
#include "imgui_internal.h"   // ImRect
#include "ui/theme.h"         // NodeFont(粗体节点标签)

#include <cmath>
#include <cctype>
#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>

namespace viz {

namespace {

constexpr float kBoxW   = 56.0f;   // 节点框宽
constexpr float kBoxH   = 36.0f;   // 节点框高
constexpr float kFloatY = 42.0f;   // 新节点悬浮行 y
constexpr float kRootFloatX = 600.0f; // 空树插入时新节点悬浮 x

ImU32 toImU32(Color c) {
    return IM_COL32(static_cast<int>(c.r * 255), static_cast<int>(c.g * 255),
                    static_cast<int>(c.b * 255), static_cast<int>(c.a * 255));
}

// 解析层序数组:数字为节点,# 为空位;其他非数字字符一律当分隔符
// (中文标点是多字节 UTF-8,逐字节替换成空格即可,不影响解析)
std::vector<std::optional<int>> parseListOpt(const char* text) {
    std::string t = text ? text : "";
    for (char& c : t)
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-' && c != '#') c = ' ';
    std::vector<std::optional<int>> out;
    std::istringstream is(t);
    std::string tok;
    while (is >> tok) {
        if (tok == "#") { out.emplace_back(std::nullopt); continue; }
        try {
            size_t used = 0;
            int v = std::stoi(tok, &used);
            if (used == tok.size()) out.emplace_back(v);   // 整段都是数字才算
        } catch (...) { /* 非数字,跳过 */ }
    }
    return out;
}

std::string joinList(const std::vector<std::optional<int>>& vals) {
    std::string s = "[";
    for (size_t i = 0; i < vals.size(); ++i) {
        s += vals[i] ? std::to_string(*vals[i]) : "#";
        if (i + 1 < vals.size()) s += ", ";
    }
    return s + "]";
}

std::string joinVals(const std::vector<int>& vals) {
    std::string s = "[";
    for (size_t i = 0; i < vals.size(); ++i)
        s += std::to_string(vals[i]) + (i + 1 < vals.size() ? ", " : "");
    s += "]";
    return s;
}

// 完全二叉树下标 → 层数:level k 覆盖下标 [2^k − 1, 2^(k+1) − 2]
int levelOfIndex(long long idx) {
    int lv = 0;
    long long span = 1;
    while (idx >= 2 * span - 1) { ++lv; span <<= 1; }
    return lv;
}

// 按值找节点(层序找第一个匹配);找不到返回 nullptr
ds::TreeNode* findValue(ds::TreeNode* r, int v) {
    std::vector<const ds::TreeNode*> st;
    if (r) st.push_back(r);
    while (!st.empty()) {
        const ds::TreeNode* n = st.back(); st.pop_back();
        if (n->val == v) return const_cast<ds::TreeNode*>(n);
        if (n->lchild) st.push_back(n->lchild);
        if (n->rchild) st.push_back(n->rchild);
    }
    return nullptr;
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

void TreeScene::markGreen() {
    for (int gid : greenIds_) rec_.markNode(gid, Palette::Success);
}

void TreeScene::layoutAndRebuild() {
    pos_ = layout_.compute(root_);
    rec_.resetWorking();
    // 先序遍历写节点与父子边(顺序无关紧要,只是全量重建)
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
    float x = anchor ? pos_[anchor].x : kRootFloatX;
    rec_.setNode(id, x, kFloatY, label, Palette::Insert);
}

// ── 一次插入的完整帧序列(列表建树与单值插入共用)──────────────
// 前置:rec_ 已 clear。成功后 freshId 记入 greenIds_。重复值返回 false。
bool TreeScene::recordInsertFrames(int val) {
    const std::string sv = std::to_string(val);
    ds::TreeNode* fresh = new ds::TreeNode(val);
    const int freshId = idOf(fresh);

    // 0) 起始帧
    layoutAndRebuild(); markGreen();
    rec_.commit("准备插入 " + sv);

    // 1) 沿搜索路径下行:当前节点橙色高亮,新节点悬浮跟随
    ds::TreeNode* cur = root_;
    ds::TreeNode* parent = nullptr;
    bool duplicate = false;
    while (cur) {
        layoutAndRebuild(); markGreen();
        drawFloating(freshId, cur, sv);
        rec_.markNode(idOf(cur), Palette::Active);
        if (val < cur->val) {
            rec_.commit(sv + " < " + std::to_string(cur->val) + ",走左子树");
            parent = cur; cur = cur->lchild;
        } else if (val > cur->val) {
            rec_.commit(sv + " > " + std::to_string(cur->val) + ",走右子树");
            parent = cur; cur = cur->rchild;
        } else {
            rec_.commit("值 " + sv + " 已存在,不插入");
            duplicate = true;
            break;
        }
    }
    if (duplicate) { idMap_.erase(fresh); delete fresh; return false; }

    // 2) 挂接新节点(演示用的最小 BST 插入;可换成你自己的实现)
    if (!parent) root_ = fresh;
    else if (val < parent->val) parent->lchild = fresh;
    else                        parent->rchild = fresh;

    // 3) 悬浮帧:新节点悬在插入位置上方,父节点高亮(空树直接成根,悬浮在中央)
    layoutAndRebuild(); markGreen();
    if (parent) {
        drawFloating(freshId, parent, sv);
        rec_.markNode(idOf(parent), Palette::Active);
        rec_.commit("找到空位,挂在 " + std::to_string(parent->val) +
                    (val < parent->val ? " 的左孩子" : " 的右孩子"));
    } else {
        rec_.setNode(freshId, kRootFloatX, kFloatY, sv, Palette::Insert);
        rec_.commit("空树," + sv + " 作为根节点");
    }

    // 4) 落位:布局重算 → 播放器自动生成"滑入 + 子树让位"动画
    layoutAndRebuild(); markGreen();
    rec_.markNode(freshId, Palette::Insert);
    rec_.commit("插入节点 " + sv + ",重连指针");
    greenIds_.push_back(freshId);
    return true;
}

// ── 层序数组建树:调用你的 BinaryTree::createbinarytree,onAttach 里逐挂接录帧 ──
void TreeScene::recordBuild(const std::vector<std::optional<int>>& vals) {
    rec_.clear();
    greenIds_.clear();

    if (vals.empty()) {                    // 解析不出内容:保留当前树,只提示
        layoutAndRebuild();
        rec_.commit("列表为空,未建树");
        player_.setFrames(rec_.frames());
        return;
    }
    if (!vals[0]) {                        // 你的建树算法要求根位置有节点
        layoutAndRebuild();
        rec_.commit("首元素为空位(#),无法建树");
        player_.setFrames(rec_.frames());
        return;
    }

    idMap_.clear();
    nextId_ = 0;

    // 按层序数组创建节点,# 位置传 nullptr
    std::vector<ds::TreeNode*> nodes;
    int created = 0;
    for (const auto& v : vals) {
        nodes.push_back(v ? new ds::TreeNode(*v) : nullptr);
        if (v) ++created;
    }

    // 树的释放由 tree_ 负责:createbinarytree 开头的 clear() 会删掉旧树再挂新树
    std::unordered_set<const ds::TreeNode*> attached;
    int placed = 0;
    root_ = tree_.createbinarytree(nodes, [&](ds::TreeNode* parent, ds::TreeNode* child) {
        root_ = tree_.getRoot();           // 建树中途,布局要用当前的部分树
        attached.insert(child);
        layoutAndRebuild(); markGreen();
        const int cid = idOf(child);
        rec_.markNode(cid, Palette::Insert);
        rec_.commit(parent ? ("把 " + std::to_string(child->val) + " 挂为 " +
                              std::to_string(parent->val) +
                              (parent->lchild == child ? " 的左孩子" : " 的右孩子"))
                           : (std::to_string(child->val) + " 作为根节点"));
        greenIds_.push_back(cid);
        ++placed;
        layoutAndRebuild(); markGreen();
        rec_.commit("已就位 " + std::to_string(child->val) +
                    " (" + std::to_string(placed) + "/" + std::to_string(created) + ")");
    });

    // 回收因 # 占位而没被挂上的节点(如 # 的孩子位置写了数字)
    for (ds::TreeNode* n : nodes)
        if (n && !attached.count(n)) delete n;

    root_ = tree_.getRoot();
    layoutAndRebuild(); markGreen();
    rec_.commit("建树完成,共 " + std::to_string(greenIds_.size()) + " 个节点(层序数组语义)");
    player_.setFrames(rec_.frames());
}

// ── 单值插入:清空录制后复用一次插入的帧序列 ────────────────────
void TreeScene::recordInsert(int val) {
    rec_.clear();
    greenIds_.clear();
    if (recordInsertFrames(val)) {
        layoutAndRebuild(); markGreen();
        rec_.commit("插入完成");
    }
    player_.setFrames(rec_.frames());
}

void TreeScene::resetDemo() { recordBuild(parseListOpt(listBuf_)); }

void TreeScene::recordTraversal(int order) {
    static const char* kNames[] = {"先序", "中序", "后序", "层序"};
    rec_.clear();
    greenIds_.clear();
    root_ = tree_.getRoot();            // 以当前树为准
    layoutAndRebuild();
    rec_.commit(std::string("调用你的 BinaryTree::") + kNames[order] + "Traversal(高亮顺序即访问顺序)");

    // 用你的遍历函数收集访问序列:遍历结构 = 你的算法,场景只负责在每步录帧。
    // 注意后序实现的 onVisit 发生在 reverse 之前(根→右→左),这里逆序回放真实后序
    std::vector<ds::TreeNode*> visits;
    const auto collect = [&](ds::TreeNode* n) { visits.push_back(n); };
    if (order == 0)      tree_.PreOrderTraversal(collect);
    else if (order == 1) tree_.InOrderTraversal(collect);
    else if (order == 2) { tree_.PostorderTraversal(collect);
                           std::reverse(visits.begin(), visits.end()); }
    else tree_.LevelOrderTraversal(collect);


    std::vector<int> seq;                           // 已访问序列,写进步骤说明
    std::vector<const ds::TreeNode*> visited;       // 已访问节点,染绿
    for (const auto* n : visits) {
        layoutAndRebuild();
        for (const auto* v : visited) rec_.markNode(idOf(v), Palette::Success);
        rec_.markNode(idOf(n), Palette::Active);
        seq.push_back(n->val);
        rec_.commit("访问 " + std::to_string(n->val) + ",输出: " + joinVals(seq));
        visited.push_back(n);
    }

    layoutAndRebuild();
    for (const auto* v : visited) rec_.markNode(idOf(v), Palette::Success);
    rec_.commit(std::string(kNames[order]) + "遍历完成: " + joinVals(seq));

    player_.setFrames(rec_.frames());
}

// ── 深度:调用你的 GetDepth,逐层染色(已完成层绿色,当前层黄色)────────
void TreeScene::recordDepth() {
    rec_.clear();
    greenIds_.clear();
    root_ = tree_.getRoot();
    layoutAndRebuild();
    rec_.commit("求深度:逐层扫描(调用你的 BinaryTree::GetDepth)");

    std::vector<int> doneIds;            // 已扫完层的节点 id(绿)
    std::vector<int> levelIds;           // 当前层节点 id(黄)
    int level = 0;
    tree_.GetDepth([&](ds::TreeNode* n, int lv) {
        if (lv != level) {               // 换层:上一层收尾染绿
            layoutAndRebuild();
            for (int id : doneIds)  rec_.markNode(id, Palette::Success);
            for (int id : levelIds) rec_.markNode(id, Palette::Success);
            rec_.commit("第 " + std::to_string(level) + " 层扫完,共 " +
                        std::to_string(levelIds.size()) + " 个节点");
            doneIds.insert(doneIds.end(), levelIds.begin(), levelIds.end());
            levelIds.clear();
            level = lv;
        }
        layoutAndRebuild();
        for (int id : doneIds)  rec_.markNode(id, Palette::Success);
        for (int id : levelIds) rec_.markNode(id, Palette::Active);
        rec_.markNode(idOf(n), Palette::Insert);          // 当前节点红
        levelIds.push_back(idOf(n));
        rec_.commit("访问 " + std::to_string(n->val) + "(第 " + std::to_string(lv) + " 层)");
    });

    layoutAndRebuild();
    for (int id : doneIds)  rec_.markNode(id, Palette::Success);
    for (int id : levelIds) rec_.markNode(id, Palette::Success);
    rec_.commit("深度 = " + std::to_string(level + 1) + "(共 " + std::to_string(level + 1) + " 层)");
    player_.setFrames(rec_.frames());
}

// ── 宽度:调用你的 GetWidth。本层宽度 = 最右/最左非空下标差 + 1,各层取最大 ──
void TreeScene::recordWidth() {
    rec_.clear();
    greenIds_.clear();
    root_ = tree_.getRoot();
    layoutAndRebuild();
    rec_.commit("求宽度:本层宽度 = 最右非空下标 − 最左非空下标 + 1(空位也占位),各层取最大");

    int level = -1;
    long long minIdx = 0, maxIdx = 0;
    int minId = -1, maxId = -1;
    long long best = 0;                       // 全局最宽层的宽度
    int bestLevel = 0, bestMinId = -1, bestMaxId = -1;

    tree_.GetWidth([&](ds::TreeNode* n, long long idx) {
        const int id = idOf(n);
        const int lv = levelOfIndex(idx);
        if (lv != level) {                    // 换层:收尾上一层
            if (level >= 0) {
                layoutAndRebuild(); markGreen();
                rec_.markNode(minId, Palette::Active);
                rec_.markNode(maxId, Palette::Active);
                rec_.commit("第 " + std::to_string(level) + " 层:最左下标 " +
                            std::to_string(minIdx) + ",最右下标 " + std::to_string(maxIdx) +
                            ",本层宽度 = " + std::to_string(maxIdx - minIdx + 1));
                if (maxIdx - minIdx + 1 > best) {
                    best = maxIdx - minIdx + 1;
                    bestLevel = level; bestMinId = minId; bestMaxId = maxId;
                }
            }
            level = lv; minIdx = maxIdx = idx; minId = maxId = id;
        }
        layoutAndRebuild(); markGreen();
        if (idx < minIdx) { minIdx = idx; minId = id; }
        if (idx > maxIdx) { maxIdx = idx; maxId = id; }
        rec_.markNode(id, Palette::Insert);          // 当前节点红
        rec_.markNode(minId, Palette::Active);       // 本层最左橙
        rec_.markNode(maxId, Palette::Active);       // 本层最右橙
        rec_.commit("访问 " + std::to_string(n->val) + "(下标 " + std::to_string(idx) +
                    ",第 " + std::to_string(lv) + " 层)");
        greenIds_.push_back(id);
    });

    // 最后一层收尾
    layoutAndRebuild(); markGreen();
    rec_.markNode(minId, Palette::Active);
    rec_.markNode(maxId, Palette::Active);
    rec_.commit("第 " + std::to_string(level) + " 层:宽度 = " +
                std::to_string(maxIdx - minIdx + 1));
    if (maxIdx - minIdx + 1 > best) {
        best = maxIdx - minIdx + 1;
        bestLevel = level; bestMinId = minId; bestMaxId = maxId;
    }

    layoutAndRebuild(); markGreen();
    if (bestMinId >= 0) { rec_.markNode(bestMinId, Palette::Insert); rec_.markNode(bestMaxId, Palette::Insert); }
    rec_.commit("宽度 = " + std::to_string(best) + "(第 " + std::to_string(bestLevel) +
                " 层最宽,红色为该层两个端点)");
    player_.setFrames(rec_.frames());
}

// ── 镜像翻转:调用你的 invert,每交换一个节点录一帧 ────────────────
void TreeScene::recordInvert() {
    rec_.clear();
    greenIds_.clear();
    root_ = tree_.getRoot();
    layoutAndRebuild();
    rec_.commit("镜像翻转:交换每个节点的左右子树(调用你的 BinaryTree::invert)");

    tree_.invert([&](ds::TreeNode* n) {
        layoutAndRebuild();               // 布局重算 → 翻转后的树形由播放器插值出"翻面"动画
        markGreen();
        rec_.markNode(idOf(n), Palette::Insert);
        rec_.commit("交换 " + std::to_string(n->val) + " 的左右子树");
        greenIds_.push_back(idOf(n));
    });

    root_ = tree_.getRoot();
    layoutAndRebuild(); markGreen();
    rec_.commit("翻转完成");
    player_.setFrames(rec_.frames());
}

// ── 最近公共祖先:调用你的 LCA,双路径探索动画 ─────────────────────
void TreeScene::recordLCA(int va, int vb) {
    rec_.clear();
    greenIds_.clear();
    root_ = tree_.getRoot();

    ds::TreeNode* pa = findValue(root_, va);
    ds::TreeNode* pb = findValue(root_, vb);
    layoutAndRebuild();
    if (!pa || !pb) {
        rec_.commit("节点 " + std::to_string(va) + " 或 " + std::to_string(vb) + " 不在树中");
        player_.setFrames(rec_.frames());
        return;
    }
    rec_.markNode(idOf(pa), Palette::Insert);
    rec_.markNode(idOf(pb), Palette::Insert);
    rec_.commit("求 " + std::to_string(va) + " 和 " + std::to_string(vb) +
                " 的最近公共祖先(迭代 DFS 两条路径)");

    std::vector<ds::TreeNode*> pathP, pathQ;
    int lastPhase = 0;
    ds::TreeNode* lca = tree_.LCA(pa, pb, &pathP, &pathQ,
        [&](ds::TreeNode* n, int phase) {
            if (phase != lastPhase) {     // 阶段切换:先给上一阶段一个总结帧
                if (lastPhase == 0) {
                    layoutAndRebuild();
                    for (auto* p : pathP) rec_.markNode(idOf(p), Palette::Success);
                    std::string s = std::to_string(va) + " 的路径: ";
                    for (size_t i = 0; i < pathP.size(); ++i)
                        s += std::to_string(pathP[i]->val) + (i + 1 < pathP.size() ? " -> " : "");
                    rec_.commit(s);
                } else if (lastPhase == 1) {
                    layoutAndRebuild();
                    for (auto* p : pathP) rec_.markNode(idOf(p), Palette::Success);
                    for (auto* p : pathQ) rec_.markNode(idOf(p), Palette::Active);
                    std::string s = std::to_string(vb) + " 的路径: ";
                    for (size_t i = 0; i < pathQ.size(); ++i)
                        s += std::to_string(pathQ[i]->val) + (i + 1 < pathQ.size() ? " -> " : "");
                    rec_.commit(s);
                }
                lastPhase = phase;
            }
            layoutAndRebuild();
            if (phase == 0) {
                for (auto* p : pathP) rec_.markNode(idOf(p), Palette::Success);
                rec_.markNode(idOf(n), Palette::Active);
                rec_.commit("找 " + std::to_string(va) + ":经过 " + std::to_string(n->val));
            } else if (phase == 1) {
                for (auto* p : pathP) rec_.markNode(idOf(p), Palette::Success);
                for (auto* p : pathQ) rec_.markNode(idOf(p), Palette::Active);
                rec_.markNode(idOf(n), Palette::Insert);
                rec_.commit("找 " + std::to_string(vb) + ":经过 " + std::to_string(n->val));
            } else {
                for (auto* p : pathP) rec_.markNode(idOf(p), Palette::Success);
                for (auto* p : pathQ) rec_.markNode(idOf(p), Palette::Active);
                rec_.markNode(idOf(n), Palette::Insert);   // 当前比对的公共前缀节点红
                rec_.commit("比对路径:共同前缀到 " + std::to_string(n->val));
            }
        });

    layoutAndRebuild();
    if (lca) {
        for (auto* p : pathP) rec_.markNode(idOf(p), Palette::Success);
        for (auto* p : pathQ) rec_.markNode(idOf(p), Palette::Active);
        rec_.markNode(idOf(lca), Palette::Insert);
        rec_.commit("最近公共祖先 = " + std::to_string(lca->val) +
                    "(p 路径绿,q 路径黄,分叉点红)");
    } else {
        rec_.commit("未找到公共祖先");
    }
    player_.setFrames(rec_.frames());
}

void TreeScene::draw() {
    // ── 第一行:建树 + 操作 ──
    ImGui::SetNextItemWidth(240);
    ImGui::InputText("##list", listBuf_, sizeof(listBuf_));
    ImGui::SameLine();
    if (ImGui::Button("建树")) recordBuild(parseListOpt(listBuf_));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70);
    ImGui::InputInt("##insval", &inputVal_);
    ImGui::SameLine();
    if (ImGui::Button("插入")) recordInsert(inputVal_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::Combo("##order", &travOrder_, "先序\0中序\0后序\0层序\0");   // 选项列表必须以 \0\0 结尾
    ImGui::SameLine();
    if (ImGui::Button("遍历")) recordTraversal(travOrder_);
    ImGui::SameLine();
    if (ImGui::Button("深度")) recordDepth();
    ImGui::SameLine();
    if (ImGui::Button("宽度")) recordWidth();
    ImGui::SameLine();
    if (ImGui::Button("重置")) resetDemo();

    // ── 第二行:翻转 / LCA + 播放控制 ──
    if (ImGui::Button("翻转")) recordInvert();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("##lcaa", &lcaA_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("##lcab", &lcaB_);
    ImGui::SameLine();
    if (ImGui::Button("LCA")) recordLCA(lcaA_, lcaB_);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.18f, 0.63f, 0.26f, 1.0f), "A=%d", lcaA_);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.84f, 0.62f, 0.18f, 1.0f), "B=%d", lcaB_);
    ImGui::SameLine();
    ImGui::TextDisabled("点画布节点可选");
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
    ImGui::TextColored(ImVec4(0.55f, 0.30f, 0.05f, 1.0f), "步骤: %s",
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

    // 点选节点作为 LCA 的目标:第一次点选 A,第二次点选 B,交替
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        ImVec2 m = ImGui::GetMousePos() - origin;
        int bestId = -1;
        float bestD = 45.0f * 45.0f;         // 命中半径
        for (const auto& n : frame.nodes) {
            float dx = m.x - n.x, dy = m.y - n.y;
            float d = dx * dx + dy * dy;
            if (d < bestD) { bestD = d; bestId = n.id; }
        }
        if (bestId >= 0) {
            for (const auto& n : frame.nodes) {
                if (n.id != bestId) continue;
                const int v = std::stoi(n.label);
                if (pickSlot_ == 0) { lcaA_ = v; pickIdA_ = bestId; }
                else                { lcaB_ = v; pickIdB_ = bestId; }
                pickSlot_ ^= 1;
            }
        }
    }

    // 先画边(父子连线,在节点下层)
    for (const auto& e : frame.edges) {
        auto a = pos.find(e.from), b = pos.find(e.to);
        if (a == pos.end() || b == pos.end()) continue;
        dl->AddLine(a->second, b->second, toImU32(e.color), 3.0f);
    }
    // 再画节点:卡通贴纸风 = 右下硬阴影 + 粉彩填充 + 同色系粗描边 + 粗体标签
    constexpr float kRad = 12.0f;
    for (const auto& n : frame.nodes) {
        ImVec2 p = pos[n.id];
        ImRect box(p - ImVec2(kBoxW * 0.5f, kBoxH * 0.5f),
                   p + ImVec2(kBoxW * 0.5f, kBoxH * 0.5f));
        dl->AddRectFilled(box.Min + ImVec2(3.0f, 4.0f), box.Max + ImVec2(3.0f, 4.0f),
                          toImU32(Darken(n.color, 0.45f)), kRad);                        // 阴影
        dl->AddRectFilled(box.Min, box.Max, toImU32(n.color), kRad);                     // 填充
        dl->AddRect(box.Min, box.Max, toImU32(Darken(n.color, 0.52f)), kRad, 0, 2.5f);   // 描边
        const bool bold = viz::NodeFont != nullptr;
        if (bold) ImGui::PushFont(viz::NodeFont);
        ImVec2 ts = ImGui::CalcTextSize(n.label.c_str());
        dl->AddText(ImVec2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f),
                    toImU32(Palette::NodeText), n.label.c_str());
        if (bold) ImGui::PopFont();

        // LCA 选中圈:A 绿 / B 黄。树重建后 id 会重排,值对不上就当作失效清除
        int ringCol = 0;
        if (n.id == pickIdA_) {
            if (std::stoi(n.label) == lcaA_) ringCol = IM_COL32(46, 160, 67, 255);
            else pickIdA_ = -1;
        } else if (n.id == pickIdB_) {
            if (std::stoi(n.label) == lcaB_) ringCol = IM_COL32(214, 158, 46, 255);
            else pickIdB_ = -1;
        }
        if (ringCol)
            dl->AddRect(p - ImVec2(kBoxW * 0.5f + 4, kBoxH * 0.5f + 4),
                        p + ImVec2(kBoxW * 0.5f + 4, kBoxH * 0.5f + 4),
                        ringCol, 15.0f, 0, 3.0f);
    }
    ImGui::EndChild();
}

} // namespace viz
