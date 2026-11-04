// Included INSIDE namespace gx by the generated iOS renderer only.
// The intermediate frame uses private copies of every writable GPU target. It
// cannot write guest RAM, increment guest frame counts, or mutate the real EFB.
static std::atomic<uint64_t> g_ios_native_frames{0}, g_ios_intermediate_frames{0};
static std::atomic<ios_scene::Mode> g_ios_scene{ios_scene::Mode::Menu};
static std::atomic<int> g_ios_motion_reason{0};
static uint64_t g_ios_frame_scene=0;
static float g_ios_canvas_source=4.f/3;
static bool g_ios_frame_sega=false;
bool ios_prepare_scene(const Batch& b,bool liveRace) {
    auto context=ios_context::read(b);
    if(!context.stamped){context.modules=ios_guest::modules.load(std::memory_order_relaxed);context.phase=ios_guest::phase.load(std::memory_order_relaxed);context.live=liveRace;}
    g_ios_frame_scene=(uint64_t(context.modules)<<32)|uint32_t(context.phase);
    std::unordered_map<uint32_t,uint64_t> incoming;
    for(const auto& t:b.new_textures)incoming[t->id]=t->hash;
    bool startup=ios_ui::startup_art(b,[&](uint32_t id){auto it=incoming.find(id);return it==incoming.end()?render_texture_hash(id):it->second;});
    g_ios_frame_sega=ios_ui::startup_art(b,[&](uint32_t id){auto it=incoming.find(id);return it==incoming.end()?render_texture_hash(id):it->second;},true);
    static bool logoTransition=false;
    static float logoAspect=4.f/3;
    if(startup)logoAspect=ios_ui::startup_canvas_aspect(b,[&](uint32_t id){auto it=incoming.find(id);return it==incoming.end()?render_texture_hash(id):it->second;},logoAspect);
    if(startup)logoTransition=true;
    else if(!ios_ui::only_transition_draws(b))logoTransition=false;
    auto mode=logoTransition?ios_scene::Mode::Flat:ios_scene::classify(b,context.live,context.modules,context.phase);
    g_ios_frame_scene|=uint64_t(mode)<<60;
    g_ios_scene.store(mode,std::memory_order_relaxed);
    g_ios_wide_view=mode==ios_scene::Mode::World;
    static float movieAspect=4.f/3;
    if(mode==ios_scene::Mode::Movie)movieAspect=ios_scene::movie_canvas_aspect(b,movieAspect);
    else movieAspect=4.f/3;
    // Preserve the encoded image's aspect and any bars inside it. Its canvas
    // can differ between the game's normal and widescreen display options.
    g_ios_fixed_aspect=mode==ios_scene::Mode::Movie?movieAspect:
                       mode==ios_scene::Mode::Flat?(logoTransition?logoAspect:4.f/3):16.f/9;
    return g_ios_wide_view;
}
const char* ios_scene_name(){
    switch(g_ios_scene.load(std::memory_order_relaxed)){
        case ios_scene::Mode::World:return "3D 전체 화면";
        case ios_scene::Mode::Movie:return "동영상 원본 비율";
        case ios_scene::Mode::Flat:return "2D 원본 비율";
        default:return "메뉴 16:9";
    }
}
const char* ios_motion_status(){
    switch(g_ios_motion_reason.load(std::memory_order_relaxed)){
        case 1:return "끄기/비대상";case 2:return "부분 프레임";
        case 3:return "다중/미완성 프레임";case 4:return "GPU 대상 불일치";
        case 5:return "정지/대응 형상 없음";case 6:return "GPU 복제 실패";
        case 7:return "첫 프레임 준비";
        default:return "보간 활성";
    }
}
void ios_frame_totals(uint64_t* native, uint64_t* intermediate) {
    *native=g_ios_native_frames.load(std::memory_order_relaxed);
    *intermediate=g_ios_intermediate_frames.load(std::memory_order_relaxed);
}
void ios_widen(Batch& b,int width,int height){
    if(width<=0 || height<=0)return;
    auto mode=g_ios_scene.load(std::memory_order_relaxed);
    // These entire presentations already use their source aspect at output.
    if(mode==ios_scene::Mode::Flat || mode==ios_scene::Mode::Movie)return;
    const float target=mode==ios_scene::Mode::World?float(width)/height:16.f/9;
    note_fullscreen_copies(b);std::vector<uint8_t> skip;mark_offscreen_passes(b,skip);
    g_ios_canvas_source=ios_canvas::source_aspect(b,skip,g_ios_canvas_source);
    const float source=g_ios_canvas_source,ratio=source/target;
    const auto frame=ios_canvas::frame(b);
    const size_t original=b.states.size();
    // Material state can be shared by a full-frame effect and a small UI
    // element. Cache by the actual presentation path, never just by state id.
    std::vector<std::array<int32_t,3>> corrected(original,{-1,-1,-1});
    for(size_t i=0;i<b.cmds.size();++i){
        auto& c=b.cmds[i];
        if(c.type!=CmdType::Draw || !c.count || !c.vcount || c.state>=original || skip[i])continue;
        PixelState st=b.states[c.state];
        if(ios_scene::movie_state(st) || (st.proj[6] && samples_fullscreen_copy(st)) || ios_canvas::solid_cover(b,c,st) || ios_canvas::screen_effect(b,c,st))continue;
        unsigned path=st.proj[6]?0:1;
        if(corrected[c.state][path]>=0){c.state=uint32_t(corrected[c.state][path]);continue;}
        if(!st.proj[6] && ios_scene::full_view(st)){
            // Expand real camera FOV rather than stretching its image. This
            // is equivalent to the 2D affine transform inside the native frame,
            // but admits world geometry beyond the old left/right edges.
            if(st.proj[0]==0)continue;
            const float camera=std::fabs(st.proj[2]/st.proj[0]);
            if(!std::isfinite(camera) || camera<1.1f || camera>2.5f)continue;
            const float cameraRatio=camera/target;
            st.proj[0]*=cameraRatio;st.proj[1]*=cameraRatio;
        }else{
            // Includes every orthographic format, untextured panel geometry,
            // atlas digits, fonts, emblems, portraits and sub-view previews.
            // The viewport AND scissor move by the same global affine map.
            ios_canvas::compose(st,ratio,frame);
        }
        corrected[c.state][path]=int32_t(b.states.size());
        c.state=uint32_t(b.states.size());b.states.push_back(st);
    }
}

struct IOSFrameTarget {
    GLuint fbo=0,color=0,depth=0;
    int w=0,h=0;
    bool create(int width,int height,bool withDepth) {
        if(fbo && w==width && h==height && bool(depth)==withDepth)return true;
        drop();
        w=width;h=height;
        glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
        if(withDepth) {
            glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);
            glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT32F,w,h,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth,0);
        }
        return glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
    }
    void copy(GLuint source) const {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,source);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fbo);
        glBlitFramebuffer(0,0,w,h,0,0,w,h,GL_COLOR_BUFFER_BIT|(depth?GL_DEPTH_BUFFER_BIT:0),GL_NEAREST);
    }
    void drop() {
        if(fbo)glDeleteFramebuffers(1,&fbo);
        if(color)glDeleteTextures(1,&color);
        if(depth)glDeleteTextures(1,&depth);
        fbo=color=depth=0;
    }
};

// Menu frames keep their complete guest command stream unchanged. Their
// perspective layers are composed through a full-size EFB texture; widening
// those layers or moving their clears to another FBO exposes black rectangles.

// Match the visible source rect to the device drawable at 1:1 pixel density.
// The extra EFB rows remain allocated for the console's render-to-texture work.
void ios_configure_native(const Batch& b,int width,int height){
    if(width<=0 || height<=0)return;
    uint32_t sourceW=640,sourceH=vi_display_height()?vi_display_height():480;
    for(const auto& c:b.cmds)if(c.type==CmdType::Present && c.copy.src_w && c.copy.src_h && (!vi_display_height() || c.copy.dst_h>=vi_display_height())){sourceW=c.copy.src_w;sourceH=c.copy.src_h;break;}
    const float sx=float(width)/sourceW,sy=float(height)/sourceH;
    if(std::fabs(sx-g_ios_scale_x)<.00001f && std::fabs(sy-g_ios_scale_y)<.00001f)return;
    const float oldX=g_ios_scale_x,oldY=g_ios_scale_y;
    const int oldW=ios_px_x(EFB_W),oldH=ios_px_y(EFB_H);
    g_ios_scale_x=sx;g_ios_scale_y=sy;
    IOSFrameTarget replacement;
    if(!replacement.create(ios_px_x(EFB_W),ios_px_y(EFB_H),true))fatal("iOS native framebuffer incomplete");
    glDisable(GL_SCISSOR_TEST);glColorMask(1,1,1,1);glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,g_efb_fbo);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,replacement.fbo);
    glBlitFramebuffer(0,0,oldW,oldH,0,0,replacement.w,replacement.h,GL_COLOR_BUFFER_BIT,GL_LINEAR);
    glBlitFramebuffer(0,0,oldW,oldH,0,0,replacement.w,replacement.h,GL_DEPTH_BUFFER_BIT,GL_NEAREST);
    glDeleteFramebuffers(1,&g_efb_fbo);glDeleteTextures(1,&g_efb_color);glDeleteTextures(1,&g_efb_depth);
    g_efb_fbo=replacement.fbo;g_efb_color=replacement.color;g_efb_depth=replacement.depth;
    static GLuint reader=0;if(!reader)glGenFramebuffers(1,&reader);
    for(auto& [id,t]:g_textures){
        if(!t.efb)continue;
        IOSFrameTarget target;if(!target.create(ios_px_x(t.w),ios_px_y(t.h),false))fatal("iOS native copy target incomplete");
        glBindFramebuffer(GL_READ_FRAMEBUFFER,reader);glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,t.tex,0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,target.fbo);
        glBlitFramebuffer(0,0,int(std::lround(t.w*oldX)),int(std::lround(t.h*oldY)),0,0,target.w,target.h,GL_COLOR_BUFFER_BIT,t.depth?GL_NEAREST:GL_LINEAR);
        glDeleteTextures(1,&t.tex);t.tex=target.color;target.color=0;target.drop();
    }
    xfbs_drop();glBindFramebuffer(GL_FRAMEBUFFER,g_efb_fbo);gl_state_invalidate();uniforms_invalidate();
    fprintf(stderr,"iOS native: drawable=%dx%d EFB=%dx%d source=%ux%u\n",width,height,ios_px_x(EFB_W),ios_px_y(EFB_H),sourceW,sourceH);
}

// Startup only: force ANGLE's lazy Metal pipelines to see real non-empty draws.
// Private 4x4 RGBA8/depth32 targets match the real attachment formats. Never
// touch the guest EFB, guest RAM, frame counters, or presentation history.
struct IOSWarmRecipe {uint64_t key;uint32_t blend;};
static constexpr IOSWarmRecipe warmRecipes[]={
#include "menu_pipeline_states.inc"
};
static void ios_warm_blend(uint32_t bm,unsigned dst,bool alpha){
    bool cw=(bm>>3)&1,aw=((bm>>4)&1)&&alpha;glColorMask(cw,cw,cw,aw);
    if(bm&(1u<<11)){glEnable(GL_BLEND);glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT,dst?GL_FUNC_ADD:GL_FUNC_REVERSE_SUBTRACT);glBlendFuncSeparate(GL_ONE,GL_ONE,GL_ONE,dst?GL_ZERO:GL_ONE);}
    else if(bm&1){glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);GLenum sf=kBlendSrc[(bm>>8)&7],df=kBlendDst[(bm>>5)&7];
        if(dst){auto src1=[](GLenum f){return f==GL_SRC_ALPHA?GLenum(GL_SRC1_ALPHA):f==GL_ONE_MINUS_SRC_ALPHA?GLenum(GL_ONE_MINUS_SRC1_ALPHA):f;};glBlendFuncSeparate(src1(sf),src1(df),GL_ONE,GL_ZERO);}else glBlendFunc(sf,df);}
    else glDisable(GL_BLEND);
}
bool ios_warm_pipeline_step(int* done,int* total) {
    struct WarmProgram {GLuint program;unsigned ntex;uint64_t key;unsigned dst;bool alpha;};
    static std::vector<WarmProgram> programs;
    static size_t observedDraws=0;
    static size_t next=0;
    static IOSFrameTarget target;
    static GLuint vao=0,vbo=0,texture=0;
    static bool initialized=false,complete=false;
    if(!initialized){
        initialized=true;
        for(const auto& entry:g_programs)programs.push_back({entry.second.prog,std::min(entry.first.num_texgens,8u),ShaderKeyHash{}(entry.first),entry.first.dst_alpha,bool(entry.first.efb_has_alpha)});
        if(!target.create(4,4,true))fatal("iOS pipeline warmup framebuffer incomplete");
        glGenVertexArrays(1,&vao);glBindVertexArray(vao);
        glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);glEnableVertexAttribArray(2);
        glGenTextures(1,&texture);
        for(int i=0;i<16;++i){
            glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,texture);glBindSampler(i,0);
        }
        const uint32_t white=0xffffffff;
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,&white);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    }
    if(!complete && next<programs.size()){
        glBindFramebuffer(GL_FRAMEBUFFER,target.fbo);glBindVertexArray(vao);
        glViewport(0,0,4,4);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);
        const auto [program,ntex,key,dst,alpha]=programs[next++];glUseProgram(program);
        // Match the packed vertex layout the real draw uses; Metal includes it
        // in its pipeline key. A universal eight-coordinate layout is insufficient.
        GpuVertex vertices[3];memset(vertices,0,sizeof(vertices));
        const float positions[3][3]={{-1,-1,-.5f},{3,-1,-.5f},{-1,3,-.5f}};
        size_t stride=packed_stride(ntex);std::vector<uint8_t> packed(stride*3);
        for(int i=0;i<3;++i){memcpy(vertices[i].pos,positions[i],12);memset(vertices[i].col,255,8);for(auto& coord:vertices[i].tex)coord[2]=1;memcpy(packed.data()+i*stride,&vertices[i],stride);}
        glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,packed.size(),packed.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(GpuVertex,pos));
        for(int i=0;i<2;++i)glVertexAttribPointer(1+i,4,GL_UNSIGNED_BYTE,GL_TRUE,stride,(void*)(offsetof(GpuVertex,col)+4*i));
        for(unsigned i=0;i<8;++i){if(i<ntex){glEnableVertexAttribArray(3+i);glVertexAttribPointer(3+i,3,GL_FLOAT,GL_FALSE,stride,(void*)(offsetof(GpuVertex,tex)+12*i));}else{glDisableVertexAttribArray(3+i);glVertexAttrib4f(3+i,0,0,1,1);}}
        const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        glUniformMatrix4fv(glGetUniformLocation(program,"u_proj"),1,GL_FALSE,identity);
        glUniform4f(glGetUniformLocation(program,"u_vp_a"),0,1,0,1);
        glUniform4f(glGetUniformLocation(program,"u_vp_b"),0,1,0,0);
        glUniform1i(glGetUniformLocation(program,"u_vr"),0);
        float sizes[16];for(auto& v:sizes)v=1;
        glUniform2fv(glGetUniformLocation(program,"u_texsize"),8,sizes);
        // Frequent opaque, alpha, additive and subtractive paths; color write
        // masks are part of a Metal pipeline too. No missing draws are allowed.
        const GLenum factors[4][2]={{GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA},{GL_ONE,GL_ONE},{GL_SRC_ALPHA,GL_ONE},{GL_ONE,GL_ONE_MINUS_SRC_ALPHA}};
        for(int mask=0;mask<2;++mask){
            glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,mask?GL_TRUE:GL_FALSE);
            glDisable(GL_BLEND);glDrawArrays(GL_TRIANGLES,0,3);
            glEnable(GL_BLEND);glBlendEquation(GL_FUNC_ADD);
            for(auto& pair:factors){glBlendFunc(pair[0],pair[1]);glDrawArrays(GL_TRIANGLES,0,3);}
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);glBlendFunc(GL_ONE,GL_ONE);glDrawArrays(GL_TRIANGLES,0,3);
        }
        for(const auto& recipe:warmRecipes)if(recipe.key==key){ios_warm_blend(recipe.blend,dst,alpha);glDrawArrays(GL_TRIANGLES,0,3);++observedDraws;}
        glFinish(); // Startup only: complete the lazy pipeline work before boot.
        GLenum error=glGetError();
        if(error!=GL_NO_ERROR)fatal("iOS pipeline warmup GL error %x",unsigned(error));
    }
    if(!complete && next==programs.size()){
        complete=true;
        glDeleteVertexArrays(1,&vao);glDeleteBuffers(1,&vbo);
        for(int i=0;i<16;++i){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,0);}
        glDeleteTextures(1,&texture);target.drop();
        glActiveTexture(GL_TEXTURE0);glBindFramebuffer(GL_FRAMEBUFFER,g_efb_fbo);
        glBindVertexArray(g_vao);glBindBuffer(GL_ARRAY_BUFFER,g_vbo);
        glBlendEquation(GL_FUNC_ADD);glDisable(GL_BLEND);glColorMask(1,1,1,1);glDepthMask(GL_TRUE);
        gl_state_invalidate();uniforms_invalidate();
        fprintf(stderr,"iOS warmup: %zu programs, %zu pipeline draws completed before guest boot (coverage is not exhaustive)\n",programs.size(),programs.size()*12+observedDraws);
    }
    if(done)*done=int(next);if(total)*total=int(programs.size());return complete;
}

bool ios_render_motion(Batch& b,bool smooth,void (*show)(void*),void* context) {
    const auto frameStart=std::chrono::steady_clock::now();
    double swapMs=0,midMs=0;auto timedShow=[&]{auto at=std::chrono::steady_clock::now();show(context);swapMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-at).count();};
    static ios_motion::History history;
    // Reset at actual guest mode changes, not a fluctuating geometry heuristic.
    static uint64_t previousScene=UINT64_MAX;
    uint64_t scene=g_ios_frame_scene;
    if(scene!=previousScene){history.clear();previousScene=scene;}
    static uint64_t ticks=0,intermediate=0,outputs=0,draws=0;
    static auto since=std::chrono::steady_clock::now();
    static std::vector<GpuVertex> realVertices;
    static std::vector<uint8_t> realPacked;
    smooth=smooth && g_ios_scene.load(std::memory_order_relaxed)!=ios_scene::Mode::Movie;
    // Toggling smooth motion back on must not pair with a frame captured
    // before a long period of ordinary 60Hz play. Warm up from a fresh frame.
    if(!smooth)history.clear();
    bool eligible=smooth && show;
    int reason=eligible?0:1;
    // The first implementation deliberately excludes multi-strip XFB frames.
    // They require a separate history per completed image, not per batch.
    size_t presents=0;
    for(const auto& c:b.cmds) {
        if(c.type==CmdType::Present) ++presents;
        if(c.type==CmdType::Present && vi_display_height() && c.copy.dst_h<vi_display_height()) {eligible=false;reason=2;}
        if(c.type==CmdType::EfbCopy && c.copy.tex_id) {
            auto it=g_textures.find(c.copy.tex_id);
            if(it!=g_textures.end() && (!it->second.efb || it->second.w!=c.copy.dst_w || it->second.h!=c.copy.dst_h)) {eligible=false;reason=4;}
        }
    }
    if(presents!=1){eligible=false;reason=3;}
    size_t moving=0;
    const bool warming=history.empty();
    if(eligible) {
        realVertices=b.verts;realPacked=b.packed;
        moving=history.midpoint(b);
        if(!moving) { b.verts.swap(realVertices);b.packed.swap(realPacked);eligible=false;reason=warming?7:5; }
    }
    if(eligible) {
        // Prepare immutable textures/cache exactly once for this guest batch.
        ++g_render_frame;evict_textures(b);upload_textures(b);g_ios_prepared=true;
        static IOSFrameTarget frame;
        static std::unordered_map<uint32_t,IOSFrameTarget> clones;
        std::unordered_map<uint32_t,GlTex> saved;
        std::unordered_set<uint32_t> targets;
        for(const auto& c:b.cmds) if(c.type==CmdType::EfbCopy && !c.copy.to_xfb && c.copy.tex_id) targets.insert(c.copy.tex_id);
        GLuint originalFbo=g_efb_fbo, originalColor=g_efb_color, originalDepth=g_efb_depth;
        bool ready=frame.create(ios_px_x(EFB_W),ios_px_y(EFB_H),true);
        if(ready)frame.copy(originalFbo);
        static GLuint reader=0;if(!reader)glGenFramebuffers(1,&reader);
        for(auto it=clones.begin();it!=clones.end();) {
            if(!targets.count(it->first)){it->second.drop();it=clones.erase(it);}else ++it;
        }
        for(uint32_t id:targets) {
            auto it=g_textures.find(id);
            if(it==g_textures.end())continue;
            saved[id]=it->second;
            IOSFrameTarget& target=clones[id];
            bool ok=target.create(ios_px_x(it->second.w),ios_px_y(it->second.h),false);
            if(ok) {
                glBindFramebuffer(GL_READ_FRAMEBUFFER,reader);
                glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,it->second.tex,0);
                ok=glCheckFramebufferStatus(GL_READ_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
                if(ok)target.copy(reader);
            }
            ready=ready&&ok;
            it->second.tex=target.color;
            // The pooled target owns this texture; the map borrows it for replay.
        }
        if(ready) {
            g_efb_fbo=frame.fbo;g_efb_color=frame.color;g_efb_depth=frame.depth;
            g_ios_transient=true;
            frame_observe(b,nullptr);panel_hide(b);
            auto midStart=std::chrono::steady_clock::now();
            bool midPresented=execute_batch(b,true);midMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-midStart).count();
            if(midPresented) { timedShow();++intermediate;++g_ios_intermediate_frames;draws+=moving; }
            g_ios_transient=false;
        }
        if(!ready)reason=6;
        g_efb_fbo=originalFbo;g_efb_color=originalColor;g_efb_depth=originalDepth;
        for(uint32_t id:targets) {
            auto it=g_textures.find(id);
            if(it!=g_textures.end()) {
                auto pool=clones.find(id);
                if(pool==clones.end() || pool->second.color!=it->second.tex)glDeleteTextures(1,&it->second.tex);
                g_textures.erase(it);
            }
            auto old=saved.find(id);if(old!=saved.end())g_textures[id]=old->second;
        }

        gl_state_invalidate();++g_uniform_gen;
        b.verts.swap(realVertices);b.packed.swap(realPacked);
    }
    if(smooth) history.remember(b);
    else history.clear();
    g_ios_motion_reason.store(reason,std::memory_order_relaxed);
    auto nativeStart=std::chrono::steady_clock::now();
    bool presented=render_execute(b);
    double nativeMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-nativeStart).count();
    g_ios_prepared=false;
    if(presented && show) { timedShow(); if(g_ios_frame_sega)ios_logo::note_presentation(host_ns()); ++outputs; ++g_ios_native_frames; }
    double frameMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-frameStart).count();
    if(frameMs>25)fprintf(stderr,"iOS slow phases: prepare/history=%.1fms intermediate=%.1fms native=%.1fms swap/wait=%.1fms\n",std::max(0.,frameMs-midMs-nativeMs-swapMs),midMs,nativeMs,swapMs);
    ++ticks;
    double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-since).count();
    if(seconds>=5) {
        fprintf(stderr,"iOS render: batches=%.1f/s swaps=%.1f/s interpolated=%.1f/s moving_draws=%llu (not a display measurement)\n",ticks/seconds,(outputs+intermediate)/seconds,intermediate/seconds,(unsigned long long)draws);
        ticks=intermediate=outputs=draws=0;since=std::chrono::steady_clock::now();
    }
    return presented;
}


