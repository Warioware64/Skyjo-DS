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
    inline constexpr const char *GamePartyTrack = "nitro:/music/gameMusic.wav";

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
}

#endif // MUSIC_HPP
