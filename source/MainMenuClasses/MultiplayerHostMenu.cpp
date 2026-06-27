#include "MultiplayerHostMenu.hpp"
#include "../Net/NetLink.hpp"
#include "../Process.hpp"

namespace
{
    // How many frames to keep resending the start handshake so a client that
    // connected at the last moment still receives it before we leave the lobby.
    constexpr int kStartResendFrames = 30;

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

    NEA_MaterialTexLoadGRF(this->BackMat[0], this->BackPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButton_png.grf");
    NEA_MaterialTexLoadGRF(this->BackMat[1], this->BackPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/BackButtonPressed_png.grf");

    NEA_MaterialTexLoadGRF(this->StartMat[0], this->StartPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButton_png.grf");
    NEA_MaterialTexLoadGRF(this->StartMat[1], this->StartPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButtonPressed_png.grf");

    this->BackButton = NEA_GUIButtonCreate(5, 160, 5 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->BackButton,
                        this->BackMat[0], NEA_White, 31,
                        this->BackMat[1], NEA_White, 31);

    this->StartButton = NEA_GUIButtonCreate(190, 160, 190 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->StartButton,
                        this->StartMat[0], NEA_White, 31,
                        this->StartMat[1], NEA_White, 31);

    this->phase = Phase::Lobby;
    this->startFrame = 0;
    this->playerCount = 0;
    this->names.clear();

    // Enter host mode and start beaconing so clients can find us.
    NetLink::StartHost();
}

void MultiplayerHostMenu::UnloadAssetsMultiplayerHostMenu()
{
    // Only NEA teardown here. The WiFi link is shut down explicitly on the Back
    // path; on the start path it must stay alive for the game.
    NEA_GUIDeleteObject(this->BackButton);
    NEA_GUIDeleteObject(this->StartButton);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);
    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);

    NEA_MaterialDelete(this->StartMat[0]);
    NEA_MaterialDelete(this->StartMat[1]);
    NEA_PaletteDelete(this->StartPal[0]);
    NEA_PaletteDelete(this->StartPal[1]);
}

std::optional<MainMenuStates> MultiplayerHostMenu::ProcessLogicMultiplayerHostMenu()
{
    // Keep the MP transfer cycle running every frame so queued host->client
    // frames (the start handshake) actually reach connected clients.
    NetLink::HostDriveCycle();

    if (this->phase == Phase::Starting)
    {
        // Keep broadcasting the handshake to every client for a short window.
        for (int aid = 1; aid < this->playerCount; ++aid)
            NetLink::HostSendStart(aid, MakeStart(this->playerCount, aid, this->names));

        if (++this->startFrame >= kStartResendFrames)
            return MainMenuStates::TransitionToHostGame;
        return std::nullopt;
    }

    if (NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        NetLink::Shutdown();
        return MainMenuStates::MultiplayerFirstMenu;
    }

    if (NEA_GUIObjectGetEvent(this->StartButton) == NEA_Clicked)
    {
        int clients = NetLink::HostNumClients();
        if (clients < 1) return std::nullopt; // need at least one other player

        NetLink::LockLobby();
        this->playerCount = clients + 1;

        this->names.clear();
        this->names.push_back(process.consoleUserName.empty()
                                  ? std::string("Host") : process.consoleUserName);
        for (int i = 1; i < this->playerCount; ++i)
            this->names.push_back("Player " + std::to_string(i + 1));

        // Seat assignment assumes contiguous client AIDs 1..clients (true for a
        // fresh lobby). Each client is told its seat == its AID.
        for (int aid = 1; aid < this->playerCount; ++aid)
            NetLink::HostSendStart(aid, MakeStart(this->playerCount, aid, this->names));

        this->phase = Phase::Starting;
        this->startFrame = 0;
    }

    return std::nullopt;
}

void MultiplayerHostMenu::ActionMultiplayerHostMenu()
{
    NEA_GUIDraw();
    NEA_RichTextRender3D(0, "HOST GAME", 90, 18);

    if (this->phase == Phase::Lobby)
    {
        int total = NetLink::HostNumClients() + 1;
        NEA_RichTextRender3D(0, ("Players: " + std::to_string(total)).c_str(), 90, 55);
        NEA_RichTextRender3D(0, "Waiting for players...", 58, 85);
        NEA_RichTextRender3D(0, "Press START when ready", 52, 112);
    }
    else
    {
        NEA_RichTextRender3D(0, "Starting game...", 75, 80);
    }
}
