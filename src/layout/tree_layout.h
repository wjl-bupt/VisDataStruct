// ═══════════════════════════════════════════════════════════════
// 树布局:递归层次布局 —— 【示例实现】
//
// 复习布局算法时建议自己重写 compute():思路(中序遍历分配列号):
//   1. 中序遍历整棵树,按访问顺序给节点依次分配递增的列号,x = 列号 * 水平间距;
//   2. 每个节点的 y = 深度 * 垂直间距;
//   3. 左子树永远画在父节点左边,同一层不重叠。
// 之后插入/删除/旋转/遍历等操作,在每一"步"重新布局 + markNode 高亮
// + commit 即可,结构变化由播放器对坐标插值自动动画化。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "core/ds/tree_node.h"

#include <unordered_map>

namespace viz {

struct Vec2 { float x = 0.0f, y = 0.0f; };

class TreeLayout {
public:
    // 计算布局:返回 每个节点指针 → 场景坐标(节点中心)
    // maxWidth 用于横向压缩:树太宽时按比例缩小水平间距
    std::unordered_map<const ds::TreeNode*, Vec2> compute(const ds::TreeNode* root,
                                                          float maxWidth = 1150.0f) {
        pos_.clear();
        col_ = 0;
        if (root) {
            countCols(root);                                  // 第一遍:数总列数
            if (col_ > 1) {
                float span = (col_ - 1) * xSpacing_;
                if (span > maxWidth) xSpacing_ = maxWidth / (col_ - 1);
            }
            col_ = 0;
            assign(root, 0);                                  // 第二遍:真正布局
        }
        return std::move(pos_);
    }

private:
    void countCols(const ds::TreeNode* n) {
        if (!n) return;
        countCols(n->lchild);
        ++col_;
        countCols(n->rchild);
    }

    // 中序遍历:左 → 根(分配列号) → 右
    void assign(const ds::TreeNode* n, int depth) {
        if (!n) return;
        assign(n->lchild, depth + 1);
        pos_[n] = Vec2{margin_ + col_ * xSpacing_, top_ + depth * ySpacing_};
        ++col_;
        assign(n->rchild, depth + 1);
    }

    std::unordered_map<const ds::TreeNode*, Vec2> pos_;
    int col_ = 0;
    float xSpacing_ = 78.0f;   // 水平间距(列宽)
    float ySpacing_ = 86.0f;   // 垂直间距(层高)
    float margin_ = 70.0f;     // 画布左边距
    float top_ = 90.0f;        // 画布上边距(留出悬浮新节点的空间)
};

} // namespace viz
