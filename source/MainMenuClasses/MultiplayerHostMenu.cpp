#include "MultiplayerHostMenu.hpp"
#include "../NeaDelete.hpp"
#include "../AssetLoader.hpp"
#include "../GuiClickSound.hpp"
#include "../Net/NetLink.hpp"
#include "../Process.hpp"

namespace
{
    // How many frames to keep resending the start handshake so a client that
    // connected at the last moment still receives it before we leave the lobby.
    constexpr int kStartResendFrames = 30;

    // Skyjo supports up to 8 seats total (host + clients + CPUs).
    constexpr int kMaxSeats = 8;

    // How long to hold the waiting screen for Download Play guests that have
    // been sent the game. Twenty seconds is several times what a reboot and
    // re-association take, and falling through afterwards means one console that
    // failed to boot cannot strand the host on a screen with no selector.
    constexpr int kGuestWaitFrames = 20 * 60;

    // How long a quiet spell ends the wait once at least one guest has arrived.
    // The twenty seconds above is sized for a console that is still rebooting;
    // five seconds after the last arrival is far longer than the gap between two
    // consoles that both really booted, so anything still missing at that point
    // is a phantom -- most often station mode's guest high-water mark counting a
    // console that joined the room but never completed the transfer.
    constexpr int kGuestSettleFrames = 5 * 60;

    GameNetStart MakeStart(int playerCount, int seat,
                           const std::vector<std::string>& names)
    {
        GameNetStart s;
        s.playerCount = static_cast<uint8_t>(playerCount);
        s.seatIndex = static_cast<uint8_t>(seat);
        s.names = names;
        return s;
    }
}

void MultiplayerHostMenu::LoadAssetsMultiplayerHostMenu()
{
    // Every button texture is read in the background; Wait() at the end of
    // this function drains the batch. The menu calls this at the fade apex,
    // so the wait is hidden by the fade.
    AsyncAssetBatch assets;

    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

    this->StartMat[0] = NEA_MaterialCreate();
    this->StartMat[1] = NEA_MaterialCreate();
    this->StartPal[0] = NEA_PaletteCreate();
    this->StartPal[1] = NEA_PaletteCreate();

    this->EmptyMat = NEA_MaterialCreate();
    this->EmptyPal = NEA_PaletteCreate();

    this->PrevMat[0] = NEA_MaterialCreate();
    this->PrevMat[1] = NEA_MaterialCreate();
    this->PrevPal[0] = NEA_PaletteCreate();
    this->PrevPal[1] = NEA_PaletteCreate();

    this->NextMat[0] = NEA_MaterialCreate();
    this->NextMat[1] = NEA_MaterialCreate();
    this->NextPal[0] = NEA_PaletteCreate();
    this->NextPal[1] = NEA_PaletteCreate();

    assets.QueueTexGRF(this->BackMat[0], this->BackPal[0],
                       "mainmenu/btns/BackButton_png.grf");
    assets.QueueTexGRF(this->BackMat[1], this->BackPal[1],
                       "mainmenu/btns/BackButtonPressed_png.grf");

    assets.QueueTexGRF(this->StartMat[0], this->StartPal[0],
                       "mainmenu/btns/StartGameButton_png.grf");
    assets.QueueTexGRF(this->StartMat[1], this->StartPal[1],
                       "mainmenu/btns/StartGameButtonPressed_png.grf");

    assets.QueueTexGRF(this->EmptyMat, this->EmptyPal,
                       "mainmenu/btns/EmptyPlayerNumberButton_png.grf");

    assets.QueueTexGRF(this->PrevMat[0], this->PrevPal[0],
                       "mainmenu/btns/PrevPlayerButton_png.grf");
    assets.QueueTexGRF(this->PrevMat[1], this->PrevPal[1],
                       "mainmenu/btns/PrevPlayerButtonPressed_png.grf");

    assets.QueueTexGRF(this->NextMat[0], this->NextPal[0],
                       "mainmenu/btns/NextPlayerButton_png.grf");
    assets.QueueTexGRF(this->NextMat[1], this->NextPal[1],
                       "mainmenu/btns/NextPlayerButtonPressed_png.grf");

    // Back exists in every phase: it is the host's only way out, including out
    // of the wait below. Everything else is created by CreateLobbyButtons().
    this->BackButton = NEA_GUIButtonCreate(5, 160, 5 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

    // Arriving from Download Play, the guests are still rebooting and
    // re-associating. Showing them a live CPU selector and a Start that silently
    // refuses (it needs a client) reads as a broken screen, so hold everything
    // back until they are actually here.
    this->expectedGuests = this->pendingExpectedGuests;
    this->pendingExpectedGuests = 0;
    this->waitFrames = 0;
    this->phase = (this->expectedGuests > 0) ? Phase::WaitingGuests : Phase::Lobby;

    this->StartButton = nullptr;
    this->PrevCpuCountButton = nullptr;
    this->NextCpuCountButton = nullptr;
    this->EmptyCpuCountButton = nullptr;
    this->PrevCpuLevelButton = nullptr;
    this->NextCpuLevelButton = nullptr;
    this->EmptyCpuLevelButton = nullptr;

    if (this->phase == Phase::Lobby)
        this->CreateLobbyButtons();

    this->startFrame = 0;
    this->playerCount = 0;
    this->humanCount = 0;
    this->cpuCount = 0;
    this->cpuLevel = CPULevel::Easy;
    this->names.clear();
    for (auto& n : this->clientNames) n.clear();
    this->settleFrames = 0;

    // Enter host mode and start beaconing so clients can find us.
    NetLink::StartHost();

    assets.Wait("Loading...");
}

// Start and the two CPU picker rows. Split out of LoadAssets because they are
// created either there (a cart-to-cart host, which has nothing to wait for) or
// later, when the last Download Play guest has rejoined.
//
// Creating them late is safe: NEA_GUIButtonCreate takes the first free slot and
// all these rectangles are disjoint from Back's, and by then the textures have
// certainly landed -- LoadAssets already drained the batch.
void MultiplayerHostMenu::CreateLobbyButtons()
{
    if (this->StartButton != nullptr)
        return;

    this->StartButton = NEA_GUIButtonCreate(190, 160, 190 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->StartButton,
                        this->StartMat[0], NEA_White, 31,
                        this->StartMat[1], NEA_White, 31);

    // CPU count row.
    this->PrevCpuCountButton = NEA_GUIButtonCreate(25, 70, 25 + 32, 70 + 32);
    NEA_GUIButtonConfig(this->PrevCpuCountButton,
                        this->PrevMat[0], NEA_White, 31,
                        this->PrevMat[1], NEA_White, 31);
    this->NextCpuCountButton = NEA_GUIButtonCreate(190, 70, 190 + 32, 70 + 32);
    NEA_GUIButtonConfig(this->NextCpuCountButton,
                        this->NextMat[0], NEA_White, 31,
                        this->NextMat[1], NEA_White, 31);
    this->EmptyCpuCountButton = NEA_GUIButtonCreate(60, 70, 60 + 128, 70 + 32);
    NEA_GUIButtonConfig(this->EmptyCpuCountButton,
                        this->EmptyMat, NEA_White, 31,
                        this->EmptyMat, NEA_White, 31);

    // CPU level row.
    this->PrevCpuLevelButton = NEA_GUIButtonCreate(25, 120, 25 + 32, 120 + 32);
    NEA_GUIButtonConfig(this->PrevCpuLevelButton,
                        this->PrevMat[0], NEA_White, 31,
                        this->PrevMat[1], NEA_White, 31);
    this->NextCpuLevelButton = NEA_GUIButtonCreate(190, 120, 190 + 32, 120 + 32);
    NEA_GUIButtonConfig(this->NextCpuLevelButton,
                        this->NextMat[0], NEA_White, 31,
                        this->NextMat[1], NEA_White, 31);
    this->EmptyCpuLevelButton = NEA_GUIButtonCreate(60, 120, 60 + 128, 120 + 32);
    NEA_GUIButtonConfig(this->EmptyCpuLevelButton,
                        this->EmptyMat, NEA_White, 31,
                        this->EmptyMat, NEA_White, 31);
}

void MultiplayerHostMenu::UnloadAssetsMultiplayerHostMenu()
{
    // Only NEA teardown here. The WiFi link is shut down explicitly on the Back
    // path; on the start path it must stay alive for the game.
    // Null-safe: leaving during the waiting phase means the selector and Start
    // were never created.
    DeleteGUI(this->BackButton);
    DeleteGUI(this->StartButton);
    DeleteGUI(this->PrevCpuCountButton);
    DeleteGUI(this->NextCpuCountButton);
    DeleteGUI(this->EmptyCpuCountButton);
    DeleteGUI(this->PrevCpuLevelButton);
    DeleteGUI(this->NextCpuLevelButton);
    DeleteGUI(this->EmptyCpuLevelButton);

    DeleteMaterial(this->BackMat[0]);
    DeleteMaterial(this->BackMat[1]);
    DeletePalette(this->BackPal[0]);
    DeletePalette(this->BackPal[1]);

    DeleteMaterial(this->StartMat[0]);
    DeleteMaterial(this->StartMat[1]);
    DeletePalette(this->StartPal[0]);
    DeletePalette(this->StartPal[1]);

    DeleteMaterial(this->EmptyMat);
    DeletePalette(this->EmptyPal);

    DeleteMaterial(this->PrevMat[0]);
    DeleteMaterial(this->PrevMat[1]);
    DeletePalette(this->PrevPal[0]);
    DeletePalette(this->PrevPal[1]);

    DeleteMaterial(this->NextMat[0]);
    DeleteMaterial(this->NextMat[1]);
    DeletePalette(this->NextPal[0]);
    DeletePalette(this->NextPal[1]);
}

std::optional<MainMenuStates> MultiplayerHostMenu::ProcessLogicMultiplayerHostMenu()
{
    // Keep the MP transfer cycle running every frame so queued host->client
    // frames (the start handshake) actually reach connected clients.
    NetLink::HostDriveCycle();

    if (this->phase == Phase::Starting)
    {
        // Keep broadcasting the handshake to every client for a short window.
        for (int aid = 1; aid < this->humanCount; ++aid)
            NetLink::HostSendStart(aid, MakeStart(this->playerCount, aid, this->names));

        if (++this->startFrame >= kStartResendFrames)
            return MainMenuStates::TransitionToHostGame;
        return std::nullopt;
    }

    // Collect the console names clients announce over their Hello frames. This
    // runs during the wait too -- a guest announces itself as soon as it
    // associates, so by the time the lobby opens the roster already has real
    // names in it. HostPollHello is one-shot, so a missed frame here is a name
    // lost for good.
    int guestsBefore = NetLink::HostNumClients();

    for (int aid = 1; aid <= NetLink::kMaxClients; ++aid)
    {
        GameNetHello hello;
        if (NetLink::HostPollHello(aid, hello))
            this->clientNames.at(aid) = hello.name;
    }

    if (this->phase == Phase::WaitingGuests)
    {
        // Back is the only control on screen, so it is the only one to test.
        if (GuiClicked(this->BackButton))
        {
            // Guests that just arrived over Download Play are already
            // associating with this beacon; tell them it is going rather than
            // leaving them to retry against a host that no longer exists.
            NetLink::HostAnnounceBye(NetByeReason::HostLeftLobby);
            NetLink::Shutdown();
            return MainMenuStates::MultiplayerFirstMenu;
        }

        int guests = NetLink::HostNumClients();
        if (guests > guestsBefore)
            this->settleFrames = 0;      // somebody just arrived
        else
            ++this->settleFrames;

        // Everyone is back; or they have stopped arriving and whatever is still
        // missing was never coming; or none of them ever showed. Any of the
        // three, the host gets its lobby rather than sitting here.
        if (guests >= this->expectedGuests ||
            (guests > 0 && this->settleFrames >= kGuestSettleFrames) ||
            ++this->waitFrames >= kGuestWaitFrames)
        {
            this->CreateLobbyButtons();
            this->phase = Phase::Lobby;
        }
        return std::nullopt;
    }

    int liveHumanCount = NetLink::HostNumClients() + 1;
    int maxCpu = kMaxSeats - liveHumanCount;
    if (maxCpu < 0) maxCpu = 0;
    if (this->cpuCount > maxCpu) this->cpuCount = maxCpu;

    if (GuiClicked(this->PrevCpuCountButton))
    {
        if (this->cpuCount > 0) this->cpuCount--;
    }
    if (GuiClicked(this->NextCpuCountButton))
    {
        if (this->cpuCount < maxCpu) this->cpuCount++;
    }
    if (GuiClicked(this->PrevCpuLevelButton))
    {
        if (this->cpuLevel != CPULevel::Easy)
            this->cpuLevel = static_cast<CPULevel>(static_cast<int>(this->cpuLevel) - 1);
    }
    if (GuiClicked(this->NextCpuLevelButton))
    {
        if (this->cpuLevel != CPULevel::Hard)
            this->cpuLevel = static_cast<CPULevel>(static_cast<int>(this->cpuLevel) + 1);
    }

    if (GuiClicked(this->BackButton))
    {
        // Anyone waiting in the lobby is told the game is off, so they drop
        // straight back to scanning instead of sitting on "waiting for start".
        NetLink::HostAnnounceBye(NetByeReason::HostLeftLobby);
        NetLink::Shutdown();
        return MainMenuStates::MultiplayerFirstMenu;
    }

    if (GuiClicked(this->StartButton))
    {
        int clients = NetLink::HostNumClients();
        if (clients < 1) return std::nullopt; // need at least one other player

        NetLink::LockLobby();
        this->humanCount = clients + 1;
        // Re-clamp CPUs against the locked human count.
        int maxCpuNow = kMaxSeats - this->humanCount;
        if (maxCpuNow < 0) maxCpuNow = 0;
        if (this->cpuCount > maxCpuNow) this->cpuCount = maxCpuNow;
        this->playerCount = this->humanCount + this->cpuCount;

        // Build the full roster: host, then clients (real names when known), then
        // CPUs (shuffled from the CPU name pool).
        this->names.clear();
        this->names.push_back(process.consoleUserName.empty()
                                  ? std::string("Host") : process.consoleUserName);
        for (int aid = 1; aid < this->humanCount; ++aid)
        {
            const std::string& cn = this->clientNames.at(aid);
            this->names.push_back(cn.empty() ? ("Player " + std::to_string(aid + 1)) : cn);
        }

        std::vector<std::string> cpuPool(CPUnames.begin(), CPUnames.end());
        std::mt19937 rngName{static_cast<std::mt19937::result_type>(time(nullptr))};
        std::shuffle(cpuPool.begin(), cpuPool.end(), rngName);
        for (int c = 0; c < this->cpuCount; ++c)
            this->names.push_back(cpuPool.at(c % static_cast<int>(cpuPool.size())));

        // Seat assignment assumes contiguous client AIDs 1..clients (true for a
        // fresh lobby). Each client is told its seat == its AID.
        for (int aid = 1; aid < this->humanCount; ++aid)
            NetLink::HostSendStart(aid, MakeStart(this->playerCount, aid, this->names));

        this->phase = Phase::Starting;
        this->startFrame = 0;
    }

    return std::nullopt;
}

void MultiplayerHostMenu::ActionMultiplayerHostMenu()
{
    NEA_GUIDraw();
    NEA_RichTextRender3D(0, "HOST GAME", 90, 6);

    if (this->phase == Phase::WaitingGuests)
    {
        NEA_RichTextRender3D(0, "Waiting for players...", 52, 60);

        std::string count = std::to_string(NetLink::HostNumClients()) + " / " +
                            std::to_string(this->expectedGuests);
        NEA_RichTextRender3D(0, count.c_str(), 110, 90);
        return;
    }

    if (this->phase == Phase::Lobby)
    {
        int humans = NetLink::HostNumClients() + 1;
        NEA_RichTextRender3D(0, ("Players: " + std::to_string(humans)).c_str(), 90, 30);

        NEA_RichTextRender3D(0, ("CPU players: " + std::to_string(this->cpuCount)).c_str(), 78, 80);

        const char* lvl = "Easy";
        int lx = 100;
        switch (this->cpuLevel)
        {
            case CPULevel::Easy:   lvl = "Easy";   lx = 100; break;
            case CPULevel::Medium: lvl = "Medium"; lx = 95;  break;
            case CPULevel::Hard:   lvl = "Hard";   lx = 100; break;
        }
        NEA_RichTextRender3D(0, "CPU Level", 87, 105);
        NEA_RichTextRender3D(0, lvl, lx, 130);

        NEA_RichTextRender3D(0, "Press START when ready", 52, 50);
    }
    else
    {
        NEA_RichTextRender3D(0, "Starting game...", 75, 80);
    }
}
