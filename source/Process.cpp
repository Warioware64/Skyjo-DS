#include "Process.hpp"
#include "DebugPrint.hpp"
#include "GameParty.hpp"
#include "MainMenu.hpp"
#include "globalHeader.hpp"


Process::Process()
{

}
Process::~Process()
{

}

void Process::CallSaveSettings()
{
    constexpr std::size_t yasFlagSAV = yas::file | yas::binary | yas::no_header;
    std::filesystem::path setting_fileSAV(fatDeviceCPP + "_nds/SkyjoDS/settings.dat");

    // First run: create the save file with the current (default) settings.
    if (std::filesystem::exists(setting_fileSAV))
    {
        std::filesystem::remove(setting_fileSAV);
        yas::file_ostream yasOutputSave(setting_fileSAV.c_str());
        yas::save<yasFlagSAV>(yasOutputSave, gamesettings);
        yasOutputSave.flush();
    }    
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

    NEA_SetTexPaletteBank(static_cast<NEA_VRAMBankFlags>(NEA_VRAM_F | NEA_VRAM_G));
    NEA_TextureSystemReset(0, 0, static_cast<NEA_VRAMBankFlags>(NEA_VRAM_AB));

    // Explicit 2D bank layout. Sub OBJ goes on bank D (128 KB) instead of the
    // default I (16 KB) so we can hold every per-CardType OBJ asset at once
    // without exhausting tile VRAM.
    NEA_Hw2DVRAMConfig hw2dCfg = {};
    hw2dCfg.main_bg  = NEA_VRAM_E;
    hw2dCfg.main_obj = static_cast<NEA_VRAMBankFlags>(0);
    hw2dCfg.sub_bg   = NEA_VRAM_H;
    hw2dCfg.sub_obj  = static_cast<NEA_VRAMBankFlags>(NEA_VRAM_D);
    if (NEA_Hw2DInit(&hw2dCfg) != 0)
    {
        error.errorReason.assign("NEA_Hw2DInit failed: bad bank config");
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
