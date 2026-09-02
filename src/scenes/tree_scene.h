// ═══════════════════════════════════════════════════════════════
// 二叉树场景:层序数组建树 + BST 单值插入 + 先/中/后序遍历动画。
// 建树与遍历直接调用你的 ds::BinaryTree(通过 onAttach / onVisit 回调
// 在算法的每一步录帧),树布局见 layout/tree_layout.h。
//
// 列表语义 = 你的 createbinarytree 约定:层序满二叉树数组,# 为空位。
// 单值"插入"暂为场景侧演示实现(你的类还没有 BST 插入),补上后可替换。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "core/animation/player.h"
#include "core/animation/recorder.h"
#include "core/ds/tree_node.h"
#include "layout/tree_layout.h"
#include "scenes/scene.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace viz {

class TreeScene : public Scene {
public:
    TreeScene() { resetDemo(); }   // 忘了它会得到空画布
    const char* name() const override { return "二叉树"; }
    void update(double dt) override;
    void draw() override;

private:
    void resetDemo();                        // 用列表输入框的内容重建树
    void recordInsert(int val);              // BST 单值插入:沿搜索路径高亮,新节点滑入
    void recordBuild(const std::vector<std::optional<int>>& vals);  // 层序数组建树动画
    void recordTraversal(int order);         // 先/中/后序遍历:访问过的节点逐步变绿
    void recordDepth();                      // 深度:逐层扫描染色(调你的 GetDepth)
    void recordWidth();                      // 宽度:数组下标追踪(调你的 GetWidth)
    void recordInvert();                     // 镜像翻转:逐节点交换动画(调你的 invert)
    void recordLCA(int va, int vb);          // 最近公共祖先:双路径探索动画(调你的 LCA)
    bool recordInsertFrames(int val);        // 向当前录制追加一次插入的全部帧;重复值返回 false
    void markGreen();                        // 恢复"已就位"节点的绿色(每帧重建后调用)
    void layoutAndRebuild();                 // 布局 + 全量重建工作帧(默认配色)
    void drawFloating(int id, const ds::TreeNode* anchor, const std::string& label);
    int  idOf(const ds::TreeNode* n);        // 节点指针 → 稳定 id(帧间匹配的关键)

    ds::BinaryTree tree_;                    // ★ 树本体:建树/遍历都调你的 BinaryTree
    ds::TreeNode* root_ = nullptr;           // tree_.getRoot() 的缓存,场景逻辑用它访问
    TreeLayout layout_;
    std::unordered_map<const ds::TreeNode*, Vec2> pos_;   // 当前帧布局缓存
    Recorder rec_;
    Player player_;
    std::unordered_map<const ds::TreeNode*, int> idMap_;
    int nextId_ = 0;
    std::vector<int> greenIds_;              // 本次录制中已就位节点的 id(持续染绿,展示建树进度)

    char listBuf_[256] = "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,#,#,#,#,#,#,#,32,#,#,34,#,#,#,#,64,#,#,69,#,#,#,#";    // 层序数组输入框,# 为空位,逗号/空格分隔
    int inputVal_ = 6;       // 插入输入框
    int lcaA_ = 1, lcaB_ = 4;                // LCA 两个目标值
    int pickIdA_ = -1, pickIdB_ = -1;        // 画布点选的节点 id(画选中圈用)
    int pickSlot_ = 0;                       // 下一次点选填入 A(0) 还是 B(1)
    int travOrder_ = 1;      // 0 先序 / 1 中序 / 2 后序
    float speed_ = 1.0f;
};

} // namespace viz
