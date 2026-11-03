// Synthetic GLES smoke test. Uses Mesa surfaceless EGL on Linux; no game data.
#include <cassert>
#include <cstdarg>
#include <dlfcn.h>
#include "render_ios.cpp"
void fatal(const char* message,...){va_list ap;va_start(ap,message);vfprintf(stderr,message,ap);va_end(ap);std::abort();}
int main(){
 void* egl=dlopen("libEGL.so.1",RTLD_NOW);assert(egl);
 auto proc=(void*(*)(const char*))dlsym(egl,"eglGetProcAddress");
 auto platform=(void*(*)(unsigned,void*,const int*))proc("eglGetPlatformDisplayEXT");assert(platform);
 void* display=platform(0x31DD,nullptr,nullptr); // EGL_PLATFORM_SURFACELESS_MESA
 auto initialize=(unsigned(*)(void*,int*,int*))dlsym(egl,"eglInitialize");assert(initialize(display,nullptr,nullptr));
 auto bind=(unsigned(*)(unsigned))dlsym(egl,"eglBindAPI");assert(bind(0x30A0));
 auto choose=(unsigned(*)(void*,const int*,void**,int,int*))dlsym(egl,"eglChooseConfig");
 int attrs[]={0x3040,0x40,0x3033,1,0x3038},count=0;void* config=nullptr;assert(choose(display,attrs,&config,1,&count)&&count);
 auto context=(void*(*)(void*,void*,void*,const int*))dlsym(egl,"eglCreateContext");int ca[]={0x3098,3,0x3038};void* ctx=context(display,config,nullptr,ca);assert(ctx);
 auto current=(unsigned(*)(void*,void*,void*,void*))dlsym(egl,"eglMakeCurrent");assert(current(display,nullptr,nullptr,ctx));
 assert(gladLoadGLES2((GLADloadfunc)proc));
 gx::g_vs=gx::compile(GL_VERTEX_SHADER,gx::gen_vertex_shader());
 // Exercise the same shipped metadata parsed by the renderer, without a disc.
 FILE* keys=fopen("src/shader_keys.bin","rb");assert(keys);uint32_t header[2];assert(fread(header,8,1,keys)==1);assert(header[0]==gx::kShaderKeysMagic&&header[1]==gx::kShaderKeyFormat);
 gx::ShaderKey key;while(fread(&key,sizeof(key),1,keys)==1){gx::ios_shader_key(key);if(!gx::g_programs.count(key)){auto source=gx::gen_pixel_shader(key);gx::register_program(key,gx::build_program(key,source,nullptr,0,0));}}fclose(keys);
 gx::IOSFrameTarget guest;assert(guest.create(4,4,true));gx::g_efb_fbo=guest.fbo;glGenVertexArrays(1,&gx::g_vao);glGenBuffers(1,&gx::g_vbo);
 int done=0,total=0;while(!gx::ios_warm_pipeline_step(&done,&total)){}assert(done==total&&total>0);assert(glGetError()==GL_NO_ERROR);
 GLint bound=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&bound);assert(GLuint(bound)==guest.fbo);
 assert(gx::ios_warm_pipeline_step(&done,&total));assert(glGetError()==GL_NO_ERROR);
 fprintf(stderr,"Warmup completed for %d shipped programs; guest framebuffer restored.\n",total);
}
