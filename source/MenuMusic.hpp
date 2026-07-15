#ifndef MENUMUSIC_HPP
#define MENUMUSIC_HPP

// Streaming background music for the main menu (mainMenu.wav from NitroFS),
// played through Nitro Engine Advanced's maxmod WAV-streaming wrapper. All the
// maxmod state lives in MenuMusic.cpp, so callers only need these plain
// functions (no maxmod types leak into their headers).
namespace MenuMusic
{
    // Initialize maxmod once for the whole app. Idempotent. Must run after
    // nitroFSInit() and NEA_Init3D() (i.e. from Process::ProcessInit()).
    void InitOnce();

    // Open nitro:/music/mainMenu.wav and start the looping stream, applying the
    // current music-volume setting. No-op if already playing; leaves music off
    // (no abort) if the file can't be opened/parsed.
    void Start();

    // Refill the streaming circular buffer. Call once per frame from the menu
    // loop while the menu is on screen.
    void Pump();

    // Stop and close the stream. No-op if not playing.
    void Stop();

    // Re-apply process.gamesettings.musicSoundVolume (0-1024) to the running
    // stream, mapped to maxmod's 0-127 range. Safe to call anytime.
    void ApplyVolume();
}

#endif // MENUMUSIC_HPP
