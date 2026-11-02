// Included INSIDE namespace gx by the generated iOS renderer only.
// The intermediate frame uses private copies of every writable GPU target. It
// cannot write guest RAM, increment guest frame counts, or mutate the real EFB.
static std::atomic<uint64_t> g_ios_native_frames{0}, g_ios_intermediate_frames{0};
void ios_set_race_view(bool race) { g_ios_wide_view=race; }
void ios_frame_totals(uint64_t* native, uint64_t* intermediate) {
    *native=g_ios_native_frames.load(std::memory_order_relaxed);
    *intermediate=g_ios_intermediate_frames.load(std::memory_order_relaxed);
}
void ios_widen(Batch& b, int width, int height) {
    if (!g_ios_wide_view || width<=0 || height<=0) return;
    const float aspect=float(width)/height;
    if (aspect<=4.0f/3.0f) return;
    note_fullscreen_copies(b);
    std::vector<uint8_t> skip;
    mark_offscreen_passes(b,skip);
    const size_t original=b.states.size();
    std::vector<int32_t> widened(original,-1);
    // Use the main camera's aspect, including the game's own widescreen option.
    // Do not infer UI shape independently from each model-preview projection.
    float cameraAspect=4.0f/3.0f; uint32_t largest=0;
    for(size_t i=0;i<b.cmds.size();++i) {
        const Cmd& c=b.cmds[i];
        if(c.type!=CmdType::Draw || skip[i])continue;
        const auto& st=b.states[c.state];
        if(st.proj[6]!=0 || st.proj[0]==0 || st.view_space || std::fabs(st.viewport[0])*2<600)continue;
        const float a=std::fabs(st.proj[2]/st.proj[0]);
        if(a>=1.2f && a<=1.9f && c.count>largest){largest=c.count;cameraAspect=a;}
    }
    for(size_t i=0;i<b.cmds.size();++i) {
        Cmd& c=b.cmds[i];
        if(c.type!=CmdType::Draw || skip[i] || c.state>=original)continue;
        if(widened[c.state]>=0){c.state=uint32_t(widened[c.state]);continue;}
        PixelState st=b.states[c.state];
        if(std::fabs(st.viewport[0])*2<600)continue;
        // A copy-compositing draw must cover exactly the already-widened image.
        // Scaling that copy a second time squashes UI and creates black strips.
        if(st.proj[6]!=0 && samples_fullscreen_copy(st))continue;
        float ratio=cameraAspect/aspect;
        if(st.proj[6]==0){
            if(st.proj[0]==0)continue;
            const float a=std::fabs(st.proj[2]/st.proj[0]);
            if(a<1.2f || a>1.9f)continue;
            ratio=a/aspect;
        } else {
            // Fullscreen solid fades/backgrounds retain full coverage, but normal
            // HUD geometry uses one consistent camera-aspect correction.
            const bool textured=st.num_texgens!=0;
            if(!textured && (st.bp[0x41]&1))continue;
        }
        st.proj[0]*=ratio;st.proj[1]*=ratio;
        widened[c.state]=int32_t(b.states.size());c.state=uint32_t(b.states.size());b.states.push_back(st);
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
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
        if(withDepth) {
            glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);
            glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT32F,w,h,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
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

bool ios_render_motion(Batch& b,bool smooth,void (*show)(void*),void* context) {
    static ios_motion::History history;
    static uint64_t ticks=0,intermediate=0,outputs=0,draws=0;
    static auto since=std::chrono::steady_clock::now();
    static std::vector<GpuVertex> realVertices;
    static std::vector<uint8_t> realPacked;
    bool eligible=smooth && show;
    // The first implementation deliberately excludes multi-strip XFB frames.
    // They require a separate history per completed image, not per batch.
    size_t presents=0;
    for(const auto& c:b.cmds) {
        if(c.type==CmdType::Present) ++presents;
        if(c.type==CmdType::Present && vi_display_height() && c.copy.dst_h<vi_display_height()) eligible=false;
        if(c.type==CmdType::EfbCopy && c.copy.tex_id) {
            auto it=g_textures.find(c.copy.tex_id);
            if(it!=g_textures.end() && (!it->second.efb || it->second.w!=c.copy.dst_w || it->second.h!=c.copy.dst_h)) eligible=false;
        }
    }
    eligible=eligible && presents==1;
    size_t moving=0;
    if(eligible) {
        realVertices=b.verts;realPacked=b.packed;
        moving=history.midpoint(b);
        if(!moving) { b.verts.swap(realVertices);b.packed.swap(realPacked);eligible=false; }
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
        bool ready=frame.create(EFB_W*g_scale,EFB_H*g_scale,true);
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
            bool ok=target.create(it->second.w*g_scale,it->second.h*g_scale,false);
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
    else {history.commands.clear();history.states.clear();history.vertices.clear();history.indices.clear();}
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
