#include "Music.hpp"
#include "globalHeader.hpp"   // <NEAMain.h> -> NEASound.h -> maxmod9.h, plus <nds.h>/<filesystem.h>
#include "Process.hpp"        // global `process` (gamesettings.music/nosesSoundVolume)
#include "nitrofs/soundbank_info.h" // generated SFX_POSECARD / SFX_TAKECARD / SFX_CLEARCOLUMN

#include <cstring>            // memcpy, strcmp
#include <cstdio>             // FILE*, fopen/fread/fseek/feof/fclose

// WAV streaming ported from the Nitro Engine Advanced example
// (examples/sound/streaming/source/main.c). The menu and game tracks are both
// 16-bit stereo 11025 Hz PCM with a canonical 44-byte header, so the PCM data
// begins at sizeof(WAVHeader) and looping is just a seek back to that offset.
//
// The streaming callback runs inside a timer interrupt (MM_TIMER0), so it must
// only copy from the circular buffer and never touch the filesystem. The file
// reads happen from Music::Pump(), driven once per frame by the active render
// loop. Only one stream exists at a time; switching tracks stops the old one.
//
// Two source formats feed that one buffer, chosen by the magic word at the
// start of the file. A cartridge plays the PCM WAVs directly. A Download Play
// guest cannot: it has no filesystem, so its music is linked into the binary
// and every byte is airtime, and it gets an 8000 Hz mono IMA-ADPCM track
// instead -- a quarter of the bytes of the mono PCM it decodes to, and decoded
// here on the main thread rather than costing a heap buffer at fopen(). The
// container and the reasoning are in child/encode_music.py.

namespace
{
    // Where the volume settings come from. The Download Play guest has no
    // settings file and no Process object -- it is a stripped client build --
    // so it just uses the defaults; everything else reads what the player saved.
    const GameSettings &Settings()
    {
#ifdef SKYJO_CLIENT_ONLY
        static const GameSettings kGuestDefaults{ 1024, 1024 };
        return kGuestDefaults;
#else
        return process.gamesettings;
#endif
    }

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

    // Both the loop point and the data-chunk end are computed from this, and
    // the tracks are canonical 44-byte-header WAVs.
    static_assert(sizeof(WAVHeader) == 44, "WAV header must be 44 bytes");

    // The IMA-ADPCM container written by child/encode_music.py: a 16-byte
    // header, then two 4-bit samples per byte, low nibble first. There are no
    // per-block state headers -- decoding is bit-exact against the encoder and
    // the only seek ever made is back to sample 0 at the loop point.
    constexpr uint32_t ADPCM_ID = 0x50444153; // "SADP"

    struct AdpcmHeader
    {
        uint32_t magic;
        uint16_t version;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t sampleCount;
    };

    enum class SourceFormat { Wav, Adpcm };

    constexpr int BUFFER_LENGTH = 16384;

    FILE       *trackFile = nullptr;
    const char *currentPath = nullptr;
    char        stream_buffer[BUFFER_LENGTH];
    int         stream_buffer_in = 0;
    int         stream_buffer_out = 0;
    bool        active = false;
    bool        inited = false;
    bool        sfxAvailable = false; // true once the soundbank + effects loaded

    SourceFormat sourceFormat = SourceFormat::Wav;

    // End of the WAV's `data` chunk. Both tracks carry LIST/id3 trailers after
    // it, so looping on end-of-*file* would play a few hundred bytes of tag
    // metadata as PCM at every loop point -- a short burst of noise every two
    // minutes.
    long wavDataEnd = 0;

    // ADPCM decoder state, carried across refills and reset at the loop point.
    int32_t  adpcmPredictor = 0;
    int      adpcmIndex = 0;
    uint32_t adpcmSampleCount = 0;
    uint32_t adpcmLeft = 0;
    int16_t  adpcmStash = 0;    // second sample of a byte, when one was spare
    bool     adpcmStashed = false;


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

    // Read `size` bytes of PCM into `buffer`, looping back to the start of the
    // `data` chunk when its end is reached.
    void readWav(char *buffer, size_t size)
    {
        while (size > 0)
        {
            long left = wavDataEnd - ftell(trackFile);
            if (left <= 0)
            {
                fseek(trackFile, sizeof(WAVHeader), SEEK_SET);
                left = wavDataEnd - (long)sizeof(WAVHeader);
            }

            size_t want = size;
            if (want > (size_t)left)
                want = (size_t)left;

            size_t res = fread(buffer, 1, want, trackFile);
            if (res == 0)
            {
                // Unreadable, not merely finished. Hand the callback silence
                // rather than spinning here, and start the next refill from the
                // top in case the read recovers.
                memset(buffer, 0, size);
                fseek(trackFile, sizeof(WAVHeader), SEEK_SET);
                return;
            }

            size -= res;
            buffer += res;
        }
    }

    // --- IMA-ADPCM ---------------------------------------------------------

    const int8_t adpcmIndexTable[16] = {
        -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8
    };

    const int16_t adpcmStepTable[89] = {
            7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
           19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
           50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
          130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
          337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
          876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
         2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
         5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
    };

    void adpcmRewind()
    {
        fseek(trackFile, sizeof(AdpcmHeader), SEEK_SET);
        adpcmPredictor = 0;
        adpcmIndex = 0;
        adpcmLeft = adpcmSampleCount;
        adpcmStashed = false;
    }

    int16_t adpcmDecode(uint8_t code)
    {
        int32_t step = adpcmStepTable[adpcmIndex];

        int32_t diff = step >> 3;
        if (code & 4) diff += step;
        if (code & 2) diff += step >> 1;
        if (code & 1) diff += step >> 2;

        adpcmPredictor += (code & 8) ? -diff : diff;
        if (adpcmPredictor < -32768)     adpcmPredictor = -32768;
        else if (adpcmPredictor > 32767) adpcmPredictor = 32767;

        adpcmIndex += adpcmIndexTable[code];
        if (adpcmIndex < 0)       adpcmIndex = 0;
        else if (adpcmIndex > 88) adpcmIndex = 88;

        return (int16_t)adpcmPredictor;
    }

    // Decode `size` bytes' worth of 16-bit mono samples. `size` is always even:
    // the callback consumes whole samples and BUFFER_LENGTH is a multiple of
    // one, so every refill asks for a whole number of them. The sample *count*
    // can still be odd, hence the one-sample stash -- a byte always yields two.
    void readAdpcm(char *buffer, size_t size)
    {
        size_t samples = size / 2;
        int16_t *out = (int16_t *)buffer;

        while (samples > 0)
        {
            if (adpcmStashed)
            {
                *out++ = adpcmStash;
                adpcmStashed = false;
                --samples;
                continue;
            }

            if (adpcmLeft == 0)
                adpcmRewind();

            uint8_t packed[256];
            size_t want = (samples + 1) / 2;
            if (want > sizeof(packed))
                want = sizeof(packed);
            if (want > (size_t)((adpcmLeft + 1) / 2))
                want = (size_t)((adpcmLeft + 1) / 2);

            size_t res = fread(packed, 1, want, trackFile);
            if (res == 0)
            {
                // Unreadable, not merely finished. Hand the callback silence
                // rather than spinning here, and start the next refill from the
                // top in case the read recovers.
                memset(out, 0, samples * 2);
                adpcmRewind();
                return;
            }

            for (size_t i = 0; i < res && samples > 0 && adpcmLeft > 0; ++i)
            {
                *out++ = adpcmDecode(packed[i] & 0x0F);
                --samples;
                --adpcmLeft;

                // An odd total leaves the last byte's high nibble as padding.
                if (adpcmLeft == 0)
                    break;

                int16_t second = adpcmDecode(packed[i] >> 4);
                --adpcmLeft;

                if (samples > 0)
                {
                    *out++ = second;
                    --samples;
                }
                else
                {
                    // Read a byte to get an odd sample; keep the spare rather
                    // than seeking back half a byte.
                    adpcmStash = second;
                    adpcmStashed = true;
                }
            }
        }
    }

    void producePcm(char *buffer, size_t size)
    {
        if (sourceFormat == SourceFormat::Adpcm)
            readAdpcm(buffer, size);
        else
            readWav(buffer, size);
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
            producePcm(&stream_buffer[stream_buffer_in], size);
            stream_buffer_in += size;
        }
        else
        {
            size_t size = BUFFER_LENGTH - stream_buffer_in;
            producePcm(&stream_buffer[stream_buffer_in], size);
            stream_buffer_in = 0;

            size = stream_buffer_out - stream_buffer_in;
            producePcm(&stream_buffer[stream_buffer_in], size);
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

    // Initialize maxmod once, WITH the soundbank, so the single instance drives
    // BOTH the streaming music (NEA_StreamOpen) and the one-shot effects
    // (mmEffect). NEA_SoundSystemResetFAT does soundEnable() + mmInitDefault() +
    // NEA pool alloc in one call and returns 0 on success.
    // Relative on purpose: it resolves through NitroFS here (nitroFSInit claims
    // the current drive, nitrofs_device.c:1025) and through the linked asset
    // filesystem in the Download Play child, which has no NitroFS at all.
    if (NEA_SoundSystemResetFAT("maxmod/soundbank.bin", 1) == 0)
    {
        // Preload the effects so the first play has no file hitch, and apply the
        // saved SFX volume (settings are loaded before ProcessInit calls us).
        NEA_SfxLoad(SFX_POSECARD);
        NEA_SfxLoad(SFX_TAKECARD);
        NEA_SfxLoad(SFX_CLEARCOLUMN);
        NEA_SfxLoad(SFX_CLICK);
        sfxAvailable = true;
        Music::ApplySfxVolume();
    }
    else
    {
        // Soundbank missing/unreadable: fall back to a soundbank-less init so at
        // least the streaming music keeps working (effects stay silent). Mirrors
        // the NEA streaming example; mmInit brings up the ARM7 sound hardware.
        mm_ds_system sys;
        sys.mod_count    = 0;
        sys.samp_count   = 0;
        sys.mem_bank     = nullptr;
        sys.fifo_channel = FIFO_MAXMOD;
        mmInit(&sys);
        NEA_SoundSystemResetPool(1);
    }

    // Power on the sound hardware AFTER maxmod init. Every BlocksDS maxmod
    // example calls soundEnable() *after* mmInitDefault; NEA_SoundSystemResetFAT
    // calls it *before*, which the soundbank init then leaves powered down — so
    // without this the stream and effects play into disabled output (silence).
    soundEnable();

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
    trackFile = fopen(path, "rb");
    if (trackFile == nullptr)
        return;

    // Which of the two source formats this is. The magic word decides: a WAV
    // opens with "RIFF", the guest's encoded track with "SADP".
    uint32_t magic = 0;
    if (fread(&magic, 1, sizeof(magic), trackFile) != sizeof(magic))
    {
        fclose(trackFile);
        trackFile = nullptr;
        return;
    }
    fseek(trackFile, 0, SEEK_SET);

    mm_word           rate;
    mm_stream_formats format;

    if (magic == ADPCM_ID)
    {
        AdpcmHeader header = {};
        if (fread(&header, 1, sizeof(header), trackFile) != sizeof(header) ||
            header.version != 1 || header.numChannels != 1 ||
            header.sampleCount == 0)
        {
            fclose(trackFile);
            trackFile = nullptr;
            return;
        }

        sourceFormat = SourceFormat::Adpcm;
        adpcmSampleCount = header.sampleCount;
        adpcmRewind();

        rate = header.sampleRate;
        format = MM_STREAM_16BIT_MONO;
    }
    else
    {
        WAVHeader header = {};
        if (fread(&header, 1, sizeof(header), trackFile) != sizeof(header) ||
            !checkWAVHeader(header))
        {
            fclose(trackFile);
            trackFile = nullptr;
            return;
        }

        sourceFormat = SourceFormat::Wav;
        wavDataEnd = (long)sizeof(WAVHeader) + (long)header.subchunk2Size;

        rate = header.sampleRate;
        format = getMMStreamType(header.numChannels, header.bitsPerSample);
    }

    stream_buffer_in = 0;
    stream_buffer_out = 0;
    streamingFillBuffer(true); // prime the circular buffer before opening

    NEA_StreamOpen(rate, 2048, streamingCallback, format, MM_TIMER0);
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
    if (trackFile != nullptr)
    {
        fclose(trackFile);
        trackFile = nullptr;
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

    int32_t v = Settings().musicSoundVolume; // 0..1024
    if (v < 0)    v = 0;
    if (v > 1024) v = 1024;

    // maxmod stream volume is 0 (silent) .. 127 (normal).
    mmStreamVolume(static_cast<mm_byte>((v * 127) / 1024));
}

// --- One-shot sound effects -----------------------------------------------

void Music::SfxPoseCard()
{
    if (sfxAvailable)
        NEA_SfxPlay(SFX_POSECARD);
}

void Music::SfxTakeCard()
{
    if (sfxAvailable)
        NEA_SfxPlay(SFX_TAKECARD);
}

void Music::SfxClearColumn()
{
    if (sfxAvailable)
        NEA_SfxPlay(SFX_CLEARCOLUMN);
}

void Music::SfxClick()
{
    if (sfxAvailable)
        NEA_SfxPlay(SFX_CLICK);
}

void Music::ApplySfxVolume()
{
    int32_t v = Settings().nosesSoundVolume; // 0..1024
    if (v < 0)    v = 0;
    if (v > 1024) v = 1024;

    // maxmod global effects master volume shares the 0..1024 scale — 1:1.
    mmSetEffectsVolume(static_cast<mm_word>(v));
}
