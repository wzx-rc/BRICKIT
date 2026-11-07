// BrickIt for iPad: the very BrickIt of the PC (src/brickit, same sources: UI, library, building, mechanics...) in a
// full-screen CAMetalLayer, rendered by the Lit renderer of the WZX engine through Diligent on MoltenVK (Vulkan
// translated to Metal). No ray tracing on the iPad Pro 2nd generation (A12Z): raster image only.
// This file is the iOS shell, as BrickAppWin.cpp is the Windows one: window, device, display loop, touch -> mouse.
#include "BrickApp.h"
#include "GfxLit.h"
#include "Platform.h"
#include "imgui.h"

#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>

#include <cfloat>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>

using namespace Diligent;

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
    std::unique_ptr<brickit::BrickApp> _app;
    CADisplayLink* _displayLink;
    BOOL _ready;
    std::map<void*, CGPoint> _touches;   // in pixels, by UITouch
    // one finger: held back a moment so that a second finger turns it into a camera gesture, not a click
    BOOL _pending, _mouseDown, _twoFingers;
    CGPoint _pendingPos;
    CFTimeInterval _pendingTime;
    float _prevDist;
}

- (void)loadView {
    BKView* view = [[BKView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    view.contentScaleFactor = [UIScreen mainScreen].nativeScale;
    view.multipleTouchEnabled = YES;
    self.view = view;
}

- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures { return UIRectEdgeAll; }

// The PC keeps assets/ and shaders/ next to BrickIt.exe and writes its caches and settings there too. The bundle is
// read only: the app folder is Application Support/BrickIt/app, assets/ and shaders/ links to the bundle in it.
- (NSString*)prepareAppFolder:(NSString*)data {
    NSFileManager* fm = [NSFileManager defaultManager];
    NSString* app = [data stringByAppendingPathComponent:@"app"];
    [fm createDirectoryAtPath:app withIntermediateDirectories:YES attributes:nil error:nil];
    NSString* res = [[NSBundle mainBundle] resourcePath];
    for (NSString* name in @[ @"assets", @"shaders" ]) {
        NSString* link = [app stringByAppendingPathComponent:name];
        NSString* target = [res stringByAppendingPathComponent:name];
        [fm removeItemAtPath:link error:nil];   // the bundle path changes with every install
        [fm createSymbolicLinkAtPath:link withDestinationPath:target error:nil];
    }
    return app;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    const CGFloat scale = self.view.contentScaleFactor;
    layer.contentsScale = scale;

    // the user's data (full LDraw library in full/, autosave, tray): Application Support/BrickIt, kept across updates
    NSURL* support = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask
                                                   appropriateForURL:nil create:YES error:nil];
    NSString* data = [support.path stringByAppendingPathComponent:@"BrickIt"];
    [[NSFileManager defaultManager] createDirectoryAtPath:data withIntermediateDirectories:YES attributes:nil error:nil];
    NSString* appDir = [self prepareAppFolder:data];
    chdir(appDir.fileSystemRepresentation);
    brickit::plat::setDataDir(brickit::plat::fromUtf8(data.UTF8String));

    IEngineFactoryVk* factory = GetEngineFactoryVk();
    EngineVkCreateInfo ci;
    ci.Features.ShaderResourceRuntimeArrays = DEVICE_FEATURE_STATE_OPTIONAL;
    ci.DynamicHeapSize = 64 << 20;
    ci.DynamicHeapPageSize = 2 << 20;
    RefCntAutoPtr<IRenderDevice> device;
    RefCntAutoPtr<IDeviceContext> context;
    factory->CreateDeviceAndContextsVk(ci, &device, &context);
    if (!device) { [self showError:@"Pas de périphérique Vulkan (MoltenVK)."]; return; }
    SwapChainDesc sc;
    sc.ColorBufferFormat = TEX_FORMAT_BGRA8_UNORM;   // the post pass writes sRGB values itself
    sc.DepthBufferFormat = TEX_FORMAT_UNKNOWN;       // the Lit renderer has its own MSAA depth
    RefCntAutoPtr<ISwapChain> swapChain;
    IOSNativeWindow window{(__bridge void*)layer};
    factory->CreateSwapChainVk(device, context, sc, window, &swapChain);
    if (!swapChain) { [self showError:@"Impossible de créer la swap chain."]; return; }

    std::string err;
    const std::string dir = appDir.UTF8String;
    auto renderer = std::make_unique<brickit::GfxRendererLit>();
    if (!renderer->init(device, context, factory, swapChain, 0, 0, sc.ColorBufferFormat, dir + "/shaders", &err)) {
        [self showError:[NSString stringWithUTF8String:("Lit : " + err).c_str()]];
        return;
    }
    // the UI at the scale of the screen (2 on the iPad Pro), as the DPI scale of the PC
    auto gui = std::make_unique<brickit::GfxGuiLit>();
    if (!gui->init(*renderer, (float)scale, dir + "/assets/fonts", &err)) {
        [self showError:[NSString stringWithUTF8String:("ImGui : " + err).c_str()]];
        return;
    }
    ImGui::GetIO().AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    brickit::Options opt;
    opt.touchHud = 1;   // the touch HUD of the PC (Tiny Glade style) on from the start
    _app = std::make_unique<brickit::BrickApp>();
    if (!_app->initCommon(opt, std::move(renderer), std::move(gui), brickit::plat::fromUtf8(dir), (float)scale)) {
        [self showError:@"Initialisation de BrickIt impossible (voir brickit_start.log)."];
        _app.reset();
        return;
    }
    _ready = YES;
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawFrame:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(willResignActive:)
                                                 name:UIApplicationWillResignActiveNotification object:nil];
}

// iOS may end the app at any time once it is in the background: settings and autosave as on PC exit
- (void)willResignActive:(NSNotification*)n {
    if (_app) _app->saveSession();
}

- (void)showError:(NSString*)text {
    UILabel* label = [[UILabel alloc] initWithFrame:CGRectInset(self.view.bounds, 40, 40)];
    label.text = text;
    label.numberOfLines = 0;
    label.textColor = [UIColor whiteColor];
    self.view.backgroundColor = [UIColor blackColor];
    [self.view addSubview:label];
}

- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    if (!_app) return;
    CGSize s = self.view.bounds.size;
    CGFloat k = self.view.contentScaleFactor;
    _app->resize((uint32_t)(s.width * k), (uint32_t)(s.height * k));
}

// --------------------------------------------------------------------------- touch -> the mouse of the PC

- (CGPoint)pixels:(UITouch*)t {
    CGPoint p = [t locationInView:self.view];
    CGFloat k = self.view.contentScaleFactor;
    return CGPointMake(p.x * k, p.y * k);
}

- (void)mouseAt:(CGPoint)p {
    ImGui::GetIO().AddMousePosEvent((float)p.x, (float)p.y);
}

- (void)mouseUp {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(0, false);
    io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);   // a finger does not hover once lifted
    _mouseDown = NO;
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) {
        _touches[(__bridge void*)t] = [self pixels:t];
        ImGui::GetIO().AddMouseSourceEvent(t.type == UITouchTypePencil ? ImGuiMouseSource_Pen : ImGuiMouseSource_TouchScreen);
    }
    if (_touches.size() >= 2) {
        _pending = NO;
        if (_mouseDown) [self mouseUp];
        _twoFingers = YES;
        _prevDist = 0;
    } else if (!_twoFingers && !_mouseDown) {
        _pending = YES;
        _pendingPos = _touches.begin()->second;
        _pendingTime = CACurrentMediaTime();
    }
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) _touches[(__bridge void*)t] = [self pixels:t];
    if (_mouseDown && _touches.size() == 1) [self mouseAt:_touches.begin()->second];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) _touches.erase((__bridge void*)t);
    if (_pending) {   // a tap: press and release at once (ImGui spreads them over the next frames)
        [self mouseAt:_pendingPos];
        ImGui::GetIO().AddMouseButtonEvent(0, true);
        _pending = NO;
        [self mouseUp];
    } else if (_mouseDown) {
        [self mouseUp];
    }
    if (_touches.empty()) _twoFingers = NO;
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) _touches.erase((__bridge void*)t);
    _pending = NO;
    if (_mouseDown) [self mouseUp];
    if (_touches.empty()) _twoFingers = NO;
}

// --------------------------------------------------------------------------- frame

- (void)drawFrame:(CADisplayLink*)link {
    if (!_ready || !_app) return;
    // a finger held still or moving: the press goes to BrickIt (UI, or the view)
    if (_pending && _touches.size() == 1) {
        CGPoint p = _touches.begin()->second;
        float dx = (float)(p.x - _pendingPos.x), dy = (float)(p.y - _pendingPos.y);
        float slop = 10.0f * (float)self.view.contentScaleFactor;
        if (CACurrentMediaTime() - _pendingTime > 0.12 || dx * dx + dy * dy > slop * slop) {
            [self mouseAt:_pendingPos];
            ImGui::GetIO().AddMouseButtonEvent(0, true);
            [self mouseAt:p];
            _pending = NO;
            _mouseDown = YES;
        }
    }
    // two fingers: pinch = the mouse wheel of the PC at their centre (the other gestures follow the web rules, next)
    if (_twoFingers && _touches.size() >= 2) {
        auto it = _touches.begin();
        CGPoint a = it->second, b = (++it)->second;
        const float dist = std::hypot((float)(b.x - a.x), (float)(b.y - a.y));
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent((float)(a.x + b.x) * 0.5f, (float)(a.y + b.y) * 0.5f);
        if (_prevDist > 1.0f && dist > 1.0f) io.AddMouseWheelEvent(0.0f, std::log(dist / _prevDist) * 8.0f);
        _prevDist = dist;
    }
    _app->frame();
}

@end

@interface BKAppDelegate : UIResponder <UIApplicationDelegate>
@property(strong, nonatomic) UIWindow* window;
@end

@implementation BKAppDelegate
- (BOOL)application:(UIApplication*)app didFinishLaunchingWithOptions:(NSDictionary*)options {
    self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
    self.window.rootViewController = [[BKViewController alloc] init];
    [self.window makeKeyAndVisible];
    return YES;
}
@end

int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([BKAppDelegate class]));
    }
}
