#ifndef MUSIC_HPP
#define MUSIC_HPP

// Streaming background music, played through Nitro Engine Advanced's maxmod
// WAV-streaming wrapper. Maxmod supports a single stream at a time, and the
// menu/game tracks never overlap, so this module drives one looping stream and
// switches tracks on demand. All the maxmod state lives in Music.cpp, so
// callers only need these plain functions (no maxmod types leak out).
namespace Music
{
    inline constexpr const char *MainMenuTrack  = "nitro:/music/mainMenu.wav";

#ifdef SKYJO_CLIENT_ONLY
    // The Download Play guest has no NitroFS, and a "nitro:" path would not
    // merely miss -- nitrofs_isdrive() claims that drive name unconditionally,
    // so the open is routed to a filesystem that was never initialised and
    // fails with ENODEV. A relative path resolves through AssetDevice instead,
    // which is the current drive there; the soundbank is reached the same way.
    //
    // The track itself is different too: 8000 Hz mono IMA-ADPCM rather than
    // 11025 Hz stereo PCM, because on a guest it travels inside the binary at
    // ~90 KB/s instead of being streamed off a card. See child/encode_music.py.
    inline constexpr const char *GamePartyTrack = "music/gameMusic.ima";
#else
    inline constexpr const char *GamePartyTrack = "nitro:/music/gameMusic.wav";
#endif

    // Initialize maxmod once for the whole app. Idempotent. Must run after
    // nitroFSInit() and NEA_Init3D() (i.e. from Process::ProcessInit()).
    void InitOnce();

    // Open `path` and start the looping stream, applying the current music
    // volume. No-op if that same track is already playing; switches tracks if a
    // different one is; leaves music off (no abort) if the file can't be
    // opened/parsed.
    void Play(const char *path);

    // Refill the streaming circular buffer. Call once per frame from whatever
    // render loop is on screen while music is playing.
    void Pump();

    // Stop and close the stream. No-op if not playing.
    void Stop();

    // Re-apply process.gamesettings.musicSoundVolume (0-1024) to the running
    // stream, mapped to maxmod's 0-127 range. Safe to call anytime.
    void ApplyVolume();

    // --- One-shot sound effects (in-game) ---------------------------------
    // Loaded once by InitOnce() from the maxmod soundbank. Each Play is a
    // no-op if the soundbank failed to load. Safe to call from game logic.
    void SfxPoseCard();     // a card was placed into the grid
    void SfxTakeCard();     // a card was taken from a pile, or revealed
    void SfxClearColumn();  // a full matching column was cleared
    void SfxClick();        // a UI button was clicked

    // Re-apply process.gamesettings.nosesSoundVolume (0-1024) as the maxmod
    // global effects master volume (0-1024, 1:1). Safe to call anytime.
    void ApplySfxVolume();
}

#endif // MUSIC_HPP
