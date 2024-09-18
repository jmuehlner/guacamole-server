/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "color.h"
#include "rdp.h"
#include "settings.h"

#include <cairo/cairo.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/graphics.h>
#include <freerdp/primary.h>
#include <guacamole/assert.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/protocol.h>
#include <winpr/wtypes.h>

#include <stddef.h>

void guac_rdp_gdi_mark_frame(rdpContext* context, int starting) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* A new frame is beginning */
    if (starting) {
        rdp_client->in_frame = 1;
        return;
    }

    /* A new frame has been received from the RDP server and processed */
    rdp_client->in_frame = 0;
    rdp_client->frames_received++;

}

BOOL guac_rdp_gdi_frame_marker(rdpContext* context, const FRAME_MARKER_ORDER* frame_marker) {
    guac_rdp_gdi_mark_frame(context, frame_marker->action == FRAME_START);
    return TRUE;
}

BOOL guac_rdp_gdi_surface_frame_marker(rdpContext* context, const SURFACE_FRAME_MARKER* surface_frame_marker) {

    guac_rdp_gdi_mark_frame(context, surface_frame_marker->frameAction != SURFACECMD_FRAMEACTION_END);

    int frame_acknowledge;
#ifdef HAVE_SETTERS_GETTERS
    frame_acknowledge = freerdp_settings_get_uint32(context->settings, FreeRDP_FrameAcknowledge);
#else
    frame_acknowledge = context->settings->FrameAcknowledge;
#endif

    if (frame_acknowledge > 0)
        IFCALL(context->update->SurfaceFrameAcknowledge, context,
                surface_frame_marker->frameId);

    return TRUE;

}

BOOL guac_rdp_gdi_begin_paint(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    rdpGdi* gdi = context->gdi;

    guac_display_layer_raw_context* current_context = rdp_client->current_context;
    current_context->buffer = gdi->primary_buffer;
    current_context->stride = gdi->stride;
    guac_rect_init(&current_context->bounds, 0, 0, gdi->width, gdi->height);

    return TRUE;

}

BOOL guac_rdp_gdi_end_paint(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    rdpGdi* gdi = context->gdi;

    guac_display_layer_raw_context* current_context = rdp_client->current_context;

    /* Ignore paint if GDI output is suppressed */
    if (gdi->suppressOutput)
        goto paint_complete;

    /* Ignore paint if nothing has been done (empty rect) */
    if (gdi->primary->hdc->hwnd->invalid->null)
        goto paint_complete;

    INT32 x = gdi->primary->hdc->hwnd->invalid->x;
    INT32 y = gdi->primary->hdc->hwnd->invalid->y;
    UINT32 w = gdi->primary->hdc->hwnd->invalid->w;
    UINT32 h = gdi->primary->hdc->hwnd->invalid->h;

    guac_rect dst_rect;
    guac_rect_init(&dst_rect, x, y, w, h);
    guac_rect_constrain(&dst_rect, &current_context->bounds);

    guac_rect_extend(&current_context->dirty, &dst_rect);

paint_complete:
    return TRUE;

}

BOOL guac_rdp_gdi_desktop_resize(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_display_layer* default_layer = guac_display_default_layer(rdp_client->display);
    rdpGdi* gdi = context->gdi;

    int width = guac_rdp_get_width(context->instance);
    int height = guac_rdp_get_height(context->instance);

    guac_display_layer_raw_context* current_context = rdp_client->current_context;

    /* Resize FreeRDP's GDI buffer */
    BOOL retval = gdi_resize(context->gdi, width, height);
    GUAC_ASSERT(gdi->primary_buffer != NULL);

    /* Update our reference to the GDI buffer, as well as any structural
     * details, which may now all be different */
    current_context->buffer = gdi->primary_buffer;
    current_context->stride = gdi->stride;
    guac_rect_init(&current_context->bounds, 0, 0, gdi->width, gdi->height);

    /* Resize layer to match new display dimensions and underlying buffer */
    guac_display_layer_resize(default_layer, gdi->width, gdi->height);
    guac_client_log(client, GUAC_LOG_DEBUG, "Server resized display to %ix%i",
            gdi->width, gdi->height);

    return retval;

}
