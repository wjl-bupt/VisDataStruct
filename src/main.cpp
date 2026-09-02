// ═══════════════════════════════════════════════════════════════
// 入口:SDL2 窗口 + ImGui 渲染循环 + 场景 Tab 切换。
//
// 新增场景:在这里的 scenes 列表里注册即可,见下方注释。
// ═══════════════════════════════════════════════════════════════
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "scenes/list_scene.h"
#include "scenes/scene.h"
#include "scenes/tree_scene.h"

#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

namespace {

// 加载中文字体:按候选路径逐个探测,找不到就用默认字体(中文会显示为方块)
void loadCjkFont() {
    const char* kCandidates[] = {
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/arphic/uming.ttc",
    };
    ImGuiIO& io = ImGui::GetIO();
    for (const char* path : kCandidates) {
        std::ifstream f(path, std::ios::binary);
        if (f.good()) {
            io.Fonts->AddFontFromFileTTF(path, 18.0f, nullptr,
                                         io.Fonts->GetGlyphRangesChineseFull());
            return;
        }
    }
    std::fprintf(stderr, "[提示] 未找到中文字体,中文将显示为方块。可安装 fonts-noto-cjk 解决。\n");
}

} // namespace

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("数据结构可视化 (ImGui + SDL2)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    loadCjkFont();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // ── 场景注册:新增场景在这里 emplace_back ──
    std::vector<std::unique_ptr<viz::Scene>> scenes;
    scenes.emplace_back(std::make_unique<viz::ListScene>());
    scenes.emplace_back(std::make_unique<viz::TreeScene>());
    // scenes.emplace_back(std::make_unique<viz::StackScene>());
    // ...
    int current = 0;

    bool running = true;
    Uint64 lastTicks = SDL_GetTicks64();
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) running = false;
        }

        Uint64 now = SDL_GetTicks64();
        double dt = std::min(0.1, (now - lastTicks) / 1000.0);   // 限制单帧最大步长
        lastTicks = now;

        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui::NewFrame();

        scenes[current]->update(dt);

        // 全屏主窗口:Tab 栏与场景都画在里面。
        // 必须先 Begin——否则 ImGui 把控件塞进自动收缩的 "Debug" 回退窗口,
        // 场景里的 BeginChild(0,0) 画布高度会变成 0,节点全部被裁掉不显示。
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::Begin("##main", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleVar();

        // 场景切换 Tab 栏
        if (ImGui::BeginTabBar("##scenes")) {
            for (int i = 0; i < static_cast<int>(scenes.size()); ++i) {
                if (ImGui::BeginTabItem(scenes[i]->name())) {
                    current = i;
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        scenes[current]->draw();

        ImGui::End();

        ImGui::Render();
        SDL_RenderSetScale(renderer, ImGui::GetIO().DisplayFramebufferScale.x,
                                   ImGui::GetIO().DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 26, 30, 35, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
