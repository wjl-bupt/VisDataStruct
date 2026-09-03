// ═══════════════════════════════════════════════════════════════
// Recorder:算法执行时把"每一步长什么样"录制成快照序列。
//
// 推荐用法(完整示例见 scenes/list_scene.cpp):
//   1. 每步先把结构当前状态全量重建到工作帧(默认配色);
//   2. 用 markNode 把本步关心的节点标成高亮色;
//   3. commit(说明文字) 把工作帧压入时间线。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "snapshot.h"

#include <string>
#include <vector>

namespace viz {

class Recorder {
public:
    // 清空时间线与工作帧(开始一次新的录制)
    void clear() {
        frames_.clear();
        working_ = Snapshot{};
    }

    // 清空工作帧(不影响已 commit 的时间线),配合全量重建使用
    void resetWorking() { working_ = Snapshot{}; }

    // 更新/添加一个节点的布局与颜色
    void setNode(int id, float x, float y, const std::string& label, Color color) {
        for (auto& n : working_.nodes) {
            if (n.id == id) {
                n.x = x; n.y = y; n.label = label; n.color = color;
                return;
            }
        }
        working_.nodes.push_back(Snapshot::Node{id, x, y, label, color});
    }

    // 只改某节点颜色(高亮/恢复);节点不存在则忽略
    void markNode(int id, Color color) {
        for (auto& n : working_.nodes) {
            if (n.id == id) { n.color = color; return; }
        }
    }

    // 更新/添加一条边,以 (from, to) 作为边的身份
    void setEdge(int from, int to, const std::string& weight, Color color) {
        for (auto& e : working_.edges) {
            if (e.from == from && e.to == to) {
                e.weight = weight; e.color = color;
                return;
            }
        }
        working_.edges.push_back(Snapshot::Edge{from, to, weight, color});
    }

    // 区间带(排序场景的拆分/归并范围展示):clear 后逐条 set
    void clearBands() { working_.bands.clear(); }
    void setBand(int x0, int x1, const std::string& label, Color color) {
        working_.bands.push_back(Snapshot::Band{x0, x1, label, color});
    }

    // 把当前工作帧压入时间线,desc 为这一步的文字说明
    void commit(const std::string& desc) {
        working_.desc = desc;
        frames_.push_back(working_);
    }

    const std::vector<Snapshot>& frames() const { return frames_; }
    int frameCount() const { return static_cast<int>(frames_.size()); }

private:
    std::vector<Snapshot> frames_;
    Snapshot working_;   // 当前正在编辑的一帧
};

} // namespace viz
