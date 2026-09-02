// ═══════════════════════════════════════════════════════════════
// Player:对快照序列做插值播放。
//
// 时间线模型:第 i 帧占据 [i*stepDur, (i+1)*stepDur),
// 相邻帧之间对节点位置/颜色做插值,因此插入、交换、旋转等
// 操作都会以连续动画呈现,而不是跳变。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "snapshot.h"

#include <algorithm>
#include <string>
#include <vector>

namespace viz {

class Player {
public:
    // 换一组帧并从第 0 帧开始播放
    void setFrames(std::vector<Snapshot> frames) {
        frames_ = std::move(frames);
        time_ = 0.0;
        playing_ = true;
    }

    // 每帧推进时间线(dt 为秒)
    void update(double dt) {
        if (!playing_ || frames_.size() < 2) return;
        time_ += dt * speed_;
        if (time_ >= maxTime()) {   // 播完自动暂停在最后一帧
            time_ = maxTime();
            playing_ = false;
        }
    }

    void play()  { if (atEnd()) time_ = 0.0; playing_ = true; }
    void pause() { playing_ = false; }
    void toggle() { playing_ ? pause() : play(); }

    // 单步:精确落到下一帧/上一帧的开头
    void stepForward() { playing_ = false; time_ = std::min(time_ + stepDur_, maxTime()); }
    void stepBack()    { playing_ = false; time_ = std::max(time_ - stepDur_, 0.0); }

    void setSpeed(double s) { speed_ = s; }
    double speed() const    { return speed_; }
    bool playing() const    { return playing_; }
    bool atEnd() const      { return frames_.size() < 2 || time_ >= maxTime(); }
    int frameCount() const  { return static_cast<int>(frames_.size()); }

    // 当前所在整帧的下标(插值起点)
    int frameIndex() const {
        if (frames_.empty()) return 0;
        return std::min(static_cast<int>(time_ / stepDur_),
                        static_cast<int>(frames_.size()) - 1);
    }

    // 播放进度 0..1(供进度条用)
    double progress() const { return frames_.size() < 2 ? 1.0 : time_ / maxTime(); }

    // 当前步骤说明文字
    const std::string& currentDesc() const {
        static const std::string kEmpty;
        return frames_.empty() ? kEmpty : frames_[frameIndex()].desc;
    }

    // 当前时刻的插值画面;空/单帧时原样返回
    Snapshot interpolated() const {
        if (frames_.empty()) return Snapshot{};
        int i = frameIndex();
        if (i == static_cast<int>(frames_.size()) - 1) return frames_[i];

        const Snapshot& a = frames_[i];
        const Snapshot& b = frames_[i + 1];
        float t = static_cast<float>((time_ - i * stepDur_) / stepDur_);
        t = std::clamp(t, 0.0f, 1.0f);
        t = t * t * (3.0f - 2.0f * t);   // smoothstep:缓入缓出,动画更自然

        Snapshot out;
        out.desc = a.desc;
        // 节点:A 有 B 无 → 淡出;A 无 B 有 → 淡入;都有 → 位置/颜色插值
        for (const auto& na : a.nodes) {
            const auto* nb = findNode(b, na.id);
            if (!nb) {
                out.nodes.push_back(na);
                out.nodes.back().color.a *= (1.0f - t);
                continue;
            }
            out.nodes.push_back(Snapshot::Node{
                na.id,
                na.x + (nb->x - na.x) * t,
                na.y + (nb->y - na.y) * t,
                nb->label,
                lerpColor(na.color, nb->color, t)});
        }
        for (const auto& nb : b.nodes) {
            if (!findNode(a, nb.id)) {
                out.nodes.push_back(nb);
                out.nodes.back().color.a *= t;
            }
        }
        // 边:按 (from, to) 匹配
        for (const auto& ea : a.edges) {
            const auto* eb = findEdge(b, ea.from, ea.to);
            if (!eb) {
                out.edges.push_back(ea);
                out.edges.back().color.a *= (1.0f - t);
                continue;
            }
            out.edges.push_back(Snapshot::Edge{ea.from, ea.to, eb->weight,
                                               lerpColor(ea.color, eb->color, t)});
        }
        for (const auto& eb : b.edges) {
            if (!findEdge(a, eb.from, eb.to)) {
                out.edges.push_back(eb);
                out.edges.back().color.a *= t;
            }
        }
        return out;
    }

private:
    static const Snapshot::Node* findNode(const Snapshot& s, int id) {
        for (const auto& n : s.nodes)
            if (n.id == id) return &n;
        return nullptr;
    }
    static const Snapshot::Edge* findEdge(const Snapshot& s, int from, int to) {
        for (const auto& e : s.edges)
            if (e.from == from && e.to == to) return &e;
        return nullptr;
    }
    double maxTime() const { return (frames_.size() - 1) * stepDur_; }

    std::vector<Snapshot> frames_;
    double time_ = 0.0;    // 时间线位置(秒)
    double stepDur_ = 0.9; // 每步基础时长(秒)
    double speed_ = 1.0;
    bool playing_ = true;
};

} // namespace viz
