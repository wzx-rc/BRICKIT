// BrickIt for iPad: the very BrickIt of the PC (src/brickit, same sources: UI, library, building, mechanics...) in a
// full-screen CAMetalLayer, rendered by the Lit renderer of the WZX engine through Diligent on MoltenVK (Vulkan
// translated to Metal). No ray tracing on the iPad Pro 2nd generation (A12Z): raster image only.
// This file is the iOS shell, as BrickAppWin.cpp is the Windows one: window, device, display loop, raw touches.
#include "BrickApp.h"
#include "CrashLog.h"
#include "GfxLit.h"
#include "Platform.h"
#include "imgui.h"

#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"

#import <MetricKit/MetricKit.h>
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

// The diagnostics of iOS (MetricKit): at the launch after a crash, a hang or a CPU / disk excess, iOS hands its own
// report (the stacks of all threads, as addresses + the UUID of the binary). Written as Rapports/metrickit_<date>.json
// next to the reports of BrickIt; « Envoyer un rapport » joins the last one.
@interface BKDiagnostics : NSObject <MXMetricManagerSubscriber>
@property(copy, nonatomic) NSString* dir;
@end

@implementation BKDiagnostics
- (void)didReceiveMetricPayloads:(NSArray<MXMetricPayload*>*)payloads {
}
- (void)didReceiveDiagnosticPayloads:(NSArray<MXDiagnosticPayload*>*)payloads API_AVAILABLE(ios(14.0)) {
    NSDateFormatter* f = [NSDateFormatter new];
    f.dateFormat = @"yyyyMMdd_HHmmss";
    int k = 0;
    for (MXDiagnosticPayload* p in payloads) {
        NSData* json = [p JSONRepresentation];
        if (!json.length) continue;
        NSString* name = [NSString stringWithFormat:@"metrickit_%@%@.json", [f stringFromDate:p.timeStampEnd ?: [NSDate date]], k ? [NSString stringWithFormat:@"_%d", k + 1] : @""];
        [json writeToFile:[self.dir stringByAppendingPathComponent:name] atomically:YES];
        brickit::crumb("Diagnostic iOS reçu : %s (%lu plantage(s))", name.UTF8String, (unsigned long)p.crashDiagnostics.count);
        ++k;
    }
}
@end

static BKDiagnostics* g_diagnostics;

@interface BKViewController : UIViewController
- (BOOL)openDocument:(NSURL*)url;
@end

@implementation BKViewController {
    std::unique_ptr<brickit::BrickApp> _app;
    CADisplayLink* _displayLink;
    BOOL _ready;
    NSString* _pendingOpen;   // a .brick handed by the system before the app was ready
}

// A .brick from Fichiers, AirDrop, Mail or another app: copied into Documents (the app's folder in Fichiers, where
// Enregistrer writes) and opened. Files opened in place stay where they are (security-scoped access while copying).
- (BOOL)openDocument:(NSURL*)url {
    if (!url.isFileURL) return NO;
    const BOOL scoped = [url startAccessingSecurityScopedResource];
    NSString* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    NSString* dest = [docs stringByAppendingPathComponent:url.lastPathComponent];
    NSFileManager* fm = [NSFileManager defaultManager];
    BOOL ok = YES;
    if (![[url.path stringByStandardizingPath] isEqualToString:[dest stringByStandardizingPath]]) {
        [fm removeItemAtPath:dest error:nil];
        ok = [fm copyItemAtURL:url toURL:[NSURL fileURLWithPath:dest] error:nil];
    }
    if (scoped) [url stopAccessingSecurityScopedResource];
    if (!ok) return NO;
    // the copy of the Inbox (AirDrop, Mail) is not needed any more
    if ([url.path rangeOfString:@"/Documents/Inbox/"].location != NSNotFound) [fm removeItemAtURL:url error:nil];
    if (!_ready || !_app) { _pendingOpen = dest; return YES; }
    _app->openSceneFile(brickit::plat::fromUtf8(dest.UTF8String));
    return YES;
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
    // the crash / freeze reports: Documents/Rapports, visible in Fichiers › BrickIt (CrashLog.cpp, started by initCommon)
    NSString* reports = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject
                         stringByAppendingPathComponent:@"Rapports"];
    [[NSFileManager defaultManager] createDirectoryAtPath:reports withIntermediateDirectories:YES attributes:nil error:nil];
    brickit::plat::setReportsDir(brickit::plat::fromUtf8(reports.UTF8String));
    g_diagnostics = [BKDiagnostics new];
    g_diagnostics.dir = reports;
    [[MXMetricManager sharedManager] addSubscriber:g_diagnostics];

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
    opt.ipad = true;    // touch gestures of the web, the simpler interface of the iPad
    opt.touchHud = 1;   // the touch HUD of the PC (Tiny Glade style) on from the start
    _app = std::make_unique<brickit::BrickApp>();
    if (!_app->initCommon(opt, std::move(renderer), std::move(gui), brickit::plat::fromUtf8(dir), (float)scale)) {
        [self showError:@"Initialisation de BrickIt impossible (voir brickit_start.log)."];
        _app.reset();
        return;
    }
    _ready = YES;
    if (_pendingOpen) { _app->openSceneFile(brickit::plat::fromUtf8(_pendingOpen.UTF8String)); _pendingOpen = nil; }
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawFrame:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(willResignActive:)
                                                 name:UIApplicationWillResignActiveNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didBecomeActive:)
                                                 name:UIApplicationDidBecomeActiveNotification object:nil];
}

// iOS may end the app at any time once it is in the background: settings and autosave as on PC exit
- (void)willResignActive:(NSNotification*)n {
    brickit::crashSetActive(false);   // (the display loop stops: not a freeze for the watchdog)
    if (_app) _app->saveSession();
}

- (void)didBecomeActive:(NSNotification*)n {
    brickit::crashSetActive(true);
}

// iOS asks for memory back: noted in the journal (an app killed for memory leaves no crash, only this line)
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    brickit::crumb("Avertissement mémoire d'iOS");
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

// --------------------------------------------------------------------------- fingers and Pencil -> BrickApp
// The raw touches go to the shared code (BrickTouch.cpp): the touch gestures of the web builder in the 3D view, the
// mouse of the PC on the panels.

- (CGPoint)pixels:(UITouch*)t {
    CGPoint p = [t locationInView:self.view];
    CGFloat k = self.view.contentScaleFactor;
    return CGPointMake(p.x * k, p.y * k);
}

static uint64_t touchId(UITouch* t) { return (uint64_t)(uintptr_t)(__bridge void*)t; }

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) {
        const CGPoint p = [self pixels:t];
        _app->touchBegan(touchId(t), (float)p.x, (float)p.y, t.type == UITouchTypePencil);
    }
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) {
        const CGPoint p = [self pixels:t];
        _app->touchMoved(touchId(t), (float)p.x, (float)p.y);
    }
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) {
        const CGPoint p = [self pixels:t];
        _app->touchEnded(touchId(t), (float)p.x, (float)p.y, false);
    }
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    if (!_ready) return;
    for (UITouch* t in touches) {
        const CGPoint p = [self pixels:t];
        _app->touchEnded(touchId(t), (float)p.x, (float)p.y, true);
    }
}

// --------------------------------------------------------------------------- frame

- (void)drawFrame:(CADisplayLink*)link {
    if (!_ready || !_app) return;
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
// a .brick opened from another app (Fichiers, AirDrop, Mail, « Ouvrir dans BrickIt »)
- (BOOL)application:(UIApplication*)app openURL:(NSURL*)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id>*)options {
    return [(BKViewController*)self.window.rootViewController openDocument:url];
}
@end

int main(int argc, char* argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([BKAppDelegate class]));
    }
}
