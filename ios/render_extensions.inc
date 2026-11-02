// Included INSIDE namespace gx by the generated iOS renderer only.
// The intermediate frame uses private copies of every writable GPU target. It
// cannot write guest RAM, increment guest frame counts, or mutate the real EFB.
static std::atomic<uint64_t> g_ios_native_frames{0}, g_ios_intermediate_frames{0};
static std::atomic<ios_scene::Mode> g_ios_scene{ios_scene::Mode::Menu};
static std::atomic<int> g_ios_motion_reason{0};
bool ios_prepare_scene(const Batch& b,bool liveRace) {
    auto mode=ios_scene::classify(b,liveRace,ios_guest::modules.load(std::memory_order_relaxed),ios_guest::phase.load(std::memory_order_relaxed));
    g_ios_scene.store(mode,std::memory_order_relaxed);
    g_ios_wide_view=mode==ios_scene::Mode::World;
    static float movieAspect=4.f/3;
    if(mode==ios_scene::Mode::Movie)movieAspect=ios_scene::movie_canvas_aspect(b,movieAspect);
    else movieAspect=4.f/3;
    // Preserve the encoded image's aspect and any bars inside it. Its canvas
    // can differ between the game's normal and widescreen display options.
    g_ios_fixed_aspect=mode==ios_scene::Mode::Movie?movieAspect:
                       mode==ios_scene::Mode::Flat?4.f/3:16.f/9;
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
    if(mode==ios_scene::Mode::Flat || mode==ios_scene::Mode::Menu)return;
    const float aspect=mode==ios_scene::Mode::World?float(width)/height:
                       mode==ios_scene::Mode::Movie?g_ios_fixed_aspect:16.f/9;
    note_fullscreen_copies(b);std::vector<uint8_t> skip;
    mark_offscreen_passes(b,skip);
    const size_t original=b.states.size();
    std::vector<int32_t> corrected(original,-1);
    for(size_t i=0;i<b.cmds.size();++i){
        auto& c=b.cmds[i];
        if(c.type!=CmdType::Draw || skip[i] || c.state>=original)continue;
        PixelState st=b.states[c.state];
        if(std::fabs(st.viewport[0])*2<600)continue;
        float ratio=1;
        if(st.proj[6]!=0){
            if(samples_fullscreen_copy(st))continue;
            float box[4];bool bounds=ios_scene::draw_bounds(b,c,st,box);
            if(ios_scene::movie_state(st))continue;
            // Solid full-frame fades must cover the whole drawable. Textured
            // overlays (including logos) use the same 16:9 UI canvas as text.
            bool textured=false;for(auto id:st.tex_id)textured|=id!=0;
            if(!textured && bounds && box[0]<=-.99f && box[1]<=-.99f && box[2]>=.99f && box[3]>=.99f)continue;
            if(corrected[c.state]>=0){c.state=uint32_t(corrected[c.state]);continue;}
            // The game's widescreen option already compensates 2D geometry.
            // Inferring its canvas from the ortho matrix double-corrected it.
            ratio=(16.f/9)/aspect;
            if(std::fabs(ratio-1.f)<.000001f)continue;
            // Move the UI's clipping rectangle with its projection. Otherwise
            // the old scissor cuts corrected portraits/panels into black strips.
            float center=st.viewport[3];
            int left=int((st.bp[0x20]>>12)&2047),right=int((st.bp[0x21]>>12)&2047)+1;
            left=std::clamp(int(std::floor(center+(left-center)*ratio)),0,2047);
            right=std::clamp(int(std::ceil(center+(right-center)*ratio))-1,left,2047);
            st.bp[0x20]=(st.bp[0x20]&~(2047u<<12))|(unsigned(left)<<12);
            st.bp[0x21]=(st.bp[0x21]&~(2047u<<12))|(unsigned(right)<<12);
        }else{
            if(mode==ios_scene::Mode::Movie)continue;
            if(corrected[c.state]>=0){c.state=uint32_t(corrected[c.state]);continue;}
            if(st.proj[0]==0)continue;
            float cameraAspect=std::fabs(st.proj[2]/st.proj[0]);
            if(cameraAspect<1.1f || cameraAspect>2.5f)continue;
            ratio=cameraAspect/aspect;
        }
        st.proj[0]*=ratio;st.proj[1]*=ratio;
        corrected[c.state]=int32_t(b.states.size());c.state=uint32_t(b.states.size());b.states.push_back(st);
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

bool ios_render_motion(Batch& b,bool smooth,void (*show)(void*),void* context) {
    static ios_motion::History history;
    // Reset at actual guest mode changes, not a fluctuating geometry heuristic.
    static uint64_t previousScene=UINT64_MAX;
    uint64_t scene=(uint64_t(ios_guest::modules.load(std::memory_order_relaxed))<<32)|uint32_t(ios_guest::phase.load(std::memory_order_relaxed));
    if(scene!=previousScene){history.clear();previousScene=scene;}
    static uint64_t ticks=0,intermediate=0,outputs=0,draws=0;
    static auto since=std::chrono::steady_clock::now();
    static std::vector<GpuVertex> realVertices;
    static std::vector<uint8_t> realPacked;
    smooth=smooth && g_ios_scene.load(std::memory_order_relaxed)!=ios_scene::Mode::Movie;
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
            if(execute_batch(b,true)) { show(context);++intermediate;++g_ios_intermediate_frames;draws+=moving; }
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
    bool presented=render_execute(b);
    g_ios_prepared=false;
    if(presented && show) { show(context); ++outputs; ++g_ios_native_frames; }
    ++ticks;
    double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-since).count();
    if(seconds>=5) {
        fprintf(stderr,"iOS render: batches=%.1f/s swaps=%.1f/s interpolated=%.1f/s moving_draws=%llu (not a display measurement)\n",ticks/seconds,(outputs+intermediate)/seconds,intermediate/seconds,(unsigned long long)draws);
        ticks=intermediate=outputs=draws=0;since=std::chrono::steady_clock::now();
    }
    return presented;
}
