#include "MultiplayerJoinMenu.hpp"
#include "../Net/NetLink.hpp"

namespace
{
    constexpr int kMaxVisible = 6; // AP rows shown at once
}

void MultiplayerJoinMenu::LoadAssetsMultiplayerJoinMenu()
{
    this->BackMat[0] = NEA_MaterialCreate();
    this->BackMat[1] = NEA_MaterialCreate();
    this->BackPal[0] = NEA_PaletteCreate();
    this->BackPal[1] = NEA_PaletteCreate();

    this->JoinMat[0] = NEA_MaterialCreate();
    this->JoinMat[1] = NEA_MaterialCreate();
    this->JoinPal[0] = NEA_PaletteCreate();
    this->JoinPal[1] = NEA_PaletteCreate();

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

    NEA_MaterialTexLoadGRF(this->JoinMat[0], this->JoinPal[0], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButton_png.grf");
    NEA_MaterialTexLoadGRF(this->JoinMat[1], this->JoinPal[1], NEA_TEXGEN_TEXCOORD,
                            "mainmenu/btns/StartGameButtonPressed_png.grf");

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

    this->JoinButton = NEA_GUIButtonCreate(190, 160, 190 + 64, 160 + 32);
    NEA_GUIButtonConfig(this->JoinButton,
                        this->JoinMat[0], NEA_White, 31,
                        this->JoinMat[1], NEA_White, 31);

    this->PrevButton = NEA_GUIButtonCreate(25, 25, 25 + 32, 25 + 32);
    NEA_GUIButtonConfig(this->PrevButton,
                        this->PrevMat[0], NEA_White, 31,
                        this->PrevMat[1], NEA_White, 31);

    this->NextButton = NEA_GUIButtonCreate(190, 25, 190 + 32, 25 + 32);
    NEA_GUIButtonConfig(this->NextButton,
                        this->NextMat[0], NEA_White, 31,
                        this->NextMat[1], NEA_White, 31);

    this->phase = Phase::Scanning;
    this->selected = 0;
    this->apIndices.clear();
    this->apNames.clear();
    this->seat = 0;
    this->playerCount = 0;
    this->names.clear();

    // Enter client mode and start scanning for Skyjo hosts.
    NetLink::StartClientScan();
}

void MultiplayerJoinMenu::UnloadAssetsMultiplayerJoinMenu()
{
    NEA_GUIDeleteObject(this->BackButton);
    NEA_GUIDeleteObject(this->JoinButton);
    NEA_GUIDeleteObject(this->PrevButton);
    NEA_GUIDeleteObject(this->NextButton);

    NEA_MaterialDelete(this->BackMat[0]);
    NEA_MaterialDelete(this->BackMat[1]);
    NEA_PaletteDelete(this->BackPal[0]);
    NEA_PaletteDelete(this->BackPal[1]);

    NEA_MaterialDelete(this->JoinMat[0]);
    NEA_MaterialDelete(this->JoinMat[1]);
    NEA_PaletteDelete(this->JoinPal[0]);
    NEA_PaletteDelete(this->JoinPal[1]);

    NEA_MaterialDelete(this->PrevMat[0]);
    NEA_MaterialDelete(this->PrevMat[1]);
    NEA_PaletteDelete(this->PrevPal[0]);
    NEA_PaletteDelete(this->PrevPal[1]);

    NEA_MaterialDelete(this->NextMat[0]);
    NEA_MaterialDelete(this->NextMat[1]);
    NEA_PaletteDelete(this->NextPal[0]);
    NEA_PaletteDelete(this->NextPal[1]);
}

void MultiplayerJoinMenu::RefreshApList()
{
    this->apIndices.clear();
    this->apNames.clear();

    int total = NetLink::ClientNumAP();
    for (int i = 0; i < total; ++i)
    {
        char name[32];
        int cur = 0, max = 0;
        if (!NetLink::ClientGetAPInfo(i, name, sizeof(name), &cur, &max))
            continue;

        std::string label = std::string(name);
        if (label.empty()) label = "Skyjo host";
        label += " (" + std::to_string(cur) + "/" + std::to_string(max) + ")";
        this->apIndices.push_back(i);
        this->apNames.push_back(label);
    }

    if (this->selected >= static_cast<int>(this->apIndices.size()))
        this->selected = static_cast<int>(this->apIndices.size()) - 1;
    if (this->selected < 0) this->selected = 0;
}

std::optional<MainMenuStates> MultiplayerJoinMenu::ProcessLogicMultiplayerJoinMenu()
{
    // Keep replying in the MP cycle so our frames flow once connected.
    NetLink::ClientDriveCycle();

    if (NEA_GUIObjectGetEvent(this->BackButton) == NEA_Clicked)
    {
        NetLink::Shutdown();
        return MainMenuStates::MultiplayerFirstMenu;
    }

    switch (this->phase)
    {
        case Phase::Scanning:
        {
            this->RefreshApList();

            if (NEA_GUIObjectGetEvent(this->PrevButton) == NEA_Clicked && this->selected > 0)
                this->selected--;
            if (NEA_GUIObjectGetEvent(this->NextButton) == NEA_Clicked &&
                this->selected + 1 < static_cast<int>(this->apIndices.size()))
                this->selected++;

            if (NEA_GUIObjectGetEvent(this->JoinButton) == NEA_Clicked &&
                !this->apIndices.empty())
            {
                if (NetLink::ClientConnectTo(this->apIndices.at(this->selected)))
                    this->phase = Phase::Connecting;
            }
            break;
        }

        case Phase::Connecting:
        {
            if (NetLink::ClientAssociated())
                this->phase = Phase::WaitingStart;
            else if (NetLink::ClientConnectFailed())
                this->phase = Phase::Scanning;
            break;
        }

        case Phase::WaitingStart:
        {
            GameNetStart start;
            if (NetLink::ClientPollStart(start))
            {
                this->seat = start.seatIndex;
                this->playerCount = start.playerCount;
                this->names.assign(start.names.begin(), start.names.end());
                return MainMenuStates::TransitionToJoinGame;
            }
            break;
        }
    }

    return std::nullopt;
}

void MultiplayerJoinMenu::ActionMultiplayerJoinMenu()
{
    NEA_GUIDraw();
    NEA_RichTextRender3D(0, "JOIN GAME", 90, 6);

    if (this->phase == Phase::Connecting)
    {
        NEA_RichTextRender3D(0, "Connecting...", 80, 90);
        return;
    }
    if (this->phase == Phase::WaitingStart)
    {
        NEA_RichTextRender3D(0, "Connected!", 90, 80);
        NEA_RichTextRender3D(0, "Waiting for host to start", 45, 105);
        return;
    }

    // Scanning: render the host list around the current selection.
    if (this->apNames.empty())
    {
        NEA_RichTextRender3D(0, "Searching for hosts...", 58, 90);
        return;
    }

    int count = static_cast<int>(this->apNames.size());
    int first = this->selected - kMaxVisible / 2;
    if (first < 0) first = 0;
    if (first + kMaxVisible > count) first = std::max(0, count - kMaxVisible);

    int y = 60;
    for (int i = first; i < count && i < first + kMaxVisible; ++i)
    {
        std::string row = (i == this->selected ? "> " : "  ") + this->apNames.at(i);
        NEA_RichTextRender3D(0, row.c_str(), 40, y);
        y += 15;
    }
}
