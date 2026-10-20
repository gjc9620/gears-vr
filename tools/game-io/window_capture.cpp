// Capture exactly one HWND with WGC; never falls back to capturing a display.
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wincodec.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <memory>
#include <vector>

using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

void save_jpeg(const wchar_t* path, UINT width, UINT height, UINT stride, BYTE* pixels, UINT max_width) {
    com_ptr<IWICImagingFactory> factory;
    check_hresult(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(factory.put())));
    com_ptr<IWICBitmap> bitmap;
    check_hresult(factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA,
                  stride, stride * height, pixels, bitmap.put()));
    UINT out_width = std::min(width, max_width);
    UINT out_height = std::max(1u, static_cast<UINT>((uint64_t(height) * out_width) / width));
    com_ptr<IWICBitmapScaler> scaler;
    check_hresult(factory->CreateBitmapScaler(scaler.put()));
    check_hresult(scaler->Initialize(bitmap.get(), out_width, out_height, WICBitmapInterpolationModeFant));
    com_ptr<IWICFormatConverter> converter;
    check_hresult(factory->CreateFormatConverter(converter.put()));
    check_hresult(converter->Initialize(scaler.get(), GUID_WICPixelFormat24bppBGR,
                  WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom));
    com_ptr<IWICStream> stream;
    check_hresult(factory->CreateStream(stream.put()));
    check_hresult(stream->InitializeFromFilename(path, GENERIC_WRITE));
    com_ptr<IWICBitmapEncoder> encoder;
    check_hresult(factory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, encoder.put()));
    check_hresult(encoder->Initialize(stream.get(), WICBitmapEncoderNoCache));
    com_ptr<IWICBitmapFrameEncode> encoded;
    com_ptr<IPropertyBag2> options;
    check_hresult(encoder->CreateNewFrame(encoded.put(), options.put()));
    PROPBAG2 property{}; property.pstrName = const_cast<wchar_t*>(L"ImageQuality");
    VARIANT quality{}; quality.vt = VT_R4; quality.fltVal = 0.72f;
    check_hresult(options->Write(1, &property, &quality));
    check_hresult(encoded->Initialize(options.get()));
    check_hresult(encoded->SetSize(out_width, out_height));
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    check_hresult(encoded->SetPixelFormat(&format));
    check_hresult(encoded->WriteSource(converter.get(), nullptr));
    check_hresult(encoded->Commit()); check_hresult(encoder->Commit());
    printf("{\"width\":%u,\"height\":%u,\"source_width\":%u,\"source_height\":%u}\n",
           out_width, out_height, width, height);
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 4) {fwprintf(stderr, L"Usage: WindowCapture HWND OUTPUT.jpg MAX_WIDTH\n"); return 2;}
    try {
        // WGC uses physical pixels; HWND geometry must not be DPI-virtualized.
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        HWND window = reinterpret_cast<HWND>(_wcstoui64(argv[1], nullptr, 0));
        UINT max_width = static_cast<UINT>(_wtoi(argv[3]));
        if (!IsWindow(window) || IsIconic(window) || max_width < 160 || max_width > 1280)
            throw hresult_error(E_INVALIDARG, L"Window is absent/minimized or width is invalid");
        init_apartment(apartment_type::multi_threaded);
        if (!GraphicsCaptureSession::IsSupported())
            throw hresult_error(E_NOTIMPL, L"Windows Graphics Capture unsupported");
        com_ptr<ID3D11Device> device; com_ptr<ID3D11DeviceContext> context;
        check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                      D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                      device.put(), nullptr, context.put()));
        com_ptr<IInspectable> inspectable;
        check_hresult(CreateDirect3D11DeviceFromDXGIDevice(device.as<IDXGIDevice>().get(), inspectable.put()));
        auto capture_device = inspectable.as<IDirect3DDevice>();
        auto interop = get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        GraphicsCaptureItem item{nullptr};
        check_hresult(interop->CreateForWindow(window, guid_of<GraphicsCaptureItem>(), put_abi(item)));
        auto size = item.Size();
        auto pool = Direct3D11CaptureFramePool::CreateFreeThreaded(capture_device,
                    DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
        auto session = pool.CreateCaptureSession(item);
        session.IsCursorCaptureEnabled(false);
        struct CaptureState {
            handle ready{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
            std::mutex mutex;
            Direct3D11CaptureFrame frame{nullptr};
            unsigned received = 0;
        };
        auto state = std::make_shared<CaptureState>();
        if (!state->ready) throw_last_error();
        auto token = pool.FrameArrived([state](auto const& sender, auto const&) {
            // Shared state also protects late callbacks during teardown/timeout.
            std::lock_guard<std::mutex> lock(state->mutex);
            try {
                auto latest = sender.TryGetNextFrame();
                // Static dialogs may produce just one frame. Keep the latest
                // available frame instead of requiring a second frame forever.
                if (latest) {
                    ++state->received;
                    state->frame = latest; SetEvent(state->ready.get());
                }
            } catch (hresult_error const&) { /* Pool may have closed. */ }
        });
        session.StartCapture();
        DWORD wait = WaitForSingleObject(state->ready.get(), 5000);
        if(wait==WAIT_OBJECT_0)Sleep(50);
        session.Close(); pool.FrameArrived(token);
        Direct3D11CaptureFrame captured{nullptr};
        { std::lock_guard<std::mutex> lock(state->mutex); captured = state->frame; }
        if (wait != WAIT_OBJECT_0 || !captured) {
            pool.Close();
            throw hresult_error(HRESULT_FROM_WIN32(WAIT_TIMEOUT), L"No window frame in five seconds");
        }
        auto access = captured.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
        com_ptr<ID3D11Texture2D> texture;
        check_hresult(access->GetInterface(IID_PPV_ARGS(texture.put())));
        D3D11_TEXTURE2D_DESC desc{}; texture->GetDesc(&desc);
        auto content = captured.ContentSize();
        if (content.Width != size.Width || content.Height != size.Height)
            throw hresult_error(E_FAIL, L"Window resized during capture; retry");
        RECT client{}; GetClientRect(window, &client);
        POINT origin{0,0}; ClientToScreen(window, &origin);
        RECT bounds{};
        if (FAILED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds))))
            throw hresult_error(E_FAIL, L"Cannot determine capture bounds");
        LONG left = std::max(0L, origin.x - bounds.left);
        LONG top = std::max(0L, origin.y - bounds.top);
        LONG width = std::min<LONG>(client.right, content.Width - left);
        LONG height = std::min<LONG>(client.bottom, content.Height - top);
        if (width <= 0 || height <= 0 || left + width > LONG(desc.Width) || top + height > LONG(desc.Height))
            throw hresult_error(E_FAIL, L"Invalid client crop");
        desc.Width = static_cast<UINT>(width); desc.Height = static_cast<UINT>(height);
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
        com_ptr<ID3D11Texture2D> staging;
        check_hresult(device->CreateTexture2D(&desc, nullptr, staging.put()));
        D3D11_BOX box{UINT(left), UINT(top), 0, UINT(left+width), UINT(top+height), 1};
        context->CopySubresourceRegion(staging.get(), 0, 0, 0, 0, texture.get(), 0, &box);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        check_hresult(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped));
        std::vector<BYTE> pixels(size_t(width) * height * 4);
        for (LONG y=0; y<height; ++y)
            memcpy(pixels.data() + size_t(y)*width*4,
                   static_cast<BYTE*>(mapped.pData)+size_t(y)*mapped.RowPitch, size_t(width)*4);
        context->Unmap(staging.get(),0);
        captured.Close(); pool.Close();
        save_jpeg(argv[2], UINT(width), UINT(height), UINT(width)*4, pixels.data(), max_width);
        return 0;
    } catch (hresult_error const& error) {
        fwprintf(stderr, L"Capture error 0x%08X: %s\n", unsigned(error.code()), error.message().c_str());
        return 1;
    } catch (std::exception const& error) {
        fprintf(stderr, "Capture error: %s\n", error.what()); return 1;
    }
}
