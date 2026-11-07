// BrickIt iPad, step J0 (docs/brickit/PORTAGE_IPAD.md): an empty Metal app that validates the chain
// GitHub Actions (macOS runner) -> unsigned .ipa -> Sideloadly on the PC -> iPad.
// It shows a Metal-drawn plate of studs, a light under every finger, and a line of diagnostics:
// version, GPU, Apple GPU family, Metal ray tracing support, screen size, frames per second, fingers.
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>

#include <cmath>
#include <cstring>

#ifndef BRICKIT_VERSION
#define BRICKIT_VERSION "dev"
#endif
#ifndef BRICKIT_GIT
#define BRICKIT_GIT "local"
#endif

// One full-screen triangle; the fragment shader draws a dark plate with studs and a soft light under each touch.
static const char* const kShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct Params {
    float2 size;      // drawable size in pixels
    float  time;
    float  stud;      // stud pitch in pixels
    uint   touchCount;
    float  pad[3];
    float4 touches[10];   // xy = pixels
};

struct VOut { float4 pos [[position]]; };

vertex VOut vmain(uint vid [[vertex_id]]) {
    float2 p = float2((vid << 1) & 2, vid & 2);
    VOut o;
    o.pos = float4(p * 2.0 - 1.0, 0.0, 1.0);
    return o;
}

fragment float4 fmain(VOut in [[stage_in]], constant Params& P [[buffer(0)]]) {
    float2 px = in.pos.xy;
    float2 uv = px / P.size;
    // plate colour: a dark bluish grey, lighter at the top
    float3 base = mix(float3(0.16, 0.17, 0.20), float3(0.08, 0.085, 0.10), uv.y);
    // studs: a disc with a lit rim in every cell
    float2 cell = fract(px / P.stud) - 0.5;
    float r = length(cell);
    float stud = smoothstep(0.30, 0.28, r);
    float rim = smoothstep(0.30, 0.27, r) - smoothstep(0.25, 0.22, r);
    float3 col = base + stud * 0.035 + rim * 0.05 * (0.5 - cell.y);
    // light under each finger, in BrickIt red
    for (uint i = 0; i < P.touchCount && i < 10; ++i) {
        float d = length(px - P.touches[i].xy) / (P.stud * 2.5);
        float g = exp(-d * d);
        col = mix(col, float3(0.85, 0.12, 0.10) * (0.55 + 0.45 * stud), g * 0.85);
    }
    // a slow highlight band so a frozen frame is easy to spot
    float bx = (uv.x + uv.y * 0.3 - fract(P.time * 0.1) * 1.6 + 0.3) * 6.0;
    float band = exp(-bx * bx);
    col += band * 0.04;
    return float4(col, 1.0);
}
)MSL";

struct Params {
    float size[2];
    float time;
    float stud;
    uint32_t touchCount;
    float pad[3];
    float touches[10][4];
};

@interface BKView : UIView
@end

@implementation BKView
+ (Class)layerClass {
    return [CAMetalLayer class];
}
@end

@interface BKViewController : UIViewController
@end

@implementation BKViewController {
    id<MTLDevice> _device;
    id<MTLCommandQueue> _queue;
    id<MTLRenderPipelineState> _pipeline;
    CADisplayLink* _link;
    UILabel* _label;
    NSMutableSet<UITouch*>* _touches;
    CFTimeInterval _start, _fpsTime;
    int _fpsFrames;
    float _fps;
    NSString* _gpuLine;
    NSString* _error;
}

- (void)loadView {
    BKView* view = [[BKView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    view.contentScaleFactor = [UIScreen mainScreen].nativeScale;
    view.multipleTouchEnabled = YES;
    view.backgroundColor = [UIColor blackColor];
    self.view = view;
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeAll;
}

static NSString* appleFamily(id<MTLDevice> d) {
    // highest Apple GPU family reported by the device (A12Z = Apple5, A13 = Apple6, M1 = Apple7, M2 = Apple8, M3/A17 = Apple9)
    const MTLGPUFamily fams[] = {MTLGPUFamilyApple9, MTLGPUFamilyApple8, MTLGPUFamilyApple7, MTLGPUFamilyApple6,
                                 MTLGPUFamilyApple5, MTLGPUFamilyApple4, MTLGPUFamilyApple3};
    const int nums[] = {9, 8, 7, 6, 5, 4, 3};
    for (int i = 0; i < 7; ++i)
        if ([d supportsFamily:fams[i]]) return [NSString stringWithFormat:@"Apple%d", nums[i]];
    return @"?";
}

- (void)viewDidLoad {
    [super viewDidLoad];
    _touches = [NSMutableSet set];
    _device = MTLCreateSystemDefaultDevice();
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    layer.device = _device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;
    layer.contentsScale = self.view.contentScaleFactor;

    if (!_device) {
        _error = @"Pas de GPU Metal";
    } else {
        _queue = [_device newCommandQueue];
        NSError* err = nil;
        id<MTLLibrary> lib = [_device newLibraryWithSource:[NSString stringWithUTF8String:kShaderSource] options:nil error:&err];
        if (!lib) {
            _error = [NSString stringWithFormat:@"Shader : %@", err.localizedDescription];
        } else {
            MTLRenderPipelineDescriptor* pd = [MTLRenderPipelineDescriptor new];
            pd.vertexFunction = [lib newFunctionWithName:@"vmain"];
            pd.fragmentFunction = [lib newFunctionWithName:@"fmain"];
            pd.colorAttachments[0].pixelFormat = layer.pixelFormat;
            _pipeline = [_device newRenderPipelineStateWithDescriptor:pd error:&err];
            if (!_pipeline) _error = [NSString stringWithFormat:@"Pipeline : %@", err.localizedDescription];
        }
        BOOL rt = NO;
        if (@available(iOS 14.0, *)) rt = _device.supportsRaytracing;
        _gpuLine = [NSString stringWithFormat:@"GPU %@ · %@ · ray tracing Metal : %@ · mémoire conseillée %.1f Go",
                                              _device.name, appleFamily(_device), rt ? @"oui" : @"non",
                                              _device.recommendedMaxWorkingSetSize / 1073741824.0];
    }

    _label = [[UILabel alloc] init];
    _label.numberOfLines = 0;
    _label.textColor = [UIColor colorWithWhite:0.92 alpha:1.0];
    _label.font = [UIFont monospacedSystemFontOfSize:15 weight:UIFontWeightRegular];
    _label.backgroundColor = [UIColor colorWithWhite:0.0 alpha:0.45];
    _label.layer.cornerRadius = 10;
    _label.layer.masksToBounds = YES;
    _label.userInteractionEnabled = NO;
    [self.view addSubview:_label];

    _start = _fpsTime = CACurrentMediaTime();
    _link = [CADisplayLink displayLinkWithTarget:self selector:@selector(frame:)];
    _link.preferredFramesPerSecond = 120;
    [_link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    const CGFloat s = self.view.contentScaleFactor;
    layer.drawableSize = CGSizeMake(self.view.bounds.size.width * s, self.view.bounds.size.height * s);
}

- (void)updateLabel {
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    UIEdgeInsets safe = self.view.safeAreaInsets;
    NSString* text = [NSString stringWithFormat:
        @" BrickIt iPad · J0 · %s · %s \n %@ \n écran %.0f x %.0f px (x%.1f) · %.0f i/s · doigts : %lu · iPadOS %@ ",
        BRICKIT_VERSION, BRICKIT_GIT, _error ? _error : _gpuLine, layer.drawableSize.width, layer.drawableSize.height,
        self.view.contentScaleFactor, _fps, (unsigned long)_touches.count, [UIDevice currentDevice].systemVersion];
    _label.text = text;
    CGSize fit = [_label sizeThatFits:CGSizeMake(self.view.bounds.size.width - 32, CGFLOAT_MAX)];
    _label.frame = CGRectMake(16 + safe.left, 16 + safe.top, fit.width, fit.height + 8);
}

- (void)frame:(CADisplayLink*)link {
    const CFTimeInterval now = CACurrentMediaTime();
    ++_fpsFrames;
    if (now - _fpsTime >= 0.5) {
        _fps = (float)(_fpsFrames / (now - _fpsTime));
        _fpsFrames = 0;
        _fpsTime = now;
        [self updateLabel];
    }
    if (!_pipeline) return;
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (!drawable) return;

    Params p;
    memset(&p, 0, sizeof(p));
    p.size[0] = (float)layer.drawableSize.width;
    p.size[1] = (float)layer.drawableSize.height;
    p.time = (float)(now - _start);
    p.stud = 24.0f * (float)self.view.contentScaleFactor;
    const CGFloat s = self.view.contentScaleFactor;
    for (UITouch* t in _touches) {
        if (p.touchCount >= 10) break;
        CGPoint q = [t locationInView:self.view];
        p.touches[p.touchCount][0] = (float)(q.x * s);
        p.touches[p.touchCount][1] = (float)(q.y * s);
        ++p.touchCount;
    }

    MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
    rp.colorAttachments[0].texture = drawable.texture;
    rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;
    rp.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLCommandBuffer> cb = [_queue commandBuffer];
    id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
    [enc setRenderPipelineState:_pipeline];
    [enc setFragmentBytes:&p length:sizeof(p) atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [enc endEncoding];
    [cb presentDrawable:drawable];
    [cb commit];
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [_touches unionSet:touches];
}
- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
}
- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [_touches minusSet:touches];
}
- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [_touches minusSet:touches];
}
@end

@interface BKAppDelegate : UIResponder <UIApplicationDelegate>
@property(strong, nonatomic) UIWindow* window;
@end

@implementation BKAppDelegate
- (BOOL)application:(UIApplication*)app didFinishLaunchingWithOptions:(NSDictionary*)options {
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.window.rootViewController = [BKViewController new];
    [self.window makeKeyAndVisible];
    return YES;
}
@end

int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([BKAppDelegate class]));
    }
}
