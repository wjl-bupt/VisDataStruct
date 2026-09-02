// ═══════════════════════════════════════════════════════════════
// 二叉树场景:BST 插入动画 + 先/中/后序遍历动画。
// 演示树布局(tree_layout)与录制模式的结合,照此接入其他树结构。
//
// 说明:场景直接操作 ds::TreeNode 指针(只读结构、演示逻辑在本层),
// 等你的 BinaryTree 补好插入接口后,可以把这里的具体逻辑换成调用你自己的实现。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "core/animation/player.h"
#include "core/animation/recorder.h"
#include "core/ds/tree_node.h"
#include "layout/tree_layout.h"
#include "scenes/scene.h"

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
    void resetDemo();                        // 重建示例 BST [5,3,8,1,4,7,9]
    void recordInsert(int val);              // BST 插入:沿搜索路径高亮,新节点滑入
    void recordTraversal(int order);         // 先/中/后序遍历:访问过的节点逐步变绿
    void layoutAndRebuild();                 // 布局 + 全量重建工作帧(默认配色)
    void drawFloating(int id, const ds::TreeNode* anchor, const std::string& label);
    int  idOf(const ds::TreeNode* n);        // 节点指针 → 稳定 id(帧间匹配的关键)
    void destroyTree(ds::TreeNode* n);       // 递归释放(演示层自己的,不依赖 BinaryTree)

    ds::TreeNode* root_ = nullptr;
    TreeLayout layout_;
    std::unordered_map<const ds::TreeNode*, Vec2> pos_;   // 当前帧布局缓存
    Recorder rec_;
    Player player_;
    std::unordered_map<const ds::TreeNode*, int> idMap_;
    int nextId_ = 0;

    int inputVal_ = 6;       // 插入输入框
    int travOrder_ = 1;      // 0 先序 / 1 中序 / 2 后序
    float speed_ = 1.0f;
};

} // namespace viz
