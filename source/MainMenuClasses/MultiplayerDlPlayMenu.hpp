#pragma once

#include "../globalHeader.hpp"
#include "MainMenuStates.hpp"

// Host side of DS Download Play: hands the child binary to guest consoles, then
// gets out of the way.
//
// Download Play is only the delivery mechanism. Its application channel carries
// 64 bytes to a guest and 7 bytes back, nowhere near a game snapshot, so once
// every guest holds the program this screen boots them all, ends the session,
// and hands over to MultiplayerHostMenu -- which starts the ordinary SKYJO host
// the guests then join by scanning, exactly like a player who owns the game.


class MultiplayerDlPlayMenu
{
    private:
        enum class Phase
        {
            Serving,  // announcing the binary, guests downloading it
            Closing,  // room locked by Start; waiting for the last download
            Booting,  // told the guests to start; waiting for them to leave
            Failed,   // no child ROM, or the session refused to start
        };

        NEA_Material * StartMat[2] = {};
        NEA_Palette * StartPal[2] = {};
        NEA_GUIObj * StartButton = nullptr;

        NEA_Material * BackMat[2] = {};
        NEA_Palette * BackPal[2] = {};
        NEA_GUIObj * BackButton = nullptr;

        Phase phase = Phase::Serving;
        int bootFrame = 0;
        int closeFrame = 0;

        // The admission window: guests that turn up close together are held so
        // they take the program on one pass of the stream instead of most of two.
        // Once released it stays released for the rest of the session.
        bool admitOpen = true;
        int  admitQuiet = 0;   // frames since the last guest turned up
        int  admitTotal = 0;   // frames since the first one did
        int  admitGuests = 0;  // guests counted at the previous check

        // The guest list as drawn: rebuilt a few times a second, drawn every
        // frame. Keeping the strings rather than the values because building
        // them is the part that costs.
        std::vector<std::string> rowLines;
        int rowFrame = 0;
        std::string lostLine;
        unsigned lostShown = 0;
        // Total download progress across the room at the last check, so the wait
        // above can tell a slow room from a stopped one. Negative until the room
        // closes, which is why the first check always counts as movement.
        int closeProgress = -1;
        const char *failReason = nullptr;

        // How many guests actually started the program. The host lobby waits for
        // exactly this many to find their way back, so counting the wrong ones
        // strands it: a guest that was in the room but not holding the whole
        // program is refused by DlPlayBootAll() and stays put, and counting it
        // leaves the lobby waiting out its full timeout for a console that was
        // never coming.
        //
        // So it is not the size of the room. bootGuestMask records who was asked
        // to start, and bootedGuests counts how many of those have since left --
        // leaving is the only evidence a console acted on it.
        int bootedGuests = 0;
        uint16_t bootGuestMask = 0;


        // The child ROM, held in RAM for the whole session: the library reads
        // blocks out of this buffer as guests ask for them instead of copying
        // it, so it must not move or be freed until the session ends.
        void *rom = nullptr;
        std::size_t romSize = 0;

        bool LoadChildRom();
        void EndSession();
        void BootGuests();

        // Let the held guests start downloading. Idempotent.
        void ReleaseAdmission();

    public:
        // Guests handed the program and told to start, for MultiplayerHostMenu
        // to wait on. Zero unless this screen actually got as far as booting.
        int GetBootedGuestCount() const { return this->bootedGuests; }

        void LoadAssetsMultiplayerDlPlayMenu();
        void UnloadAssetsMultiplayerDlPlayMenu();
        std::optional<MainMenuStates> ProcessLogicMultiplayerDlPlayMenu();
        void ActionMultiplayerDlPlayMenu();
};
