#include "MultiplayerHostMenu.hpp"
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

    NEA_MaterialTexLoadGRF(this->BackMat[0], this->BackPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButton_png.grf");
    NEA_MaterialTexLoadGRF(this->BackMat[1], this->BackPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->StartMat[0], this->StartPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButton_png.grf");
    NEA_MaterialTexLoadGRF(this->StartMat[1], this->StartPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->EmptyMat, this->EmptyPal, NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/EmptyPlayerNumberButton_png.grf");

    NEA_MaterialTexLoadGRF(this->PrevMat[0], this->PrevPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/PrevPlayerButton_png.grf");
    NEA_MaterialTexLoadGRF(this->PrevMat[1], this->PrevPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/PrevPlayerButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->NextMat[0], this->NextPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/NextPlayerButton_png.grf");
    NEA_MaterialTexLoadGRF(this->NextMat[1], this->NextPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/NextPlayerButtonPressed_png.grf");

    this->BackButton = NEA_GUIButtonCreate(5, 160, 5 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

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

    this->phase = Phase::Lobby;
    this->startFrame = 0;
    this->playerCount = 0;
    this->humanCount = 0;
    this->cpuCount = 0;
    this->cpuLevel = CPULevel::Easy;
    this->names.clear();
    for (auto& n : this->clientNames) n.clear();

    // Enter host mode and start beaconing so clients can find us.
    NetLink::StartHost();
}

void MultiplayerHostMenu::UnloadAssetsMultiplayerHostMenu()
{
    // Only NEA teardown here. The WiFi link is shut down explicitly on the Back
    // path; on the start path it must stay alive for the game.
    NEA_GUIDeleteObject(this->BackButton);
    NEA_GUIDeleteObject(this->StartButton);
    NEA_GUIDeleteObject(this->PrevCpuCountButton);
    NEA_GUIDeleteObject(this->NextCpuCountButton);
    NEA_GUIDeleteObject(this->EmptyCpuCountButton);
    NEA_GUIDeleteObject(this->PrevCpuLevelButton);
    NEA_GUIDeleteObject(this->NextCpuLevelButton);
    NEA_GUIDeleteObject(this->EmptyCpuLevelButton);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);
    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);

    NEA_MaterialDelete(this->StartMat[0]);
    NEA_MaterialDelete(this->StartMat[1]);
    NEA_PaletteDelete(this->StartPal[0]);
    NEA_PaletteDelete(this->StartPal[1]);

    NEA_MaterialDelete(this->EmptyMat);
    NEA_PaletteDelete(this->EmptyPal);

    NEA_MaterialDelete(this->PrevMat[0]);
    NEA_MaterialDelete(this->PrevMat[1]);
    NEA_PaletteDelete(this->PrevPal[0]);
    NEA_PaletteDelete(this->PrevPal[1]);

    NEA_MaterialDelete(this->NextMat[0]);
    NEA_MaterialDelete(this->NextMat[1]);
    NEA_PaletteDelete(this->NextPal[0]);
    NEA_PaletteDelete(this->NextPal[1]);
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

    // Collect the console names clients announce over their Hello frames.
    for (int aid = 1; aid <= NetLink::kMaxClients; ++aid)
    {
        GameNetHello hello;
        if (NetLink::HostPollHello(aid, hello))
            this->clientNames.at(aid) = hello.name;
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
