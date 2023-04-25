#include "stdafx.h"
#include "helper.hpp"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bFOVFix;
bool bHUDBorderLimit;
int iHUDBorderLimit;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
float fAdditionalFOV;
int iAspectFix;
int iFOVFix;

// Variables
int iNarrowAspect;
float fNewX;
float fNewY;
float fNativeAspect = (float)16/9;
float fPi = 3.14159265358979323846f;
float fNewAspect;
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "1.0.1";

// CurrResolution Hook
DWORD64 CurrResolutionReturnJMP;
void __declspec(naked) CurrResolution_CC()
{
    __asm
    {
        mov[rdi + 0x000000A0], edx              // Original Code
        mov[rdi + 0x000000A4], eax              // Original Code
        mov[rdi + 0x000000A8], r9d              // Original Code

        mov[iCustomResX], r9d                 // Grab current resX
        mov[iCustomResY], ecx                 // Grab current resY
        cvtsi2ss xmm14, r9d
        cvtsi2ss xmm15, ecx
        divss xmm14,xmm15
        movss [fNewAspect], xmm14              // Grab current aspect ratio
        movss xmm15, [fNativeAspect]
        comiss xmm14, xmm15
        ja narrowAspect
        mov [iNarrowAspect], 1
        xorps xmm14, xmm14
        xorps xmm15, xmm15
        jmp[CurrResolutionReturnJMP]

        narrowAspect:
            mov [iNarrowAspect], 0
            xorps xmm14, xmm14
            xorps xmm15, xmm15
            jmp[CurrResolutionReturnJMP]
    }
}

// Aspect Ratio/FOV Hook
DWORD64 AspectFOVFixReturnJMP;
float FOVPiDiv;
float FOVDivPi;
float FOVFinalValue;
void __declspec(naked) AspectFOVFix_CC()
{
    __asm
    {
        cmp rax, 1
        jne originalCode
        cmp [iAspectFix], 1
        je modifyAspect
        cmp [iFOVFix], 1                       // Check if FOVFix is enabled
        je modifyFOV                           // jmp to FOV fix
        jmp originalCode                       // jmp to originalCode

        modifyAspect:
            mov eax, [fNewAspect]                  // Move new aspect to eax
            mov [rsi+0x3C], eax
            cmp [iFOVFix], 1                        // Check if FOVFix is enabled
            je modifyFOV                           // jmp to FOV fix
            jmp originalCode

        modifyFOV:
            cmp [iNarrowAspect], 1
            je originalCode
            fld dword ptr[rsi + 0x28]         // Push original FOV to FPU register st(0)
            fmul[FOVPiDiv]                     // Multiply st(0) by Pi/360
            fptan                              // Get partial tangent. Store result in st(1). Store 1.0 in st(0)
            fxch st(1)                         // Swap st(1) to st(0)
            fdiv[fNativeAspect]                // Divide st(0) by 1.778~
            fmul[fNewAspect]                   // Multiply st(0) by new aspect ratio
            fxch st(1)                         // Swap st(1) to st(0)
            fpatan                             // Get partial arc tangent from st(0), st(1)
            fmul[FOVDivPi]                     // Multiply st(0) by 360/Pi
            fadd[fAdditionalFOV]               // Add additional FOV
            fstp[FOVFinalValue]                // Store st(0) 
            movss xmm0, [FOVFinalValue]        // Copy final FOV value to xmm0
            movss[rsi + 0x28], xmm0
            jmp originalCode

        originalCode:
            mov rdx, [rdi + 0x00003508]
            mov rcx, [rdx + 0x00000838]
            jmp [AspectFOVFixReturnJMP]
    }
}

// FOVLimit Hook
DWORD64 FOVLimitReturnJMP;
float fMinFOV;
float fMaxFOV;
void __declspec(naked) FOVLimit_CC()
{
    __asm
    {
        //cmp[iFOVLimit], 1
        //jne originalCode
        cmp dword ptr[rsi + 0xC8], 0x428c0000 // MinFOV = 70
        jne originalCode
        cmp dword ptr[rsi + 0xCC], 0x42c80000 // MaxFOV = 100
        jne originalCode
        cmp [rsi + 0xEC], 6                   // Steps = 6
        jne originalCode

        movss xmm6, [fMinFOV]    // Min FOV
        movss [rsi + 0xC8], xmm6
        movss xmm6, [fMaxFOV]    // Max FOV
        movss[rsi + 0xCC], xmm6
        movss[rsi + 0xDC], xmm6
        mov[rsi + 0xEC], 22      // FOV steps (increments of 5)
        jmp originalCode

        originalCode :
            mov[rsp + 0x08], rbx
            mov[rsp + 0x10], rsi
            mov[rsp + 0x18], rdi
            jmp[FOVLimitReturnJMP]
    }
}

// HUDBorderLimit Hook
DWORD64 HUDBorderLimitReturnJMP;
float fHUDBorderLimit;
void __declspec(naked) HUDBorderLimit_CC()
{
    __asm
    {
        cmp [iHUDBorderLimit], 1
        jne originalCode
        movss xmm2, [fHUDBorderLimit]
        movss xmm0, [fHUDBorderLimit]
        jmp originalCode

        originalCode:
            movss[rax + 0x000000AC], xmm2 // Original code
            movss[rax + 0x000000A8], xmm0 // Original code
            jmp[HUDBorderLimitReturnJMP]
    }
}

void Logging()
{
    loguru::add_file("DeadIsland2Fix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");

    LOG_F(INFO, "DeadIsland2Fix v%s loaded", sFixVer.c_str());
}

void ReadConfig()
{
    // Get game name and exe path
    LPWSTR exePath = new WCHAR[_MAX_PATH];
    GetModuleFileNameW(baseModule, exePath, _MAX_PATH);
    wstring exePathWString(exePath);
    sExePath = string(exePathWString.begin(), exePathWString.end());
    wstring wsGameName = Memory::GetVersionProductName();
    sExeName = sExePath.substr(sExePath.find_last_of("/\\") + 1);
    sGameName = string(wsGameName.begin(), wsGameName.end());

    LOG_F(INFO, "Game Name: %s", sGameName.c_str());
    LOG_F(INFO, "Game Path: %s", sExePath.c_str());

    // Initialize config
    // UE4 games use launchers so config path is relative to launcher
    std::ifstream iniFile(".\\DeadIsland\\Binaries\\Win64\\DeadIsland2Fix.ini");
    if (!iniFile)
    {
        LOG_F(ERROR, "Failed to load config file.");
        LOG_F(ERROR, "Trying alternate config path.");

        std::ifstream iniFile("DeadIsland2Fix.ini");
        if (!iniFile)
        {
            LOG_F(ERROR, "Failed to load config file. (Alternate path)");
            LOG_F(ERROR, "Please ensure that the ini configuration file is in the correct place.");
        }
        else
        {
            ini.parse(iniFile);
            LOG_F(INFO, "Successfuly loaded config file. (Alternate path)");
        }
    }
    else
    {
        ini.parse(iniFile);
        LOG_F(INFO, "Successfuly loaded config file.");
    }

    LOG_F(INFO, "Game Version: %s", sGameVersion.c_str());

    inipp::get_value(ini.sections["DeadIsland2Fix Parameters"], "InjectionDelay", iInjectionDelay);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    iAspectFix = (int)bAspectFix;
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    iFOVFix = (int)bFOVFix;
    inipp::get_value(ini.sections["Fix FOV"], "AdditionalFOV", fAdditionalFOV);
    inipp::get_value(ini.sections["Remove HUD Size Limits"], "Enabled", bHUDBorderLimit);
    iHUDBorderLimit = (int)bHUDBorderLimit;

    // Custom resolution
    if (iCustomResX > 0 && iCustomResY > 0)
    {
        fNewX = (float)iCustomResX;
        fNewY = (float)iCustomResY;
        fNewAspect = (float)iCustomResX / (float)iCustomResY;
    }
    else
    {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        fNewX = (float)desktop.right;
        fNewY = (float)desktop.bottom;
        iCustomResX = (int)desktop.right;
        iCustomResY = (int)desktop.bottom;
        fNewAspect = (float)desktop.right / (float)desktop.bottom;
    }

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    LOG_F(INFO, "Config Parse: bFOVFix: %d", bFOVFix);
    LOG_F(INFO, "Config Parse: bHUDBorderLimit: %d", bHUDBorderLimit);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: fNewX: %.2f", fNewX);
    LOG_F(INFO, "Config Parse: fNewY: %.2f", fNewY);
    LOG_F(INFO, "Config Parse: fNewAspect: %.4f", fNewAspect);
}

void AspectFOVFix()
{
    if (bAspectFix || bFOVFix)
    {
        uint8_t* CurrResolutionScanResult = Memory::PatternScan(baseModule, "89 ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? 44 ?? ?? ??");
        if (CurrResolutionScanResult)
        {
            DWORD64 CurrResolutionAddress = (uintptr_t)CurrResolutionScanResult;
            int CurrResolutionHookLength = Memory::GetHookLength((char*)CurrResolutionAddress, 13);
            CurrResolutionReturnJMP = CurrResolutionAddress + CurrResolutionHookLength;
            Memory::DetourFunction64((void*)CurrResolutionAddress, CurrResolution_CC, CurrResolutionHookLength);

            LOG_F(INFO, "Current Resolution: Hook length is %d bytes", CurrResolutionHookLength);
            LOG_F(INFO, "Current Resolution: Hook address is 0x%" PRIxPTR, (uintptr_t)CurrResolutionAddress);
        }
        else if (!CurrResolutionScanResult)
        {
            LOG_F(INFO, "Current Resolution: Pattern scan failed.");
        }

        uint8_t* AspectFOVFixScanResult = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? F3 0F ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 48 ?? ?? 75 ??");
        if (AspectFOVFixScanResult)
        {
            FOVPiDiv = fPi / 360;
            FOVDivPi = 360 / fPi;

            DWORD64 AspectFOVFixAddress = (uintptr_t)AspectFOVFixScanResult + 0xA;
            int AspectFOVFixHookLength = Memory::GetHookLength((char*)AspectFOVFixAddress, 13);
            AspectFOVFixReturnJMP = AspectFOVFixAddress + AspectFOVFixHookLength;
            Memory::DetourFunction64((void*)AspectFOVFixAddress, AspectFOVFix_CC, AspectFOVFixHookLength);

            LOG_F(INFO, "Aspect Ratio/FOV: Hook length is %d bytes", AspectFOVFixHookLength);
            LOG_F(INFO, "Aspect Ratio/FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)AspectFOVFixAddress);
        }
        else if (!AspectFOVFixScanResult)
        {
            LOG_F(INFO, "Aspect Ratio/FOV: Pattern scan failed.");
        }  

      
    }
}

void HUDBorders()
{
    uint8_t* HUDBorderLimitScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 ?? ?? ?? ?? ??");
    if (HUDBorderLimitScanResult)
    {
        fHUDBorderLimit = (float)0.45; // 0.5 would result in an effectively invisible HUD so lets avoid that by limiting it a bit lower.

        DWORD64 HUDBorderLimitAddress = (uintptr_t)HUDBorderLimitScanResult;
        int HUDBorderLimitHookLength = Memory::GetHookLength((char*)HUDBorderLimitAddress, 13);
        HUDBorderLimitReturnJMP = HUDBorderLimitAddress + HUDBorderLimitHookLength;
        Memory::DetourFunction64((void*)HUDBorderLimitAddress, HUDBorderLimit_CC, HUDBorderLimitHookLength);

        LOG_F(INFO, "HUD Border Limit: Hook length is %d bytes", HUDBorderLimitHookLength);
        LOG_F(INFO, "HUD Border Limit: Hook address is 0x%" PRIxPTR, (uintptr_t)HUDBorderLimitAddress);
    }
    else if (!HUDBorderLimitScanResult)
    {
        LOG_F(INFO, "HUD Border Limit: Pattern scan failed.");
    }
}

void FOVLimit()
{
    // Disabled for now until the setting sticks correctly on startup
    bool bFOVLimit = false;
    if (bFOVLimit)
    {
        uint8_t* FOVLimitScanResult = Memory::PatternScan(baseModule, "E8 ? ? ? ? 45 33 ? 49 8B ? 49 8B ? E8 ? ? ? ? 48 8B ? 48 89");
        if (FOVLimitScanResult)
        {
            fMinFOV = (float)40;
            fMaxFOV = (float)150;

            DWORD64 FOVLimitAddress = Memory::GetAbsolute((uintptr_t)FOVLimitScanResult + 0x1);
            int FOVLimitHookLength = Memory::GetHookLength((char*)FOVLimitAddress, 13);
            FOVLimitReturnJMP = FOVLimitAddress + FOVLimitHookLength;
            Memory::DetourFunction64((void*)FOVLimitAddress, FOVLimit_CC, FOVLimitHookLength);

            LOG_F(INFO, "FOV Limit: Hook length is %d bytes", FOVLimitHookLength);
            LOG_F(INFO, "FOV Limit: Hook address is 0x%" PRIxPTR, (uintptr_t)FOVLimitAddress);
        }
        else if (!FOVLimitScanResult)
        {
            LOG_F(INFO, "FOV Limit: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    Sleep(iInjectionDelay);
    AspectFOVFix();
    HUDBorders();
    FOVLimit();
    return true; // end thread
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
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);

        if (mainHandle)
        {
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

