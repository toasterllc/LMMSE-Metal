#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import "Lib/Toastbox/Mac/MetalUtil.h"
#import "Lib/Toastbox/Mac/CFA.h"

class LMMSE {
public:
    static void Run(
        Toastbox::Renderer& renderer,
        const Toastbox::CFADesc& cfaDesc,
        bool applyGamma,
        id<MTLTexture> srcRaw,
        id<MTLTexture> dstRGB
    ) {
        
        using namespace Toastbox;
        
        const size_t w = [srcRaw width];
        const size_t h = [srcRaw height];
        
        Renderer::Txt raw = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
        
        // Copy `srcRaw` so we can modify it
        renderer.copy(srcRaw, raw);
        
        // Gamma before (improves quality of edges)
        if (applyGamma) {
            renderer.render(raw,
                renderer.FragmentShader("LMMSE::GammaForward",
                    // Texture args
                    raw
                )
            );
        }
        
        // Horizontal interpolation
        Renderer::Txt filteredHTxt = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
        {
            const bool h = true;
            renderer.render(filteredHTxt,
                renderer.FragmentShader("LMMSE::Interp5",
                    // Buffer args
                    h,
                    // Texture args
                    raw
                )
            );
        }
        
        // Vertical interpolation
        Renderer::Txt filteredVTxt = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
        {
            const bool h = false;
            renderer.render(filteredVTxt,
                renderer.FragmentShader("LMMSE::Interp5",
                    // Buffer args
                    h,
                    // Texture args
                    raw
                )
            );
        }
        
        // Calculate DiffH
        Renderer::Txt diffHTxt = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
        {
            renderer.render(diffHTxt,
                renderer.FragmentShader("LMMSE::NoiseEst",
                    // Buffer args
                    cfaDesc,
                    // Texture args
                    raw,
                    filteredHTxt
                )
            );
        }
        
        // Calculate DiffV
        Renderer::Txt diffVTxt = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
        {
            renderer.render(diffVTxt,
                renderer.FragmentShader("LMMSE::NoiseEst",
                    // Buffer args
                    cfaDesc,
                    // Texture args
                    raw,
                    filteredVTxt
                )
            );
        }
        
        // Smooth DiffH
        {
            const bool h = true;
            renderer.render(filteredHTxt,
                renderer.FragmentShader("LMMSE::Smooth9",
                    // Buffer args
                    h,
                    // Texture args
                    diffHTxt
                )
            );
        }
        
        // Smooth DiffV
        {
            const bool h = false;
            renderer.render(filteredVTxt,
                renderer.FragmentShader("LMMSE::Smooth9",
                    // Buffer args
                    h,
                    // Texture args
                    diffVTxt
                )
            );
        }
        
        // Calculate dstRGB.g
        {
            renderer.render(dstRGB,
                renderer.FragmentShader("LMMSE::CalcG",
                    // Buffer args
                    cfaDesc,
                    // Texture args
                    raw,
                    filteredHTxt,
                    diffHTxt,
                    filteredVTxt,
                    diffVTxt
                )
            );
        }
        
        // Calculate diffGRTxt.r
        Renderer::Txt diffGRTxt = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
        {
            const bool modeGR = true;
            renderer.render(diffGRTxt,
                renderer.FragmentShader("LMMSE::CalcDiffGRGB",
                    // Buffer args
                    cfaDesc,
                    modeGR,
                    // Texture args
                    raw,
                    dstRGB
                )
            );
        }
        
        // Calculate diffGBTxt.b
        Renderer::Txt diffGBTxt = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
        {
            const bool modeGR = false;
            renderer.render(diffGBTxt,
                renderer.FragmentShader("LMMSE::CalcDiffGRGB",
                    // Buffer args
                    cfaDesc,
                    modeGR,
                    // Texture args
                    raw,
                    dstRGB
                )
            );
        }
        
        // Calculate diffGRTxt.b
        {
            Renderer::Txt tmp = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
            const bool modeGR = true;
            renderer.render(tmp,
                renderer.FragmentShader("LMMSE::CalcDiagAvgDiffGRGB",
                    // Buffer args
                    cfaDesc,
                    modeGR,
                    // Texture args
                    raw,
                    dstRGB,
                    diffGRTxt
                )
            );
            diffGRTxt = std::move(tmp);
        }
        
        // Calculate diffGBTxt.r
        {
            Renderer::Txt tmp = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
            const bool modeGR = false;
            renderer.render(tmp,
                renderer.FragmentShader("LMMSE::CalcDiagAvgDiffGRGB",
                    // Buffer args
                    cfaDesc,
                    modeGR,
                    // Texture args
                    raw,
                    dstRGB,
                    diffGBTxt
                )
            );
            diffGBTxt = std::move(tmp);
        }
        
        // Calculate diffGRTxt.g
        {
            Renderer::Txt tmp = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
            renderer.render(tmp,
                renderer.FragmentShader("LMMSE::CalcAxialAvgDiffGRGB",
                    // Buffer args
                    cfaDesc,
                    // Texture args
                    raw,
                    dstRGB,
                    diffGRTxt
                )
            );
            diffGRTxt = std::move(tmp);
        }
        
        // Calculate diffGBTxt.g
        {
            Renderer::Txt tmp = renderer.textureCreate(MTLPixelFormatR32Float, w, h);
            renderer.render(tmp,
                renderer.FragmentShader("LMMSE::CalcAxialAvgDiffGRGB",
                    // Buffer args
                    cfaDesc,
                    // Texture args
                    raw,
                    dstRGB,
                    diffGBTxt
                )
            );
            diffGBTxt = std::move(tmp);
        }
        
        // Calculate dstRGB.rb
        {
            renderer.render(dstRGB,
                renderer.FragmentShader("LMMSE::CalcRB",
                    // Texture args
                    dstRGB,
                    diffGRTxt,
                    diffGBTxt
                )
            );
        }
        
        // Gamma after (improves quality of edges)
        if (applyGamma) {
            renderer.render(dstRGB,
                renderer.FragmentShader("LMMSE::GammaReverse",
                    // Texture args
                    dstRGB
                )
            );
        }
    }
};
