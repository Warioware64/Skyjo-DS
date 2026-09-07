#include "MultiplayerDlPlayMenu.hpp"
#include "../NeaDelete.hpp"
#include "../AssetLoader.hpp"
#include "../GuiClickSound.hpp"
#include "../Net/NetLink.hpp"
#include "../Process.hpp"   // process.fatDeviceCPP

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace
{
    // Built by child/build.py and dropped into the ROM by build.py.
    constexpr const char *kChildRomPath = "nitro:/dlplay/skyjo-child.nds";

    // Guests leave the session one at a time as they boot. If one never does,
    // stop waiting rather than hanging the menu: it just doesn't join.
    constexpr int kBootTimeoutFrames = 300;

    // How long the closed room will wait with *nothing moving* before it gives
    // up on the stragglers and boots whoever is ready. Start deliberately hides
    // both buttons, so without a cap a guest that stalls without dropping off
    // would leave the host on this screen with no way out but the power switch.
    //
    // Time without progress, not time. This used to be a flat minute from the
    // moment Start was pressed, which quietly assumed a room takes about as long
    // as one console does. It does not: guests share a single broadcast stream
    // whose cursor follows whichever of them is furthest along, so one that
    // joined late is served only once the ones ahead of it have finished, and
    // several consoles are closer to one transfer after another than to one
    // transfer for all of them. A room that was still downloading perfectly well
    // when the minute expired had its stragglers booted without the program --
    // which, on the console it was happening to, is a download that stopped part
    // way through for no reason.
    //
    // The library draws the same distinction for its own give-up counters, and
    // says so: they count time without progress rather than time spent.
    constexpr int kClosingStallFrames = 30 * 60;

    // The admission window. Guests are held at the point just before their
    // download starts until the room has been quiet for kAdmitQuietFrames, or
    // until kAdmitCapFrames have passed since the first one arrived.
    //
    // Guests share one stream whose cursor follows whichever of them is furthest
    // along, so a console that starts late rides that to the end and then wraps
    // round for what it missed: never stuck, but close to a second pass of the
    // whole program. Released together they need one pass between them. Four
    // quiet seconds is the difference between two people pressing Download Play
    // "at the same time" and the room paying twice for it.
    //
    // The cap is there because the quiet timer is reset by every arrival, and a
    // room people keep trickling into would otherwise never start.
    constexpr int kAdmitQuietFrames = 4 * 60;
    constexpr int kAdmitCapFrames = 12 * 60;

    // Seats Skyjo supports in total, host included. Matches kMaxSeats in
    // MultiplayerHostMenu.
    // Frames between rebuilds of the guest rows. Eight is about seven updates a
    // second -- faster than anyone reads a percentage, and eight times less work
    // than doing it every frame.
    constexpr int kRowRefreshFrames = 8;

    constexpr int kMaxSeats = 8;

}

bool MultiplayerDlPlayMenu::LoadChildRom()
{
    std::FILE *f = std::fopen(kChildRomPath, "rb");
    if (f == nullptr)
    {
        this->failReason = "Child binary missing";
        return false;
    }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (size <= 0)
    {
        std::fclose(f);
        this->failReason = "Child binary is empty";
        return false;
    }

    void *buffer = std::malloc(static_cast<std::size_t>(size));
    if (buffer == nullptr)
    {
        std::fclose(f);

        // This is the biggest single allocation the game ever makes, so if it
        // is going to fail it will fail here. Say by how much rather than just
        // "no": headroom well above the request means the heap is fragmented
        // rather than full, which is a different problem with a different fix.
        static char reason[64];
        std::snprintf(reason, sizeof(reason), "Need %ldK, %dK free",
                      size / 1024,
                      static_cast<int>((getHeapLimit() - getHeapEnd()) / 1024));
        this->failReason = reason;
        return false;
    }

    std::size_t read = std::fread(buffer, 1, static_cast<std::size_t>(size), f);
    std::fclose(f);

    if (read != static_cast<std::size_t>(size))
    {
        std::free(buffer);
        this->failReason = "Child binary unreadable";
        return false;
    }

    this->rom = buffer;
    this->romSize = read;
    return true;
}

void MultiplayerDlPlayMenu::BootGuests()
{
    // Remember exactly who was asked, while they are all still here:
    // DlPlayBootAll() is what makes them leave, and by the next frame the room
    // is already draining.
    //
    // Only guests holding the whole program are asked -- the library refuses
    // the rest -- so this is the set that can be expected to go, and the
    // Booting phase counts how many of them actually do.
    this->bootGuestMask = 0;
    for (int aid = 1; aid <= NetLink::kMaxDlPlayGuests; ++aid)
        if (NetLink::DlPlayGuestState(aid) == NetLink::GuestPhase::Ready)
            this->bootGuestMask |= static_cast<uint16_t>(1u << aid);

    this->bootedGuests = 0;

    NetLink::DlPlayBootAll();
    this->phase = Phase::Booting;
    this->bootFrame = 0;
}

void MultiplayerDlPlayMenu::ReleaseAdmission()
{
    if (!this->admitOpen) return;

    this->admitOpen = false;
    NetLink::DlPlayHoldNewGuests(false);
}

void MultiplayerDlPlayMenu::EndSession()
{
    // Stop the wireless first: the library reads the client buffer right up
    // until this returns, so nothing it touches may be freed before it.
    NetLink::DlPlayStop();

    if (this->rom != nullptr)
    {
        std::free(this->rom);
        this->rom = nullptr;
        this->romSize = 0;
    }
}

void MultiplayerDlPlayMenu::LoadAssetsMultiplayerDlPlayMenu()
{
    AsyncAssetBatch assets;

    this->StartMat[0] = NEA_MaterialCreate();
    this->StartMat[1] = NEA_MaterialCreate();
    this->StartPal[0] = NEA_PaletteCreate();
    this->StartPal[1] = NEA_PaletteCreate();

    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

    assets.QueueTexGRF(this->StartMat[0], this->StartPal[0],
                       "mainmenu/btns/StartGameButton_png.grf");
    assets.QueueTexGRF(this->StartMat[1], this->StartPal[1],
                       "mainmenu/btns/StartGameButtonPressed_png.grf");
    assets.QueueTexGRF(this->BackMat[0], this->BackPal[0],
                       "mainmenu/btns/BackButton_png.grf");
    assets.QueueTexGRF(this->BackMat[1], this->BackPal[1],
                       "mainmenu/btns/BackButtonPressed_png.grf");

    // Bottom corners, same convention as MultiplayerHostMenu. Both textures are
    // 64x32, so these are their native size; and the two rectangles are disjoint,
    // which the previous 128-wide Start at x=60 was not -- it overlapped Back
    // between x=60 and x=69 and a touch there fired both.
    this->StartButton = NEA_GUIButtonCreate(190, 160, 190 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->StartButton,
                        this->StartMat[0], NEA_White, 31,
                        this->StartMat[1], NEA_White, 31);

    this->BackButton = NEA_GUIButtonCreate(5, 160, 5 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

    this->phase = Phase::Serving;
    this->bootFrame = 0;
    this->closeFrame = 0;
    this->closeProgress = -1;
    this->admitOpen = true;
    this->admitQuiet = 0;
    this->admitTotal = 0;
    this->admitGuests = 0;
    this->rowLines.clear();
    this->rowFrame = kRowRefreshFrames;   // build the list on the first draw
    this->lostLine.clear();
    this->lostShown = 0;
    this->bootedGuests = 0;
    this->bootGuestMask = 0;
    this->failReason = nullptr;

    // Everything that touches the cartridge happens before Download Play
    // starts, and in this order on purpose.
    //
    // First drain the button textures. dswifi's own documentation is blunt
    // about it -- "a card read blocks for long enough to disturb the wireless
    // frame timing" -- and once Wifi_DlPlayStart() is beaconing, the four
    // little GRF reads below were competing with it for the ARM7. Sometimes
    // they finished, sometimes they stalled past the batch's timeout and took
    // the game down with them, which is what made this screen crash at random.
    //
    // It also means the async worker is idle before the 442 KB read below, so
    // that allocation gets a quiet heap and the two are never reading the
    // cartridge at the same time.
    assets.Wait("Loading...");

    // The embedded child. The buffer stays alive until EndSession(); the
    // library reads blocks straight out of it rather than taking a copy.
    if (!this->LoadChildRom())
    {
        this->phase = Phase::Failed;
        return;
    }

    // And only now, with no filesystem work left to do, take over the wireless.
    if (!NetLink::DlPlayStart(this->rom, this->romSize, kMaxSeats))
    {
        // Either wifi failed to come up, or the ROM overruns what the protocol
        // can carry. child/check_size.py catches the latter at build time.
        // DlPlayStart() brings wifi up before it can fail, and EndSession()
        // does nothing about that because no session ever started -- so without
        // this the screen sat in Failed with the radio on until the player
        // happened to press Back.
        this->EndSession();
        NetLink::Shutdown();
        this->failReason = "Cannot start Download Play";
        this->phase = Phase::Failed;
    }
}

void MultiplayerDlPlayMenu::UnloadAssetsMultiplayerDlPlayMenu()
{
    // The session is ended on the Back and Start paths already; doing it again
    // here costs nothing (DlPlayStop() returns at once when nothing is running,
    // and the ROM pointer is null-checked) and makes it impossible for a new
    // exit to leak the 442 KB child buffer or leave the room on the air. The
    // fade-apex unload in MainMenu is the last thing that runs for this screen,
    // whichever way it was left.
    this->EndSession();

    // Both buttons are usually gone already -- Start removes them when it closes
    // the room -- so these go through the null-safe helper.
    DeleteGUI(this->StartButton);
    DeleteGUI(this->BackButton);

    DeleteMaterial(this->StartMat[0]);
    DeleteMaterial(this->StartMat[1]);
    DeletePalette(this->StartPal[0]);
    DeletePalette(this->StartPal[1]);

    DeleteMaterial(this->BackMat[0]);
    DeleteMaterial(this->BackMat[1]);
    DeletePalette(this->BackPal[0]);
    DeletePalette(this->BackPal[1]);
}

std::optional<MainMenuStates> MultiplayerDlPlayMenu::ProcessLogicMultiplayerDlPlayMenu()
{
    // Sends the frame each guest is waiting for. Skipping it stalls every
    // transfer in progress.
    NetLink::DlPlayUpdate();

    // What the room says about itself to a console that is only looking at it.
    // Guarded inside NetLink against being sent when nothing changed, because
    // every real change restarts the beacon's fragment cycle.
    //
    // Failed maps to Open only because a screen that never got a session running
    // is not announcing anything at all: DlPlaySetRoomInfo() returns straight
    // away when there is no room.
    DlPlayRoomPhase roomPhase = DlPlayRoomPhase::Open;
    if (this->phase == Phase::Closing) roomPhase = DlPlayRoomPhase::Closing;
    else if (this->phase == Phase::Booting) roomPhase = DlPlayRoomPhase::Booting;

    NetLink::DlPlaySetRoomInfo(NetLink::DlPlayNumGuests(), kMaxSeats, roomPhase);

    if (GuiClicked(this->BackButton))
    {
        this->EndSession();
        NetLink::Shutdown();
        return MainMenuStates::MultiplayerFirstMenu;
    }

    if (this->phase == Phase::Failed)
        return std::nullopt;

    if (this->phase == Phase::Closing)
    {
        // The room is shut to newcomers and we are only waiting for whoever is
        // still downloading. Booting the moment the last one is ready is what
        // lets the host press Start once, at any time, instead of having to
        // catch the instant everybody happens to be at 100%.
        if (NetLink::DlPlayNumGuests() == 0)
        {
            // Everyone left while we waited; there is no game to start.
            this->EndSession();
            NetLink::Shutdown();
            return MainMenuStates::MultiplayerFirstMenu;
        }

        // Anything moving anywhere in the room resets the patience below. The
        // sum is over percentages rather than blocks because it only has to
        // change, and 1% of the child is about 18 blocks -- a tenth of a second
        // of a healthy transfer, so a room that is getting anywhere at all keeps
        // this alive comfortably.
        int progress = 0;
        for (int aid = 1; aid <= NetLink::kMaxDlPlayGuests; ++aid)
        {
            if (NetLink::DlPlayGuestPresent(aid))
                progress += NetLink::DlPlayGuestPercent(aid);
        }

        if (progress != this->closeProgress)
        {
            this->closeProgress = progress;
            this->closeFrame = 0;
        }

        if (NetLink::DlPlayAllGuestsReady() ||
            ++this->closeFrame >= kClosingStallFrames)
        {
            // On the timeout path DlPlayBootAll() still only starts the guests
            // that hold the whole program -- Wifi_DlPlayBootClient() refuses any
            // other stage -- so a stalled guest is left behind rather than sent
            // a program it hasn't finished receiving.
            this->BootGuests();
        }
        return std::nullopt;
    }

    if (this->phase == Phase::Booting)
    {
        // A guest that has left the room is a guest that started the program.
        // Counting departures rather than the size of the room is what keeps a
        // console that never went from being waited for in the lobby.
        uint16_t stillHere = NetLink::DlPlayGuestMask();
        uint16_t gone = static_cast<uint16_t>(this->bootGuestMask & ~stillHere);
        if (gone != 0)
        {
            for (int aid = 1; aid <= NetLink::kMaxDlPlayGuests; ++aid)
                if (gone & (1u << aid)) ++this->bootedGuests;
            this->bootGuestMask =
                static_cast<uint16_t>(this->bootGuestMask & stillHere);
        }

        // Guests drop out of the session as they start the program. Once they
        // have all gone (or one has stopped responding) the wireless is ours
        // again and the ordinary host lobby can take over.
        if (NetLink::DlPlayNumGuests() == 0 ||
            ++this->bootFrame >= kBootTimeoutFrames)
        {
            this->EndSession();

            // Start beaconing right now rather than waiting for the lobby
            // screen to do it after the menu fade. The guests are booting and
            // looking for us at this exact moment, so a second of dead air here
            // is a second every one of them spends failing to find the host.
            // MultiplayerHostMenu's own StartHost() is then a no-op.
            NetLink::StartHost();
            return MainMenuStates::MultiplayerHostMenu;
        }
        return std::nullopt;
    }

    // Serving. Hold new arrivals just long enough for the room to settle, then
    // let the whole batch download together.
    if (this->admitOpen)
    {
        const int guests = NetLink::DlPlayNumGuests();

        // Somebody new turned up: wait a little longer for whoever is next.
        if (guests > this->admitGuests)
            this->admitQuiet = 0;

        this->admitGuests = guests;

        // The clock only runs once there is somebody to wait for. An empty room
        // holds the window open indefinitely, which costs nothing and means the
        // first guest to arrive still gets the full quiet spell.
        if (guests > 0)
        {
            ++this->admitQuiet;
            ++this->admitTotal;

            if ((this->admitQuiet >= kAdmitQuietFrames) ||
                (this->admitTotal >= kAdmitCapFrames) ||
                (guests >= NetLink::kMaxDlPlayGuests))
            {
                this->ReleaseAdmission();
            }
        }
    }

    if (GuiClicked(this->StartButton))
    {
        // Needs somebody to start with. Start closes the room and releases the
        // gathered guests; the Closing phase above boots as soon as the last
        // download lands.
        if (NetLink::DlPlayNumGuests() == 0)
            return std::nullopt;

        // The host is done waiting, so anyone still held goes now rather than
        // sitting through a window that has been overtaken by events.
        this->ReleaseAdmission();

        // Start is otherwise only about shutting the room: no more consoles, and
        // boot whoever is holding the whole program once the stragglers land.
        NetLink::DlPlayLockRoom(true);
        this->phase = Phase::Closing;
        this->closeFrame = 0;
        this->closeProgress = -1;

        // Take both buttons away for the rest of the session. From here on every
        // touch on this screen is a mistake: Start has already done its work, and
        // Back would throw away downloads that are nearly finished. Deleting is
        // the only way to do this -- NEA has no visible or enabled flag, and an
        // undrawn button is still hit-tested every frame.
        DeleteGUI(this->StartButton);
        DeleteGUI(this->BackButton);
    }

    return std::nullopt;
}

void MultiplayerDlPlayMenu::ActionMultiplayerDlPlayMenu()
{
    NEA_GUIDraw();
    NEA_RichTextRender3D(0, "DOWNLOAD PLAY", 66, 6);

    if (this->phase == Phase::Failed)
    {
        NEA_RichTextRender3D(0, this->failReason != nullptr
                                    ? this->failReason : "Download Play failed",
                             40, 80);
        return;
    }

    if (this->phase == Phase::Booting)
    {
        NEA_RichTextRender3D(0, "Starting guests...", 70, 80);
        return;
    }

    // Status line at the top, so the guest list below always has the same room
    // no matter what it says.
    if (this->phase == Phase::Closing)
    {
        NEA_RichTextRender3D(0, "Room closed - starting when", 26, 26);
        NEA_RichTextRender3D(0, "everyone is ready", 68, 42);
    }
    else if (NetLink::DlPlayNumGuests() > 0)
    {
        NEA_RichTextRender3D(0, "Press START when ready", 46, 30);
    }
    else
    {
        NEA_RichTextRender3D(0, "On each DS: choose Download Play", 16, 30);
    }

    // The guest rows are rebuilt a few times a second and drawn from the cache
    // every frame.
    //
    // Drawing every frame is not negotiable -- these are 3D-rendered strings, so
    // a frame that skips one is a frame it is missing from -- but *building*
    // them every frame is pure waste: a std::string per guest, each with its own
    // allocation, sixty times a second for a percentage that changes far more
    // slowly. That is ARM9 time taken from the transfer, and the host being busy
    // is what makes it miss an exchange; every missed exchange is a block a
    // guest has to pick up on a later pass. dswifi's own example throttles its
    // status display for the same reason and says so.
    if (++this->rowFrame >= kRowRefreshFrames)
    {
        this->rowFrame = 0;
        this->rowLines.clear();

        for (int aid = 1; aid <= NetLink::kMaxDlPlayGuests; ++aid)
        {
            if (!NetLink::DlPlayGuestPresent(aid)) continue;

            const char *name = NetLink::DlPlayGuestName(aid);
            std::string line = (name != nullptr && name[0] != '\0')
                                   ? std::string(name)
                                   : ("Player " + std::to_string(aid + 1));
            line += "  ";

            // What this guest is actually doing. "waiting" covers the handshake
            // and the few seconds of the admission window; after that it is a
            // percentage that climbs for the rest of the time the guest is on
            // screen. Guests that fell out of step with each other climb at
            // different rates -- the number is honest either way.
            switch (NetLink::DlPlayGuestState(aid))
            {
                case NetLink::GuestPhase::Ready:
                    line += "ready";
                    break;
                case NetLink::GuestPhase::Sending:
                    line += std::to_string(NetLink::DlPlayGuestPercent(aid)) + "%";
                    break;
                case NetLink::GuestPhase::Verifying:
                    line += "checking";
                    break;
                case NetLink::GuestPhase::Failed:
                    line += "failed";
                    break;
                default:
                    line += "waiting";
                    break;
            }

            this->rowLines.push_back(std::move(line));
        }
    }

    // A full room is 7 guests. At 14 px a row starting from y=58 that ends at
    // y=142, clear of the buttons along the bottom edge.
    int y = 58;
    const int guests = static_cast<int>(this->rowLines.size());

    for (const std::string& line : this->rowLines)
    {
        NEA_RichTextRender3D(0, line.c_str(), 30, y);
        y += 14;
    }

    if (guests == 0)
        NEA_RichTextRender3D(0, "Waiting for players...", 52, 76);

    // Guests the library stopped waiting for. Saying so is the difference
    // between a console whose owner walked away and one this host dropped --
    // which, unreported, look identical and leave the player with a room that
    // silently got smaller.
    const unsigned lost = NetLink::DlPlayGaveUpCount();
    if (lost > 0)
    {
        // Built only when the count moves. It rarely does, and once it has this
        // would otherwise be another allocation every frame for the rest of the
        // session.
        if (lost != this->lostShown)
        {
            this->lostShown = lost;
            this->lostLine = (lost == 1)
                ? std::string("1 player lost connection")
                : (std::to_string(lost) + " players lost connection");
        }

        NEA_RichTextRender3D(0, this->lostLine.c_str(), 40, 146);
    }
}
