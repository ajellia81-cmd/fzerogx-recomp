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
#include <atomic>
#include <thread>
#include <unistd.h>

uint32_t boot_load(const char*);
bool audio_open();
static std::atomic<bool> running{false};

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
@property(nonatomic,strong) CADisplayLink* inputLink;
@property(nonatomic) BOOL started;
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
    self.start = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.start setTitle:@"게임 시작" forState:UIControlStateNormal];
    [self.start addTarget:self action:@selector(startGame) forControlEvents:UIControlEventTouchUpInside];
    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[self.status,self.importButton,self.start]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 18;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.tag = 100;
    [self.view addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[[stack.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [stack.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [stack.widthAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.widthAnchor constant:-32]]];
    self.inputLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(pollInput)];
    self.inputLink.preferredFramesPerSecond = 60;
    [self.inputLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    CAMetalLayer* layer = (CAMetalLayer*)self.view.layer;
    CGFloat scale = self.view.window.screen.nativeScale ?: UIScreen.mainScreen.nativeScale;
    layer.contentsScale = scale;
    layer.drawableSize = CGSizeMake(self.view.bounds.size.width*scale,self.view.bounds.size.height*scale);
}
- (void)pollInput {
    pad_set_state(0, ios_controller_pad(ios_controller_snapshot()));
}
- (void)importImage {
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:NO];
    picker.delegate = self;
    [self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController*)picker didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    NSURL* source = urls.firstObject;
    NSString* ext = source.pathExtension.lowercaseString;
    if (![ext isEqualToString:@"iso"] && ![ext isEqualToString:@"ciso"]) {
        self.status.text = @".iso 또는 .ciso 파일을 선택해 주세요.";
        return;
    }
    self.importButton.enabled = NO;
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
    self.started = YES;
    running = true;
    UIApplication.sharedApplication.idleTimerDisabled = YES;
    self.importButton.hidden = YES;
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
            gx::render_init(3);
            fprintf(stderr,"iOS: GL %s / %s\n",glGetString(GL_VERSION),glGetString(GL_RENDERER));
            if (!audio_open()) fprintf(stderr,"iOS: audio unavailable\n");
            uint32_t entry=boot_load(path.c_str());
            threads_start_boot(entry);
            dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf.view viewWithTag:100].hidden=YES; });
            while (running) {
                auto batch=gx::take_batch(4);
                if (!batch) continue;
                EGLint width=0,height=0;
                query(display,surface,EGL_WIDTH,&width); query(display,surface,EGL_HEIGHT,&height);
                if (width<=0 || height<=0) continue;
                gx::render_set_window_size(width,height);
                if (gx::render_execute(*batch)) {
                    glBindFramebuffer(GL_FRAMEBUFFER,0);
                    if (!swap(display,surface)) { fprintf(stderr,"iOS: surface presentation failed\n"); break; }
                }
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
