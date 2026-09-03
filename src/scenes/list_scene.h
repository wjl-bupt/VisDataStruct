// ═══════════════════════════════════════════════════════════════
// 链表场景【示例】:演示"算法录制 → 快照播放"的完整流程。
// 新增其他场景时,照这个文件的模式抄即可。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "core/animation/player.h"
#include "core/animation/recorder.h"
#include "core/ds/linked_list.h"
#include "scenes/scene.h"

#include <string>
#include <unordered_map>

namespace viz {

class ListScene : public Scene {
public:
    ListScene();

    const char* name() const override { return "链表"; }
    void update(double dt) override;
    void draw() override;
    bool isAnimating() const override { return player_.playing() && !player_.atEnd(); }

private:
    void resetDemo();                       // 重建初始链表 [2, 4, 6, 8]
    void recordInsert(int val);             // 录制一次有序插入
    void rebuildWorking();                  // 把链表当前状态重建到 Recorder 工作帧
    int idOf(const ds::ListNode* n);        // 节点指针 → 稳定 id(帧间匹配的关键)
    float nodeX(int index) const;           // 第 index 个节点的 x 坐标

    ds::LinkedList list_;
    Recorder rec_;
    Player player_;
    std::unordered_map<const ds::ListNode*, int> idMap_;
    int nextId_ = 0;

    int inputVal_ = 5;      // 输入框
    float speed_ = 1.0f;    // 播放速度
};

} // namespace viz
