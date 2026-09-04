#include "MainMenu.hpp"
#include "AssetLoader.hpp"
#include "ErrorHandler.hpp"
#include "MainMenuClasses/MainMenuStates.hpp"
#include "MainMenuClasses/MainSelectionMenu.hpp"
#include "Music.hpp"
#include "Process.hpp"
#include "globalHeader.hpp"



MainMenu::MainMenu()
{

}

MainMenu::~MainMenu()
{

}

void MainMenu::SCREEN_TOP()
{
    int angle = (this->emitFrame * 1) & 0x1FF;
    int32_t cx = mulf32(floattof32(3.5f), cosLerp(angle << 6));
    int32_t cz = mulf32(floattof32(3.5f), sinLerp(angle << 6));
    NEA_CameraSetI(this->emitCam, cx, floattof32(1.5f), cz,
                              0, floattof32(0.4f), 0,
                              0, floattof32(1.0f), 0);
    NEA_CameraUse(this->emitCam);
    NEA_ParticleEmitterDraw(this->hexEmit);

    NEA_2DViewInit();
    NEA_ClearColorSet(NEA_White, 0, 63);

    // The hex background is on a hardware 2D BG layer — the GPU renders it
    // every scanline automatically. We only submit 3D content here (text).
    // BG0 (3D) sits in front of BG1 (the 2D hex layer) per the priorities
    // set in LoadAssetsMainMenu, so text appears above the pattern with no
    // submission-order tricks.

    switch (this->mainmenustates){
        case MainMenuStates::MainTitle:
        {
            if ((this->showOrNotTouchScreenText / 30) % 2 == 0)
            {
                NEA_RichTextRender3D(0, "Touch screen!", 70, 90);
            }
            this->showOrNotTouchScreenText++;

            break;
        }

        case MainMenuStates::MainSelectionMenu:
        {
            this->mainSelec.ActionMainSelectionMenu();
            break;
        }

        case MainMenuStates::PlaySelectionMenu:
        {
            this->playSelec.ActionPlaySelectionMenu();
            break;
        }

        case MainMenuStates::OnePlayerPartyStart:
        {
            this->onePlayerParty.ActionOnePlayerPartyStart();
            break;
        }

        case MainMenuStates::MultiplayerFirstMenu:
        {
            this->multiplayerFirstMenu.ActionMultiplayerFirstMenu();
            break;
        }
        case MainMenuStates::MultiplayerHostMenu:
        {
            this->multiplayerHostmenu.ActionMultiplayerHostMenu();
            break;
        }
        case MainMenuStates::MultiplayerJoinMenu:
        {
            this->multiplayerJoinmenu.ActionMultiplayerJoinMenu();
            break;
        }
        case MainMenuStates::SettingsMenu:
        {
            this->settingsMenu.ActionSettingsMenu();
            break;
        }
        case MainMenuStates::TransitionToPlayOnePlayer:
        case MainMenuStates::TransitionToHostGame:
        case MainMenuStates::TransitionToJoinGame:
        {
            break;
        }
    }



}

void MainMenu::SCREEN_BOTTOM()
{
    NEA_2DViewInit();
    NEA_ClearColorSet(NEA_White, 0, 63);
    // Hardware 2D BG renders itself; nothing to submit here yet.
}

// Tile VRAM the hex backgrounds need, from the GFX chunk of their GRF files
// (45252 and 57796 bytes). NEA_Hw2DBGCreate would hand out a single 16 KB
// block and NEA_Hw2DBGLoadTiles would then clip the tileset, leaving
// everything past the first quarter of the screen as garbage. Main BG lives on
// bank E (three usable blocks after the reserved map block), sub BG on bank C
// (seven), so both fit.
constexpr std::size_t kHexBgTopTileBytes = 48 * 1024; // mainmenu/hex_background2
constexpr std::size_t kHexBgBotTileBytes = 64 * 1024; // mainmenu/hex_background

// Creates the two hex background layers and loads their artwork. Split out of
// LoadAssetsMainMenu because both (re)entry paths need it.
//
// Layer 1 on main (layer 0 is reserved for the 3D output); priority 3 puts both
// behind the 3D layer, so text submitted as a 3D quad renders above the pattern
// with no submission-order tricks.
//
// The palette_slot argument is ignored for 8bpp backgrounds -- the engine only
// computes a first-colour offset for 4bpp ones -- so each engine's whole
// 256-colour BG palette comes from its own background. One per engine, so that
// is exactly what we want.
void MainMenu::CreateHexBackgrounds()
{
    AsyncAssetBatch assets;

    this->hexBGtop = NEA_Hw2DBGCreateTiles(NEA_ENGINE_MAIN, 1,
                                            NEA_HW2D_BG_TILED_8BPP, 256, 256,
                                            kHexBgTopTileBytes);
    if (this->hexBGtop == nullptr)
    {
        error.errorReason.assign("Couldn't create the menu main BG");
        std::terminate();
    }
    NEA_Hw2DBGSetPriority(this->hexBGtop, 3);
    // Freshly created layers are shown by default and their tile blocks still
    // hold whatever the previous owner left there, so keep them hidden until
    // the artwork has landed.
    NEA_Hw2DBGSetVisible(this->hexBGtop, false);
    assets.QueueBGGRF(this->hexBGtop, "mainmenu/hex_background2_png.grf", 0);

    this->hexBGbot = NEA_Hw2DBGCreateTiles(NEA_ENGINE_SUB, 0,
                                            NEA_HW2D_BG_TILED_8BPP, 256, 256,
                                            kHexBgBotTileBytes);
    if (this->hexBGbot == nullptr)
    {
        error.errorReason.assign("Couldn't create the menu sub BG");
        std::terminate();
    }
    NEA_Hw2DBGSetPriority(this->hexBGbot, 3);
    NEA_Hw2DBGSetVisible(this->hexBGbot, false);
    assets.QueueBGGRF(this->hexBGbot, "mainmenu/hex_background_png.grf", 0);

    assets.Wait("Loading...");

    // Only show them once the tiles and maps are really in VRAM.
    NEA_Hw2DBGSetVisible(this->hexBGtop, true);
    NEA_Hw2DBGSetVisible(this->hexBGbot, true);
}

void MainMenu::LoadAssetsMainMenu()
{
    // Reset the menu back to the title screen on every (re)entry. When the menu
    // is re-loaded after quitting a game party it is otherwise left in the
    // TransitionToPlayOnePlayer state, which renders nothing and handles no
    // input -> a dead screen.
    std::filesystem::path save_party(process.fatDeviceCPP + "_nds/SkyjoDS/save_party.dat");
    if ( std::filesystem::exists(save_party) && std::filesystem::is_regular_file(save_party))
    {
        this->mainSelec.resumableParty = true;
    }
    else 
    {
        this->mainSelec.resumableParty = false;
    }
    // The menu always fades in from white, so start there: it also hides the
    // background load below, the same way the fade apex hides the per-screen
    // loads in RenderMainMenu().
    this->brightness = 16;
    setBrightness(3, this->brightness);

    // Both (re)entry paths below need the hex backgrounds. They were previously
    // created once and never freed, which meant the game party rendered on top
    // of the *menu's* layers; now each side creates and frees its own pair.
    this->CreateHexBackgrounds();

    if (!this->bypassableChangeMenuStates)
    {
        // Normal (re)entry: start on the title screen.
        this->mainmenustates = MainMenuStates::MainTitle;
        this->canTouchDetect = false;
        this->triggerCanTouchDetect = false;
        this->frameTouchDetect = 0;
    }
    else
    {
        // Bypass (re)entry that lands directly on a sub-menu, e.g. quitting a
        // game back to the party-setup screen. The per-state asset load
        // normally happens at the fade apex in RenderMainMenu(); this path
        // skips it, so load the current state's assets here. Arm touch
        // detection via the usual 20-frame debounce since there is no state
        // transition to trigger it (and the debounce swallows the lingering
        // quit-button touch).
        switch (this->mainmenustates)
        {
            case MainMenuStates::MainSelectionMenu:
                this->mainSelec.LoadAssetsMainSelectionMenu(); break;
            case MainMenuStates::PlaySelectionMenu:
                this->playSelec.LoadAssetsPlaySelectionMenu(); break;
            case MainMenuStates::OnePlayerPartyStart:
                this->onePlayerParty.LoadAssetsOnePlayerPartyStart(); break;
            case MainMenuStates::MultiplayerFirstMenu:
                this->multiplayerFirstMenu.LoadAssetsMultiplayerFirstMenu(); break;
            case MainMenuStates::MultiplayerHostMenu:
                this->multiplayerHostmenu.LoadAssetsMultiplayerHostMenu(); break;
            case MainMenuStates::MultiplayerJoinMenu:
                this->multiplayerJoinmenu.LoadAssetsMultiplayerJoinMenu(); break;
            case MainMenuStates::SettingsMenu:
                this->settingsMenu.LoadAssetsSettingsMenu(); break;
            default: break;
        }
        this->canTouchDetect = false;
        this->triggerCanTouchDetect = true;
        this->frameTouchDetect = 0;
        this->bypassableChangeMenuStates = false; // one-shot
    }

    this->frameTrigger = 0;
    this->triggerPlayPartyOnePlayer = false;
    this->triggerPlayPartyMultiplayerHost = false;
    this->triggerPlayPartyMultiplayerClient = false;
    
    this->fadePhase = FadePhase::FadingIn;
    this->fadeStepInterval = 5;
    this->pendingNextState.reset();

    this->emitCam = NEA_CameraCreate();
    NEA_ParticleSystemReset(0);
    NEA_ParticleSystemSetCamera(this->emitCam);

    this->hexParMat = NEA_MaterialCreate();
    this->hexParPal = NEA_PaletteCreate();
    this->hexEmit = NEA_ParticleEmitterCreate();

    NEA_MaterialSetName(this->hexParMat, "hexPart");

    AsyncAssetBatch assets;
    assets.QueueTexGRF(this->hexParMat, this->hexParPal,
                       "mainmenu/hex/hexParticle_png.grf");
    assets.QueueParticleEmitter(this->hexEmit, "mainmenu/hex/hexNPE.npe");
    assets.Wait("Loading...");

    NEA_ParticleEmitterSetPosition(this->hexEmit, 0, floattof32(-2.0), 0);
    NEA_ParticleEmitterPlay(this->hexEmit);

    // The rich-text font is loaded once at boot (Process::ProcessInit) and used
    // by the menu, the game party and the loading overlay alike. Re-initialising
    // the slot here would throw away and re-read the same font on every menu
    // entry, and would leave the loading overlay with no font at all.

    // Per-screen materials/palettes are created in each sub-menu's own
    // LoadAssets* and freed in the matching UnloadAssets* — creating them
    // here once would leave dangling handles after the first Unload.

    // Start the looping menu music (both the normal and bypass re-entry paths
    // fall through to here). No-op if already playing; honors the music-volume
    // setting, so 0% simply plays silently.
    Music::Play(Music::MainMenuTrack);
}

// Symmetric teardown for LoadAssetsMainMenu(). Must run before leaving the menu
// (e.g. when launching a game party); otherwise the hardware BG layers stay
// occupied and the next LoadAssetsMainMenu() re-creates them over the live
// originals, getting NULL from NEA_Hw2DBGCreate -> data abort. Mirrors the
// Intro::UnloadAssetsIntro() pattern. Delete in reverse creation order.
void MainMenu::UnloadAssetsMainMenu()
{
    // Stop the menu music before leaving the menu (e.g. launching a party). The
    // game party starts its own track; the menu track restarts on the next
    // LoadAssetsMainMenu().
    Music::Stop();

    NEA_ParticleEmitterDelete(this->hexEmit);
    this->hexEmit = nullptr;
    NEA_MaterialDelete(this->hexParMat);
    this->hexParMat = nullptr;
    NEA_PaletteDelete(this->hexParPal);
    this->hexParPal = nullptr;

    NEA_CameraDelete(this->emitCam);
    this->emitCam = nullptr;

    // The backgrounds go last: deleting them hands their tile and map blocks
    // back to the allocator and hides the layers, which is what lets the game
    // party claim the same two layers for its own artwork.
    NEA_Hw2DBGDelete(this->hexBGbot);
    this->hexBGbot = nullptr;
    NEA_Hw2DBGDelete(this->hexBGtop);
    this->hexBGtop = nullptr;
}

void MainMenu::ProcessLogicMainTitle()
{
    if (this->keys & KEY_TOUCH)
    {
        this->StartTransitionTo(MainMenuStates::MainSelectionMenu);
    }
}

void MainMenu::StartTransitionTo(MainMenuStates next)
{
    // Defer the actual asset swap to the fade apex (when brightness == 16,
    // i.e. fully white). Step interval 2 is quicker than the 5-frame step
    // used by the intro / initial boot fade.
    this->pendingNextState = next;
    this->fadePhase = FadePhase::FadingOut;
    this->fadeStepInterval = 2;
    this->frameTrigger = 0;
}

void MainMenu::RenderMainMenu()
{
    while(1)
    {
        this->OLDmainmenustates = this->mainmenustates;

        if (this->canTouchDetect)
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_GUI |
                                                         NEA_UPDATE_PARTICLES |
                                                         NEA_UPDATE_HW2D |
                                                         NEA_UPDATE_ASSETS));
        else
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_PARTICLES |
                                                         NEA_UPDATE_HW2D |
                                                         NEA_UPDATE_ASSETS));

        // Keep the music stream's circular buffer topped up every frame.
        Music::Pump();

        scanKeys();
        this->keys = keysDown();


        // Only run menu logic when no fade is in progress — clicks during a
        // transition would either be lost (assets are mid-swap) or queue up
        // a second transition before the first one finishes.
        if (this->fadePhase == FadePhase::None)
        {
            switch (this->mainmenustates){
                case MainMenuStates::MainTitle:
                {
                    this->ProcessLogicMainTitle();
                    break;
                }

                case MainMenuStates::MainSelectionMenu:
                {
                    if (auto next = this->mainSelec.ProcessLogicMainSelectionMenu())
                        this->StartTransitionTo(*next);
                    break;
                }

                case MainMenuStates::PlaySelectionMenu:
                {
                    if (auto next = this->playSelec.ProcessLogicPlaySelectionMenu())
                        this->StartTransitionTo(*next);
                    break;
                }

                case MainMenuStates::OnePlayerPartyStart:
                {
                    if (auto next = this->onePlayerParty.ProcessLogicOnePlayerPartyStart())
                        this->StartTransitionTo(*next);
                    break;
                }

                case MainMenuStates::MultiplayerFirstMenu:
                {
                    if (auto next = this->multiplayerFirstMenu.ProcessLogicMultiplayerFirstMenu())
                        this->StartTransitionTo(*next);
                    break;
                }

                case MainMenuStates::MultiplayerHostMenu:
                {
                    if (auto next = this->multiplayerHostmenu.ProcessLogicMultiplayerHostMenu())
                        this->StartTransitionTo(*next);
                    break;
                }
                case MainMenuStates::MultiplayerJoinMenu:
                {
                    if (auto next = this->multiplayerJoinmenu.ProcessLogicMultiplayerJoinMenu())
                        this->StartTransitionTo(*next);
                    break;
                }
                case MainMenuStates::SettingsMenu:
                {
                    if (auto next = this->settingsMenu.ProcessLogicSettingsMenu())
                        this->StartTransitionTo(*next);
                    break;
                }
                case MainMenuStates::TransitionToPlayOnePlayer:
                case MainMenuStates::TransitionToHostGame:
                case MainMenuStates::TransitionToJoinGame:
                {
                    break;
                }
            }
        }

        // Fade tick. fadeStepInterval is 5 for the initial boot fade-in and
        // 2 for menu-to-menu transitions, giving menu switches a quicker
        // white fade than the intro / first MainMenu fade.
        if (this->fadePhase != FadePhase::None)
        {
            this->frameTrigger++;
            if (this->frameTrigger >= this->fadeStepInterval)
            {
                this->frameTrigger = 0;
                if (this->fadePhase == FadePhase::FadingOut)
                {
                    this->brightness++;
                    if (this->brightness >= 16)
                    {
                        this->brightness = 16;
                        // Apex of the fade: screen is fully white, swap
                        // assets out of sight.
                        switch (this->mainmenustates)
                        {
                            case MainMenuStates::MainSelectionMenu:
                                this->mainSelec.UnloadAssetsMainSelectionMenu(); break;
                            case MainMenuStates::PlaySelectionMenu:
                                this->playSelec.UnloadAssetsPlaySelectionMenu(); break;
                            case MainMenuStates::OnePlayerPartyStart:
                                this->onePlayerParty.UnloadAssetsOnePlayerPartyStart(); break;
                            case MainMenuStates::MultiplayerFirstMenu:
                                this->multiplayerFirstMenu.UnloadAssetsMultiplayerFirstMenu(); break;
                            case MainMenuStates::MultiplayerHostMenu:
                                this->multiplayerHostmenu.UnloadAssetsMultiplayerHostMenu(); break;
                            case MainMenuStates::MultiplayerJoinMenu:
                                this->multiplayerJoinmenu.UnloadAssetsMultiplayerJoinMenu(); break;
                            case MainMenuStates::SettingsMenu:
                                this->settingsMenu.UnloadAssetsSettingsMenu(); break;
                            case MainMenuStates::TransitionToPlayOnePlayer:
                            case MainMenuStates::TransitionToHostGame:
                            case MainMenuStates::TransitionToJoinGame:
                                break;
                            default: break;
                        }
                        if (this->pendingNextState)
                        {
                            this->mainmenustates = *this->pendingNextState;
                            this->pendingNextState.reset();
                        }
                        switch (this->mainmenustates)
                        {
                            case MainMenuStates::MainSelectionMenu:
                                this->mainSelec.LoadAssetsMainSelectionMenu(); break;
                            case MainMenuStates::PlaySelectionMenu:
                                this->playSelec.LoadAssetsPlaySelectionMenu(); break;
                            case MainMenuStates::OnePlayerPartyStart:
                                this->onePlayerParty.LoadAssetsOnePlayerPartyStart(); break;
                            case MainMenuStates::MultiplayerFirstMenu:
                                this->multiplayerFirstMenu.LoadAssetsMultiplayerFirstMenu(); break;
                            case MainMenuStates::MultiplayerHostMenu:
                                this->multiplayerHostmenu.LoadAssetsMultiplayerHostMenu(); break;
                            case MainMenuStates::MultiplayerJoinMenu:
                                this->multiplayerJoinmenu.LoadAssetsMultiplayerJoinMenu(); break;
                            case MainMenuStates::SettingsMenu:
                                this->settingsMenu.LoadAssetsSettingsMenu(); break;
                            case MainMenuStates::TransitionToPlayOnePlayer:
                                this->triggerPlayPartyOnePlayer = true; break;
                            case MainMenuStates::TransitionToHostGame:
                                this->triggerPlayPartyMultiplayerHost = true; break;
                            case MainMenuStates::TransitionToJoinGame:
                                this->triggerPlayPartyMultiplayerClient = true; break;
                            default: break;
                        }
                        this->fadePhase = FadePhase::FadingIn;
                    }
                }
                else // FadingIn
                {
                    this->brightness--;
                    if (this->brightness <= 0)
                    {
                        this->brightness = 0;
                        this->fadePhase = FadePhase::None;
                    }
                }
            }
        }

        setBrightness(3, this->brightness);
        // Render the 3D scene every frame. SCREEN_TOP() clears the main
        // engine's 3D layer (BG0) with alpha 0 so the hardware hex
        // background on BG1 shows through, then submits the menu text on
        // top. The sub engine is pure hardware 2D — nothing to submit.
        NEA_Process([](){
            mainmenu.SCREEN_TOP();
        });

        if (this->mainmenustates != this->OLDmainmenustates)
        {
            this->triggerCanTouchDetect = true;
            this->frameTouchDetect = 0;
        }

        if (this->triggerCanTouchDetect)
        {
            if (!this->canTouchDetect)
                this->frameTouchDetect++;

            if (this->frameTouchDetect == 20)
            {
                this->canTouchDetect = true;
                this->triggerCanTouchDetect = false;
                this->frameTouchDetect = 0;
            }

        }

        if (this->triggerPlayPartyOnePlayer ||
            this->triggerPlayPartyMultiplayerHost ||
            this->triggerPlayPartyMultiplayerClient)
            break;
    }

    if (this->triggerPlayPartyOnePlayer)
    {
        // Tear the menu down before handing off to the game party so its BG
        // layers / camera / particle assets are freed; the matching reload on
        // quit then starts from a clean slate.
        this->UnloadAssetsMainMenu();
        if (this->mainSelec.resumeSelected)
        {
            this->mainSelec.resumeSelected = false;
            process.CallResumeOnePlayerParty();
        }
        else
        {
            process.CallInitializationOnePlayerParty(this->onePlayerParty.Get_player_number(),
                                                     this->onePlayerParty.Get_CPULevel());
        }
    }
    else if (this->triggerPlayPartyMultiplayerHost)
    {
        // The host lobby already locked the lobby and sent the start handshake
        // to every client; just hand the player count to the game party. The
        // dswifi link stays live across the transition (it is IRQ-driven).
        this->UnloadAssetsMainMenu();
        process.CallInitializationMultiplayerHost(this->multiplayerHostmenu.GetPlayerCount(),
                                                  this->multiplayerHostmenu.GetHumanCount(),
                                                  this->multiplayerHostmenu.GetCpuLevel(),
                                                  this->multiplayerHostmenu.GetNames());
    }
    else if (this->triggerPlayPartyMultiplayerClient)
    {
        // The join lobby already connected and received the host's start
        // handshake (assigned seat + roster).
        this->UnloadAssetsMainMenu();
        process.CallInitializationMultiplayerClient(this->multiplayerJoinmenu.GetSeat(),
                                                    this->multiplayerJoinmenu.GetPlayerCount(),
                                                    this->multiplayerJoinmenu.GetNames());
    }
}
MainMenu mainmenu;
