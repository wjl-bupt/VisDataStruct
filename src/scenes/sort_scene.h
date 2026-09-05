// ═══════════════════════════════════════════════════════════════
// 排序场景:柱状图动画。
//   - 每个数值一根柱子,高度按全数组最大值归一化,颜色按值映射色相(小=蓝,大=红)
//   - 数值有稳定 id,交换时两根柱子由播放器插值滑过对方
//   - 已就位元素染绿;框架的 Band(区间带)预留给归并排序的拆分/合并展示
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "core/animation/player.h"
#include "core/animation/recorder.h"
#include "scenes/scene.h"

#include <string>
#include <vector>

namespace viz {

class SortScene : public Scene {
public:
    SortScene() { resetDemo(); }
    const char* name() const override { return "排序"; }
    void update(double dt) override;
    void draw() override;
    bool isAnimating() const override { return player_.playing() && !player_.atEnd(); }

private:
    void resetDemo();                        // 用输入框内容重建柱子(单帧)
    void recordSort();                       // 调你的 BubbleSortAlgo,录制全过程
    void rebuildBars(int hole = -1);         // 按当前数组全量重建工作帧;hole:空槽(插入排序 key 悬浮时不绘制)
    void markGreen();
    float slotX(int slot) const;             // 槽位下标 → 柱中心 x
    float barH(int val) const;               // 值 → 归一化柱高
    float barNorm(int val) const;            // 值 → [0,1] 归一化(颜色映射用)

    std::vector<int> vals_;                  // 当前数组(排序中会被算法改动)
    std::vector<int> ids_;                   // 与 vals_ 平行的元素 id(id 随交换移动)
    int maxVal_ = 1;                         // 归一化用的最大值
    int minVal_ = 0;                         // 归一化用的最小值(支持负数)
    float slotSpacing_ = 105.0f;             // 槽位间距(随元素数量自适应收缩)
    float barW_ = 62.0f;                     // 柱宽(随间距收缩)
    int sortedFrom_ = -1;                    // [sortedFrom_, n) 已就位(染绿)
    bool heapMode_ = false;                  // 堆排序双视图模式(柱子压缩在上,堆树在下)

    Recorder rec_;
    Player player_;

    char listBuf_[256] = "38,17,52,9,41,73,24,60,20,86,100,60";  // 待排序数值,逗号/空格分隔
    int algo_ = 0;                           // 算法下拉框(目前一种)
    float speed_ = 2.0f;                     // 帧多,默认 2 倍速
};

} // namespace viz
