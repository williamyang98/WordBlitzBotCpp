#include "App.h"

#include <algorithm>
#include <memory>

#include "model.h"
#include "util/MSS.h"
#include "util/AutoGui.h"
#include "util/KeyListener.h"

void DrawRectInBuffer(
    int cx, int cy, int wx, int wy,
    const RGBA<uint8_t> pen_color, const int pen_width,
    RGBA<uint8_t> *buffer, int width, int height, int row_stride);

App::App(ID3D11Device *dx11_device, ID3D11DeviceContext *dx11_context)
{
    m_mss = std::make_shared<util::MSS>();
    m_dx11_device = dx11_device;
    m_dx11_context = dx11_context;

    // setup screen shotter
    SetScreenshotSize(320, 452);

    m_is_render_running = true;

    // create application bindings
    util::InitGlobalListener();

    // util::AttachKeyboardListener(VK_F1, [this](WPARAM type) {
    //     if (type == WM_KEYDOWN) {
    //         bool v = m_player->GetIsRunning();
    //         m_player->SetIsRunning(!v);
    //     }
    // });
}

void App::SetScreenshotSize(const int width, const int height) {
    m_screen_width = width;
    m_screen_height = height;
    m_mss->SetSize(width, height);
    m_screenshot_texture = CreateTexture(width, height);
}

void App::Update() {
    // something do to here? 
}

App::TextureWrapper App::CreateTexture(const int width, const int height) {
    App::TextureWrapper wrapper;
    wrapper.texture = NULL;
    wrapper.view = NULL;
    wrapper.width = width;
    wrapper.height = height;

    // Create texture
    // We are creating a dynamic texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    auto status = m_dx11_device->CreateTexture2D(&desc, NULL, &wrapper.texture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    m_dx11_device->CreateShaderResourceView(wrapper.texture, &srvDesc, &wrapper.view);

    return wrapper;
}


void App::UpdateScreenshotTexture() {
    auto bitmap = m_mss->GetBitmap();
    auto buffer_size = bitmap.GetSize();
    auto buffer_max_size = m_mss->GetMaxSize();
    DIBSECTION &sec = bitmap.GetBitmap();
    BITMAP &bmp = sec.dsBm;
    uint8_t *buffer = (uint8_t*)(bmp.bmBits);

    // setup dx11 to modify texture
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    const UINT subresource = 0;
    m_dx11_context->Map(m_screenshot_texture.texture, subresource, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    // update texture from screen shotter
    int row_width = mappedResource.RowPitch / 4;

    RGBA<uint8_t> *dst_buffer = (RGBA<uint8_t> *)(mappedResource.pData);
    RGBA<uint8_t> *src_buffer = (RGBA<uint8_t> *)(buffer);

    for (int x = 0; x < buffer_size.x; x++) {
        for (int y = 0; y < buffer_size.y; y++) {
            int i = x + y*row_width;
            int j = x + (buffer_max_size.y-y-1)*buffer_max_size.x;
            dst_buffer[i] = src_buffer[j];
        }
    }

    // render the bounding box of ball prediction
    // auto raw_pred = m_player->GetRawPrediction();
    // auto filtered_pred = m_player->GetFilteredPrediction();

    // int wx = (int)(m_params->relative_ball_width * buffer_size.x * 0.5f);
    // int wy = wx;
    // const int pen_width = (int)std::max(1.0f, 0.01f*buffer_size.x);
    // const RGBA<uint8_t> raw_color = {255,0,0,255};
    // const RGBA<uint8_t> filtered_color = {0,0,255,255};

    // if (raw_pred.confidence > m_params->confidence_threshold) {
    //     int cx = (int)((     raw_pred.x) * buffer_size.x);
    //     int cy = (int)((1.0f-raw_pred.y) * buffer_size.y);
    //     DrawRectInBuffer(
    //         cx, cy, wx, wy, 
    //         raw_color, pen_width,
    //         dst_buffer, buffer_size.x, buffer_size.y, row_width);
    // }
    // if (filtered_pred.confidence > m_params->confidence_threshold) {
    //     int cx = (int)((     filtered_pred.x) * buffer_size.x);
    //     int cy = (int)((1.0f-filtered_pred.y) * buffer_size.y);
    //     DrawRectInBuffer(
    //         cx, cy, wx, wy, 
    //         filtered_color, pen_width,
    //         dst_buffer, buffer_size.x, buffer_size.y, row_width);
    // }

    m_dx11_context->Unmap(m_screenshot_texture.texture, subresource);
}

void App::UpdateModelTexture() {
    
}

void DrawRectInBuffer(
    int cx, int cy, int wx, int wy,
    const RGBA<uint8_t> pen_color, const int pen_width,
    RGBA<uint8_t> *buffer, int width, int height, int row_stride)
{
    int px_start = std::clamp(cx-wx, 0          , width-pen_width);
    int px_end   = std::clamp(cx+wx, pen_width-1, width-1);
    int py_start = std::clamp(cy-wy, 0          , height-pen_width);
    int py_end   = std::clamp(cy+wy, pen_width-1, height-1);

    // draw each of the line
    for (int x = px_start; x <= px_end; x++) {
        for (int j = 0; j < pen_width; j++) {
            buffer[x + (py_start+j)*row_stride] = pen_color;
            buffer[x + (py_end  -j)*row_stride] = pen_color;
        }
    }
    for (int y = py_start; y <= py_end; y++) {
        for (int j = 0; j < pen_width; j++) {
            buffer[px_start+j + y*row_stride] = pen_color;
            buffer[px_end  -j + y*row_stride] = pen_color;
        }
    }
}