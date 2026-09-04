#include "Process.hpp"
#include "DebugPrint.hpp"
#include "GameParty.hpp"
#include "MainMenu.hpp"
#include "Music.hpp"
#include "globalHeader.hpp"
#include "AssetLoader.hpp"
#include "ErrorHandler.hpp"

#include <nds/arm9/dldi.h>   // dldiSetMode / DLDI_MODE_ARM7


Process::Process()
{

}
Process::~Process()
{

}

void Process::CallSaveSettings()
{
    if (this->fatDeviceCPP.empty())
        return; // no writable filesystem; the defaults stay in RAM

    std::error_code ec;
    if (!std::filesystem::exists(this->fatDeviceCPP + "_nds/SkyjoDS", ec))
        return; // ProcessInit couldn't create the folder; nothing to save into

    // Serialize to RAM and hand the buffer to the engine, which writes it in
    // the background via a temporary file and a rename. Nothing is deleted
    // first: the rename replaces the old file atomically, so a save that fails
    // or gets cut short leaves the previous settings intact.
    constexpr std::size_t yasFlagSAV = yas::mem | yas::binary | yas::no_header;
    std::filesystem::path setting_fileSAV(this->fatDeviceCPP + "_nds/SkyjoDS/settings.dat");

    yas::shared_buffer buffer = yas::save<yasFlagSAV>(gamesettings);

    AsyncAssetBatch save;
    if (!save.QueueFileWrite(setting_fileSAV.c_str(), buffer.data.get(),
                             buffer.size))
    {
        DEBUG_PRINT("settings.dat write could not be queued");
        return;
    }

    // A failed settings save is survivable, so this is checked rather than
    // fatal: the in-RAM settings stay correct for the rest of the session.
    if (!save.TryWait("Saving..."))
        DEBUG_PRINT("settings.dat could not be written");
}

void Process::CallInitializationOnePlayerParty(int cpu_number, CPULevel cpu_level)
{
   this->classstates = ClassStates::Init;
   this->menustates = MenusStates::PartyGameOnePlayer; 

   this->cpu_number_arg = cpu_number;
   this->cpu_level_arg = cpu_level;
   this->party_type_arg = PartyType::OnePlayerCPU;

}

void Process::CallResumeOnePlayerParty()
{
   this->classstates = ClassStates::Init;
   this->menustates = MenusStates::PartyGameOnePlayer;
   this->resumeRequested = true;
}

void Process::CallInitializationMultiplayerHost(int player_count, int human_count,
                                                CPULevel cpu_level,
                                                const std::vector<std::string>& names)
{
   this->classstates = ClassStates::Init;
   this->menustates = MenusStates::PartyGameMultiplayerHost;
   this->mp_player_count_arg = player_count;
   this->mp_human_count_arg = human_count;
   this->mp_cpu_level_arg = cpu_level;
   this->mp_names_arg = names;
}

void Process::CallInitializationMultiplayerClient(int seat, int player_count,
                                                  const std::vector<std::string>& names)
{
   this->classstates = ClassStates::Init;
   this->menustates = MenusStates::PartyGameMultiplayerClient;
   this->mp_seat_arg = seat;
   this->mp_player_count_arg = player_count;
   this->mp_names_arg = names;
}

void Process::ProcessInit()
{
    irqEnable(IRQ_HBLANK);
    irqSet(IRQ_VBLANK, NEA_VBLFunc);
    irqSet(IRQ_HBLANK, NEA_HBLFunc);
    // Run the DLDI filesystem driver on the ARM7. Without this, filesystem
    // access on a flashcart happens on the ARM9 and blocks it for the whole
    // read, which would defeat every asynchronous load in the game (DSi SD and
    // cartridge NitroFS are already off-ARM9). Must come before nitroFSInit.
    dldiSetMode(DLDI_MODE_ARM7);

    if (!nitroFSInit(NULL))
    {
        error.errorReason.assign("NitroFile filesystem failed!");
        std::terminate();
    }
    DEBUG_PRINT("Nitrofiles successful");
    bool fatInit = fatInitDefault();
    fatDevice = fatGetDefaultDrive();

    // Defaults used whenever there's no readable save file. Set first so the
    // game always has valid settings even if FAT/file access fails below.
    gamesettings.musicSoundVolume = 1024;
    gamesettings.nosesSoundVolume = 1024;

    // IMPORTANT: this build is compiled with -fno-exceptions, so yas turns a
    // failed file open into std::abort() (see yas exception_base.hpp). We must
    // therefore only construct yas file streams when the open is guaranteed to
    // succeed: FAT initialized, the folder exists (for writing), and the file
    // exists (for reading). Otherwise we keep the defaults above.
    const bool hasFat = fatInit && (fatDevice != nullptr);
    if (!hasFat)
    {
        DEBUG_PRINT("FAT init failed - using default settings");
    }
    else
    {
        fatDeviceCPP = fatDevice;

        std::error_code ec;
        std::filesystem::path nds_folder(fatDeviceCPP + "_nds");
        if (!std::filesystem::exists(nds_folder, ec))
        {
            std::filesystem::create_directory(nds_folder, ec);
        }

        std::filesystem::path skyjo_folder(fatDeviceCPP + "_nds/SkyjoDS");
        if (!std::filesystem::exists(skyjo_folder, ec))
        {
            std::filesystem::create_directory(skyjo_folder, ec);
        }

        constexpr std::size_t yasFlag = yas::file | yas::binary | yas::no_header;
        std::filesystem::path setting_file(fatDeviceCPP + "_nds/SkyjoDS/settings.dat");

        // First run: create the save file with the current (default) settings.
        // This one still writes synchronously: it happens before NEA_Init3D(),
        // so there is no vertical blank to pump an asynchronous write from.
        if (std::filesystem::exists(skyjo_folder, ec) &&
            !std::filesystem::exists(setting_file, ec))
        {
            yas::file_ostream yasOutput(setting_file.c_str());
            yas::save<yasFlag>(yasOutput, gamesettings);
            yasOutput.flush();
        }

        // Load persisted settings only if the file is actually there.
        if (std::filesystem::exists(setting_file, ec))
        {
            yas::file_istream yasInput(setting_file.c_str());
            yas::load<yasFlag>(yasInput, gamesettings);
        }
    }

    NEA_Init3D();
    NEA_MainScreenSetOnBottom();

    // Bring up maxmod once (no soundbank) so the menu / game can stream music.
    // Runs after nitroFSInit above, which the streaming file reads depend on.
    Music::InitOnce();

    NEA_SetTexPaletteBank(static_cast<NEA_VRAMBankFlags>(NEA_VRAM_F | NEA_VRAM_G));
    NEA_TextureSystemReset(0, 0, static_cast<NEA_VRAMBankFlags>(NEA_VRAM_AB));

    // Explicit 2D bank layout. Sub OBJ goes on bank D (128 KB) instead of the
    // default I (16 KB) so we can hold every per-CardType OBJ asset at once
    // without exhausting tile VRAM.
    //
    // Sub BG is on bank C (128 KB) rather than H (32 KB). NEA reserves the
    // first 16 KB block of a BG bank for map data, so H left a single block =
    // 16 KB of tiles, and the top-screen hex background alone needs 57796
    // bytes: everything past the first quarter rendered as garbage. C gives
    // seven usable blocks. Banks A and B are the 3D texture pool
    // (NEA_TextureSystemReset above), F and G are texture palettes, so C and I
    // were the only free banks and only C is big enough.
    NEA_Hw2DVRAMConfig hw2dCfg = {};
    hw2dCfg.main_bg  = NEA_VRAM_E;
    hw2dCfg.main_obj = static_cast<NEA_VRAMBankFlags>(0);
    hw2dCfg.sub_bg   = NEA_VRAM_C;
    hw2dCfg.sub_obj  = static_cast<NEA_VRAMBankFlags>(NEA_VRAM_D);
    if (NEA_Hw2DInit(&hw2dCfg) != 0)
    {
        error.errorReason.assign("NEA_Hw2DInit failed: bad bank config");
        std::terminate();
    }

    // Rich-text font (3D quad path), loaded once for the whole run: the menu,
    // the game party and the asset-loading overlay all draw from slot 0. The
    // metadata read is asynchronous; the bitmap has no async loader in the
    // engine, and at boot there is nothing to overlap it with anyway.
    NEA_RichTextResetSystem();
    NEA_RichTextInit(0);
    {
        AsyncAssetBatch font;
        font.QueueRichTextMetadata(0, "mainmenu/font/DejaVuSans-Bold.fnt");
        font.Wait("Loading...");
    }
    if (NEA_RichTextMaterialLoadGRF(0, "mainmenu/font/DejaVuSans-Bold_0_png.grf") == 0)
    {
        error.errorReason.assign("Couldn't load the rich text font bitmap");
        std::terminate();
    }

    swiWaitForVBlank();
    swiWaitForVBlank();
    char name[50];
    utf16_to_utf8(name, sizeof(name), (char16_t *)PersonalData->name,
                    PersonalData->nameLen * sizeof(char16_t));
    consoleUserName = name;
    DEBUG_PRINT("Enter into intro sequence");

    intro.LoadAssetsIntro();
    intro.RenderIntro();
    intro.UnloadAssetsIntro();
    
    DEBUG_PRINT("Exit intro sequence");
    this->classstates = ClassStates::Init;
    this->menustates = MenusStates::MainMenu;

    
}
void Process::ProcessGame()
{
    if (this->classstates == ClassStates::Init)
    {
        if (this->menustates == MenusStates::MainMenu)
        {
            mainmenu.LoadAssetsMainMenu();
            this->classstates = ClassStates::Playing;
        }
        else if (this->menustates == MenusStates::PartyGameOnePlayer)
        {
            if (this->resumeRequested)
            {
                this->resumeRequested = false;
                gameparty.ResumeGamePartySituation();
            }
            else
            {
                gameparty.InitGamePartySituation(this->cpu_number_arg, this->cpu_level_arg, this->party_type_arg);
            }
            this->classstates = ClassStates::Playing;
        }
        else if (this->menustates == MenusStates::PartyGameMultiplayerHost)
        {
            gameparty.InitGamePartyHost(this->mp_player_count_arg, this->mp_human_count_arg,
                                        this->mp_cpu_level_arg, this->mp_names_arg);
            this->classstates = ClassStates::Playing;
        }
        else if (this->menustates == MenusStates::PartyGameMultiplayerClient)
        {
            gameparty.InitGamePartyClient(this->mp_seat_arg, this->mp_player_count_arg,
                                          this->mp_names_arg);
            this->classstates = ClassStates::Playing;
        }
    }
    else if(this->classstates == ClassStates::Playing)
    {
        if (this->menustates == MenusStates::MainMenu)
        {
            mainmenu.RenderMainMenu();
        }
        else if (this->menustates == MenusStates::PartyGameOnePlayer)
        {
            gameparty.RenderGameParty();
        }
        else if (this->menustates == MenusStates::PartyGameMultiplayerHost)
        {
            gameparty.RenderGameParty();
        }
        else if (this->menustates == MenusStates::PartyGameMultiplayerClient)
        {
            gameparty.RenderGamePartyClient();
        }
    }
    //std::terminate();
}


Process process;
