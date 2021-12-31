#include <assert.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "colors.h"

#define DEBUG_OVERLAY_KEY "F1"
#define DRAW_THING_KEY "Space"
static void OverlayDebugWindow(bool* p_open)
{
    static int corner = 0;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav;
    if (corner != -1)
    {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Debug Overlay", p_open, window_flags))
    {
        { // Show framerate
            ImGui::Text("Framerate: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Separator();
        }
        { // Show mouse position
            if (ImGui::IsMousePosValid())
                ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
            else
                ImGui::Text("Mouse Position: <invalid>");
            ImGui::Separator();
        }
        { // Global ImGuiStyle
            // See ImGuiStyle::ImGuiStyle()
            /* ImGuiStyle& style = ImGui::GetStyle(); */
            /* float alpha = style.Alpha; */
            /* ImGui::Text("ImGui::GetStyle().Alpha: %f",alpha); */
            /* ImGui::Separator(); */
        }
        { // Bitmask colors to set alpha
            /* ImU32 color32; */
            /* color32 = ImGui::GetColorU32(bwc_tardis); */
            /* ImGui::Text("Tardis: 0x%8X",color32); */
            /* ImGui::Text("A_MASK: 0x%8X",IM_COL32_A_MASK); */
            /* ImGui::Text("~A_MASK: 0x%8X",~IM_COL32_A_MASK); */
            /* color32 &= ~(IM_COL32_A_MASK); // clear alpha */
            /* ImGui::Text("Tardis & ~A_MASK: 0x%8X",color32); */
            /* ImU32 alpha = 100<<IM_COL32_A_SHIFT; */
            /* color32 |= alpha; */
            /* ImGui::Text("Tardis | alpha: 0x%8X",color32); */
            /* ImGui::Separator(); */
        }
        { // Move/close/re-open the debug overlay
            ImGui::Text("Right-click to move this debug overlay, %s to close/open.", DEBUG_OVERLAY_KEY);
            ImGui::Text("%s to draw a thing.", DRAW_THING_KEY);
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
                if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
                if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
                if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
                if (p_open && ImGui::MenuItem("Close")) *p_open = false;
                ImGui::EndPopup();
            }
        }
    }
    ImGui::End();
}
static void DrawCircles(const ImVec4 color)
{
    // Center the circles in the screen.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 center;
    center.x = work_pos.x + work_size.x/2;
    center.y = work_pos.y + work_size.y/2;
    // Convert ImVec4 RGBA (4x1 float) to ImU32 ABGR (packed 4x1 byte)
    ImU32 color32 = ImGui::GetColorU32(color);
    // Decrease alpha to darken the circles
    ImU32 dim_alpha = 100<<IM_COL32_A_SHIFT;
    ImU32 full_alpha = 255<<IM_COL32_A_SHIFT;
    /* color32 &= ~(IM_COL32_A_MASK); // clear alpha */
    /* color32 |= dim_alpha; // set new alpha */
    ImGuiIO& io = ImGui::GetIO();
    for (int i=3; i<30; i++)
    {
        float radius = work_size.x/i;
        // If the mouse is within the circle, make it bright.
        float x_dist = io.MousePos.x - center.x;
        float y_dist = io.MousePos.y - center.y;
        float sq_dist = x_dist*x_dist + y_dist*y_dist;
        if ((radius*radius) > sq_dist)
        {
            color32 &= ~(IM_COL32_A_MASK); // clear alpha
            color32 |= full_alpha; // set new alpha
        }
        else
        {
            color32 &= ~(IM_COL32_A_MASK); // clear alpha
            color32 |= dim_alpha; // set new alpha
        }
        ImGui::GetBackgroundDrawList()->AddCircle(
                center, radius, color32,
                0, // num segments
                3 // thickness
                );
    }
}


// Main code
int main(int, char**)
{
    /* =====[ Setup ]===== */

    // Start up SDL subsystems.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Could not start up SDL: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    // https://wiki.libsdl.org/SDL_GL_SetAttribute
    // Set OpenGL attributes before creating an OpenGL window.
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // https://wiki.libsdl.org/SDL_CreateWindow
    // Set window flags
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
            // IMGUI says use these flags:
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
            // Other flags I like:
            | SDL_WINDOW_FULLSCREEN_DESKTOP
            /* | SDL_WINDOW_BORDERLESS */
            );
    // Create the window
    SDL_Window* window = SDL_CreateWindow(
            "GRID",
            SDL_WINDOWPOS_CENTERED, // or SDL_WINDOWPOS_UNDEFINED
            SDL_WINDOWPOS_CENTERED, // or SDL_WINDOWPOS_UNDEFINED
            1280, 720, // WxH for windowed, 0x0 OK for FULLSCREEN_DESKTOP
            window_flags
            );
    if (window == NULL)
    {
        printf("Could not create window: %s\n", SDL_GetError());
        return -1;
    }
    else
    {
        printf("SDL Initialized.\n");
        fflush(stdout);
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    /* =====[ IMGUI BOILERPLATE START ]===== */
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    /* io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls */
    /* io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls */

    // Setup Dear ImGui style
    /* ImGui::StyleColorsDark(); */
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    /* =====[ IMGUI BOILERPLATE END ]===== */

    // Our state
    bool show_demo_window = false;
    bool show_debug_overlay = true;
    bool draw_circles = true;

    // clear_color is the color of the background
    ImVec4 clear_color = bwc_blackestgravel;

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Click window close
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            // Alt+F4 close
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            // Catch keystrokes
            SDL_Keycode code = event.key.keysym.sym;

            switch (code)
            {
                case SDLK_ESCAPE: // Esc - Quit
                    done = true;
                    break;
                case SDLK_F1: // F1 - Toggle the debug overlay
                    if (event.type == SDL_KEYDOWN)
                    {
                        // DEBUG_OVERLAY_KEY
                        show_debug_overlay = !show_debug_overlay;
                    }
                    break;
                case SDLK_SPACE: // Space - Draw a thing
                    if (event.type == SDL_KEYDOWN)
                    {
                        draw_circles = !draw_circles;
                    }
                default:
                    break;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (show_debug_overlay) OverlayDebugWindow(&show_debug_overlay);
        if (draw_circles) DrawCircles(bwc_tardis);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);


        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    /* =====[ Cleanup ]===== */

    /* =====[ IMGUI BOILERPLATE START ]===== */
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    /* =====[ IMGUI BOILERPLATE END ]===== */

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);

    // Shutdown all SDL subsystems.
    SDL_Quit();

    return 0;
}
// vim:set fdm=syntax:
