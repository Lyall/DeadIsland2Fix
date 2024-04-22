#include "stdafx.h"
#include "helper.hpp"
#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Logger and config setup
inipp::Ini<char> ini;
std::shared_ptr<spdlog::logger> logger;
string sFixName = "DeadIsland2Fix";
string sFixVer = "1.0.3";
string sLogFile = "DeadIsland2Fix.log";
string sConfigFile = "DeadIsland2Fix.ini";
string sExeName;
filesystem::path sExePath;
filesystem::path sThisModulePath;
std::pair DesktopDimensions = { 0,0 };

// Ini Variables
bool bAspectFix;
bool bHUDLimit;

// Aspect ratio + HUD stuff
float fPi = (float)3.141592653;
float fNativeAspect = (float)16 / 9;
float fNativeWidth;
float fNativeHeight;
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fDefaultHUDWidth = (float)1920;
float fDefaultHUDHeight = (float)1080;
float fHUDWidthOffset;
float fHUDHeightOffset;

void Logging()
{
    // Get this module path
    WCHAR thisModulePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(thisModule, thisModulePath, MAX_PATH);
    sThisModulePath = thisModulePath;
    sThisModulePath = sThisModulePath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // Calculate aspect ratio / use desktop res instead
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();

    // spdlog initialisation
    {
        try
        {
            logger = spdlog::basic_logger_st(sFixName.c_str(), sThisModulePath.string() + sLogFile, true);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Path to logfile: {}", sThisModulePath.string() + sLogFile);
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
            spdlog::info("----------");
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        }
    }
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(sThisModulePath.string() + sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sThisModulePath.string().c_str() << std::endl;
    }
    else
    {
        spdlog::info("Path to config file: {}", sThisModulePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Read ini file
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    inipp::get_value(ini.sections["Remove HUD Size Limits"], "Enabled", bHUDLimit);

    // Log config parse
    spdlog::info("Config Parse: bAspectFix: {}", bAspectFix);
    spdlog::info("Config Parse: bHUDLimit: {}", bHUDLimit);
    spdlog::info("----------");
}

void AspectFOV()
{
    if (bAspectFix)
    {
        // Cutscene Aspect Ratio
        uint8_t* CutsceneAspectRatioScanResult = Memory::PatternScan(baseModule, "0F ?? ?? ?? 77 ?? F3 0F ?? ?? ?? 40 ?? 01");
        if (CutsceneAspectRatioScanResult)
        {
            spdlog::info("Cutscene Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CutsceneAspectRatioScanResult - (uintptr_t)baseModule);
            // Jump over code that compares current aspect ratio to 1.7778~
            Memory::PatchBytes((uintptr_t)CutsceneAspectRatioScanResult + 0x4, "\xEB", 1);
            spdlog::info("Cutscene Aspect Ratio: Patched instruction.");
        }
        else if (!CutsceneAspectRatioScanResult)
        {
            spdlog::error("Cutscene Aspect Ratio: Pattern scan failed.");
        }
    }
}

void Miscellaneous()
{
    if (bHUDLimit)
    {
        // HUD Border Limit
        uint8_t* HUDBorderLimitScanResult = Memory::PatternScan(baseModule, "0F ?? ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? 0F ?? ?? 72 ?? F3 0F ?? ?? A8 ?? ?? ??");
        if (HUDBorderLimitScanResult)
        {
            spdlog::info("Gameplay Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)HUDBorderLimitScanResult - (uintptr_t)baseModule);
            static SafetyHookMid HUDBorderLimitMidHook{};
            HUDBorderLimitMidHook = safetyhook::create_mid(HUDBorderLimitScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rcx + 0xA8)
                    {
                        // Width limit
                        *reinterpret_cast<float*>(ctx.rcx + 0xA8) = 0.45f;
                        // Height limit
                        *reinterpret_cast<float*>(ctx.rcx + 0xAC) = 0.45f;
                    }
                });
        }
        else if (!HUDBorderLimitScanResult)
        {
            spdlog::error("Aspect Ratio: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    AspectFOV();
    Miscellaneous();
    return true;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        thisModule = hModule;
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST); // set our Main thread priority higher than the games thread
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

