#import <Foundation/Foundation.h>
#import <filesystem>
#import <MetalKit/MetalKit.h>
#import "Lib/Toastbox/Mmap.h"
#import "Lib/Toastbox/Mac/Renderer.h"
#import "LMMSE.h"
#import <vector>
#import <chrono>
namespace fs = std::filesystem;

static bool _IsPNGFile(const fs::path& path) {
    return fs::is_regular_file(path) && path.extension() == ".png";
}

int main(int argc, const char* argv[]) {
    using namespace std::chrono;
    
    static constexpr Toastbox::CFADesc CFADesc = {
        Toastbox::CFAColor::Green, Toastbox::CFAColor::Red,
        Toastbox::CFAColor::Blue, Toastbox::CFAColor::Green,
    };
    
    const fs::path imagesDir = fs::path(argv[0]).parent_path() / "Images";
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    Toastbox::Renderer renderer(device, [device newDefaultLibrary], [device newCommandQueue]);
    MTKTextureLoader* txtLoader = [[MTKTextureLoader alloc] initWithDevice:device];
    std::vector<id<MTLTexture>> txts;
    Toastbox::Renderer::Txt txtRgb;
    for (const fs::path& p : fs::directory_iterator(imagesDir)) @autoreleasepool {
        if (_IsPNGFile(p)) {
            id<MTLTexture> txtRaw = [txtLoader newTextureWithContentsOfURL:[NSURL fileURLWithPath:@(p.c_str())]
                options:nil error:nil];
            txts.push_back(txtRaw);
            
            constexpr MTLTextureUsage TxtRgbUsage =
                MTLTextureUsageShaderRead   |
                MTLTextureUsageShaderWrite  |
                MTLTextureUsageRenderTarget ;
            
            if (!txtRgb) {
                txtRgb = renderer.textureCreate(MTLPixelFormatRGBA32Float,
                    [txtRaw width], [txtRaw height], TxtRgbUsage);
            }
            
            LMMSE::Run(renderer, CFADesc, true, txtRaw, txtRgb);
            
            const fs::path outputPath = fs::path(p).replace_extension()+=std::string("-debayered.png");
            printf("Writing %s\n", outputPath.c_str());
            renderer.debugTextureWrite(txtRgb, outputPath);
            system((std::string("open ") + outputPath.string()).c_str());
        }
    }
    
    printf("\n");
    printf("Testing performance...\n");
    for (;;) @autoreleasepool {
        auto timeStart = std::chrono::steady_clock::now();
        
        constexpr int ChunkCount = 200;
        for (int i=0; i<ChunkCount; i++) {
            for (id<MTLTexture> txtRaw : txts) {
                LMMSE::Run(renderer, CFADesc, true, txtRaw, txtRgb);
            }
            renderer.commitAndWait();
        }
        
        const microseconds duration = duration_cast<microseconds>(steady_clock::now()-timeStart);
        const microseconds durationAvg = duration / (ChunkCount*txts.size());
        printf("FFCC took %ju us / image\n", (uintmax_t)durationAvg.count());
        
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
    }
    
    
    return 0;
}
