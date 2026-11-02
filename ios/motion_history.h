#pragma once
#include "gx/render.h"
#include <cmath>
#include <cstring>

namespace ios_motion {
struct History {
    std::vector<gx::Cmd> commands;
    std::vector<gx::PixelState> states;
    std::vector<gx::GpuVertex> vertices;
    std::vector<uint32_t> indices;
    void remember(const gx::Batch& b) {
        commands = b.cmds; states = b.states; vertices = b.verts; indices = b.indices;
    }
    // Accept only stable draw order AND identical topology, attributes, projection
    // and material. New/removed/reordered geometry and animated texture effects
    // keep the current tick. Never guess an object's identity from its draw index.
    size_t midpoint(gx::Batch& b) const {
        size_t changed = 0;
        for (size_t i = 0; i < b.cmds.size() && i < commands.size(); ++i) {
            const auto& a = commands[i]; const auto& c = b.cmds[i];
            if (a.type != gx::CmdType::Draw || c.type != a.type || c.vcount != a.vcount
                || c.count != a.count || c.prim != a.prim || !c.vcount
                || c.state >= b.states.size() || a.state >= states.size()
                || size_t(c.vfirst)+c.vcount > b.verts.size()
                || size_t(a.vfirst)+a.vcount > vertices.size()
                || size_t(c.first)+c.count > b.indices.size()
                || size_t(a.first)+a.count > indices.size()) continue;
            const auto& ps = states[a.state]; const auto& cs = b.states[c.state];
            if (cs.proj[6] != 0 || std::memcmp(&ps, &cs, sizeof(cs))) continue;
            bool match = true;
            for (uint32_t j = 0; match && j < c.count; ++j)
                match = indices[a.first+j]-a.vfirst == b.indices[c.first+j]-c.vfirst;
            float maxMovement = 0;
            for (uint32_t j = 0; match && j < c.vcount; ++j) {
                const auto& old = vertices[a.vfirst+j]; const auto& v = b.verts[c.vfirst+j];
                match = old.src_idx == v.src_idx
                    && !std::memcmp(old.col, v.col, sizeof(v.col))
                    && !std::memcmp(old.tex, v.tex, gx::draw_ntex(cs)*3*sizeof(float));
                for (unsigned k = 0; match && k < 3; ++k) {
                    if (!std::isfinite(v.pos[k]) || !std::isfinite(old.pos[k])) { match = false; break; }
                    float scale = std::fmax(1.0f, std::fmax(std::fabs(v.pos[2]), std::fabs(old.pos[2])));
                    maxMovement = std::fmax(maxMovement, std::fabs(v.pos[k]-old.pos[k])/scale);
                }
            }
            // Camera cuts, large teleports and particles recycling a slot must snap.
            if (!match || maxMovement > .15f) continue;
            const size_t stride = gx::packed_stride(gx::draw_ntex(cs));
            bool bounds = true;
            for (uint32_t j=0; j<c.vcount; ++j) {
                int64_t vertex = int64_t(c.vfirst)+j+c.base_vertex;
                if (vertex < 0 || size_t(vertex)*stride+12 > b.packed.size()) { bounds=false; break; }
            }
            if (!bounds) continue;
            for (uint32_t j = 0; j < c.vcount; ++j) {
                auto& v = b.verts[c.vfirst+j]; const auto& old = vertices[a.vfirst+j];
                for (unsigned k=0;k<3;++k) v.pos[k] = (old.pos[k]+v.pos[k])*.5f;
                std::memcpy(b.packed.data()+(int64_t(c.vfirst)+j+c.base_vertex)*stride,v.pos,12);
            }
            if (maxMovement > 0) changed++;
        }
        return changed;
    }
};
}
