/*
 * Copyright 2021 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/list.h"
#include "wine/unixlib.h"

#define MAX_PULSE_NAME_LEN 256

struct pulse_stream;

enum phys_device_bus_type {
    phys_device_bus_invalid = -1,
    phys_device_bus_pci,
    phys_device_bus_usb
};

struct pulse_config
{
    struct
    {
        WAVEFORMATEXTENSIBLE format;
        REFERENCE_TIME def_period;
        REFERENCE_TIME min_period;
    } modes[2];
};

struct endpoint
{
    WCHAR *name;
    char *pulse_name;
};

struct main_loop_params
{
    HANDLE event;
};

struct get_endpoint_ids_params
{
    EDataFlow flow;
    struct endpoint *endpoints;
    unsigned int size;
    HRESULT result;
    unsigned int num;
    unsigned int default_idx;
};

struct create_stream_params
{
    const char *name;
    const char *pulse_name;
    EDataFlow dataflow;
    AUDCLNT_SHAREMODE mode;
    DWORD flags;
    REFERENCE_TIME duration;
    const WAVEFORMATEX *fmt;
    HRESULT result;
    UINT32 *channel_count;
    struct pulse_stream **stream;
};

struct release_stream_params
{
    struct pulse_stream *stream;
    HANDLE timer;
    HRESULT result;
};

struct start_params
{
    struct pulse_stream *stream;
    HRESULT result;
};

struct stop_params
{
    struct pulse_stream *stream;
    HRESULT result;
};

struct reset_params
{
    struct pulse_stream *stream;
    HRESULT result;
};

struct timer_loop_params
{
    struct pulse_stream *stream;
};

struct get_render_buffer_params
{
    struct pulse_stream *stream;
    UINT32 frames;
    HRESULT result;
    BYTE **data;
};

struct release_render_buffer_params
{
    struct pulse_stream *stream;
    UINT32 written_frames;
    DWORD flags;
    HRESULT result;
};

struct get_capture_buffer_params
{
    struct pulse_stream *stream;
    HRESULT result;
    BYTE **data;
    UINT32 *frames;
    DWORD *flags;
    UINT64 *devpos;
    UINT64 *qpcpos;
};

struct release_capture_buffer_params
{
    struct pulse_stream *stream;
    BOOL done;
    HRESULT result;
};

struct get_buffer_size_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT32 *size;
};

struct get_latency_params
{
    struct pulse_stream *stream;
    HRESULT result;
    REFERENCE_TIME *latency;
};

struct get_current_padding_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT32 *padding;
};

struct get_device_mix_format_params
{
    const char *pulse_name;
    HRESULT result;
    BOOL render;
    WAVEFORMATEXTENSIBLE fmt;
};

struct get_device_period_params
{
    const char *pulse_name;
    HRESULT result;
    BOOL render;
    REFERENCE_TIME def_period;
    REFERENCE_TIME min_period;
};

struct get_next_packet_size_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT32 *frames;
};

struct get_frequency_params
{
    struct pulse_stream *stream;
    HRESULT result;
    UINT64 *freq;
};

struct get_position_params
{
    struct pulse_stream *stream;
    BOOL device;
    HRESULT result;
    UINT64 *pos;
    UINT64 *qpctime;
};

struct set_volumes_params
{
    struct pulse_stream *stream;
    float master_volume;
    const float *volumes;
    const float *session_volumes;
};

struct set_event_handle_params
{
    struct pulse_stream *stream;
    HANDLE event;
    HRESULT result;
};

struct set_sample_rate_params
{
    struct pulse_stream *stream;
    float new_rate;
    HRESULT result;
};

struct test_connect_params
{
    const char *name;
    HRESULT result;
    struct pulse_config *config;
};

struct is_started_params
{
    struct pulse_stream *stream;
    BOOL started;
};

struct get_prop_value_params
{
    const char *pulse_name;
    const GUID *guid;
    const PROPERTYKEY *prop;
    EDataFlow flow;
    HRESULT result;
    VARTYPE vt;
    union
    {
        WCHAR wstr[128];
        ULONG ulVal;
    };
};

enum unix_funcs
{
    process_attach,
    process_detach,
    main_loop,
    get_endpoint_ids,
    create_stream,
    release_stream,
    start,
    stop,
    reset,
    timer_loop,
    get_render_buffer,
    release_render_buffer,
    get_capture_buffer,
    release_capture_buffer,
    get_buffer_size,
    get_latency,
    get_current_padding,
    get_next_packet_size,
    get_frequency,
    get_position,
    set_volumes,
    set_event_handle,
    set_sample_rate,
    test_connect,
    is_started,
    get_prop_value,
    get_device_mix_format,
    get_device_period,
};
