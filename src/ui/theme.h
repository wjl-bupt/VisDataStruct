// ═══════════════════════════════════════════════════════════════
// 卡通浅色主题:统一调整 ImGui 风格(浅色 + 圆角 + 粉彩控件)。
// 节点画布配色在 core/animation/snapshot.h 的 Palette;想换风格改这两处即可。
// ═══════════════════════════════════════════════════════════════
#pragma once

#include "imgui.h"

namespace viz {

// 节点标签用的粗体字体;未找到中文字体时为 nullptr,场景绘制需判空
inline ImFont* NodeFont = nullptr;

inline void ApplyCartoonTheme() {
    ImGui::StyleColorsLight();
    ImGuiStyle& st = ImGui::GetStyle();

    // 大圆角是卡通感的关键
    st.WindowRounding    = 10.0f;
    st.ChildRounding     = 10.0f;
    st.FrameRounding     = 7.0f;
    st.PopupRounding     = 8.0f;
    st.GrabRounding      = 6.0f;
    st.ScrollbarRounding = 8.0f;
    st.TabRounding       = 8.0f;
    st.WindowBorderSize  = 0.0f;
    st.FramePadding      = ImVec2(8, 5);
    st.WindowPadding     = ImVec2(12, 10);

    const ImVec4 cream(0.99f, 0.96f, 0.90f, 1.0f);      // 奶油纸底
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]        = cream;
    c[ImGuiCol_ChildBg]         = cream;
    c[ImGuiCol_PopupBg]         = ImVec4(0.995f, 0.975f, 0.935f, 0.985f);
    c[ImGuiCol_Button]          = ImVec4(1.00f, 0.89f, 0.65f, 1.0f);   // 粉彩黄按钮
    c[ImGuiCol_ButtonHovered]   = ImVec4(1.00f, 0.83f, 0.50f, 1.0f);
    c[ImGuiCol_ButtonActive]    = ImVec4(0.97f, 0.74f, 0.38f, 1.0f);
    c[ImGuiCol_PlotHistogram]   = ImVec4(1.00f, 0.74f, 0.42f, 1.0f);   // 进度条
    c[ImGuiCol_SliderGrab]      = ImVec4(0.98f, 0.68f, 0.36f, 1.0f);
    c[ImGuiCol_SliderGrabActive]= ImVec4(0.93f, 0.58f, 0.24f, 1.0f);
    c[ImGuiCol_Separator]       = ImVec4(0.87f, 0.80f, 0.68f, 1.0f);
    c[ImGuiCol_TextSelectedBg]  = ImVec4(1.00f, 0.84f, 0.44f, 0.60f);

    // 粗体中文字体(节点标签用);找不到会顺次尝试兜底路径,全部失败则保持 nullptr
    // AddFontFromFileTTF 失败时返回 nullptr,无需预先探测文件
    ImGuiIO& io = ImGui::GetIO();
    const char* kBoldCandidates[] = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Bold.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Bold.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",   // 兜底:与默认字体同款
    };
    for (const char* path : kBoldCandidates) {
        ImFont* f = io.Fonts->AddFontFromFileTTF(path, 18.0f, nullptr,
                                                 io.Fonts->GetGlyphRangesChineseFull());
        if (f) { NodeFont = f; break; }
    }
}

} // namespace viz
