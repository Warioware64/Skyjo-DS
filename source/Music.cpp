#include "Music.hpp"
#include "globalHeader.hpp"   // <NEAMain.h> -> NEASound.h -> maxmod9.h, plus <nds.h>/<filesystem.h>
#include "Process.hpp"        // global `process` (gamesettings.musicSoundVolume)

#include <cstring>            // memcpy, strcmp
#include <cstdio>             // FILE*, fopen/fread/fseek/feof/fclose

// WAV streaming ported from the Nitro Engine Advanced example
// (examples/sound/streaming/source/main.c). The menu and game tracks are both
// 16-bit stereo 11025 Hz PCM with a canonical 44-byte header, so the PCM data
// begins at sizeof(WAVHeader) and looping is just a seek back to that offset on
// EOF.
//
// The streaming callback runs inside a timer interrupt (MM_TIMER0), so it must
// only copy from the circular buffer and never touch the filesystem. The file
// reads happen from Music::Pump(), driven once per frame by the active render
// loop. Only one stream exists at a time; switching tracks stops the old one.

namespace
{
    constexpr uint32_t DATA_ID = 0x61746164; // "data"
    constexpr uint32_t FMT_ID  = 0x20746d66; // "fmt "
    constexpr uint32_t RIFF_ID = 0x46464952; // "RIFF"
    constexpr uint32_t WAVE_ID = 0x45564157; // "WAVE"

    struct WAVHeader
    {
        uint32_t chunkID;
        uint32_t chunkSize;
        uint32_t format;
        uint32_t subchunk1ID;
        uint32_t subchunk1Size;
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
        uint32_t subchunk2ID;
        uint32_t subchunk2Size;
    };

    constexpr int BUFFER_LENGTH = 16384;

    FILE       *wavFile = nullptr;
    const char *currentPath = nullptr;
    char        stream_buffer[BUFFER_LENGTH];
    int         stream_buffer_in = 0;
    int         stream_buffer_out = 0;
    bool        active = false;
    bool        inited = false;

    mm_word streamingCallback(mm_word length, mm_addr dest,
                              mm_stream_formats format)
    {
        size_t multiplier = 0;
        if (format == MM_STREAM_8BIT_MONO)         multiplier = 1;
        else if (format == MM_STREAM_8BIT_STEREO)  multiplier = 2;
        else if (format == MM_STREAM_16BIT_MONO)   multiplier = 2;
        else if (format == MM_STREAM_16BIT_STEREO) multiplier = 4;

        size_t size = length * multiplier;
        size_t bytes_until_end = BUFFER_LENGTH - stream_buffer_out;

        if (bytes_until_end > size)
        {
            memcpy(dest, &stream_buffer[stream_buffer_out], size);
            stream_buffer_out += size;
        }
        else
        {
            char *dst = static_cast<char *>(dest);
            memcpy(dst, &stream_buffer[stream_buffer_out], bytes_until_end);
            dst += bytes_until_end;
            size -= bytes_until_end;
            memcpy(dst, &stream_buffer[0], size);
            stream_buffer_out = size;
        }
        return length;
    }

    // Read `size` bytes into `buffer`, looping back to the start of the PCM data
    // (just past the header) whenever the end of the file is reached.
    void readFile(char *buffer, size_t size)
    {
        while (size > 0)
        {
            int res = fread(buffer, 1, size, wavFile);
            size -= res;
            buffer += res;

            if (feof(wavFile))
            {
                fseek(wavFile, sizeof(WAVHeader), SEEK_SET);
                res = fread(buffer, 1, size, wavFile);
                size -= res;
                buffer += res;
            }
        }
    }

    void streamingFillBuffer(bool force_fill)
    {
        if (!force_fill)
        {
            if (stream_buffer_in == stream_buffer_out)
                return;
        }

        if (stream_buffer_in < stream_buffer_out)
        {
            size_t size = stream_buffer_out - stream_buffer_in;
            readFile(&stream_buffer[stream_buffer_in], size);
            stream_buffer_in += size;
        }
        else
        {
            size_t size = BUFFER_LENGTH - stream_buffer_in;
            readFile(&stream_buffer[stream_buffer_in], size);
            stream_buffer_in = 0;

            size = stream_buffer_out - stream_buffer_in;
            readFile(&stream_buffer[stream_buffer_in], size);
            stream_buffer_in += size;
        }

        if (stream_buffer_in >= BUFFER_LENGTH)
            stream_buffer_in -= BUFFER_LENGTH;
    }

    bool checkWAVHeader(const WAVHeader &h)
    {
        return h.chunkID == RIFF_ID && h.format == WAVE_ID &&
               h.subchunk1ID == FMT_ID && h.subchunk2ID == DATA_ID;
    }

    mm_stream_formats getMMStreamType(uint16_t numChannels, uint16_t bitsPerSample)
    {
        if (numChannels == 1)
            return bitsPerSample == 8 ? MM_STREAM_8BIT_MONO : MM_STREAM_16BIT_MONO;
        return bitsPerSample == 8 ? MM_STREAM_8BIT_STEREO : MM_STREAM_16BIT_STEREO;
    }
}

void Music::InitOnce()
{
    if (inited)
        return;

    // No soundbank: initialize maxmod manually (mirrors the NEA streaming
    // example). mmInit brings up the ARM7 sound hardware itself.
    mm_ds_system sys;
    sys.mod_count    = 0;
    sys.samp_count   = 0;
    sys.mem_bank     = nullptr;
    sys.fifo_channel = FIFO_MAXMOD;
    mmInit(&sys);

    // Allocate NEA's sound-source pool (maxmod is already inited above).
    NEA_SoundSystemResetPool(1);

    inited = true;
}

void Music::Play(const char *path)
{
    if (active)
    {
        if (currentPath != nullptr && strcmp(currentPath, path) == 0)
            return;      // already playing this track
        Music::Stop();   // switch to a different track
    }

    // This build is compiled with -fno-exceptions, so on any failure we bail
    // out cleanly and leave things silent rather than aborting.
    wavFile = fopen(path, "rb");
    if (wavFile == nullptr)
        return;

    WAVHeader header = {};
    if (fread(&header, 1, sizeof(header), wavFile) != sizeof(header) ||
        !checkWAVHeader(header))
    {
        fclose(wavFile);
        wavFile = nullptr;
        return;
    }

    stream_buffer_in = 0;
    stream_buffer_out = 0;
    streamingFillBuffer(true); // prime the circular buffer before opening

    NEA_StreamOpen(header.sampleRate, 2048, streamingCallback,
                   getMMStreamType(header.numChannels, header.bitsPerSample),
                   MM_TIMER0);
    active = true;
    currentPath = path;

    Music::ApplyVolume();
}

void Music::Pump()
{
    if (active)
        streamingFillBuffer(false);
}

void Music::Stop()
{
    if (!active)
        return;

    NEA_StreamClose();
    if (wavFile != nullptr)
    {
        fclose(wavFile);
        wavFile = nullptr;
    }
    stream_buffer_in = 0;
    stream_buffer_out = 0;
    currentPath = nullptr;
    active = false;
}

void Music::ApplyVolume()
{
    if (!active)
        return;

    int32_t v = process.gamesettings.musicSoundVolume; // 0..1024
    if (v < 0)    v = 0;
    if (v > 1024) v = 1024;

    // maxmod stream volume is 0 (silent) .. 127 (normal).
    mmStreamVolume(static_cast<mm_byte>((v * 127) / 1024));
}
