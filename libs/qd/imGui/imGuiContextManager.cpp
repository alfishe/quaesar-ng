#include "qd/imGui/imGuiContextManager.h"
#if QD_USE_SDL
#include "SDL.h"
#include "qd/imGui/backends/sdl2/imgui_impl_sdl2.h"
#include "qd/imGui/backends/sdl2/imgui_impl_sdlrenderer2.h"
#else
#include "qd/imGui/backends/win32/imgui_impl_win32.h"
#include "qd/imGui/backends/win32/imgui_impl_dx11.h"
#endif
#include "qd/stl/algorithm.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// Merge Font Awesome icons into ImGui font atlases.
// Called for each new (non-shared) atlas. A set tracks which atlases
// already have FA loaded so we don't double-merge.
#include <unordered_set>
static std::unordered_set<ImFontAtlas*> s_faLoadedAtlases;
static void loadFontAwesomeIcons(ImFontAtlas* atlas)
{
    if (!atlas || s_faLoadedAtlases.count(atlas))
        return;
    s_faLoadedAtlases.insert(atlas);

    // Build path: <exe_dir>/data/static/fa-solid-900.ttf
    char* basePath = SDL_GetBasePath();
    if (!basePath) {
        SDL_Log("[FA] SDL_GetBasePath() returned NULL");
        return;
    }
    qtd::string fontPath = qtd::string(basePath) + "data/static/fa-solid-900.ttf";
    SDL_Log("[FA] loading into atlas=%p, path=%s", (void*)atlas, fontPath.c_str());
    SDL_free(basePath);

    // Single broad range: covers all FA PUA glyphs we might need.
    // Individual ranges were causing missing glyphs due to stb_rect_pack issues
    // with sparse codepoints.
    static const ImWchar fa_ranges[] = {
        0xF000, 0xF1FF,  // FA solid icons (wide range, ~512 glyphs)
        0,
    };

    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.PixelSnapH = true;
    // Let atlas own the data — it copies the buffer internally and
    // manages the lifetime correctly through Build() and destruction.
    SDL_RWops* rw = SDL_RWFromFile(fontPath.c_str(), "rb");
    if (!rw) {
        SDL_Log("[FA] failed to open font file");
        return;
    }
    Sint64 size = SDL_RWsize(rw);
    if (size <= 0) {
        SDL_Log("[FA] font file size=%lld (invalid)", (long long)size);
        SDL_RWclose(rw);
        return;
    }
    SDL_Log("[FA] font file size=%lld", (long long)size);
    void* data = SDL_malloc((size_t)size);
    if (SDL_RWread(rw, data, 1, (size_t)size) != (size_t)size) {
        SDL_Log("[FA] failed to read font data");
        SDL_free(data);
        SDL_RWclose(rw);
        return;
    }
    SDL_RWclose(rw);

    ImFont* result = atlas->AddFontFromMemoryTTF(data, (int)size, 13.0f, &cfg, fa_ranges);
    // Atlas now owns 'data' (default FontDataOwnedByAtlas=true), will free it later.
    SDL_Log("[FA] loaded into atlas=%p, font=%p, atlas fonts=%d",
            (void*)atlas, (void*)result, atlas->Fonts.Size);
}


QD_MODULE_REGISTRATION(qd::ImGuiContextManager);


namespace qd {


ImGuiContextManager::ImGuiContextManager(const qd::ModuleCreateParams& mc)
    : TSuper(mc) {}


QImGuiContext* ImGuiContextManager::createContextImGui(SDL_Window* window, SDL_Renderer* renderer) {

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImFontAtlas* pSharedFontAtlas = m_pSharedFontAtlas;
    if (pSharedFontAtlas && !m_pImContexts.empty()) {
        if (m_pImContexts[0]->m_pRenderer != renderer)
            pSharedFontAtlas = nullptr;
    }

    ::ImGuiContext* pImCon = ImGui::CreateContext(pSharedFontAtlas);

    QImGuiContext* pQContext = new qd::QImGuiContext(pImCon, this);
    pQContext->m_windowId = SDL_GetWindowID(window);
    pQContext->m_pRenderer = renderer;
    pQContext->useCurrent();
    m_pImContexts.push_back(pQContext);

    ImGuiIO& io = pQContext->getIO();
    // For every new (non-shared) atlas, add default font + FA icons.
    // The shared atlas already has them from the first context.
    if (!pSharedFontAtlas) {
        if (io.Fonts->Fonts.Size == 0)
            io.Fonts->AddFontDefault();
        loadFontAwesomeIcons(io.Fonts);
    }
    // Remember the first atlas as the shared one.
    if (!m_pSharedFontAtlas)
        m_pSharedFontAtlas = io.Fonts;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    return pQContext;
}


void ImGuiContextManager::destroyImContext(QImGuiContext* pQContext) {

    auto It = qtd::find(m_pImContexts.begin(), m_pImContexts.end(), pQContext);
    if (It != m_pImContexts.end()) {
        pQContext->m_pParentModule = nullptr;
        ImGuiContext* pPrevCtx = pQContext->useCurrent();
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(pQContext->m_pImGuiContext);
        pQContext->m_pImGuiContext = nullptr;
        m_pImContexts.erase(It);

        ImGui::SetCurrentContext(pPrevCtx);
    }
}


ImGuiContextManager::~ImGuiContextManager() {
    assert(m_pImContexts.empty());
}


void ImGuiContextManager::destroyModule() {
    while (!m_pImContexts.empty()) {
        destroyImContext(m_pImContexts.back());
    }
}


ImGuiContext* QImGuiContext::useCurrent() const {
    assert(m_pImGuiContext);
    ImGuiContext* pOldContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(m_pImGuiContext);
    return pOldContext;
}


void QImGuiContext::setImGuiContext(ImGuiContext* pImGuiContext) {
    ImGui::SetCurrentContext(pImGuiContext);
}


ImGuiIO& QImGuiContext::getIO() const {
    assert(m_pImGuiContext);
    return m_pImGuiContext->IO;
}


qd::EFlow QImGuiContext::onSdlEventProc(SDL_Event& event) {
    ImGuiContext* pOldCtx = useCurrent();
    qd::EFlow r;
    r = ImGui_ImplSDL2_ProcessEvent(&event);
    setImGuiContext(pOldCtx);
    return r;
}


void QImGuiContext::newFrame() {

    m_frameStarted = true;
    useCurrent();
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}


void QImGuiContext::endFrame() {
    ImGui::EndFrame();
    m_frameEnded = true;
}


void QImGuiContext::render(qd::Color clear_color) {

    QImGuiContext* pContext = this;
    pContext->useCurrent();

    ImGuiIO& io = getIO();
    ImGui::Render();
    SDL_RenderSetScale(m_pRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    if (clear_color.a) {
        SDL_SetRenderDrawColor(m_pRenderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        SDL_RenderClear(m_pRenderer);
    }

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_pRenderer);
    SDL_RenderPresent(m_pRenderer);
}


void QImGuiContext::destroy() {

    if (m_pParentModule) {
        m_pParentModule->destroyImContext(this);
        m_pParentModule = nullptr;
    }
}


void CImGuiDrawListRender::destroy() {}


}; // namespace qd
//////////////////////////////////////////////////////////////////////////
