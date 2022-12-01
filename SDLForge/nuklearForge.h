//
// Created by gamer on 12/1/2022.
//

#pragma once

#ifndef NK_IMPLEMENTATION
#include "nuklear.h"
#endif

#include <SDL2/SDL.h>

#include "Graphics/Interfaces/IGraphics.h"

/*
 * USAGE:
 *    - This function will initialize a new nuklear rendering context. The context will be bound to a GLOBAL DirectX 12 rendering state.
 */
struct nk_context *nk_d3d12_init(Renderer *device, int width, int height, unsigned int max_vertex_buffer, unsigned int max_index_buffer, unsigned int max_user_textures);
/*
 * USAGE:
 *    - A call to this function prepares the global nuklear d3d12 backend for receiving font information’s. Use the obtained atlas pointer to load all required fonts and do all required font setup.
 */
 void nk_d3d12_font_stash_begin(struct nk_font_atlas **atlas);
/*
 * USAGE:
 *    - Call this function after a call to nk_d3d12_font_stash_begin(...) when all fonts have been loaded and configured.
 *    - This function will place commands on the supplied ID3D12GraphicsCommandList.
 *    - This function will allocate temporary data that is required until the command list has finish executing. The temporary data can be free by calling nk_d3d12_font_stash_cleanup(...)
 */
 void nk_d3d12_font_stash_end(Cmd *command_list);
/*
 * USAGE:
 *    - This function will free temporary data that was allocated by nk_d3d12_font_stash_begin(...)
 *    - Only call this function after the command list used in the nk_d3d12_font_stash_begin(...) function call has finished executing.
 *    - It is NOT required to call this function but highly recommended.
 */
 void nk_d3d12_font_stash_cleanup();
/*
 * USAGE:
 *    - This function will setup the supplied texture (ID3D12Resource) for rendering custom images using the supplied D3D12_SHADER_RESOURCE_VIEW_DESC.
 *    - This function may override any previous calls to nk_d3d12_set_user_texture(...) while using the same index.
 *    - The returned handle can be used as texture handle to render custom images.
 *    - The caller must keep track of the state of the texture when it comes to rendering with nk_d3d12_render(...).
 */
 bool nk_d3d12_set_user_texture(unsigned int index, Texture* texture, nk_handle* handle_out);
/*
 * USAGE:
 *    - This function should be called within the user window proc to allow nuklear to listen to window events
 */
 int nk_d3d12_handle_event(SDL_Event* e);
/*
 * USAGE:
 *    - A call to this function renders any previous placed nuklear draw calls and will flush all nuklear buffers for the next frame
 *    - This function will place commands on the supplied ID3D12GraphicsCommandList.
 *    - When using custom images for rendering make sure they are in the correct resource state (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) when calling this function.
 *    - This function will upload data to the gpu (64 + max_vertex_buffer + max_index_buffer BYTES).
 */
 void nk_d3d12_render(Cmd *command_list, enum nk_anti_aliasing AA);
/*
 * USAGE:
 *    - This function will notify nuklear that the framebuffer dimensions have changed.
 */
 void nk_d3d12_resize(int width, int height);
/*
 * USAGE:
 *    - This function will free the global d3d12 rendering state.
 */
 void nk_d3d12_shutdown(void);

#define NUKLEAR_FORGE_IMPLEMENTATION
#ifdef NUKLEAR_FORGE_IMPLEMENTATION

struct nk_d3d12_vertex
{
    float position[2];
    float uv[2];
    nk_byte col[4];
};

static struct forge
{
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_buffer cmds;

    struct nk_draw_null_texture tex_null;
    unsigned int max_vertex_buffer;
    unsigned int max_index_buffer;
    unsigned int max_user_textures;

    int vpWidth;
    int vpHeight;

    Renderer *device;
    RootSignature *root_signature;
    Pipeline *pipeline_state;
    DescriptorSet *descriptor;
    Texture *font_texture;



    Buffer *const_buffer;
    Buffer *index_buffer;
    Buffer *vertex_buffer;

} forge;

NK_API void
nk_d3d12_render(Cmd *command_list, enum nk_anti_aliasing AA)
{

#ifdef NK_UINT_DRAW_INDEX
    IndexType index_buffer_format = INDEX_TYPE_UINT32;
#else
    IndexType index_buffer_format = INDEX_TYPE_UINT16;
#endif
    const UINT stride = sizeof(struct nk_d3d12_vertex);
    const struct nk_draw_command *cmd;
    UINT offset = 0;



    cmdBindPipeline(command_list, forge.pipeline_state);
    uint32_t vertexStride = sizeof(struct nk_d3d12_vertex);
    cmdBindVertexBuffer(command_list, 1, &forge.vertex_buffer, &vertexStride, NULL);
    cmdBindIndexBuffer(command_list, forge.index_buffer, index_buffer_format, 0);

    /* Issue draw commands */
    nk_draw_foreach(cmd, &forge.ctx, &forge.cmds)
    {
        D3D12_RECT scissor;
        UINT32 texture_id;

        /* Only place a drawcall in case the command contains drawable data */
        if(cmd->elem_count)
        {

            /* Setup texture (index to descriptor heap table) to use for draw call */
            texture_id = (UINT32)cmd->texture.id;
            cmdBindPushConstants
            ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(command_list, 1, 1, &texture_id, 0);

            cmdDrawIndexed(command_list, cmd->elem_count, offset, 0);
            offset += cmd->elem_count;
        }
    }

    /* Default nuklear context and command buffer clear */
    nk_clear(&d3d12.ctx);
    nk_buffer_clear(&d3d12.cmds);

    /* Map upload buffer to cpu accessible pointer */
    map_range.Begin = sizeof(float) * 4 * 4;
    map_range.End = map_range.Begin + d3d12.max_vertex_buffer + d3d12.max_index_buffer;
    hr = ID3D12Resource_Map(d3d12.upload_buffer, 0, &map_range, &ptr_data);
    NK_ASSERT(SUCCEEDED(hr));

    /* Nuklear convert and copy to upload buffer */
    {
        struct nk_convert_config config;
        NK_STORAGE const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_d3d12_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_d3d12_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_d3d12_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_d3d12_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_d3d12_vertex);
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.tex_null = d3d12.tex_null;

        struct nk_buffer vbuf, ibuf;
        nk_buffer_init_fixed(&vbuf, &ptr_data[sizeof(float) * 4 * 4], (size_t)d3d12.max_vertex_buffer);
        nk_buffer_init_fixed(&ibuf, &ptr_data[sizeof(float) * 4 * 4 + d3d12.max_vertex_buffer], (size_t)d3d12.max_index_buffer);
        nk_convert(&d3d12.ctx, &d3d12.cmds, &vbuf, &ibuf, &config);
    }

    /* Close mapping range */
    ID3D12Resource_Unmap(d3d12.upload_buffer, 0, &map_range);

    /* Issue GPU resource change for copying */
    resource_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barriers[0].Transition.pResource = d3d12.const_buffer;
    resource_barriers[0].Transition.Subresource = 0;
    resource_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    resource_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barriers[1].Transition.pResource = d3d12.vertex_buffer;
    resource_barriers[1].Transition.Subresource = 0;
    resource_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    resource_barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barriers[2].Transition.pResource = d3d12.index_buffer;
    resource_barriers[2].Transition.Subresource = 0;
    resource_barriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    resource_barriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ID3D12GraphicsCommandList_ResourceBarrier(command_list, 3, resource_barriers);

    /* Copy from upload buffer to gpu buffers */
    ID3D12GraphicsCommandList_CopyBufferRegion(command_list, d3d12.const_buffer, 0, d3d12.upload_buffer, 0, sizeof(float) * 4 * 4);
    ID3D12GraphicsCommandList_CopyBufferRegion(command_list, d3d12.vertex_buffer, 0, d3d12.upload_buffer, sizeof(float) * 4 * 4, d3d12.max_vertex_buffer);
    ID3D12GraphicsCommandList_CopyBufferRegion(command_list, d3d12.index_buffer, 0, d3d12.upload_buffer, sizeof(float) * 4 * 4 + d3d12.max_vertex_buffer, d3d12.max_index_buffer);

    /* Issue GPU resource change for rendering */
    resource_barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barriers[0].Transition.pResource = d3d12.const_buffer;
    resource_barriers[0].Transition.Subresource = 0;
    resource_barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    resource_barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barriers[1].Transition.pResource = d3d12.vertex_buffer;
    resource_barriers[1].Transition.Subresource = 0;
    resource_barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    resource_barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barriers[2].Transition.pResource = d3d12.index_buffer;
    resource_barriers[2].Transition.Subresource = 0;
    resource_barriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    resource_barriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ID3D12GraphicsCommandList_ResourceBarrier(command_list, 3, resource_barriers);


}

#endif