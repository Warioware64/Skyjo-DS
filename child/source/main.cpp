// DS Download Play child binary for Skyjo DS.
//
// This program is never started from a card in normal use: the host game sends
// it over the air and the guest console boots it out of RAM. It has no
// filesystem of any kind -- Download Play transmits only the ROM header and the
// two CPU binaries -- so its assets are linked in and served by AssetDevice,
// which lets the game's ordinary fopen()-based loaders work untouched.
//
// It is a client-only build (SKYJO_CLIENT_ONLY): the host half of GameParty --
// the rules engine, the pause and end-game menus, the CPU strategies, the
// suspend-save -- is compiled out. Everything this renders comes from snapshots
// the host broadcasts.
//
// Started from a card it says so and stops, which is also the quickest way to
// check its rendering without involving any wireless.

#include "AssetDevice.hpp"

#include "AssetLoader.hpp"
#include "GameParty.hpp"
#include "Net/NetLink.hpp"
#include "Music.hpp"
#include "globalHeader.hpp"

#include <cstdio>

namespace
{
    // The Download Play loader leaves these behind for the program it starts.
    // GBATEK documents the block at 0x027FFC40 and notes outright that
    // "multiplayer games can use that info for communicating with each other
    // after the upload".
    constexpr u16 kBootFromDlPlay = 2;

    volatile u16 &BootIndicator() { return *(vu16 *)0x027FFC40; }

    // The host's MAC, three halfwords, left by the loader right after the
    // indicator. GBATEK says outright that "multiplayer games can use that info
    // for communicating with each other after the upload" -- so there is no
    // need to go looking for a console we were just handed the address of.
    // Same offset and unpacking as the dswifi dlplay_child example's FindHost().
    volatile u16 *HostBssidWords() { return (vu16 *)0x027FFC46; }

    void ReadHostBssid(uint8_t out[6])
    {
        const volatile u16 *w = HostBssidWords();
        for (int i = 0; i < 3; ++i)
        {
            out[i * 2 + 0] = static_cast<uint8_t>(w[i] & 0xFF);
            out[i * 2 + 1] = static_cast<uint8_t>(w[i] >> 8);
        }
    }

    // Which channel to come back on.
    //
    // Deliberately NOT the loader's MB_HOST_CHANNEL at 0x027FFC78: that is the
    // channel the transfer ran on, and by the time we rejoin, the host has
    // ended its Download Play session and moved to its own beacon channel. The
    // host writes that channel into the 32-byte user parameter instead, which
    // the loader leaves at kDlPlayParamAddress.
    int ReadHostChannel()
    {
        SkyjoDlPlayParam param;
        memcpy(&param, reinterpret_cast<const void *>(kDlPlayParamAddress),
               sizeof(param));

        if (param.magic == kDlPlayParamMagic && param.channel >= 1 &&
            param.channel <= 14)
            return param.channel;

        // Sent by a host that predates the parameter, or the bytes were never
        // written: fall back to where the host has always beaconed.
        return NetLink::kHostChannel;
    }

    // This console's owner name, read from the firmware. Announced to the host
    // so its lobby and the in-game roster show it instead of "Player N".
    std::string g_guestName;

    bool StartedByDownloadPlay()
    {
        return BootIndicator() == kBootFromDlPlay;
    }

    void WaitForever()
    {
        while (true)
            swiWaitForVBlank();
    }

    // Set once InitEngine() has run. Before that a text console is the only way
    // to say anything; after it there is no console at all, because the engine
    // owns both screens.
    bool g_engineUp = false;

    // What FailScreen() is showing. NEA_Process takes a plain function, so the
    // message has to reach the draw callback through here.
    const char *g_failWhy = nullptr;
    std::string g_failDetail;

    void DrawFailScreen()
    {
        NEA_RichTextRender3D(0, "Download Play guest", 46, 40);
        if (g_failWhy != nullptr)
            NEA_RichTextRender3D(0, g_failWhy, 20, 80);
        if (!g_failDetail.empty())
            NEA_RichTextRender3D(0, g_failDetail.c_str(), 20, 100);
    }

    // Stops, saying why, on whichever output this console actually has.
    //
    // A guest has no text console: consoleDemoInit() only ever runs on the
    // card-boot path above, before the engine claims the screens. So printf here
    // went nowhere and every failure after InitEngine() -- including wifi
    // refusing to start -- looked like a console that had simply hung on a white
    // screen, with nothing to say which of them it was.
    void Fail(const char *why, const std::string &detail = std::string())
    {
        if (!g_engineUp)
        {
            std::printf("\n%s\n", why);
            if (!detail.empty()) std::printf("%s\n", detail.c_str());
            WaitForever();
        }

        g_failWhy = why;
        g_failDetail = detail;
        setBrightness(3, 0);

        while (true)
        {
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D));
            NEA_Process(DrawFailScreen);
        }
    }

    // Same engine bring-up as Process::ProcessInit, minus everything a guest
    // cannot do: no nitroFSInit, no FAT, no settings file, no intro, no menu.
    void InitEngine()
    {
        irqEnable(IRQ_HBLANK);
        irqSet(IRQ_VBLANK, NEA_VBLFunc);
        irqSet(IRQ_HBLANK, NEA_HBLFunc);

        NEA_Init3D();
        NEA_MainScreenSetOnBottom();

        Music::InitOnce();

        NEA_SetTexPaletteBank(static_cast<NEA_VRAMBankFlags>(NEA_VRAM_F | NEA_VRAM_G));
        NEA_TextureSystemReset(0, 0, static_cast<NEA_VRAMBankFlags>(NEA_VRAM_AB));

        // Identical bank layout to the parent, so the shared rendering code
        // finds the VRAM it expects. Sub BG is bank C because the hex
        // backgrounds need more than one 16 KB tile block.
        NEA_Hw2DVRAMConfig hw2dCfg = {};
        hw2dCfg.main_bg  = NEA_VRAM_E;
        hw2dCfg.main_obj = static_cast<NEA_VRAMBankFlags>(0);
        hw2dCfg.sub_bg   = NEA_VRAM_C;
        hw2dCfg.sub_obj  = static_cast<NEA_VRAMBankFlags>(NEA_VRAM_D);
        if (NEA_Hw2DInit(&hw2dCfg) != 0)
            Fail("Hardware 2D init failed.");

        // The rich-text font is used by the in-game overlays and the join
        // screen below, so it is loaded once here as the parent does.
        NEA_RichTextResetSystem();
        NEA_RichTextInit(0);
        NEA_RichTextMetadataLoadFAT(0, "mainmenu/font/DejaVuSans-Bold.fnt");
        NEA_RichTextMaterialLoadGRF(0, "mainmenu/font/DejaVuSans-Bold_0_png.grf");

        char name[50];
        utf16_to_utf8(name, sizeof(name), (char16_t *)PersonalData->name,
                      PersonalData->nameLen * sizeof(char16_t));
        g_guestName = name;

        g_engineUp = true;
    }

    // What the join screen is doing, for the player to look at.
    enum class JoinPhase
    {
        Scanning,
        Connecting,
        WaitingStart,
        HostLeft,      // the party just ended because the host went away
    };

    JoinPhase g_phase = JoinPhase::Scanning;

    // What to say in the HostLeft phase. Set from the reason the party's render
    // loop gave for stopping.
    const char *g_leftMessage = nullptr;

    // How long to hold that message before going back to looking for a host. A
    // guest that drops straight into "Connecting..." looks like it crashed;
    // two and a half seconds is long enough to read one line and understand
    // that the game ended rather than broke.
    constexpr int kHostLeftFrames = 150;

    // How long one association attempt is given before it is re-issued.
    //
    // "Connecting..." is not a console sitting idle -- it is dswifi sweeping the
    // channels looking for the host's beacon. Wifi_ConnectOpenAP() does not talk
    // to the address it is given: it stores it, calls Wifi_ScanMode(), and waits
    // in ASSOCSTATUS_SEARCHING until Wifi_FindMatchingAP() spots that BSSID
    // among the beacons the ARM7 has collected (dswifi access_point.c:230,
    // :299). The ARM7 hops one channel per tick, so a sweep of all 13 takes a
    // couple of seconds and may have to run more than once before it is
    // listening on the host's channel at the moment a beacon goes out.
    //
    // This was fifteen seconds, because a retry meant tearing the whole stack
    // down and building it back up: that throws away the AP list the sweep had
    // built AND leaves the console deaf while it re-enters client mode, which is
    // long enough to miss the very beacon it is waiting for. Re-issuing the
    // connect instead costs nothing and keeps the radio listening, so the guest
    // can afford to ask again every few seconds -- which is the whole fix for a
    // host that is simply not on the air yet when the guest first looks.
    //
    constexpr int kConnectRetryFrames = 4 * 60;

    // After this many cheap re-issues, fall back to the expensive path: a full
    // teardown and a channel scan, in case the host came back somewhere other
    // than where the loader said it would be.
    constexpr int kRetriesBeforeRescan = 5;

    void DrawJoinScreen();

    // The join screen's own copy of the hex backgrounds. They have to be given
    // back before the party starts: LoadGamePartyAssets creates its own pair on
    // these same two layers, and NEA_Hw2DBGCreate returns NULL over a layer that
    // is still claimed, which RequireHandle turns into a terminate.
    NEA_Hw2DBG *g_joinBgTop = nullptr;
    NEA_Hw2DBG *g_joinBgBot = nullptr;

    void JoinSceneLoad()
    {
        if (g_joinBgTop != nullptr)
            return;

        // Both screens go white for the duration. There is nothing worth
        // showing while the backgrounds are still arriving, and white is what
        // hides the backdrop turning magenta as each palette lands.
        setBrightness(3, 16);

        AsyncAssetBatch assets;

        // Sized, not the plain Create: one 16 KB tile block holds 256 tiles at
        // 8bpp and these backgrounds need 45252 and 57796 bytes. The plain call
        // would let NEA_Hw2DBGLoadTiles clip the tileset and everything past the
        // first quarter of the screen would render as garbage.
        g_joinBgTop = NEA_Hw2DBGCreateTiles(NEA_ENGINE_MAIN, 1,
                                            NEA_HW2D_BG_TILED_8BPP, 256, 256,
                                            48 * 1024);
        g_joinBgBot = NEA_Hw2DBGCreateTiles(NEA_ENGINE_SUB, 0,
                                            NEA_HW2D_BG_TILED_8BPP, 256, 256,
                                            64 * 1024);
        if (g_joinBgTop == nullptr || g_joinBgBot == nullptr)
            Fail("Background layers unavailable.");

        NEA_Hw2DBGSetPriority(g_joinBgTop, 3);
        NEA_Hw2DBGSetPriority(g_joinBgBot, 3);
        NEA_Hw2DBGSetVisible(g_joinBgTop, false);
        NEA_Hw2DBGSetVisible(g_joinBgBot, false);

        assets.QueueBGGRF(g_joinBgTop, "mainmenu/hex_background2_png.grf", 0);
        assets.QueueBGGRF(g_joinBgBot, "mainmenu/hex_background_png.grf", 0);
        assets.Wait("Loading...");

        ClearBackdropToWhite();
        NEA_Hw2DBGSetVisible(g_joinBgTop, true);
        NEA_Hw2DBGSetVisible(g_joinBgBot, true);

        // Draw one full frame before lifting the white, so the first thing the
        // player sees is the finished screen rather than a half-built one.
        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D));
        NEA_Process(DrawJoinScreen);
        setBrightness(3, 0);
    }

    void JoinSceneUnload()
    {
        if (g_joinBgTop == nullptr)
            return;

        NEA_Hw2DBGDelete(g_joinBgBot);
        NEA_Hw2DBGDelete(g_joinBgTop);
        g_joinBgBot = nullptr;
        g_joinBgTop = nullptr;
    }

    // Association attempts so far. Drives how often the cheap re-issue gives
    // way to a full teardown and channel sweep.
    int g_connectTries = 0;

    void DrawJoinScreen()
    {
        NEA_2DViewInit();
        NEA_ClearColorSet(NEA_White, 0, 63);

        NEA_RichTextRender3D(0, "SKYJO", 108, 40);

        switch (g_phase)
        {
            case JoinPhase::Scanning:
                NEA_RichTextRender3D(0, "Looking for the host...", 46, 90);
                break;
            case JoinPhase::Connecting:
                NEA_RichTextRender3D(0, "Connecting...", 82, 90);
                break;
            case JoinPhase::WaitingStart:
                NEA_RichTextRender3D(0, "Waiting for the game", 54, 90);
                NEA_RichTextRender3D(0, "to start...", 90, 106);
                break;
            case JoinPhase::HostLeft:
                NEA_RichTextRender3D(0, g_leftMessage != nullptr
                                            ? g_leftMessage : "Host left the game",
                                     50, 90);
                NEA_RichTextRender3D(0, "Looking for a new game...", 40, 106);
                break;
        }
    }

    // Finds the host and plays one game, then returns so the caller can go
    // round again. This is the whole of MultiplayerJoinMenu's scan UI reduced
    // to what a console with no menu needs: there is exactly one Skyjo host in
    // range worth joining -- the one that just sent us this program.
    void JoinAndPlayOneGame()
    {
        g_phase = JoinPhase::Scanning;

        // Put the game's own screen up BEFORE touching wifi. StartClientScan()
        // brings dswifi up and then spins on Wifi_LibraryModeReady() without
        // rendering anything, which takes a couple of seconds; without this the
        // console sits on a blank screen for all of it and looks hung.
        JoinSceneLoad();
        for (int i = 0; i < 2; ++i)
        {
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D));
            NEA_Process(DrawJoinScreen);
        }

        // Go straight to the console that sent us the program instead of
        // sweeping the channels looking for it. Scanning was most of the wait
        // between the guest booting and reaching the lobby, and it is pure
        // waste here: we already know the address, and the host always beacons
        // on the same channel.
        uint8_t hostBssid[6];
        ReadHostBssid(hostBssid);
        const int hostChannel = ReadHostChannel();

        if (!NetLink::ClientConnectDirect(hostBssid, hostChannel))
        {
            // The library's own reason, which is the only thing that tells a
            // refusal to bring the hardware up apart from everything else that
            // can go wrong here.
            Fail("WiFi failed to start.",
                 "init stage " + std::to_string(NetLink::InitFailStage()));
        }

        g_phase = JoinPhase::Connecting;
        int connectingFrames = 0;
        g_connectTries = 0;

        while (true)
        {
            NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D |
                                                         NEA_UPDATE_ASSETS));
            scanKeys();

            if (g_phase == JoinPhase::Scanning)
            {
                // Take the first host advertising the Skyjo game id and
                // accepting players; ClientGetAPInfo already filters both.
                int count = NetLink::ClientNumAP();
                for (int i = 0; i < count; ++i)
                {
                    if (!NetLink::ClientGetAPInfo(i, nullptr, 0, nullptr, nullptr))
                        continue;
                    if (NetLink::ClientConnectTo(i))
                    {
                        g_phase = JoinPhase::Connecting;
                        connectingFrames = 0;
                    }
                    break;
                }
            }
            else if (g_phase == JoinPhase::Connecting)
            {
                if (NetLink::ClientAssociated())
                {
                    g_phase = JoinPhase::WaitingStart;
                }
                else if (NetLink::ClientConnectFailed() || ++connectingFrames > kConnectRetryFrames)
                {
                    // The host is very likely still tearing down its Download
                    // Play session and hasn't started beaconing yet, so retry
                    // rather than give up.
                    //
                    // Cheaply, and often. Every attempt used to tear the stack
                    // down and build it back up, which leaves the console deaf
                    // for a second or two -- and the beacon it is waiting for
                    // tends to arrive precisely then, costing it another fifteen
                    // seconds. Re-issuing the connect keeps the radio listening
                    // throughout, so a host that turns up mid-retry is heard.
                    connectingFrames = 0;
                    ++g_connectTries;

                    if (g_connectTries % kRetriesBeforeRescan != 0)
                    {
                        // Still asking for the console the loader named.
                        if (!NetLink::ClientReconnectDirect(hostBssid, hostChannel))
                            Fail("WiFi failed to restart.");
                    }
                    else
                    {
                        // Occasionally sweep instead, in case the host came back
                        // somewhere other than where the loader said.
                        NetLink::Shutdown();
                        g_phase = JoinPhase::Scanning;
                        if (!NetLink::StartClientScan())
                            Fail("WiFi failed to restart.");
                    }
                }
            }
            else // WaitingStart
            {
                NetLink::ClientDriveCycle();

                // Keep announcing this console's name so the host lobby can
                // show it rather than "Player N".
                GameNetHello hello;
                hello.name = g_guestName;
                NetLink::ClientSendHello(hello);

                GameNetStart start;
                if (NetLink::ClientPollStart(start))
                {
                    // Hand the BG layers back before the party claims them.
                    JoinSceneUnload();
                    gameparty.InitGamePartyClient(start.seatIndex,
                                                  start.playerCount,
                                                  start.names);
                    GameParty::ClientExit why = gameparty.RenderGamePartyClient();

                    // Say what happened before disappearing into the join screen
                    // again. RenderGamePartyClient has already torn the party
                    // down, so the two BG layers are free: JoinSceneLoad takes
                    // them back and leaves the screen lit, and the next round
                    // finds it already loaded and does not flash white again.
                    JoinSceneLoad();
                    g_phase = JoinPhase::HostLeft;
                    g_leftMessage = (why == GameParty::ClientExit::HostSilent)
                                        ? "Lost connection to the host"
                                        : "Host left the game";

                    // Paced by the vblank wait, like every other frame here:
                    // NEA_Process only renders, so on its own this would spin
                    // through all of them in a fraction of a second.
                    for (int i = 0; i < kHostLeftFrames; ++i)
                    {
                        NEA_WaitForVBL(static_cast<NEA_UpdateFlags>(NEA_UPDATE_HW2D));
                        NEA_Process(DrawJoinScreen);
                    }

                    return; // the party ended; the caller re-joins
                }

                if (NetLink::ClientLostHost())
                {
                    // Lost the host before the game began -- most likely it went
                    // back to its lobby. Go straight back to its address rather
                    // than sweeping the channels for a console we can name.
                    NetLink::Shutdown();
                    g_phase = JoinPhase::Connecting;
                    connectingFrames = 0;
                    g_connectTries = 0;
                    if (!NetLink::ClientConnectDirect(hostBssid, hostChannel))
                        Fail("WiFi failed to restart.");
                }
            }

            NEA_Process(DrawJoinScreen);
        }
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    defaultExceptionHandler();

    // White both screens immediately. Everything up to the join screen is
    // set-up with nothing worth looking at, and the alternative is whatever the
    // loader happened to leave in VRAM.
    setBrightness(3, 16);

    if (!StartedByDownloadPlay())
    {
        // No host to connect back to, so say so on a plain console rather than
        // hanging on a scan that can never succeed.
        videoSetMode(MODE_0_2D);
        vramSetBankA(VRAM_A_MAIN_BG);
        consoleDemoInit();
        setBrightness(3, 0);

        std::printf("Skyjo DS - Download Play guest\n\n");
        std::printf("Not started by Download Play.\n\n");
        std::printf("This program is meant to be sent\n");
        std::printf("by a Skyjo DS host over wireless.\n\n");

        if (AssetDeviceMount())
            std::printf("Assets OK: %d files.\n", AssetDeviceFileCount());
        else
            std::printf("Asset filesystem FAILED to mount.\n");

        WaitForever();
    }

    // Everything below needs the assets, so mount them before the engine that
    // will immediately start loading from them.
    if (!AssetDeviceMount())
    {
        videoSetMode(MODE_0_2D);
        vramSetBankA(VRAM_A_MAIN_BG);
        consoleDemoInit();
        setBrightness(3, 0);
        Fail("Asset filesystem failed to mount.");
    }

    InitEngine();

    while (true)
        JoinAndPlayOneGame();

    return 0;
}
