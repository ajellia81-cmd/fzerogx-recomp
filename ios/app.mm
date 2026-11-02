#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "angle.h"
#include "gx/gl.h"
#include "gx/render_gl.h"
#include "runtime.h"
#include "input_script.h"
#include "controller.h"
#include "save_card.h"
#include "render_ios.h"
#include "rumble.h"
#include "guest_state.h"
#include "vr_game.h"
#import "touch_controls.h"
#include <atomic>
#include <thread>
#include <unistd.h>

uint32_t boot_load(const char*);
bool audio_open();
static std::atomic<bool> running{false};
static std::atomic<bool> smoothMotion{true};
struct IOSPresentContext { EGLDisplay display; EGLSurface surface; PFNEGLSWAPBUFFERSPROC swap; bool failed=false; };
static void show_frame(void* opaque) {
    auto& p=*static_cast<IOSPresentContext*>(opaque);
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    if (!p.swap(p.display,p.surface)) { p.failed=true; running=false; }
}

static NSString* documents() {
    return NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
}

static bool metal_context(CAMetalLayer* layer, EGLDisplay* display, EGLSurface* surface) {
    using namespace vos::angle;
    if (!load()) return false;
    auto getDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)proc("eglGetPlatformDisplayEXT");
    auto initialize = (PFNEGLINITIALIZEPROC)proc("eglInitialize");
    auto bindAPI = (PFNEGLBINDAPIPROC)proc("eglBindAPI");
    auto choose = (PFNEGLCHOOSECONFIGPROC)proc("eglChooseConfig");
    auto createContext = (PFNEGLCREATECONTEXTPROC)proc("eglCreateContext");
    auto createSurface = (PFNEGLCREATEWINDOWSURFACEPROC)proc("eglCreateWindowSurface");
    auto makeCurrent = (PFNEGLMAKECURRENTPROC)proc("eglMakeCurrent");
    if (!getDisplay || !initialize || !bindAPI || !choose || !createContext || !createSurface || !makeCurrent) return false;
    const EGLint da[] = {EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE, EGL_NONE};
    *display = getDisplay(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, da);
    EGLint major, minor;
    if (!initialize(*display, &major, &minor) || !bindAPI(EGL_OPENGL_ES_API)) return false;
    const EGLint ca[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_NONE};
    EGLConfig config;
    EGLint count;
    if (!choose(*display, ca, &config, 1, &count) || count < 1) return false;
    const EGLint ctx[] = {EGL_CONTEXT_MAJOR_VERSION,3,EGL_CONTEXT_MINOR_VERSION,0,EGL_NONE};
    EGLContext context = createContext(*display, config, EGL_NO_CONTEXT, ctx);
    *surface = createSurface(*display, config, (__bridge EGLNativeWindowType)layer, nullptr);
    if (context == EGL_NO_CONTEXT || *surface == EGL_NO_SURFACE || !makeCurrent(*display,*surface,*surface,context)) return false;
    return gl_load_with(proc) >= GCN_GL_VERSION_MIN;
}

@interface GameView : UIView
@end
@implementation GameView
+ (Class)layerClass { return CAMetalLayer.class; }
@end

@interface GameController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic,strong) UILabel* status;
@property(nonatomic,strong) UIButton* start;
@property(nonatomic,strong) UIButton* importButton;
@property(nonatomic,strong) UIButton* saveButton;
@property(nonatomic,strong) CADisplayLink* inputLink;
@property(nonatomic) BOOL started;
@property(nonatomic,strong) FZTouchControls* touchControls;
@property(nonatomic,strong) UISwitch* smoothSwitch;
@property(nonatomic,strong) UILabel* fpsLabel;
@end

@implementation GameController
- (void)loadView { self.view = [[GameView alloc] initWithFrame:UIScreen.mainScreen.bounds]; }
- (BOOL)prefersStatusBarHidden { return YES; }
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures { return UIRectEdgeAll; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
- (void)viewDidLoad {
    [super viewDidLoad];
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    layer.device = MTLCreateSystemDefaultDevice();
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = NO;
    layer.contentsScale = self.view.window.screen.nativeScale ?: UIScreen.mainScreen.nativeScale;
    self.view.backgroundColor = UIColor.blackColor;
    self.status = [[UILabel alloc] init];
    self.status.textColor = UIColor.whiteColor;
    self.status.textAlignment = NSTextAlignmentCenter;
    self.status.numberOfLines = 0;
    self.status.text = @"F-Zero GX\n미국판 GFZE01 디스크 이미지를 선택해 주세요.\n플레이하려면 게임 컨트롤러를 연결해 주세요.";
    self.importButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.importButton setTitle:@"디스크 이미지 가져오기" forState:UIControlStateNormal];
    [self.importButton addTarget:self action:@selector(importImage) forControlEvents:UIControlEventTouchUpInside];
    self.saveButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.saveButton setTitle:@"세이브 카드 가져오기 (.raw)" forState:UIControlStateNormal];
    [self.saveButton addTarget:self action:@selector(importImage) forControlEvents:UIControlEventTouchUpInside];
    self.start = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.start setTitle:@"게임 시작" forState:UIControlStateNormal];
    [self.start addTarget:self action:@selector(startGame) forControlEvents:UIControlEventTouchUpInside];
    UILabel* smoothLabel = [[UILabel alloc] init];
    smoothLabel.text = @"120fps 움직임 보간 (실험적)";
    smoothLabel.textColor = UIColor.whiteColor;
    self.smoothSwitch = [[UISwitch alloc] init];
    self.smoothSwitch.on = UIScreen.mainScreen.maximumFramesPerSecond >= 120;
    self.smoothSwitch.enabled = UIScreen.mainScreen.maximumFramesPerSecond >= 120;
    UIStackView* smoothRow = [[UIStackView alloc] initWithArrangedSubviews:@[smoothLabel,self.smoothSwitch]];
    smoothRow.axis = UILayoutConstraintAxisHorizontal;
    smoothRow.spacing = 16;
    UIButton* rumbleTest = [UIButton buttonWithType:UIButtonTypeSystem];
    [rumbleTest setTitle:@"진동 테스트" forState:UIControlStateNormal];
    [rumbleTest addTarget:self action:@selector(testRumble) forControlEvents:UIControlEventTouchUpInside];
    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[self.status,self.importButton,self.saveButton,smoothRow,rumbleTest,self.start]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 8;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.tag = 100;
    [self.view addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[[stack.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [stack.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [stack.widthAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.widthAnchor constant:-32]]];
    self.touchControls = [[FZTouchControls alloc] initWithFrame:self.view.bounds];
    self.touchControls.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.touchControls.hidden = YES;
    [self.view addSubview:self.touchControls];
    self.fpsLabel = [[UILabel alloc] init];
    self.fpsLabel.font = [UIFont monospacedDigitSystemFontOfSize:12 weight:UIFontWeightSemibold];
    self.fpsLabel.textColor = UIColor.whiteColor;
    self.fpsLabel.backgroundColor = [UIColor colorWithWhite:0 alpha:.65];
    self.fpsLabel.numberOfLines = 3;
    self.fpsLabel.userInteractionEnabled = NO;
    self.fpsLabel.hidden = YES;
    self.fpsLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:self.fpsLabel];
    [NSLayoutConstraint activateConstraints:@[[self.fpsLabel.leadingAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor constant:8], [self.fpsLabel.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:6]]];
    self.inputLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(pollInput)];
    self.inputLink.preferredFrameRateRange = CAFrameRateRangeMake(120,120,120);
    [self.inputLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    CGFloat scale = self.view.window.screen.nativeScale ?: UIScreen.mainScreen.nativeScale;
    layer.contentsScale = scale;
    layer.drawableSize = CGSizeMake(self.view.bounds.size.width*scale,self.view.bounds.size.height*scale);
}
- (void)testRumble { ios_rumble_test(); }
- (void)pollInput {
    ios_rumble_update(self.started && ios_guest::motor.load(std::memory_order_relaxed));
    if (self.started) {
        static double lastTime=0;
        static uint64_t lastNative=0,lastIntermediate=0;
        const double now=CACurrentMediaTime();
        if(lastTime==0) { lastTime=now;gx::ios_frame_totals(&lastNative,&lastIntermediate); }
        if(now-lastTime>=1) {
            uint64_t native=0,intermediate=0;gx::ios_frame_totals(&native,&intermediate);
            double seconds=now-lastTime;
            self.fpsLabel.text = [NSString stringWithFormat:@" 출력 %.1f | 원본 %.1f | 보간 %.1f FPS \n 진동: %@ | %@ \n %@ ",(native-lastNative+intermediate-lastIntermediate)/seconds,(native-lastNative)/seconds,(intermediate-lastIntermediate)/seconds,[NSString stringWithUTF8String:ios_rumble_route()],[NSString stringWithUTF8String:gx::ios_scene_name()],[NSString stringWithUTF8String:gx::ios_motion_status()]];
            lastTime=now;lastNative=native;lastIntermediate=intermediate;
        }
    }
    IOSControllerState hardware = ios_controller_snapshot();
    BOOL hidden = !self.started || hardware.connected;
    if (self.touchControls.hidden != hidden) {
        [self.touchControls resetInput];
        self.touchControls.hidden = hidden;
    }
    pad_set_state(0, hardware.connected ? ios_controller_pad(hardware) : [self.touchControls padState]);
}
- (void)importImage {
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:NO];
    picker.delegate = self;
    [self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController*)picker didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    if (self.started || !urls.count) return;
    NSURL* source = urls.firstObject;
    NSString* ext = source.pathExtension.lowercaseString;
    if ([ext isEqualToString:@"raw"]) {
        BOOL access = [source startAccessingSecurityScopedResource];
        NSError* error = nil;
        NSData* card = [NSData dataWithContentsOfURL:source options:0 error:&error];
        if (access) [source stopAccessingSecurityScopedResource];
        if (!card || !ios_valid_save_card((const uint8_t*)card.bytes, card.length)) {
            self.status.text = @"유효한 512KiB 미국판 메모리카드 .raw 파일이 필요합니다.\n.gci는 메모리카드로 변환한 후 가져와 주세요.";
            return;
        }
        NSString* target = [documents() stringByAppendingPathComponent:@"memcard_a.raw"];
        NSFileManager* fm = NSFileManager.defaultManager;
        if ([fm fileExistsAtPath:target]) {
            NSString* backup = [target stringByAppendingFormat:@".backup-%@", NSUUID.UUID.UUIDString];
            if (![fm copyItemAtPath:target toPath:backup error:&error]) {
                self.status.text = @"기존 세이브를 백업하지 못해 가져오기를 중단했습니다.";
                return;
            }
        }
        BOOL ok = [card writeToFile:target options:NSDataWritingAtomic error:&error];
        self.status.text = ok ? @"세이브 카드 가져오기 완료. 게임을 시작해 주세요.\n기존 카드가 있으면 백업을 남겼습니다." : [@"세이브 저장 실패: " stringByAppendingString:error.localizedDescription ?: @"알 수 없는 오류"];
        return;
    }
    if (![ext isEqualToString:@"iso"] && ![ext isEqualToString:@"ciso"]) {
        self.status.text = @"디스크는 .iso/.ciso, 세이브 카드는 .raw 파일을 선택해 주세요.";
        return;
    }
    self.importButton.enabled = NO;
    self.saveButton.enabled = NO;
    self.start.enabled = NO;
    self.status.text = @"이미지 복사 중… 완료될 때까지 앱을 유지해 주세요.";
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0), ^{
        BOOL access = [source startAccessingSecurityScopedResource];
        NSString* target = [documents() stringByAppendingPathComponent:[@"game." stringByAppendingString:ext]];
        NSString* temp = [target stringByAppendingString:@".importing"];
        NSFileManager* fm = NSFileManager.defaultManager;
        [fm removeItemAtPath:temp error:nil];
        NSError* error = nil;
        BOOL ok = [fm copyItemAtURL:source toURL:[NSURL fileURLWithPath:temp] error:&error];
        if (access) [source stopAccessingSecurityScopedResource];
        if (ok) {
            [fm removeItemAtPath:target error:nil];
            ok = [fm moveItemAtPath:temp toPath:target error:&error];
            if (ok) [fm removeItemAtPath:[documents() stringByAppendingPathComponent:[ext isEqualToString:@"iso"] ? @"game.ciso" : @"game.iso"] error:nil];
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            self.importButton.enabled = YES;
            self.saveButton.enabled = YES;
            self.start.enabled = YES;
            self.status.text = ok ? @"이미지 가져오기 완료. 컨트롤러를 연결하고 게임을 시작해 주세요." : [@"복사 실패: " stringByAppendingString:error.localizedDescription ?: @"알 수 없는 오류"];
        });
    });
}
- (void)startGame {
    if (self.started) return;
    NSString* image = [documents() stringByAppendingPathComponent:@"game.iso"];
    if (![NSFileManager.defaultManager fileExistsAtPath:image]) image = [documents() stringByAppendingPathComponent:@"game.ciso"];
    FILE* disc = fopen(image.fileSystemRepresentation,"rb");
    char magic[4]{}, id[6]{};
    bool valid = disc && fread(magic,1,4,disc)==4;
    if (valid) valid = fseek(disc,memcmp(magic,"CISO",4)==0?0x8000:0,SEEK_SET)==0 && fread(id,1,6,disc)==6 && memcmp(id,"GFZE01",6)==0;
    if (disc) fclose(disc);
    if (!valid) { self.status.text = @"미국판 GFZE01 .iso/.ciso 이미지가 필요합니다.\n파일 앱에서 이 앱의 Documents에 game.iso로 넣어도 됩니다."; return; }
    smoothMotion = self.smoothSwitch.on && self.view.window.screen.maximumFramesPerSecond >= 120;
    self.inputLink.preferredFrameRateRange = smoothMotion ? CAFrameRateRangeMake(120,120,120) : CAFrameRateRangeMake(60,60,60);
    self.fpsLabel.hidden = NO;
    self.started = YES;
    running = true;
    UIApplication.sharedApplication.idleTimerDisabled = YES;
    self.importButton.hidden = YES;
    self.saveButton.hidden = YES;
    self.start.hidden = YES;
    self.status.text = @"Metal 및 셰이더 초기화 중…";
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    std::string path(image.fileSystemRepresentation);
    std::string dir(documents().fileSystemRepresentation);
    __weak GameController* weakSelf = self;
    std::thread([layer,path,dir,weakSelf] {
        @autoreleasepool {
            freopen((dir+"/log.txt").c_str(),"w",stderr);
            setvbuf(stderr,nullptr,_IONBF,0);
            EGLDisplay display;
            EGLSurface surface;
            if (!metal_context(layer,&display,&surface)) {
                fprintf(stderr,"iOS: ANGLE Metal initialization failed\n");
                dispatch_async(dispatch_get_main_queue(), ^{ weakSelf.status.text = @"Metal 초기화 실패. Documents/log.txt를 확인해 주세요."; });
                return;
            }
            auto swap = (PFNEGLSWAPBUFFERSPROC)vos::angle::proc("eglSwapBuffers");
            auto query = (PFNEGLQUERYSURFACEPROC)vos::angle::proc("eglQuerySurface");
            auto interval = (PFNEGLSWAPINTERVALPROC)vos::angle::proc("eglSwapInterval");
            if (!swap || !query || !interval) return;
            interval(display,1);
            mem_init(); timing_init(); step_set_scale(1.0); input_script_init();
            static std::string save,cache;
            save=dir; cache=dir+"/shaders.bin";
            g_save_dir=save.c_str();
            gx::render_set_shader_cache(cache.c_str());
            gx::render_init(1);
            fprintf(stderr,"iOS: GL %s / %s\n",glGetString(GL_VERSION),glGetString(GL_RENDERER));
            if (!audio_open()) fprintf(stderr,"iOS: audio unavailable\n");
            uint32_t entry=boot_load(path.c_str());
            threads_start_boot(entry);
            dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf.view viewWithTag:100].hidden=YES; });
            IOSPresentContext presentation{display,surface,swap};
            while (running) {
                auto batch=gx::take_batch(4);
                if (!batch) continue;
                EGLint width=0,height=0;
                query(display,surface,EGL_WIDTH,&width); query(display,surface,EGL_HEIGHT,&height);
                if (width<=0 || height<=0) continue;
                gx::render_set_window_size(width,height);
                const auto& hooks=vr::game_hooks();
                const bool race=hooks.wants_stereo && hooks.wants_stereo(*batch);
                gx::ios_configure_native(*batch,width,height);
                gx::ios_prepare_scene(*batch,race);
                gx::ios_widen(*batch,width,height);
                gx::ios_render_motion(*batch,smoothMotion.load(),show_frame,&presentation);
                if (presentation.failed) { fprintf(stderr,"iOS: surface presentation failed\n"); break; }
            }
            threads_request_quit();
        }
    }).detach();
}
@end

static void end_background_game() {
    // The upstream guest has no suspend/resume path. End this first port
    // rather than keep a game and audio running behind another app.
    if (running.exchange(false)) { threads_request_quit(); fflush(nullptr); _exit(0); }
}
@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic,strong) UIWindow* window;
@end
@implementation SceneDelegate
- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    if (![scene isKindOfClass:UIWindowScene.class]) return;
    self.window=[[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
    self.window.rootViewController=[[GameController alloc] init];
    [self.window makeKeyAndVisible];
}
- (void)sceneDidEnterBackground:(UIScene*)scene { end_background_game(); }
@end
@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation AppDelegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
    return YES;
}
- (UISceneConfiguration*)application:(UIApplication*)application configurationForConnectingSceneSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    UISceneConfiguration* config=[[UISceneConfiguration alloc] initWithName:@"Default" sessionRole:session.role];
    config.delegateClass=SceneDelegate.class;
    return config;
}
- (void)applicationDidEnterBackground:(UIApplication*)application {
    end_background_game();
}
@end
int main(int argc,char** argv) {
    @autoreleasepool { return UIApplicationMain(argc,argv,nil,NSStringFromClass(AppDelegate.class)); }
}
