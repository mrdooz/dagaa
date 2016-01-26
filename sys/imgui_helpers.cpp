#if WITH_IMGUI
#ifdef GetWindowFont
#undef GetWindowFont
#endif

#include <contrib/imgui/imgui.cpp>
#include <contrib/imgui/imgui_demo.cpp>
#include <contrib/imgui/imgui_draw.cpp>
#include "imgui_helpers.hpp"
#include <sys/msys_graphics.hpp>
#include <sys/gpu_objects.hpp>
//#include "graphics_context.hpp"
//#include "init_sequence.hpp"
//#include "tokko.hpp"

//using namespace tokko;
//using namespace tokko;

using namespace std;

namespace
{
  struct CUSTOMVERTEX
  {
    float pos[2];
    float uv[2];
    unsigned int col;
  };

  struct VERTEX_CONSTANT_BUFFER
  {
    float mvp[4][4];
  };

  LARGE_INTEGER ticksPerSecond;
  LARGE_INTEGER lastTime = {0};

  //GraphicsContext* g_Graphics;
  //ConstantBuffer<VERTEX_CONSTANT_BUFFER> g_cb;

  GpuObjects g_gpuObjects;
  GpuState g_gpuState;
  ObjectHandle g_texture;
  ObjectHandle g_constantBuffer;
}

static int VB_SIZE = 1024 * 1024;
static int IB_SIZE = 1024 * 1024;

//------------------------------------------------------------------------------
bool InitDeviceD3D()
{
  //BEGIN_INIT_SEQUENCE();

  // Setup rasterizer
  D3D11_RASTERIZER_DESC RSDesc;
  memset(&RSDesc, 0, sizeof(D3D11_RASTERIZER_DESC));
  RSDesc.FillMode = D3D11_FILL_SOLID;
  RSDesc.CullMode = D3D11_CULL_NONE;
  RSDesc.FrontCounterClockwise = FALSE;
  RSDesc.DepthBias = 0;
  RSDesc.SlopeScaledDepthBias = 0.0f;
  RSDesc.DepthBiasClamp = 0;
  RSDesc.DepthClipEnable = TRUE;
  RSDesc.ScissorEnable = TRUE;
  RSDesc.AntialiasedLineEnable = FALSE;

  // MAGNUS: check for multisampling
  // RSDesc.MultisampleEnable = (sd.SampleDesc.Count > 1) ? TRUE : FALSE;

  // Create the blending setup
  D3D11_BLEND_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.AlphaToCoverageEnable = false;
  desc.RenderTarget[0].BlendEnable = true;
  desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  CD3D11_DEPTH_STENCIL_DESC depthDesc = CD3D11_DEPTH_STENCIL_DESC(D3D11_DEFAULT);
  depthDesc.DepthEnable = FALSE;

  g_gpuState.Create(&depthDesc, &desc, &RSDesc);
  g_gpuObjects.CreateDynamicVb(VB_SIZE, sizeof(CUSTOMVERTEX));
  g_gpuObjects.CreateDynamicIb(IB_SIZE, DXGI_FORMAT_R32_UINT);

  g_constantBuffer = g_Graphics->CreateBuffer(
      D3D11_BIND_CONSTANT_BUFFER, sizeof(VERTEX_CONSTANT_BUFFER), true, nullptr);

  // TODO(magnus)...
  // Create shaders
  //vector<D3D11_INPUT_ELEMENT_DESC> inputDesc = {
  //    CD3D11_INPUT_ELEMENT_DESC("POSITION", DXGI_FORMAT_R32G32_FLOAT),
  //    CD3D11_INPUT_ELEMENT_DESC("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT),
  //    CD3D11_INPUT_ELEMENT_DESC("COLOR", DXGI_FORMAT_R8G8B8A8_UNORM),
  //};
  //INIT(g_gpuObjects.LoadVertexShader("shaders/out/imgui", "VsMain", 0, &inputDesc));
  //INIT(g_gpuObjects.LoadPixelShader("shaders/out/imgui", "PsMain"));

  //// Create the constant buffer
  //INIT(g_cb.Create());

  //END_INIT_SEQUENCE();
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting
// up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or
// (0.375f,0.375f)
static void ImImpl_RenderDrawLists(ImDrawData* draw_data)
{

  if (draw_data->TotalVtxCount > VB_SIZE || draw_data->TotalIdxCount > IB_SIZE)
  {
    //LOG_WARN("Too many verts or indices");
    return;
  }

  g_Graphics->SetRenderTarget(g_Graphics->_defaultBackBuffer, g_Graphics->_defaultDepthStencil, nullptr);
  ImDrawVert* vtx_dst = g_Graphics->MapWriteDiscard<ImDrawVert>(g_gpuObjects._vb);
  ImDrawIdx* idx_dst = g_Graphics->MapWriteDiscard<ImDrawIdx>(g_gpuObjects._ib);
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    memcpy(vtx_dst, &cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
    memcpy(idx_dst, &cmd_list->IdxBuffer[0], cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));
    vtx_dst += cmd_list->VtxBuffer.size();
    idx_dst += cmd_list->IdxBuffer.size();
  }

  g_Graphics->Unmap(g_gpuObjects._vb);
  g_Graphics->Unmap(g_gpuObjects._ib);

  // Setup orthographic projection matrix into our constant buffer
  {
    const float L = 0.0f;
    const float R = ImGui::GetIO().DisplaySize.x;
    const float B = ImGui::GetIO().DisplaySize.y;
    const float T = 0.0f;
    const float mvp[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {
            0.0f, 2.0f / (T - B), 0.0f, 0.0f,
        },
        {0.0f, 0.0f, 0.5f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f},
    };
    float* cb =
        g_Graphics->MapWriteDiscard<float>(g_constantBuffer);
    memcpy(cb, mvp, sizeof(mvp));
  } g_Graphics->Unmap(g_constantBuffer);

   

  // Setup viewport
  {
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(D3D11_VIEWPORT));
    vp.Width = ImGui::GetIO().DisplaySize.x;
    vp.Height = ImGui::GetIO().DisplaySize.y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_Graphics->SetViewport(vp);
  }

  // Bind shader and vertex buffers
  unsigned int stride = sizeof(CUSTOMVERTEX);
  unsigned int offset = 0;

  //g_Graphics->SetGpuObjects(g_gpuObjects);
  g_Graphics->SetGpuState(g_gpuState);
  //g_Graphics->SetGpuStateSamplers(g_gpuState, ShaderType::PixelShader);
  //g_Graphics->SetConstantBuffer(g_cb, ShaderType::VertexShader, 0);

  // Render command list
  int vtx_offset = 0;
  int idx_offset = 0;
  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback)
      {
        pcmd->UserCallback(cmd_list, pcmd);
      }
      else
      {
        const D3D11_RECT r = {(LONG)pcmd->ClipRect.x,
            (LONG)pcmd->ClipRect.y,
            (LONG)pcmd->ClipRect.z,
            (LONG)pcmd->ClipRect.w};
        ObjectHandle* h = (ObjectHandle*)pcmd->TextureId;
        if (h && h->IsValid())
          g_Graphics->SetShaderResource(*h, ShaderType::PixelShader);
        else
          g_Graphics->SetShaderResource(g_texture, ShaderType::PixelShader);

        g_Graphics->SetScissorRect(r);
        g_Graphics->_context->DrawIndexed(pcmd->ElemCount, idx_offset, vtx_offset);
      }
      idx_offset += pcmd->ElemCount;
    }
    vtx_offset += cmd_list->VtxBuffer.size();
  }

  // reset to full screen scissor rect
  DXGI_SWAP_CHAIN_DESC& desc = g_Graphics->_swapChainDesc;
  CD3D11_RECT rect = CD3D11_RECT(0, 0, desc.BufferDesc.Width, desc.BufferDesc.Height);
  g_Graphics->SetScissorRect(rect);
}

//------------------------------------------------------------------------------
void LoadFontsTexture()
{
  // Load one or more font
  ImGuiIO& io = ImGui::GetIO();
  // ImFont* my_font1 = io.Fonts->AddFontDefault();
  // ImFont* my_font2 = io.Fonts->AddFontFromFileTTF("extra_fonts/Karla-Regular.ttf", 15.0f);
  ImFont* my_font3 = io.Fonts->AddFontFromFileTTF("contrib/imgui/extra_fonts/ProggyClean.ttf", 13.0f);
  my_font3->DisplayOffset.y += 1;
  // ImFont* my_font4 = io.Fonts->AddFontFromFileTTF("imgui/extra_fonts/ProggyTiny.ttf", 10.0f);
  // my_font4->DisplayOffset.y += 1;
  // ImFont* my_font5 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 20.0f,
  // io.Fonts->GetGlyphRangesJapanese());

  // Build
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  g_texture =
      g_Graphics->CreateTexture(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, pixels, width * 4);

  // Store our identifier
  // io.Fonts->TexID = (void *)*(u32*)&h;
}

namespace tokko
{
  //------------------------------------------------------------------------------
  LRESULT WINAPI ImGuiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
      case WM_LBUTTONDOWN: io.MouseDown[0] = true; return true;
      case WM_LBUTTONUP: io.MouseDown[0] = false; return true;
      case WM_RBUTTONDOWN: io.MouseDown[1] = true; return true;
      case WM_RBUTTONUP: io.MouseDown[1] = false; return true;
      case WM_MOUSEWHEEL:
        io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
        return true;
      case WM_MOUSEMOVE:
        // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
        io.MousePos.x = (float)(signed short)(lParam);
        io.MousePos.y = (float)(signed short)(lParam >> 16);
        return true;
      case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
          io.AddInputCharacter((unsigned short)wParam);
        return true;
      case WM_KEYDOWN:
        io.KeysDown[wParam & 0xff] = true;
        io.KeyCtrl = wParam == VK_CONTROL;
        io.KeyShift = wParam == VK_SHIFT;
        return true;
      case WM_KEYUP:
        io.KeysDown[wParam & 0xff] = false;
        //g_KeyUpTrigger.SetTrigger(wParam & 0xff);
        return true;
    }

    return false;
  }

  //------------------------------------------------------------------------------
  void UpdateImGui()
  {
    ImGuiIO& io = ImGui::GetIO();

    // Setup time step
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    io.DeltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / ticksPerSecond.QuadPart;
    lastTime = currentTime;

    // Setup inputs
    // (we already got mouse position, buttons, wheel from the window message callback)
    //     BYTE keystate[256];
    //     GetKeyboardState(keystate);
    //     for (int i = 0; i < 256; i++)
    //       io.KeysDown[i] = (keystate[i] & 0x80) != 0;
    //     io.KeyCtrl = (keystate[VK_CONTROL] & 0x80) != 0;
    //     io.KeyShift = (keystate[VK_SHIFT] & 0x80) != 0;
    // io.MousePos : filled by WM_MOUSEMOVE event
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events

    // Start the frame
    ImGui::NewFrame();
  }

  //------------------------------------------------------------------------------
  bool InitImGui(HWND hWnd)
  {
    InitDeviceD3D();

    int display_w = g_Graphics->_swapChainDesc.BufferDesc.Width;
    int display_h = g_Graphics->_swapChainDesc.BufferDesc.Height;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)display_w,
        (float)display_h);       // Display size, in pixels. For clamping windows positions.
    io.DeltaTime = 1.0f / 60.0f; // Time elapsed since last frame, in seconds (in this sample app
                                 // we'll override this every frame because our time step is
                                 // variable)
    io.KeyMap[ImGuiKey_Tab] = VK_TAB; // Keyboard mapping. ImGui will use those indices to peek into
                                      // the io.KeyDown[] array that we will update during the
                                      // application lifetime.
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_UP;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';
    io.RenderDrawListsFn = ImImpl_RenderDrawLists;
    io.ImeWindowHandle = hWnd;

    QueryPerformanceFrequency(&ticksPerSecond);

    // Load fonts
    LoadFontsTexture();

    return true;
  }
}

#endif