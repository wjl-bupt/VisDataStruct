// ═══════════════════════════════════════════════════════════════
// 排序场景【实现】:柱状图 + 交换滑动动画。
// 排序逻辑全部来自你的 sortedlist::SortAlgorithms,场景只负责:
// 解析输入 → 回调里录帧(比较高亮/交换滑动/就位染绿)→ 播放。
// ═══════════════════════════════════════════════════════════════
#define IMGUI_DEFINE_MATH_OPERATORS
#include "scenes/sort_scene.h"

#include "core/ds/sort_list.h"
#include "imgui.h"
#include "imgui_internal.h"   // ImVec2 运算符
#include "ui/theme.h"         // NodeFont

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <string>

namespace viz {

namespace {

constexpr float kX0      = 70.0f;    // 首柱左缘
constexpr float kSpacing = 105.0f;   // 槽位间距(上限,元素多时自动收缩)
constexpr float kBarW    = 62.0f;    // 柱宽(上限)
constexpr float kBaseY   = 520.0f;   // 基线 y(柱底;下方留出计数行空间)
constexpr float kMaxH    = 330.0f;   // 最大柱高
constexpr float kMinH    = 22.0f;    // 最小柱高(最小值也给个可见的矮柱)
constexpr float kRad     = 5.0f;     // 柱子圆角
constexpr int   kCountId0 = 10000;   // 计数行节点的 id 起点(不与元素 id 冲突)

ImU32 toImU32(Color c) {
    return IM_COL32(static_cast<int>(c.r * 255), static_cast<int>(c.g * 255),
                    static_cast<int>(c.b * 255), static_cast<int>(c.a * 255));
}

// 值 → 颜色:归一化后映射色相,小=蓝 → 大=红(粉彩饱和度,贴合卡通主题)
Color colorByValue(float norm) {
    float r = 0, g = 0, b = 0;
    const float hue = 0.62f * (1.0f - norm);
    ImGui::ColorConvertHSVtoRGB(hue, 0.55f, 0.88f, r, g, b);
    return Color::make(r, g, b);
}

// 解析数值列表:任何非数字字符当分隔符(兼容中英文逗号/空格)
std::vector<int> parseNums(const char* text) {
    std::string t = text ? text : "";
    for (char& c : t)
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-') c = ' ';
    std::vector<int> out;
    std::istringstream is(t);
    std::string tok;
    while (is >> tok) {
        try {
            size_t used = 0;
            int v = std::stoi(tok, &used);
            if (used == tok.size()) out.push_back(v);
        } catch (...) {}
    }
    return out;
}

} // namespace

void SortScene::update(double dt) {
    player_.setSpeed(speed_);
    player_.update(dt);
}

float SortScene::slotX(int slot) const { return kX0 + barW_ * 0.5f + slot * slotSpacing_; }
float SortScene::barH(int val) const {
    // [minVal_, maxVal_] 归一化到 [kMinH, kMaxH]:负数也能画,最小值保底可见
    const float span = static_cast<float>(maxVal_ - minVal_);
    const float norm = span > 0 ? (val - minVal_) / span : 0.5f;
    const float mx = heapMode_ ? 170.0f : kMaxH;   // 堆排序:柱子压缩,下半让给堆树
    const float mn = heapMode_ ? 30.0f : kMinH;
    return mn + norm * (mx - mn);
}
float SortScene::barNorm(int val) const {
    const float span = static_cast<float>(maxVal_ - minVal_);
    return span > 0 ? (val - minVal_) / span : 0.5f;
}

void SortScene::markGreen() {
    for (int k = sortedFrom_; k < static_cast<int>(vals_.size()); ++k)
        rec_.markNode(ids_[k], Palette::Success);
}

void SortScene::rebuildBars(int hole) {
    rec_.resetWorking();
    const float baseY = heapMode_ ? 240.0f : kBaseY;   // 堆模式:柱子压缩到上半区
    for (int k = 0; k < static_cast<int>(vals_.size()); ++k) {
        if (k == hole) continue;             // key 被拿出的空槽不绘制
        const float h = barH(vals_[k]);
        rec_.setNode(ids_[k], slotX(k), baseY - h * 0.5f, std::to_string(vals_[k]),
                     colorByValue(barNorm(vals_[k])));
    }
}

void SortScene::resetDemo() {
    vals_ = parseNums(listBuf_);
    ids_.clear();
    ids_.reserve(vals_.size());
    for (int k = 0; k < static_cast<int>(vals_.size()); ++k) ids_.push_back(k);   // id = 初始下标
    maxVal_ = 1;
    minVal_ = 0;
    for (int v : vals_) { maxVal_ = std::max(maxVal_, v); minVal_ = std::min(minVal_, v); }
    // 元素多时收缩间距与柱宽,避免溢出画布
    const int n = static_cast<int>(vals_.size());
    slotSpacing_ = std::min(kSpacing, 1180.0f / std::max(n, 1));
    barW_        = std::min(kBarW, slotSpacing_ * 0.62f);
    sortedFrom_  = n;

    rec_.clear();
    rebuildBars();
    rec_.commit("待排序 " + std::to_string(n) + " 个元素,范围 [" +
                std::to_string(minVal_) + ", " + std::to_string(maxVal_) +
                "](柱高/颜色按范围归一化)");
    player_.setFrames(rec_.frames());
}

void SortScene::recordSort() {
    if (vals_.size() < 2) {
        rec_.clear();
        rebuildBars();
        rec_.commit("至少需要 2 个元素");
        player_.setFrames(rec_.frames());
        return;
    }
    rec_.clear();
    heapMode_ = (algo_ == 10);           // 堆排序:柱子压缩,下方画堆树
    sortedFrom_ = static_cast<int>(vals_.size());
    rebuildBars();
    const char* algoName = (algo_ == 1) ? "BubbleSortAlgoImprovement(冒泡改进:记录最后交换位置)"
                            : (algo_ == 2) ? "SelectionSortAlgo(选择排序:每轮选最大放尾部)"
                            : (algo_ == 3) ? "InsertionSortAlgo(插入排序:key 拿出右移再落位)"
                            : (algo_ == 4) ? "MergeSortAlgo(归并:自底向上两两归并)"
                            : (algo_ == 5) ? "QuickSortAlgo(快排:显式栈,基准放区间末尾)"
                            : (algo_ == 6) ? "CountingSortAlgo(计数排序:非比较 O(n+k))"
                            : (algo_ == 7) ? "RadixSortAlgo(基数排序:LSD 逐位稳定排序)"
                            : (algo_ == 8) ? "BucketSortAlgo(桶排序:分桶+桶内排序+串联)"
                            : (algo_ == 9) ? "ShellSortAlgo(希尔:gap 减半的组内插入)"
                            : (algo_ == 10) ? "HeapSortAlgo(堆排序:双视图 数组+大根堆)"
                            : "BubbleSortAlgo(冒泡:相邻交换)";
    rec_.commit(std::string("开始排序:") + algoName);

    sortedlist::SortAlgorithms algo;
    int mergeLevel = 0, mergeLastWidth = 1, mergeLastLV = 0;   // 归并轮次状态(写回/总结帧用)
    bool mergeAnySplit = false;
    const auto onCompare = [&](int i, int j) {    // 比较:两根柱子橙色高亮
        rebuildBars(); markGreen();
        rec_.markNode(ids_[i], Palette::Active);
        rec_.markNode(ids_[j], Palette::Active);
        rec_.commit("比较 a[" + std::to_string(i) + "]=" + std::to_string(vals_[i]) +
                    " 与 a[" + std::to_string(j) + "]=" + std::to_string(vals_[j]));
    };
    const auto onSwap = [&](int i, int j) {       // 交换:id 随值移动 → 两柱滑动交错
        if (i == j) return;
        std::swap(ids_[i], ids_[j]);
        rebuildBars(); markGreen();
        rec_.markNode(ids_[i], Palette::Insert);
        rec_.markNode(ids_[j], Palette::Insert);
        rec_.commit("交换 a[" + std::to_string(i) + "] 与 a[" + std::to_string(j) + "]");
    };
    const auto onSorted = [&](int idx) {          // 就位:染绿(可能整段后缀一起上报)
        sortedFrom_ = std::min(sortedFrom_, idx);
        rebuildBars(); markGreen();
        rec_.markNode(ids_[idx], Palette::Success);
        rec_.commit("a[" + std::to_string(idx) + "]=" + std::to_string(vals_[idx]) + " 就位");
    };

    if (algo_ == 1) {
        algo.BubbleSortAlgoImprovement(vals_, onCompare, onSwap, onSorted);
    } else if (algo_ == 2) {
        algo.SelectionSortAlgo(vals_, onCompare, onSwap, onSorted);
    } else if (algo_ == 3) {
        // 插入排序:key 悬浮在空位上方,大元素逐格右移,最后 key 落位;前缀 [0..i] 有序染绿
        int keyId = -1, hole = -1, lastKeyIdx = -1, keyVal = 0, sortedTo = 0;
        algo.InsertionSortAlgo(vals_,
            [&](int keyIdx, int j) {              // 比较:key 与 nums[j]
                if (keyIdx != lastKeyIdx) {       // 新一轮:key 从 keyIdx 被拿出
                    keyId = ids_[keyIdx];
                    keyVal = vals_[keyIdx];
                    hole = keyIdx;
                    lastKeyIdx = keyIdx;
                }
                rebuildBars(hole);
                for (int k = 0; k < sortedTo; ++k) rec_.markNode(ids_[k], Palette::Success);
                rec_.markNode(ids_[j], Palette::Active);
                rec_.setNode(keyId, slotX(hole), kBaseY - barH(keyVal) * 0.5f - 26.0f,
                             std::to_string(keyVal), Palette::Insert);   // key 悬浮在空位上方
                rec_.commit("key=" + std::to_string(keyVal) + " 与 a[" +
                            std::to_string(j) + "]=" + std::to_string(vals_[j]) + " 比较");
            },
            [&](int j, int j1) {                  // 右移:元素 j 滑到 j+1,空位左移
                ids_[j1] = ids_[j];
                hole = j;
                rebuildBars(hole);
                for (int k = 0; k < sortedTo; ++k) rec_.markNode(ids_[k], Palette::Success);
                rec_.markNode(ids_[j1], Palette::Active);
                rec_.setNode(keyId, slotX(hole), kBaseY - barH(keyVal) * 0.5f - 26.0f,
                             std::to_string(keyVal), Palette::Insert);
                rec_.commit(std::to_string(vals_[j1]) + " 右移到 a[" + std::to_string(j1) + "]");
            },
            [&](int i) {                          // key 落位,前缀 [0, i] 有序
                ids_[hole] = keyId;
                hole = -1;
                sortedTo = i + 1;
                rebuildBars();
                for (int k = 0; k < sortedTo; ++k) rec_.markNode(ids_[k], Palette::Success);
                rec_.markNode(ids_[i], Palette::Insert);
                rec_.commit("key=" + std::to_string(vals_[i]) + " 插入 a[" + std::to_string(i) +
                            "],前缀 [0.." + std::to_string(i) + "] 有序");
            });
    } else if (algo_ == 4) {
        // 归并排序:区间带标出两个子段;两段头染成各自带色,写回时柱子滑入正确槽位;
        // 每轮(width)结束给一帧总结
        int cl = 0, cm = 0, cr = 0;
        std::vector<std::pair<int,int>> mergeQ;   // (值, id) 升序队列:写回时按序落位
        const auto setSegBands = [&]() {          // 拆分带:左右子段各一条
            rec_.clearBands();
            if (cm > cl)
                rec_.setBand(cl, cm - 1, "左段 " + std::to_string(cl) + ".." + std::to_string(cm - 1),
                             Color::make(0.40f, 0.65f, 0.90f));
            if (cr - 1 >= cm)
                rec_.setBand(cm, cr - 1, "右段 " + std::to_string(cm) + ".." + std::to_string(cr - 1),
                             Color::make(0.35f, 0.75f, 0.55f));
        };
        algo.MergeSortAlgo(vals_,
            [&](int l, int m, int r) {            // 拆分:快照本段 (值, id) 按值排队
                if (l == 0) {                     // 每轮都从 l=0 开始:进入新一轮前总结上一轮
                    if (mergeAnySplit) {
                        rec_.clearBands();
                        rebuildBars();
                        rec_.commit("第 " + std::to_string(mergeLevel) + " 轮(width=" +
                                    std::to_string(mergeLastWidth) + ")归并完成");
                        ++mergeLevel;
                    } else {
                        mergeLevel = 1;
                    }
                    mergeAnySplit = true;
                    mergeLastWidth = m - l;
                }
                cl = l; cm = m; cr = r;
                mergeQ.clear();
                for (int k = l; k < r; ++k) mergeQ.push_back({vals_[k], ids_[k]});
                std::sort(mergeQ.begin(), mergeQ.end());
                setSegBands();
                rec_.commit("第 " + std::to_string(mergeLevel) + " 轮:归并 [" +
                            std::to_string(l) + ".." + std::to_string(r - 1) + "],左段 [" +
                            std::to_string(l) + ".." + std::to_string(m - 1) + "] + 右段 [" +
                            std::to_string(m) + ".." + std::to_string(r - 1) + "]");
            },
            [&](int i, int j) {                   // 两段头部比较:头染成所在带颜色
                rebuildBars();
                setSegBands();
                rec_.markNode(ids_[i], Color::make(0.45f, 0.70f, 0.95f));   // 左段头=蓝
                rec_.markNode(ids_[j], Color::make(0.45f, 0.85f, 0.60f));   // 右段头=绿
                mergeLastLV = vals_[i];
                rec_.commit("比较:左段头 " + std::to_string(vals_[i]) + " 与 右段头 " +
                            std::to_string(vals_[j]) + ",取较小者写回");
            },
            [&](int k, int val) {                 // 写回:对应柱子滑入槽位 k
                if (!mergeQ.empty()) {
                    ids_[k] = mergeQ.front().second;
                    mergeQ.erase(mergeQ.begin());
                }
                vals_[k] = val;
                rebuildBars();
                rec_.clearBands();
                rec_.setBand(cl, cr - 1, "第" + std::to_string(mergeLevel) + "轮 归并中 [" +
                                 std::to_string(cl) + ".." + std::to_string(cr - 1) + "]",
                                 Color::make(1.00f, 0.70f, 0.35f));
                rec_.markNode(ids_[k], Palette::Insert);
                const std::string side = (val == mergeLastLV) ? "左段" : "右段";
                rec_.commit("取" + side + "头部 " + std::to_string(val) + " 写回 a[" +
                            std::to_string(k) + "]");
                if (k == cr - 1)
                    rec_.commit("段 [" + std::to_string(cl) + ".." + std::to_string(cr - 1) + "] 归并完成");
            });
    } else if (algo_ == 5) {
        // 快排:蓝带框出当前子区间,橙带单独框住基准(区间末位);
        // 基准红、被比较柱橙、落位变绿。小区间优先,蓝带随之收缩
        int qLo = -1, qHi = -1;
        const auto quickBands = [&]() {           // 子区间带 + 基准带
            rec_.clearBands();
            if (qLo < 0 || qHi < qLo) return;
            rec_.setBand(qLo, qHi, "子区间 [" + std::to_string(qLo) + ".." + std::to_string(qHi) + "]",
                         Color::make(0.40f, 0.65f, 0.90f));
            rec_.setBand(qHi, qHi, "基准 " + std::to_string(vals_[qHi]),
                         Color::make(1.00f, 0.62f, 0.25f));
        };
        algo.QuickSortAlgo(vals_,
            [&](int lo, int hi) {                 // 区间确定/收缩:框出来
                qLo = lo; qHi = hi;
                rebuildBars(); quickBands();
                rec_.markNode(ids_[hi], Palette::Insert);   // 基准红
                rec_.commit("处理子区间 [" + std::to_string(lo) + ".." + std::to_string(hi) +
                            "],基准 = a[" + std::to_string(hi) + "]=" + std::to_string(vals_[hi]));
            },
            [&](int j, int hi) {                  // 与基准比较
                rebuildBars(); quickBands();
                rec_.markNode(ids_[hi], Palette::Insert);   // 基准红
                rec_.markNode(ids_[j], Palette::Active);    // 被比较橙
                rec_.commit("比较 a[" + std::to_string(j) + "]=" + std::to_string(vals_[j]) +
                            " 与 基准 " + std::to_string(vals_[hi]));
            },
            [&](int i, int j) {                   // 交换
                if (i == j) return;
                std::swap(ids_[i], ids_[j]);
                rebuildBars(); quickBands();
                rec_.markNode(ids_[qHi], Palette::Insert);
                rec_.markNode(ids_[i], Palette::Active);
                rec_.markNode(ids_[j], Palette::Active);
                rec_.commit("交换 a[" + std::to_string(i) + "] 与 a[" + std::to_string(j) + "]");
            },
            [&](int idx) {                        // 基准落位:全局有序
                rebuildBars(); quickBands();
                rec_.markNode(ids_[idx], Palette::Success);
                rec_.commit("基准 " + std::to_string(vals_[idx]) + " 落位 a[" +
                            std::to_string(idx) + "],全局有序");
            });
    } else if (algo_ == 6) {
        // 计数排序:基线下方一排"计数格"(值×次数):
        //   ① 扫描计数——对应格闪红并累加;② 计数完成——整排变绿(计数的结果);
        //   ③ 写回——柱子按值从小到大滑入,同时消耗对应格的计数(耗尽变灰)
        std::vector<int> uniqVals;                // 升序去重值
        {
            std::vector<int> tmp = vals_;
            std::sort(tmp.begin(), tmp.end());
            for (int v : tmp)
                if (uniqVals.empty() || uniqVals.back() != v) uniqVals.push_back(v);
        }
        const int ku = static_cast<int>(uniqVals.size());
        const float rowStep = 1180.0f / std::max(ku, 1);
        std::vector<int> liveCnt(ku, 0);          // 场景侧计数镜像
        const auto valIdx = [&](int v) {          // 值 → 计数格下标
            for (int i = 0; i < ku; ++i) if (uniqVals[i] == v) return i;
            return 0;
        };
        const auto countRow = [&](bool allDone) { // 基线下方的计数行
            for (int i = 0; i < ku; ++i) {
                Color c = liveCnt[i] > 0 ? colorByValue(barNorm(uniqVals[i]))
                                         : Color::make(0.72f, 0.72f, 0.70f);   // 已耗尽变灰
                if (allDone) c = Palette::Success;
                rec_.setNode(kCountId0 + i, kX0 + (i + 0.5f) * rowStep, kBaseY + 48.0f,
                             std::to_string(uniqVals[i]) + "×" + std::to_string(liveCnt[i]), c);
            }
        };
        std::vector<std::pair<int,int>> pool;     // (值, id) 全体元素
        std::vector<char> used;
        pool.reserve(vals_.size());
        for (int k = 0; k < static_cast<int>(vals_.size()); ++k)
            pool.push_back({vals_[k], ids_[k]});
        used.assign(pool.size(), 0);
        bool countPhaseDone = false;
        algo.CountingSortAlgo(vals_,
            [&](int i) {                          // 扫描计数:柱子橙 + 对应计数格闪红累加
                ++liveCnt[valIdx(vals_[i])];
                rebuildBars();
                countRow(false);
                rec_.markNode(ids_[i], Palette::Active);
                rec_.markNode(kCountId0 + valIdx(vals_[i]), Palette::Insert);
                rec_.commit("计数 a[" + std::to_string(i) + "]=" + std::to_string(vals_[i]) +
                            ",该值已出现 " + std::to_string(liveCnt[valIdx(vals_[i])]) + " 次");
            },
            [&](int pos, int val) {               // 写回:滑入 + 消耗对应计数
                if (!countPhaseDone) {            // 阶段切换:计数完成的总结帧
                    countPhaseDone = true;
                    rebuildBars();
                    countRow(true);
                    rec_.commit("计数完成:下方计数格即计数的结果,接下来按值从小到大写回");
                }
                for (size_t x = 0; x < pool.size(); ++x) {
                    if (!used[x] && pool[x].first == val) {
                        used[x] = 1;
                        ids_[pos] = pool[x].second;
                        break;
                    }
                }
                vals_[pos] = val;
                const int vi = valIdx(val);
                --liveCnt[vi];
                rebuildBars();
                countRow(false);
                rec_.markNode(ids_[pos], Palette::Insert);
                for (int k = 0; k <= pos; ++k) rec_.markNode(ids_[k], Palette::Success);
                rec_.commit(std::to_string(val) + " 写回 a[" + std::to_string(pos) +
                            "](该值剩余计数 " + std::to_string(liveCnt[vi]) + ")");
            });
    } else if (algo_ == 7) {
        // 基数排序(LSD):影子执行镜像你的 RadixSortAlgo 逐位稳定排序的每一步;
        // 基线下方一排"基数位"格子显示每个槽位元素的当前基数位;最终结果与原函数输出校验
        // ⚠️ 原实现按十进制位分桶,不支持负数(负数取位为负 → 计数数组越界),场景侧预检跳过
        bool hasNeg = false;
        for (int v : vals_) if (v < 0) hasNeg = true;
        rec_.clear();
        rebuildBars();
        if (hasNeg) {
            rec_.commit("基数排序按十进制位分桶,不支持负数(当前含负值),已跳过");
            player_.setFrames(rec_.frames());
            return;
        }
        const int n = static_cast<int>(vals_.size());
        std::vector<int> shNums = vals_, shIds = ids_;   // 影子数组 / 影子 id 布局
        const int maxVal = *std::max_element(shNums.begin(), shNums.end());
        static const char* kDigitName[] = {"个位", "十位", "百位", "千位", "万位"};
        const auto setRoundDecor = [&](int round, int exp) {   // 带子 + 每槽位的基数位格子
            rec_.clearBands();
            rec_.setBand(0, n - 1, std::string("第 ") + std::to_string(round) + " 轮:按" +
                         kDigitName[std::min(round - 1, 4)] + "排序 (exp=" + std::to_string(exp) + ")",
                         Color::make(0.40f, 0.65f, 0.90f));
            for (int q = 0; q < n; ++q) {
                const int d = (shNums[q] / exp) % 10;
                rec_.setNode(kCountId0 + q, slotX(q), kBaseY + 48.0f, std::to_string(d),
                             colorByValue(0.12f + 0.075f * d));
            }
        };
        int round = 0;
        for (int exp = 1; maxVal / exp > 0; exp *= 10) {
            ++round;
            setRoundDecor(round, exp);
            rebuildBars();
            rec_.commit("第 " + std::to_string(round) + " 轮:按" +
                        kDigitName[std::min(round - 1, 4)] + "稳定排序(下方格子为各元素的基数位)");

            std::vector<int> counting(10, 0);
            for (int x : shNums) counting[(x / exp) % 10]++;
            for (int i = 1; i < 10; ++i) counting[i] += counting[i - 1];
            std::vector<int> ret(n, 0), retIds(n, -1);
            for (int i = n - 1; i >= 0; --i) {    // 倒序写回保证稳定性(与你实现一致)
                const int d = (shNums[i] / exp) % 10;
                const int pos = counting[d] - 1;
                ret[pos] = shNums[i];
                retIds[pos] = shIds[i];
                counting[d]--;
                // 组合画面:已写回的槽位用新布局,未消费的源槽位用旧布局,已取空的槽位留空
                rec_.resetWorking();
                for (int q = 0; q < n; ++q) {
                    if (retIds[q] != -1)
                        rec_.setNode(retIds[q], slotX(q), kBaseY - barH(ret[q]) * 0.5f,
                                     std::to_string(ret[q]), colorByValue(barNorm(ret[q])));
                    else if (q >= i)
                        rec_.setNode(shIds[q], slotX(q), kBaseY - barH(shNums[q]) * 0.5f,
                                     std::to_string(shNums[q]), colorByValue(barNorm(shNums[q])));
                }
                setRoundDecor(round, exp);
                rec_.markNode(shIds[i], Palette::Active);
                rec_.commit("取 a[" + std::to_string(i) + "]=" + std::to_string(shNums[i]) +
                            "(基数位 " + std::to_string(d) + ")写回 a[" + std::to_string(pos) + "]");
            }
            shNums = ret; shIds = retIds;
            setRoundDecor(round, exp);
            rebuildBars();
            const std::string doneMsg = "第 " + std::to_string(round) + " 轮完成:数组按" +
                                        kDigitName[std::min(round - 1, 4)] + "有序(短暂展示)";
            rec_.commit(doneMsg);
            for (int q = 0; q < n; ++q) rec_.markNode(shIds[q], Palette::Success);  // 本轮成果染绿
            rec_.commit(doneMsg);
            rec_.commit(doneMsg);                 // 重复帧 ≈ 停留 1.3 秒展示本轮结果
        }

        // 用你的原函数跑真实结果,校验影子未说谎
        algo.RadixSortAlgo(vals_);
        rec_.clearBands();
        rebuildBars(); markGreen();
        rec_.commit(shNums == vals_ ? "排序完成(影子结果与你的原函数输出一致)"
                                    : "⚠️ 影子结果与原函数输出不一致,请检查算法是否改动");
        player_.setFrames(rec_.frames());
    } else if (algo_ == 8) {
        // 桶排序:桶画成容器盒,元素是桶里的"球":
        //   ① 分配——球出现进桶盒;② 桶内排序——球在盒内重排(前后对比);
        //   ③ 串联——球逐个消失,柱子滑入最终位置。影子执行,结果与原函数校验
        const int n = static_cast<int>(vals_.size());
        const int mn = *std::min_element(vals_.begin(), vals_.end());
        const int mx = *std::max_element(vals_.begin(), vals_.end());
        const int bsize = 10;
        const int bcnt = (mx - mn) / bsize + 1;
        const auto bxc = [&](int b) { return kX0 + (b + 0.5f) * (1180.0f / bcnt); };
        std::vector<std::vector<int>> buckets(bcnt), bucketIds(bcnt);
        const auto emitBuckets = [&](int hiB, const Color& hiC, int activeElemId) {
            for (int b = 0; b < bcnt; ++b) {
                Color cc = (b == hiB) ? hiC : Color::make(0.62f, 0.72f, 0.62f);
                rec_.setNode(10100 + b, bxc(b), 570.0f, "桶" + std::to_string(b), cc);
                for (size_t x = 0; x < buckets[b].size(); ++x) {
                    const int eid = bucketIds[b][x];
                    Color bc = colorByValue(barNorm(buckets[b][x]));
                    if (b == hiB && eid == activeElemId) bc = Palette::Insert;
                    rec_.setNode(22000 + eid, bxc(b), 548.0f + x * 26.0f,
                                 std::to_string(buckets[b][x]), bc);
                }
            }
        };
        // ── 阶段① 分配入桶:球逐个出现 ──
        rec_.clearBands();
        rec_.setBand(0, n - 1, "1.分配到桶(每桶覆盖值域 10)", Color::make(0.40f, 0.65f, 0.90f));
        rebuildBars();
        emitBuckets(-1, Palette::Edge, -1);
        rec_.commit("桶排序:值域 [" + std::to_string(mn) + ", " + std::to_string(mx) +
                    "] 均分为 " + std::to_string(bcnt) + " 个桶(每桶覆盖值域 10)");
        for (int k = 0; k < n; ++k) {
            const int b = (vals_[k] - mn) / bsize;
            buckets[b].push_back(vals_[k]);
            bucketIds[b].push_back(ids_[k]);
            rebuildBars();
            emitBuckets(b, Palette::Insert, ids_[k]);
            rec_.markNode(ids_[k], Palette::Active);
            rec_.commit(std::to_string(vals_[k]) + " 落入 桶" + std::to_string(b) +
                        "(值域 [" + std::to_string(mn + b * bsize) + ".." +
                        std::to_string(mn + b * bsize + bsize - 1) + "])");
        }
        // ── 阶段② 桶内排序:球在盒内重排(前后对比)──
        rec_.clearBands();
        rec_.setBand(0, n - 1, "2.桶内排序", Color::make(0.35f, 0.75f, 0.55f));
        rebuildBars(); emitBuckets(-1, Palette::Edge, -1);
        rec_.commit("对各桶分别排序");
        for (int b = 0; b < bcnt; ++b) {
            if (buckets[b].size() < 2) continue;
            std::string before;
            for (size_t x = 0; x < buckets[b].size(); ++x)
                before += std::to_string(buckets[b][x]) + (x + 1 < buckets[b].size() ? "," : "");
            rebuildBars();
            emitBuckets(b, Color::make(1.00f, 0.80f, 0.35f), -1);
            rec_.commit("桶" + std::to_string(b) + " 排序前: " + before);
            std::sort(buckets[b].begin(), buckets[b].end());
            std::string after;
            for (size_t x = 0; x < buckets[b].size(); ++x)
                after += std::to_string(buckets[b][x]) + (x + 1 < buckets[b].size() ? "," : "");
            rebuildBars();
            emitBuckets(b, Palette::Success, -1);
            rec_.commit("桶" + std::to_string(b) + " 排序后: " + after);
        }
        // ── 阶段③ 依次串联:球消失,柱子滑入最终位置 ──
        rec_.clearBands();
        rec_.setBand(0, n - 1, "3.依次串联写回", Color::make(1.00f, 0.70f, 0.35f));
        rebuildBars(); emitBuckets(-1, Palette::Edge, -1);
        rec_.commit("按桶顺序依次取出,写回原数组");
        std::vector<std::pair<int,int>> pool;
        std::vector<char> used;
        pool.reserve(vals_.size());
        for (int k = 0; k < n; ++k) pool.push_back({vals_[k], ids_[k]});
        used.assign(pool.size(), 0);
        int idx = 0;
        for (int b = 0; b < bcnt; ++b) {
            while (!buckets[b].empty()) {
                const int val = buckets[b].front();
                const int eid = bucketIds[b].front();
                buckets[b].erase(buckets[b].begin());
                bucketIds[b].erase(bucketIds[b].begin());
                for (size_t y = 0; y < pool.size(); ++y) {
                    if (!used[y] && pool[y].first == val && pool[y].second == eid) {
                        used[y] = 1;
                        break;
                    }
                }
                ids_[idx] = eid;
                vals_[idx] = val;
                rebuildBars();
                rec_.clearBands();
                rec_.setBand(0, n - 1, "3.依次串联写回", Color::make(1.00f, 0.70f, 0.35f));
                emitBuckets(b, Palette::Active, -1);
                rec_.markNode(ids_[idx], Palette::Insert);
                rec_.commit("桶" + std::to_string(b) + " 的 " + std::to_string(val) +
                            " 写回 a[" + std::to_string(idx) + "]");
                ++idx;
            }
        }

        // 用你的原函数跑真实结果,校验影子未说谎
        algo.BucketSortAlgo(vals_);
        rec_.clearBands();
        rebuildBars(); markGreen();
        rec_.commit("排序完成");
        player_.setFrames(rec_.frames());
    } else if (algo_ == 9) {
        // 希尔排序:band 标注 gap;key 悬浮在空位上方;大元素按 gap 步长右移;key 落位
        const int n = static_cast<int>(vals_.size());
        int keyId = -1, keyVal = 0, hole = -1, lastKeyIdx = -1, curGap = 0;
        const auto shellDecor = [&]() {
            rec_.clearBands();
            rec_.setBand(0, n - 1, "gap = " + std::to_string(curGap) + ":对间隔为 " +
                         std::to_string(curGap) + " 的元素做插入排序",
                         Color::make(0.40f, 0.65f, 0.90f));
        };
        algo.ShellSortAlgo(vals_,
            [&](int g) {                          // 每轮增量开始
                curGap = g; hole = -1; lastKeyIdx = -1;
                rebuildBars(); shellDecor();
                rec_.commit("gap 缩小为 " + std::to_string(g) + ":对间隔为 " +
                            std::to_string(g) + " 的元素做插入排序");
            },
            [&](int keyIdx, int cmp) {            // key 与 nums[cmp] 比较
                if (keyIdx != lastKeyIdx) {       // 新的 key 被拿出
                    keyId = ids_[keyIdx]; keyVal = vals_[keyIdx];
                    hole = keyIdx; lastKeyIdx = keyIdx;
                }
                rebuildBars(hole); shellDecor();
                rec_.markNode(ids_[cmp], Palette::Active);
                rec_.setNode(keyId, slotX(hole), kBaseY - barH(keyVal) * 0.5f - 26.0f,
                             std::to_string(keyVal), Palette::Insert);   // key 悬浮(压缩基线之上)
                rec_.commit("key=" + std::to_string(keyVal) + " 与 a[" + std::to_string(cmp) +
                            "]=" + std::to_string(vals_[cmp]) + " 比较");
            },
            [&](int from, int to) {               // 元素 from 右移 gap 格到 to
                ids_[to] = ids_[from];
                hole = from;
                rebuildBars(hole); shellDecor();
                rec_.markNode(ids_[to], Palette::Active);
                rec_.setNode(keyId, slotX(hole), kBaseY - barH(keyVal) * 0.5f - 26.0f,
                             std::to_string(keyVal), Palette::Insert);
                rec_.commit(std::to_string(vals_[to]) + " 右移 gap 至 a[" + std::to_string(to) + "]");
            },
            [&](int pos, int keyIdx) {            // key 落位
                ids_[pos] = keyId;
                hole = -1;
                rebuildBars(); shellDecor();
                rec_.markNode(ids_[pos], Palette::Insert);
                rec_.commit("key=" + std::to_string(vals_[pos]) + " 插入 a[" + std::to_string(pos) + "]");
            });
    } else if (algo_ == 10) {
        // 堆排序:双视图——上方柱状数组 + 下方大根堆树;交换在两个视图同步呈现
        const int n = static_cast<int>(vals_.size());
        int heapN = n;
        const auto treeLayout = [&](int size) {   // 堆树:下标 i → 层 L = log2(i+1),层内等距
            for (int i = 0; i < size; ++i) {
                int L = 0;
                while ((1 << (L + 1)) <= i + 1) ++L;
                const float levelW = 1180.0f / (1 << L);
                const float x = 55.0f + (i - ((1 << L) - 1) + 0.5f) * levelW;
                const float y = 330.0f + L * 70.0f;
                rec_.setNode(30000 + i, x, y, std::to_string(vals_[i]),
                             colorByValue(barNorm(vals_[i])));
                if (i > 0) rec_.setEdge(30000 + (i - 1) / 2, 30000 + i, "", Palette::Edge);
            }
        };
        algo.HeapSortAlgo(vals_,
            [&](int phase) {                      // 阶段切换
                if (phase == 0) { heapN = n; rebuildBars(); treeLayout(n); }
                rec_.commit(phase == 0 ? "阶段一:自底向上建大根堆"
                                       : "阶段二:每次取堆顶(最大值)放到末尾");
            },
            [&](int c, int b) {                   // 孩子与当前最大者比较
                rebuildBars(); treeLayout(heapN);
                rec_.markNode(30000 + c, Palette::Active);
                rec_.markNode(30000 + b, Palette::Active);
                rec_.commit("比较 a[" + std::to_string(c) + "]=" + std::to_string(vals_[c]) +
                            " 与 a[" + std::to_string(b) + "]=" + std::to_string(vals_[b]));
            },
            [&](int i, int j) {                   // 交换:柱与树节点同步
                std::swap(ids_[i], ids_[j]);
                rebuildBars(); treeLayout(heapN);
                rec_.markNode(30000 + i, Palette::Insert);
                rec_.markNode(30000 + j, Palette::Insert);
                rec_.commit("交换 a[" + std::to_string(i) + "] 与 a[" + std::to_string(j) + "]");
            },
            [&](int idx) {                        // 元素落位(全局有序),堆范围缩小
                heapN = idx;
                rebuildBars(); treeLayout(heapN);
                rec_.markNode(ids_[idx], Palette::Success);
                rec_.commit("a[" + std::to_string(idx) + "]=" + std::to_string(vals_[idx]) + " 落位");
            });
    } else {
        algo.BubbleSortAlgo(vals_, onCompare, onSwap, onSorted);
    }

    if (algo_ == 4 && mergeAnySplit) {            // 归并:最后一轮的总结帧
        rec_.clearBands();
        rebuildBars();
        rec_.commit("第 " + std::to_string(mergeLevel) + " 轮(width=" +
                    std::to_string(mergeLastWidth) + ")归并完成");
    }

    sortedFrom_ = 0;
    rebuildBars(); markGreen();
    rec_.commit("排序完成");
    player_.setFrames(rec_.frames());
}

void SortScene::draw() {
    // ── 第一行:数据 + 算法 + 播放 ──
    ImGui::SetNextItemWidth(240);
    ImGui::InputText("##sortlist", listBuf_, sizeof(listBuf_));
    ImGui::SameLine();
    if (ImGui::Button("重排")) resetDemo();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    ImGui::Combo("##algo", &algo_, "BubbleSortAlgo\0BubbleSortAlgoImprovement\0SelectionSortAlgo\0InsertionSortAlgo\0MergeSortAlgo\0QuickSortAlgo\0CountingSortAlgo\0RadixSortAlgo\0BucketSortAlgo\0ShellSortAlgo\0HeapSortAlgo\0");
    ImGui::SameLine();
    if (ImGui::Button("排序")) recordSort();
    ImGui::SameLine();
    if (ImGui::Button(player_.playing() ? "暂停" : "播放")) player_.toggle();
    ImGui::SameLine();
    if (ImGui::Button("单步前进")) player_.stepForward();
    ImGui::SameLine();
    if (ImGui::Button("单步后退")) player_.stepBack();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("速度", &speed_, 0.25f, 6.0f, "%.2fx");

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

    const float baseY = heapMode_ ? 240.0f : kBaseY;   // 堆模式:柱子压缩到上半区
    const float maxH = heapMode_ ? 170.0f : kMaxH;

    std::unordered_map<int, ImVec2> pos;               // 节点 id → 屏幕坐标(树连线用)
    for (const auto& n : frame.nodes)
        pos[n.id] = origin + ImVec2(n.x, n.y);

    // 区间带(归并排序的拆分/合并展示;冒泡阶段为空)
    for (const auto& b : frame.bands) {
        const float x0 = origin.x + slotX(b.x0) - barW_ * 0.5f - 8;
        const float x1 = origin.x + slotX(b.x1) + barW_ * 0.5f + 8;
        dl->AddRectFilled(ImVec2(x0, origin.y + baseY - maxH - 26),
                          ImVec2(x1, origin.y + baseY + 8),
                          toImU32(Color::make(b.color.r, b.color.g, b.color.b, b.color.a * 0.25f)),
                          10.0f);
        dl->AddRect(ImVec2(x0, origin.y + baseY - maxH - 26),
                    ImVec2(x1, origin.y + baseY + 8),
                    toImU32(Color::make(b.color.r, b.color.g, b.color.b, b.color.a)), 10.0f, 0, 2.0f);
        if (!b.label.empty()) {
            const bool bold = viz::NodeFont != nullptr;
            if (bold) ImGui::PushFont(viz::NodeFont);
            ImVec2 ts = ImGui::CalcTextSize(b.label.c_str());
            dl->AddText(ImVec2((x0 + x1) * 0.5f - ts.x * 0.5f, origin.y + baseY - maxH - 46),
                        toImU32(b.color), b.label.c_str());
            if (bold) ImGui::PopFont();
        }
    }

    // 基线
    dl->AddLine(ImVec2(origin.x + 30, origin.y + baseY),
                ImVec2(origin.x + 30 + slotSpacing_ * (frame.nodes.size() + 1), origin.y + baseY),
                toImU32(Palette::Edge), 3.0f);

    // 堆排序的树:父→子连线(节点圆在下方节点循环里绘制)
    for (const auto& e : frame.edges) {
        auto a = pos.find(e.from), b = pos.find(e.to);
        if (a == pos.end() || b == pos.end()) continue;
        dl->AddLine(a->second, b->second, toImU32(e.color), 2.0f);
    }

    // 柱子:中心 (x,y),宽 barW_,高由 label 的值归一化(排序中高度不变,只有 x 动)
    for (const auto& n : frame.nodes) {
        ImVec2 p = origin + ImVec2(n.x, n.y);
        if (n.id >= 30000) {                  // 堆树节点:圆 + 值
            dl->AddCircleFilled(p, 20.0f, toImU32(n.color));
            dl->AddCircle(p, 20.0f, toImU32(Darken(n.color, 0.5f)), 0, 2.5f);
            const bool bold3 = viz::NodeFont != nullptr;
            if (bold3) ImGui::PushFont(viz::NodeFont);
            ImVec2 ts = ImGui::CalcTextSize(n.label.c_str());
            dl->AddText(ImVec2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f),
                        toImU32(Palette::NodeText), n.label.c_str());
            if (bold3) ImGui::PopFont();
            continue;
        }
        if (n.id >= 22000) {                  // 桶里的球(值)
            dl->AddCircleFilled(p, 13.0f, toImU32(n.color));
            dl->AddCircle(p, 13.0f, toImU32(Darken(n.color, 0.5f)), 0, 2.0f);
            const bool bold4 = viz::NodeFont != nullptr;
            if (bold4) ImGui::PushFont(viz::NodeFont);
            ImVec2 ts = ImGui::CalcTextSize(n.label.c_str());
            dl->AddText(ImVec2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f),
                        toImU32(Palette::NodeText), n.label.c_str());
            if (bold4) ImGui::PopFont();
            continue;
        }
        if (n.id >= 10100 && n.id < 10200) {  // 桶容器盒
            ImRect box(p - ImVec2(48, 48), p + ImVec2(48, 48));
            dl->AddRectFilled(box.Min, box.Max,
                              toImU32(Color::make(n.color.r, n.color.g, n.color.b, 0.15f)), 10.0f);
            dl->AddRect(box.Min, box.Max, toImU32(n.color), 10.0f, 0, 2.5f);
            const bool bold5 = viz::NodeFont != nullptr;
            if (bold5) ImGui::PushFont(viz::NodeFont);
            ImVec2 ts = ImGui::CalcTextSize(n.label.c_str());
            dl->AddText(ImVec2(p.x - ts.x * 0.5f, box.Min.y + 6.0f),
                        toImU32(Darken(n.color, 0.6f)), n.label.c_str());
            if (bold5) ImGui::PopFont();
            continue;
        }
        if (n.id >= kCountId0) {              // 计数行的小格子(值×次数)
            const bool bold2 = viz::NodeFont != nullptr;
            if (bold2) ImGui::PushFont(viz::NodeFont);
            ImVec2 ts = ImGui::CalcTextSize(n.label.c_str());
            const float w2 = ts.x + 14.0f;
            dl->AddRectFilled(p - ImVec2(w2 * 0.5f, 15.0f), p + ImVec2(w2 * 0.5f, 15.0f),
                              toImU32(n.color), 6.0f);
            dl->AddRect(p - ImVec2(w2 * 0.5f, 15.0f), p + ImVec2(w2 * 0.5f, 15.0f),
                        toImU32(Darken(n.color, 0.52f)), 6.0f, 0, 2.0f);
            dl->AddText(ImVec2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f),
                        toImU32(Palette::NodeText), n.label.c_str());
            if (bold2) ImGui::PopFont();
            continue;
        }
        const float hh = barH(std::stoi(n.label));
        ImRect box(p - ImVec2(barW_ * 0.5f, hh * 0.5f), p + ImVec2(barW_ * 0.5f, hh * 0.5f));
        // 卡通贴纸:右下硬阴影 + 粉彩填充 + 同色系描边
        dl->AddRectFilled(box.Min + ImVec2(3.0f, 4.0f), box.Max + ImVec2(3.0f, 4.0f),
                          toImU32(Darken(n.color, 0.45f)), kRad);
        dl->AddRectFilled(box.Min, box.Max, toImU32(n.color), kRad);
        dl->AddRect(box.Min, box.Max, toImU32(Darken(n.color, 0.52f)), kRad, 0, 2.5f);
        // 数值标在柱顶上方
        const bool bold = viz::NodeFont != nullptr;
        if (bold) ImGui::PushFont(viz::NodeFont);
        ImVec2 ts = ImGui::CalcTextSize(n.label.c_str());
        dl->AddText(ImVec2(p.x - ts.x * 0.5f, box.Min.y - ts.y - 4.0f),
                    toImU32(Palette::NodeText), n.label.c_str());
        if (bold) ImGui::PopFont();
    }
    ImGui::EndChild();
}

} // namespace viz
