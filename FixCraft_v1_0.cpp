// ============================================================
//  FixCraft v1.1  —  C++ / Win32
//  Minecraft diagnostic & repair tool
//  Not affiliated with Mojang. Use at your own risk.
//
//  Build:
//    g++ fixcraft.cpp -o fixcraft.exe -mwindows -lcomctl32
//        -lcomdlg32 -lshell32 -lpsapi -lole32 -lstdc++fs
//        -std=c++17 -DUNICODE -D_UNICODE
//
//  Or with MSVC:
//    cl /std:c++17 /DUNICODE /D_UNICODE fixcraft.cpp
//       /link comctl32.lib comdlg32.lib shell32.lib psapi.lib ole32.lib
// ============================================================

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#ifdef __has_include
#  if __has_include(<Windows.h>)
#    include <Windows.h>
#  elif __has_include(<windows.h>)
#    include <windows.h>
#  else
#    error "Windows SDK headers not found. Please configure includePath for Windows SDK."
#  endif
#else
#  include <windows.h>
#endif
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <shellapi.h>

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <algorithm>
#include <functional>
#include <ctime>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "ole32.lib")

namespace fs = std::filesystem;

// ── CONTROL IDs ──────────────────────────────────────────────────────────────
#define IDC_BTN_ANALYZE         1001
#define IDC_BTN_QUICKFIX        1002
#define IDC_BTN_FULLFIX         1003
#define IDC_BTN_VIRTMEM         1004
#define IDC_BTN_SYSINFO         1005
#define IDC_BTN_CRASHLOGS       1006
#define IDC_BTN_QUICKSCAN       1007
#define IDC_BTN_FULLSCAN        1008
#define IDC_BTN_RESETCFG        1009
#define IDC_BTN_RAMPRESSURE     1010
#define IDC_BTN_OPENCRASHLOGS   1011
#define IDC_BTN_CLEAROUTPUT     1012
#define IDC_BTN_DARKMODE        1013
#define IDC_BTN_SETTINGS        1014
#define IDC_BTN_HELP            1015
#define IDC_BTN_EXIT            1016
#define IDC_OUTPUT              1017
#define IDC_STATUS              1018

// Settings window controls
#define IDC_CHK_AUTOANALYZE     2001
#define IDC_CHK_DARKMODE        2002
#define IDC_CHK_SHADER          2003
#define IDC_CHK_LAUNCHER        2004
#define IDC_CHK_RAMWARN         2005
#define IDC_EDIT_PATH           2006
#define IDC_BTN_BROWSE          2007
#define IDC_BTN_SAVE            2008
#define IDC_BTN_CANCEL          2009
#define IDC_BTN_RESETDEF        2010

// Thread-safe messages
#define WM_LOG_MESSAGE          (WM_USER + 1)
#define WM_SET_STATUS           (WM_USER + 2)
#define WM_TASK_DONE            (WM_USER + 3)

// ── COLOUR THEME ─────────────────────────────────────────────────────────────
struct Theme {
    COLORREF bg;     // window / frame background
    COLORREF bg2;    // button background
    COLORREF bg3;    // input / output area background
    COLORREF fg;     // primary text
    COLORREF fg2;    // secondary / muted text
    COLORREF selBg;  // hover / selection
    COLORREF accent; // accent highlight
    COLORREF barBg;  // top bar background
    COLORREF statusOk;
};

static const Theme DARK_THEME = {
    0x1A1A1A, 0x2B2B2B, 0x232323,
    0xE8E8E8, 0xA0A0A0, 0x3D3D3D,
    0xFF9E4A, 0x181818,
    RGB(80, 200, 100)
};

static const Theme LIGHT_THEME = {
    0xF5F5F5, 0xE8E8E8, 0xFFFFFF,
    0x111111, 0x555555, 0xD0D0D0,
    0xCC7700, 0x2C2C2C,
    RGB(0, 140, 0)
};

// ── GLOBALS ───────────────────────────────────────────────────────────────────
static HWND    g_hWnd        = nullptr;
static HWND    g_hOutput     = nullptr;
static HWND    g_hStatus     = nullptr;
static HFONT   g_hFontNormal = nullptr;
static HFONT   g_hFontBold   = nullptr;
static HFONT   g_hFontSmall  = nullptr;
static HBRUSH  g_hBrushBg    = nullptr;
static HBRUSH  g_hBrushBg2   = nullptr;
static HBRUSH  g_hBrushBg3   = nullptr;

static std::atomic<bool> g_isRunning(false);
static Theme   g_theme       = LIGHT_THEME;
static COLORREF g_statusColor = RGB(0, 140, 0);

static std::map<std::wstring, std::wstring> g_config;
static std::wstring g_configDir;
static std::wstring g_configFile;

// ── STRING UTILITIES ──────────────────────────────────────────────────────────
static std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

static bool StrContains(const std::wstring& hay, const std::wstring& needle) {
    return hay.find(needle) != std::wstring::npos;
}

static std::wstring DblToWStr(double v, int prec = 2) {
    std::wostringstream ss;
    ss.precision(prec);
    ss << std::fixed << v;
    return ss.str();
}

static std::wstring GetAppDataPath() {
    wchar_t path[MAX_PATH] = {};
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path);
    return path;
}

// ── CONFIG ────────────────────────────────────────────────────────────────────
static void InitDefaultConfig() {
    g_config[L"dark_mode"]                   = L"false";
    g_config[L"bg_color"]                    = L"#f5f5f5";
    g_config[L"minecraft_path"]              = GetAppDataPath() + L"\\.minecraft";
    g_config[L"auto_analyze"]               = L"false";
    g_config[L"clear_shader_on_quick_fix"]  = L"true";
    g_config[L"clear_launcher_on_quick_fix"]= L"true";
}

static void LoadConfig() {
    InitDefaultConfig();
    std::wifstream f(g_configFile);
    if (!f.is_open()) return;
    std::wstring line;
    bool inApp = false;
    while (std::getline(f, line)) {
        if (line == L"[app]") { inApp = true; continue; }
        if (!line.empty() && line[0] == L'[') { inApp = false; continue; }
        if (!inApp) continue;
        auto eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        g_config[line.substr(0, eq)] = line.substr(eq + 1);
    }
}

static void SaveConfig() {
    fs::create_directories(g_configDir);
    std::wofstream f(g_configFile);
    if (!f.is_open()) return;
    f << L"[app]\n";
    for (auto& [k, v] : g_config) f << k << L"=" << v << L"\n";
}

static std::wstring CfgGet(const std::wstring& k) {
    auto it = g_config.find(k);
    return it != g_config.end() ? it->second : L"";
}

static bool CfgGetBool(const std::wstring& k) {
    return ToLower(CfgGet(k)) == L"true";
}

static void CfgSet(const std::wstring& k, const std::wstring& v) { g_config[k] = v; }
static void CfgSet(const std::wstring& k, bool v) { g_config[k] = v ? L"true" : L"false"; }

// ── THEME ─────────────────────────────────────────────────────────────────────
static void DeleteBrushes() {
    if (g_hBrushBg)  { DeleteObject(g_hBrushBg);  g_hBrushBg  = nullptr; }
    if (g_hBrushBg2) { DeleteObject(g_hBrushBg2); g_hBrushBg2 = nullptr; }
    if (g_hBrushBg3) { DeleteObject(g_hBrushBg3); g_hBrushBg3 = nullptr; }
}

static void ApplyTheme(HWND hWnd) {
    g_theme = CfgGetBool(L"dark_mode") ? DARK_THEME : LIGHT_THEME;

    DeleteBrushes();
    g_hBrushBg  = CreateSolidBrush(g_theme.bg);
    g_hBrushBg2 = CreateSolidBrush(g_theme.bg2);
    g_hBrushBg3 = CreateSolidBrush(g_theme.bg3);

    if (g_hOutput)
        SendMessage(g_hOutput, EM_SETBKGNDCOLOR, 0, (LPARAM)g_theme.bg3);

    InvalidateRect(hWnd, nullptr, TRUE);
    UpdateWindow(hWnd);
    EnumChildWindows(hWnd, [](HWND h, LPARAM) -> BOOL {
        InvalidateRect(h, nullptr, TRUE);
        return TRUE;
    }, 0);
}

// ── THREAD-SAFE LOGGING ───────────────────────────────────────────────────────
static void Log(const std::wstring& msg) {
    wchar_t* buf = new wchar_t[msg.size() + 1];
    wcscpy_s(buf, msg.size() + 1, msg.c_str());
    PostMessage(g_hWnd, WM_LOG_MESSAGE, 0, (LPARAM)buf);
}

static void SetStatus(const std::wstring& msg, COLORREF color = RGB(0, 140, 0)) {
    struct Payload { std::wstring msg; COLORREF color; };
    PostMessage(g_hWnd, WM_SET_STATUS, 0, (LPARAM)(new Payload{ msg, color }));
}

static void TaskDone() {
    PostMessage(g_hWnd, WM_TASK_DONE, 0, 0);
}

// ── SYSTEM HELPERS ────────────────────────────────────────────────────────────
struct MemInfo { double totalGB, availGB; DWORD usedPct; };

static MemInfo GetMemInfo() {
    MEMORYSTATUSEX ms = {};
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    return {
        ms.ullTotalPhys / (1024.0 * 1024 * 1024),
        ms.ullAvailPhys / (1024.0 * 1024 * 1024),
        ms.dwMemoryLoad
    };
}

struct DiskInfo { double totalGB, freeGB; };

static DiskInfo GetDiskInfo() {
    ULARGE_INTEGER avail, total, free;
    GetDiskFreeSpaceExW(L"C:\\", &avail, &total, &free);
    return {
        total.QuadPart / (1024.0 * 1024 * 1024),
        free.QuadPart  / (1024.0 * 1024 * 1024)
    };
}

static DWORD GetCpuUsage() {
    static FILETIME prevIdle{}, prevKernel{}, prevUser{};
    FILETIME idle, kernel, user;
    GetSystemTimes(&idle, &kernel, &user);
    auto sub = [](FILETIME a, FILETIME b) {
        ULARGE_INTEGER ua, ub;
        ua.LowPart = a.dwLowDateTime; ua.HighPart = a.dwHighDateTime;
        ub.LowPart = b.dwLowDateTime; ub.HighPart = b.dwHighDateTime;
        return ua.QuadPart - ub.QuadPart;
    };
    ULONGLONG dI = sub(idle, prevIdle);
    ULONGLONG dK = sub(kernel, prevKernel);
    ULONGLONG dU = sub(user, prevUser);
    prevIdle = idle; prevKernel = kernel; prevUser = user;
    ULONGLONG tot = dK + dU;
    return tot == 0 ? 0 : (DWORD)(100 - dI * 100 / tot);
}

static bool IsJavaRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (StrContains(ToLower(pe.szExeFile), L"java")) { found = true; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

static std::wstring ReadFile(const std::wstring& path) {
    std::wifstream f(path);
    if (!f.is_open()) return L"";
    std::wostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static int DeleteFilesInDir(const std::wstring& dir) {
    if (!fs::exists(dir)) return 0;
    int n = 0;
    for (auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        std::error_code ec; fs::remove(e.path(), ec);
        if (!ec) n++;
    }
    return n;
}

static void RemoveDirAll(const std::wstring& dir) {
    if (!fs::exists(dir)) return;
    std::error_code ec; fs::remove_all(dir, ec);
}

static std::wstring GetLatestLog(const std::wstring& dir) {
    if (!fs::exists(dir)) return L"";
    std::wstring latest;
    fs::file_time_type latestTime;
    bool first = true;
    for (auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        if (ToLower(e.path().extension().wstring()) != L".log") continue;
        if (first || e.last_write_time() > latestTime) {
            latest = e.path().wstring();
            latestTime = e.last_write_time();
            first = false;
        }
    }
    return latest;
}

// ── TASK: SYSTEM ANALYSIS ─────────────────────────────────────────────────────
static void ThreadAnalyze() {
    Log(L"============================================================");
    Log(L"FIXCRAFT SYSTEM ANALYSIS ENGINE");
    Log(L"============================================================");

    std::wstring mcPath = CfgGet(L"minecraft_path");
    std::vector<std::wstring> decisions;

    // [1] ENVIRONMENT
    Log(L"\n[1] ENVIRONMENT CHECK");
    {
        OSVERSIONINFOEXW os = {}; os.dwOSVersionInfoSize = sizeof(os);
        typedef NTSTATUS(WINAPI* RGV)(OSVERSIONINFOW*);
        auto fn = (RGV)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlGetVersion");
        if (fn) fn((OSVERSIONINFOW*)&os);
        Log(L"  Windows " + std::to_wstring(os.dwMajorVersion) + L"." +
            std::to_wstring(os.dwMinorVersion) +
            L" Build " + std::to_wstring(os.dwBuildNumber));
    }

    // [2] RESOURCE CHECK
    Log(L"\n[2] RESOURCE CHECK");
    {
        auto mem  = GetMemInfo();
        auto disk = GetDiskInfo();
        GetCpuUsage(); Sleep(500); DWORD cpu = GetCpuUsage();

        Log(L"  RAM:  " + DblToWStr(mem.availGB) + L" GB free (" +
            std::to_wstring(mem.usedPct) + L"% used)");
        Log(L"  CPU:  " + std::to_wstring(cpu) + L"%");
        Log(L"  Disk: " + DblToWStr(disk.freeGB) + L" GB free");

        if      (mem.availGB < 2)   decisions.push_back(L"ADVISE_RAM (CRITICAL)");
        else if (mem.availGB < 4)   decisions.push_back(L"ADVISE_RAM (LOW)");
        if      (cpu > 90)          decisions.push_back(L"CPU_OVERLOAD");
        if      (disk.freeGB < 5)   decisions.push_back(L"ADVISE_DISK (CRITICAL)");
        else if (disk.freeGB < 15)  decisions.push_back(L"ADVISE_DISK (LOW)");
    }

    // [3] PATH VALIDATION
    Log(L"\n[3] PATH VALIDATION");
    if (!fs::exists(mcPath)) {
        Log(L"  [X] Minecraft path invalid: " + mcPath);
        Log(L"  -> Fix path in Settings and re-run Analysis.");
        SetStatus(L"Analysis failed — bad path", RGB(200, 0, 0));
        TaskDone(); return;
    }
    Log(L"  [OK] Minecraft path valid");

    // [4] FILE INTEGRITY
    Log(L"\n[4] FILE INTEGRITY");
    {
        std::wstring opt = mcPath + L"\\options.txt";
        if (!fs::exists(opt)) {
            Log(L"  [!] Missing: options.txt");
            decisions.push_back(L"RESET_CONFIG");
        } else if (fs::file_size(opt) == 0) {
            Log(L"  [X] Corrupt (empty): options.txt");
            decisions.push_back(L"RESET_CONFIG");
        } else {
            Log(L"  [OK] options.txt");
        }
    }

    // [5] RUNTIME RISK
    Log(L"\n[5] RUNTIME RISK DETECTION");
    {
        std::wstring modsPath = mcPath + L"\\mods";
        if (fs::exists(modsPath)) {
            int mc = 0;
            for (auto& e : fs::directory_iterator(modsPath))
                if (e.is_regular_file()) mc++;
            if (mc > 20) {
                Log(L"  [!] Heavy mod load (" + std::to_wstring(mc) + L" mods)");
                decisions.push_back(L"CHECK_MODS");
            } else if (mc > 0) {
                Log(L"  [OK] Mods: " + std::to_wstring(mc));
            }
        }
        if (IsJavaRunning()) Log(L"  [!] Java process active");
        else                 Log(L"  [OK] No Java process running");
    }

    // [6] LOG INTELLIGENCE
    Log(L"\n[6] LOG INTELLIGENCE");
    {
        std::wstring latest = GetLatestLog(mcPath + L"\\logs");
        if (latest.empty()) {
            Log(L"  [OK] No log files found");
        } else {
            std::wstring c = ToLower(ReadFile(latest));
            if (StrContains(c, L"outofmemory")) {
                Log(L"  [X] OutOfMemory in log");
                decisions.push_back(L"INCREASE_MEMORY");
            }
            if (StrContains(c, L"exception")) Log(L"  [!] Exception in log");
            if (StrContains(c, L"failed"))    Log(L"  [!] Failure in log");
            if (!StrContains(c, L"outofmemory") &&
                !StrContains(c, L"exception") &&
                !StrContains(c, L"failed"))   Log(L"  [OK] Log looks clean");
        }
    }

    // [7] DECISION ENGINE
    Log(L"\n[7] FIX DECISION ENGINE");
    if (decisions.empty()) {
        Log(L"  [OK] No major actions required");
    } else {
        Log(L"  Recommended Actions:");
        for (auto& d : decisions) Log(L"  -> " + d);
    }

    Log(L"\nAnalysis Complete.");
    SetStatus(L"Analysis complete", g_theme.statusOk);
    TaskDone();
}

// ── TASK: QUICK FIX ───────────────────────────────────────────────────────────
static void ThreadQuickFix() {
    Log(L"============================================================");
    Log(L"FIXCRAFT QUICK FIX ENGINE");
    Log(L"============================================================");

    std::wstring mcPath = CfgGet(L"minecraft_path");
    std::vector<std::wstring> done;

    Log(L"\n[1] RE-CHECK SYSTEM STATE");
    auto mem  = GetMemInfo();
    auto disk = GetDiskInfo();
    bool ramLow  = mem.availGB < 4;
    bool diskLow = disk.freeGB < 15;
    if (ramLow)  Log(L"  [!] Low RAM detected");
    if (diskLow) Log(L"  [!] Low disk space");
    if (!ramLow && !diskLow) Log(L"  [OK] Resources OK");

    Log(L"\n[2] PATH VALIDATION");
    if (!fs::exists(mcPath)) {
        Log(L"  [X] Minecraft path invalid — aborting");
        SetStatus(L"Quick fix aborted", RGB(200, 0, 0));
        TaskDone(); return;
    }
    Log(L"  [OK] Path valid");

    Log(L"\n[3] SAFE CLEANUP");
    {
        int n = DeleteFilesInDir(mcPath + L"\\logs");
        Log(L"  [OK] Removed " + std::to_wstring(n) + L" log files");
        if (n > 0) done.push_back(L"LOGS_CLEARED");
    }
    for (auto& sub : { std::wstring(L"shadercache"), std::wstring(L"cache") }) {
        std::wstring p = mcPath + L"\\" + sub;
        if (fs::exists(p)) { RemoveDirAll(p); Log(L"  [OK] Cleared: " + sub); done.push_back(L"CACHE_CLEARED"); }
    }

    Log(L"\n[4] CONFIG CHECK");
    {
        std::wstring cfg = mcPath + L"\\options.txt";
        if (fs::exists(cfg) && fs::file_size(cfg) == 0) {
            Log(L"  [X] options.txt corrupt (empty) — will regenerate on next launch");
            done.push_back(L"CONFIG_CORRUPT_NOTED");
        } else {
            Log(L"  [OK] options.txt OK");
        }
    }

    Log(L"\n[5] MEMORY RESPONSE");
    if (ramLow) { Log(L"  [!] Close browsers / background apps"); done.push_back(L"RAM_WARNING"); }
    else         Log(L"  [OK] Memory pressure acceptable");

    Log(L"\n============================================================");
    Log(L"QUICK FIX COMPLETE");
    Log(L"============================================================");
    for (auto& a : done) Log(L"  -> " + a);
    if (done.empty()) Log(L"  [OK] No fixes needed");

    SetStatus(L"Quick fix complete", g_theme.statusOk);
    TaskDone();
}

// ── TASK: FULL FIX ────────────────────────────────────────────────────────────
static void ThreadFullFix() {
    Log(L"============================================================");
    Log(L"FIXCRAFT FULL FIX ENGINE");
    Log(L"============================================================");

    std::wstring mcPath = CfgGet(L"minecraft_path");
    std::vector<std::wstring> actions;

    Log(L"\n[1] SAFETY CHECK");
    auto mem  = GetMemInfo();
    auto disk = GetDiskInfo();
    if (!fs::exists(mcPath)) {
        Log(L"  [X] Minecraft path invalid — aborting");
        SetStatus(L"Full fix aborted", RGB(200, 0, 0));
        TaskDone(); return;
    }
    Log(L"  [OK] Environment safe");

    Log(L"\n[2] WIPE LOGS");
    {
        int n = DeleteFilesInDir(mcPath + L"\\logs");
        Log(L"  [OK] Logs wiped (" + std::to_wstring(n) + L" files)");
        if (n > 0) actions.push_back(L"LOGS_WIPED");
    }

    Log(L"\n[3] WIPE CRASH REPORTS");
    {
        int n = DeleteFilesInDir(mcPath + L"\\crash-reports");
        Log(L"  [OK] Crash reports removed (" + std::to_wstring(n) + L")");
        if (n > 0) actions.push_back(L"CRASH_REPORTS_CLEARED");
    }

    Log(L"\n[4] CACHE CLEANUP");
    for (auto& sub : { std::wstring(L"cache"), std::wstring(L"shadercache"), std::wstring(L"caches") }) {
        std::wstring p = mcPath + L"\\" + sub;
        if (fs::exists(p)) { RemoveDirAll(p); Log(L"  [OK] Removed: " + sub); actions.push_back(L"CACHE_REMOVED_" + sub); }
    }

    Log(L"\n[5] CONFIG REPAIR");
    {
        std::wstring cfg = mcPath + L"\\options.txt";
        if (fs::exists(cfg)) {
            if (fs::file_size(cfg) == 0) {
                fs::remove(cfg);
                Log(L"  [OK] options.txt was empty — deleted (will regenerate)");
                actions.push_back(L"CONFIG_RESET_EMPTY");
            } else {
                std::error_code ec;
                fs::copy_file(cfg, cfg + L".bak", fs::copy_options::overwrite_existing, ec);
                if (!ec) { Log(L"  [OK] options.txt backed up"); actions.push_back(L"CONFIG_BACKED_UP"); }
            }
        }
    }

    Log(L"\n[6] RESOURCE RESPONSE");
    if (mem.availGB < 4) { Log(L"  [!] LOW RAM — performance may still degrade"); actions.push_back(L"LOW_RAM_WARNING"); }
    if (disk.freeGB < 10){ Log(L"  [!] LOW DISK — consider cleanup");              actions.push_back(L"LOW_DISK_WARNING"); }
    if (mem.availGB >= 4 && disk.freeGB >= 10) Log(L"  [OK] Resources OK");

    Log(L"\n[7] FINAL VALIDATION");
    auto mem2 = GetMemInfo();
    Log(mem2.usedPct < 70 ? L"  [OK] System load improved" : L"  [!] System still under pressure");

    Log(L"\n============================================================");
    Log(L"FULL FIX COMPLETE");
    Log(L"============================================================");
    for (auto& a : actions) Log(L"  -> " + a);
    if (actions.empty()) Log(L"  [OK] No actions needed");

    SetStatus(L"Full fix complete", g_theme.statusOk);
    TaskDone();
}

// ── TASK: VIRTUAL MEMORY ──────────────────────────────────────────────────────
static void ThreadVirtMem() {
    Log(L"============================================================");
    Log(L"VIRTUAL MEMORY OPTIMIZATION ENGINE");
    Log(L"============================================================");

    auto mem  = GetMemInfo();
    auto disk = GetDiskInfo();

    Log(L"\n[1] RAM Status");
    Log(L"  Total:     " + DblToWStr(mem.totalGB, 1) + L" GB");
    Log(L"  Available: " + DblToWStr(mem.availGB, 1) + L" GB");
    Log(L"  Usage:     " + std::to_wstring(mem.usedPct) + L"%");

    if      (mem.usedPct > 85) Log(L"  [!] High memory pressure");
    else if (mem.usedPct < 50) Log(L"  [OK] Memory usage healthy");
    else                        Log(L"  Moderate usage");

    Log(L"\n[2] Disk Support");
    Log(L"  Free: " + DblToWStr(disk.freeGB, 1) + L" GB");
    if (disk.freeGB < 5) Log(L"  [!] Low disk — may affect paging file");

    int minMB = (int)(mem.totalGB * 1.5 * 1024);
    int maxMB = (int)(mem.totalGB * 3.0 * 1024);

    Log(L"\n[3] Recommended Paging File");
    Log(L"  Min: " + std::to_wstring(minMB) + L" MB  (1.5x RAM)");
    Log(L"  Max: " + std::to_wstring(maxMB) + L" MB  (3x RAM)");

    Log(L"\n[4] How to Apply");
    if (mem.usedPct > 85 || mem.availGB < 2) {
        Log(L"  [!] ACTION REQUIRED");
        Log(L"  -> System Properties -> Advanced -> Performance -> Settings");
        Log(L"  -> Advanced -> Virtual Memory -> Change");
        Log(L"  -> Uncheck 'Automatically manage paging file size'");
        Log(L"  -> Select C: drive, choose Custom size");
        Log(L"  -> Initial: " + std::to_wstring(minMB) + L" MB");
        Log(L"  -> Maximum: " + std::to_wstring(maxMB) + L" MB");
        Log(L"  -> Click Set -> OK -> Restart");
    } else {
        Log(L"  [OK] Memory stable — increase is optional");
        Log(L"  Optional: Min " + std::to_wstring(minMB) + L" MB / Max " + std::to_wstring(maxMB) + L" MB");
    }

    Log(L"\n============================================================");
    Log(L"VIRTUAL MEMORY ANALYSIS COMPLETE");
    Log(L"============================================================");
    SetStatus(L"Virtual memory check done", g_theme.statusOk);
    TaskDone();
}

// ── TASK: SYSTEM INFO ─────────────────────────────────────────────────────────
static void ThreadSysInfo() {
    Log(L"============================================================");
    Log(L"SYSTEM DIAGNOSTIC ENGINE");
    Log(L"============================================================");

    Log(L"\n[1] OS");
    {
        OSVERSIONINFOEXW os = {}; os.dwOSVersionInfoSize = sizeof(os);
        typedef NTSTATUS(WINAPI* RGV)(OSVERSIONINFOW*);
        auto fn = (RGV)GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlGetVersion");
        if (fn) fn((OSVERSIONINFOW*)&os);
        SYSTEM_INFO si = {}; GetSystemInfo(&si);
        Log(L"  Windows " + std::to_wstring(os.dwMajorVersion) + L"." +
            std::to_wstring(os.dwMinorVersion) + L" Build " + std::to_wstring(os.dwBuildNumber));
        Log(L"  Logical CPUs: " + std::to_wstring(si.dwNumberOfProcessors));
    }

    Log(L"\n[2] Resources");
    {
        auto mem  = GetMemInfo();
        auto disk = GetDiskInfo();
        GetCpuUsage(); Sleep(500); DWORD cpu = GetCpuUsage();
        Log(L"  CPU:        " + std::to_wstring(cpu) + L"%");
        Log(L"  RAM Total:  " + DblToWStr(mem.totalGB) + L" GB");
        Log(L"  RAM Avail:  " + DblToWStr(mem.availGB) + L" GB  (" + std::to_wstring(mem.usedPct) + L"% used)");
        Log(L"  Disk Total: " + DblToWStr(disk.totalGB, 1) + L" GB");
        Log(L"  Disk Free:  " + DblToWStr(disk.freeGB,  1) + L" GB");
    }

    Log(L"\n[3] GPU");
    {
        HKEY hKey;
        bool found = false;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Class\\"
            L"{4d36e968-e325-11ce-bfc1-08002be10318}\\0000",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t gpuName[256] = {}; DWORD sz = sizeof(gpuName);
            if (RegQueryValueExW(hKey, L"DriverDesc", nullptr, nullptr,
                                 (LPBYTE)gpuName, &sz) == ERROR_SUCCESS) {
                Log(L"  " + std::wstring(gpuName));
                found = true;
            }
            RegCloseKey(hKey);
        }
        if (!found) Log(L"  GPU info unavailable");
    }

    Log(L"\n[4] Risk Assessment");
    {
        auto mem = GetMemInfo();
        GetCpuUsage(); Sleep(300); DWORD cpu = GetCpuUsage();
        auto disk = GetDiskInfo();
        int risk = 0;
        if (cpu > 85)         { Log(L"  [X] High CPU");   risk += 2; }
        if (mem.usedPct > 85) { Log(L"  [X] High RAM");   risk += 2; }
        if (disk.freeGB < 10) { Log(L"  [!] Low disk");   risk += 1; }
        if (risk == 0)        Log(L"  [OK] System stable");
        int health = max(0, 100 - risk * 20);
        Log(L"\n  Health Score: " + std::to_wstring(health) + L"/100");
        if      (health >= 80) Log(L"  [OK] Excellent condition");
        else if (health >= 50) Log(L"  [!] Moderate — optimise recommended");
        else                   Log(L"  [X] Poor — fixes recommended");
    }

    Log(L"\n============================================================");
    Log(L"SYSTEM INFO COMPLETE");
    Log(L"============================================================");
    SetStatus(L"System info complete", g_theme.statusOk);
    TaskDone();
}

// ── TASK: CRASH LOG ANALYSIS ──────────────────────────────────────────────────
static void ThreadCrashLogs() {
    Log(L"============================================================");
    Log(L"CRASH LOG FORENSIC ENGINE");
    Log(L"============================================================");

    auto mem = GetMemInfo();
    GetCpuUsage(); Sleep(300); DWORD cpu = GetCpuUsage();
    Log(L"\n[1] System Snapshot");
    Log(L"  CPU: " + std::to_wstring(cpu) + L"%   RAM: " + std::to_wstring(mem.usedPct) + L"%   Avail: " + DblToWStr(mem.availGB) + L" GB");

    Log(L"\n[2] Log Path");
    std::wstring logDir = CfgGet(L"minecraft_path") + L"\\logs";
    if (!fs::exists(logDir)) {
        Log(L"  [X] Log directory not found — Minecraft may not be installed");
        SetStatus(L"No logs found", RGB(200, 150, 0));
        TaskDone(); return;
    }
    Log(L"  [OK] " + logDir);

    Log(L"\n[3] Log Discovery");
    std::wstring latest = GetLatestLog(logDir);
    if (latest.empty()) {
        Log(L"  [OK] No crash logs found (clean state)");
        SetStatus(L"Logs clean", g_theme.statusOk);
        TaskDone(); return;
    }
    Log(L"  Analysing: " + latest);

    Log(L"\n[4] Pattern Detection");
    std::wstring c = ToLower(ReadFile(latest));
    struct Issue { std::wstring name; int score; };
    std::vector<Issue> issues = {
        {L"RAM", 0}, {L"GPU", 0}, {L"SHADER", 0},
        {L"CONFIG", 0}, {L"ENGINE", 0}, {L"CORRUPTION", 0}
    };

    if (StrContains(c, L"outofmemory") || StrContains(c, L"out of memory"))
        { issues[0].score += 2; Log(L"  [X] RAM CRASH DETECTED"); }
    if (StrContains(c, L"opengl") || StrContains(c, L"lwjgl"))
        { issues[1].score += 2; Log(L"  [X] GPU / GRAPHICS FAILURE"); }
    if (StrContains(c, L"shader"))
        { issues[2].score += 2; Log(L"  [!] SHADER ISSUE"); }
    if (StrContains(c, L"options") || StrContains(c, L"config"))
        { issues[3].score += 1; Log(L"  [!] CONFIG ISSUE"); }
    if (StrContains(c, L"fatal") || StrContains(c, L"crash"))
        { issues[4].score += 2; Log(L"  [X] ENGINE CRASH"); }
    if (StrContains(c, L"corrupt"))
        { issues[5].score += 2; Log(L"  [X] CORRUPTION"); }

    Log(L"\n[5] Severity");
    int total = 0;
    for (auto& i : issues) { total += i.score; Log(L"  " + i.name + L": " + (i.score>=2?L"HIGH":i.score==1?L"LOW":L"NONE")); }

    std::wstring sev = total>=6 ? L"CRITICAL" : total>=3 ? L"MODERATE" : total>0 ? L"LOW" : L"CLEAN";
    Log(L"\n  Severity: " + sev);

    Log(L"\n[6] Recommended Actions");
    if (issues[0].score >= 2) Log(L"  -> Increase Virtual Memory");
    if (issues[1].score >= 2) Log(L"  -> Update GPU drivers");
    if (issues[2].score >= 2) Log(L"  -> Clear Shader Cache");
    if (issues[3].score >= 1) Log(L"  -> Reset Minecraft config files");
    if (issues[4].score >= 2) Log(L"  -> Verify Minecraft files in Mojang Launcher");
    if (issues[5].score >= 2) Log(L"  -> Check world/mod files for corruption");
    if (total == 0)           Log(L"  [OK] No issues detected");

    Log(L"\n============================================================");
    Log(L"CRASH ANALYSIS COMPLETE");
    Log(L"============================================================");
    SetStatus(L"Crash analysis complete", g_theme.statusOk);
    TaskDone();
}

// ── TASK: QUICK SCAN ──────────────────────────────────────────────────────────
static void ThreadQuickScan() {
    Log(L"============================================================");
    Log(L"QUICK SCAN ENGINE");
    Log(L"============================================================");

    auto mem = GetMemInfo();
    GetCpuUsage(); Sleep(200); DWORD cpu = GetCpuUsage();

    Log(L"\n[1] Resources");
    Log(L"  CPU: " + std::to_wstring(cpu) + L"%");
    Log(L"  RAM: " + std::to_wstring(mem.usedPct) + L"%  (" + DblToWStr(mem.availGB) + L" GB free)");

    Log(L"\n[2] Game Path");
    std::wstring mcPath = CfgGet(L"minecraft_path");
    Log(fs::exists(mcPath) ? L"  [OK] Minecraft path valid" : L"  [X] Minecraft path missing");

    Log(L"\n[3] Risk Detection");
    int risk = 0;
    if (mem.usedPct > 85) { Log(L"  [X] High RAM usage");  risk += 2; }
    if (cpu > 85)         { Log(L"  [X] High CPU usage");  risk += 2; }
    if (mem.availGB < 2)  { Log(L"  [!] Low available RAM"); risk += 1; }
    if (risk == 0)        Log(L"  [OK] System stable");

    std::wstring chance =
        risk >= 4 ? L"HIGH (70-90%)" :
        risk == 3 ? L"MEDIUM (40-60%)" :
        risk >  0 ? L"LOW (10-30%)" : L"VERY LOW (<10%)";
    Log(L"\n  Crash Probability: " + chance);

    Log(L"\n[4] Suggestions");
    if (mem.usedPct > 85) Log(L"  -> Close background apps");
    if (cpu > 85)         Log(L"  -> Reduce CPU load");
    if (mem.availGB < 2)  Log(L"  -> Free up system memory");
    if (!fs::exists(mcPath)) Log(L"  -> Fix Minecraft path in Settings");

    int score = max(0, 100 - risk * 25);
    Log(L"\n  Health Score: " + std::to_wstring(score) + L"/100");
    Log(score >= 80 ? L"  [OK] Healthy" : score >= 50 ? L"  [!] Moderate" : L"  [X] Poor — action recommended");

    Log(L"\n============================================================");
    Log(L"QUICK SCAN COMPLETE");
    Log(L"============================================================");
    SetStatus(L"Quick scan complete", g_theme.statusOk);
    TaskDone();
}

// ── TASK: FULL SCAN ───────────────────────────────────────────────────────────
static void ThreadFullScan() {
    Log(L"============================================================");
    Log(L"FULL SYSTEM FORENSIC SCAN");
    Log(L"============================================================");

    auto mem  = GetMemInfo();
    auto disk = GetDiskInfo();
    GetCpuUsage(); Sleep(500); DWORD cpu = GetCpuUsage();

    Log(L"\n[1] Resources");
    Log(L"  CPU: " + std::to_wstring(cpu) + L"%   RAM: " + std::to_wstring(mem.usedPct) + L"%   Disk Free: " + DblToWStr(disk.freeGB) + L" GB");

    Log(L"\n[2] Game Path Integrity");
    std::wstring mcPath = CfgGet(L"minecraft_path");
    std::vector<std::wstring> missing;
    if (fs::exists(mcPath)) {
        Log(L"  [OK] Minecraft found");
        for (auto& sub : { std::wstring(L"saves"), std::wstring(L"mods") })
            if (!fs::exists(mcPath + L"\\" + sub)) missing.push_back(sub);
        if (!missing.empty()) {
            std::wstring m; for (auto& s : missing) m += s + L" ";
            Log(L"  [!] Missing: " + m);
        } else Log(L"  [OK] Core structure intact");
    } else {
        Log(L"  [X] Minecraft path invalid");
        missing.push_back(L"installation");
    }

    Log(L"\n[3] File System");
    {
        std::wstring logsPath = mcPath + L"\\logs";
        if (fs::exists(logsPath)) {
            int lc = 0;
            for (auto& e : fs::directory_iterator(logsPath)) if (e.is_regular_file()) lc++;
            Log(L"  Log files: " + std::to_wstring(lc));
            if (lc > 20) Log(L"  [!] Excessive log accumulation");
        } else Log(L"  [!] Log directory missing");

        Log(fs::exists(mcPath + L"\\options.txt") ?
            L"  [OK] options.txt found" : L"  [X] options.txt missing");
        Log(fs::exists(mcPath + L"\\shadercache") ?
            L"  [OK] Shader cache exists" : L"  [!] Shader cache missing");
    }

    Log(L"\n[4] Risk Correlation");
    int risk = 0;
    if (mem.usedPct > 85)  { Log(L"  [X] RAM overload");    risk += 2; }
    if (cpu > 85)          { Log(L"  [X] CPU overload");    risk += 2; }
    if (disk.freeGB < 10)  { Log(L"  [!] Low disk");        risk += 1; }
    if (!missing.empty())  { Log(L"  [X] Game structure risk"); risk += 2; }

    Log(L"\n[5] Behavioral Analysis");
    {
        std::wstring logsPath = mcPath + L"\\logs";
        if (fs::exists(logsPath)) {
            int cc = 0;
            for (auto& e : fs::directory_iterator(logsPath))
                if (StrContains(ToLower(e.path().filename().wstring()), L"crash")) cc++;
            if (cc > 0) { Log(L"  [!] Crash logs: " + std::to_wstring(cc)); risk++; }
            else         Log(L"  [OK] No crash patterns");
        }
    }

    int score = max(0, 100 - risk * 20);
    Log(L"\n[6] Final Report");
    Log(L"  Stability Score: " + std::to_wstring(score) + L"/100");
    Log(score >= 80 ? L"  [OK] System stable" : score >= 50 ? L"  [!] Moderate — optimise" : L"  [X] Critical — full fix recommended");
    Log(score < 50  ? L"  -> Run FULL FIX immediately" :
        score < 80  ? L"  -> Run QUICK FIX + clear cache" :
                      L"  -> Optional maintenance only");

    Log(L"\n============================================================");
    Log(L"FULL SCAN COMPLETE");
    Log(L"============================================================");
    SetStatus(L"Full scan complete", g_theme.statusOk);
    TaskDone();
}

// ── TASK: RESET CONFIG ────────────────────────────────────────────────────────
static void ThreadResetConfig() {
    Log(L"=== CONFIG RESET ENGINE ===");

    std::wstring basePath = CfgGet(L"minecraft_path") + L"\\config";
    Log(L"Target: " + basePath);

    if (!fs::exists(basePath)) {
        Log(L"[X] Config directory not found — Minecraft may not be installed");
        SetStatus(L"Config path missing", RGB(200, 0, 0));
        TaskDone(); return;
    }

    time_t now = time(nullptr); tm tm_b = {}; localtime_s(&tm_b, &now);
    wchar_t ts[32]; wcsftime(ts, 32, L"%Y%m%d_%H%M%S", &tm_b);
    std::wstring backupDir = g_configDir + L"\\backup_configs\\backup_" + ts;
    fs::create_directories(backupDir);
    Log(L"[OK] Backup folder created");

    int backed=0, deleted=0, failed=0, missing=0;
    for (auto& file : { std::wstring(L"options.txt"), std::wstring(L"servers.dat") }) {
        std::wstring fp = basePath + L"\\" + file;
        if (fs::exists(fp)) {
            std::error_code ec;
            fs::copy_file(fp, backupDir + L"\\" + file, fs::copy_options::overwrite_existing, ec);
            if (!ec) { backed++; Log(L"  Backed up: " + file); }
            fs::remove(fp, ec);
            if (!ec) { deleted++; Log(L"  Deleted:   " + file); }
            else      { failed++;  Log(L"  [X] Failed: " + file); }
        } else { missing++; Log(L"  Missing:   " + file); }
    }

    Log(L"\nSummary — Backed: " + std::to_wstring(backed) +
        L"  Deleted: " + std::to_wstring(deleted) +
        L"  Missing: " + std::to_wstring(missing) +
        L"  Failed: "  + std::to_wstring(failed));

    if      (failed == 0) { Log(L"[OK] CONFIG RESET COMPLETE"); SetStatus(L"Reset successful",  g_theme.statusOk); }
    else if (failed < 2)  { Log(L"[!] Partial reset");          SetStatus(L"Partial reset",      RGB(200, 120, 0)); }
    else                  { Log(L"[X] No files modified");       SetStatus(L"Reset failed",       RGB(200, 0, 0)); }

    TaskDone();
}

// ── TASK: RAM PRESSURE ────────────────────────────────────────────────────────
static void ThreadRamPressure() {
    Log(L"=== RAM PRESSURE ANALYSIS ===");

    auto mem = GetMemInfo();
    Log(L"  Total:     " + DblToWStr(mem.totalGB, 1) + L" GB");
    Log(L"  Available: " + DblToWStr(mem.availGB, 1) + L" GB");
    Log(L"  Usage:     " + std::to_wstring(mem.usedPct) + L"%");

    Log(L"\nTop Memory Consumers:");
    struct ProcMem { std::wstring name; SIZE_T rss; };
    std::vector<ProcMem> procs;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                if (h) {
                    PROCESS_MEMORY_COUNTERS pmc = {};
                    if (GetProcessMemoryInfo(h, &pmc, sizeof(pmc)))
                        procs.push_back({ pe.szExeFile, pmc.WorkingSetSize });
                    CloseHandle(h);
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
    std::sort(procs.begin(), procs.end(), [](auto& a, auto& b){ return a.rss > b.rss; });
    int shown = 0;
    for (auto& p : procs) {
        if (shown++ >= 5) break;
        Log(L"  " + p.name + L": " + DblToWStr(p.rss / (1024.0 * 1024), 1) + L" MB");
    }

    std::wstring level;
    COLORREF col;
    std::vector<std::wstring> recs;

    if (mem.usedPct >= 90) {
        level = L"CRITICAL"; col = RGB(200, 0, 0);
        recs = { L"Close background apps immediately",
                 L"Restart before launching Minecraft",
                 L"Disable overlays (Discord / Xbox)" };
    } else if (mem.usedPct >= 75) {
        level = L"WARNING"; col = RGB(200, 120, 0);
        recs = { L"Close browser tabs", L"Avoid background downloads",
                 L"Restart Minecraft if stuttering" };
    } else if (mem.usedPct >= 60) {
        level = L"MODERATE"; col = RGB(180, 160, 0);
        recs = { L"System stable but slightly loaded", L"Monitor background apps" };
    } else {
        level = L"SAFE"; col = g_theme.statusOk;
        recs = { L"System is optimal for Minecraft" };
    }
    if (mem.availGB < 3) recs.push_back(L"Low free RAM — crash risk increased");

    Log(L"\n  STATUS: " + level);
    for (auto& r : recs) Log(L"  -> " + r);

    SetStatus(L"RAM: " + level, col);
    TaskDone();
}

// ── RUN TASK ──────────────────────────────────────────────────────────────────
static void RunTask(std::function<void()> fn) {
    if (g_isRunning.exchange(true)) return;
    SetWindowTextW(g_hOutput, L"");
    std::thread([fn]{ fn(); }).detach();
}

// ── SETTINGS WINDOW ───────────────────────────────────────────────────────────
static void ShowSettingsWindow(HWND hParent) {
    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, L"STATIC", L"",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 440,
        hParent, nullptr, GetModuleHandleW(nullptr), nullptr);
    SetWindowTextW(hDlg, L"FixCraft Settings");

    auto mkCheck = [&](const wchar_t* txt, int id, int y, bool v) {
        HWND h = CreateWindowW(L"BUTTON", txt,
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            20, y, 440, 22, hDlg, (HMENU)(UINT_PTR)id, nullptr, nullptr);
        if (v) SendMessage(h, BM_SETCHECK, BST_CHECKED, 0);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        return h;
    };
    auto mkBtn = [&](const wchar_t* txt, int id, int x, int y, int w) {
        HWND h = CreateWindowW(L"BUTTON", txt,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            x, y, w, 28, hDlg, (HMENU)(UINT_PTR)id, nullptr, nullptr);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        return h;
    };

    int y = 14;
    auto mkHdr = [&](const wchar_t* txt) {
        HWND h = CreateWindowW(L"STATIC", txt, WS_CHILD | WS_VISIBLE,
            12, y, 460, 18, hDlg, nullptr, nullptr, nullptr);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
        y += 24;
    };

    mkHdr(L"General");
    mkCheck(L"Auto Analyze on Startup",          IDC_CHK_AUTOANALYZE, y, CfgGetBool(L"auto_analyze")); y += 28;
    mkCheck(L"Enable Dark Mode",                 IDC_CHK_DARKMODE,    y, CfgGetBool(L"dark_mode"));    y += 28;
    y += 8;
    mkHdr(L"Quick Fix Behaviour");
    mkCheck(L"Clear Shader Cache on Quick Fix",  IDC_CHK_SHADER,      y, CfgGetBool(L"clear_shader_on_quick_fix"));   y += 28;
    mkCheck(L"Clear Launcher Cache on Quick Fix",IDC_CHK_LAUNCHER,    y, CfgGetBool(L"clear_launcher_on_quick_fix")); y += 28;
    mkCheck(L"Enable RAM Pressure Warnings",     IDC_CHK_RAMWARN,     y, true); y += 36;

    mkHdr(L"Minecraft Path (.minecraft folder)");
    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT",
        CfgGet(L"minecraft_path").c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        12, y, 354, 26, hDlg, (HMENU)IDC_EDIT_PATH, nullptr, nullptr);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    mkBtn(L"Browse", IDC_BTN_BROWSE, 372, y, 100); y += 44;

    mkBtn(L"Save",           IDC_BTN_SAVE,    12,  y, 100);
    mkBtn(L"Cancel",         IDC_BTN_CANCEL,  120, y, 100);
    mkBtn(L"Reset Defaults", IDC_BTN_RESETDEF,230, y, 140);

    ShowWindow(hDlg, SW_SHOW); UpdateWindow(hDlg);

    bool done = false;
    MSG m;
    while (!done && GetMessageW(&m, nullptr, 0, 0)) {
        if (m.hwnd == hDlg && m.message == WM_COMMAND) {
            int id = LOWORD(m.wParam);
            if (id == IDC_BTN_BROWSE) {
                BROWSEINFOW bi = {}; bi.hwndOwner = hDlg;
                bi.lpszTitle = L"Select .minecraft folder";
                bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
                if (pidl) {
                    wchar_t sel[MAX_PATH] = {}; SHGetPathFromIDListW(pidl, sel);
                    SetWindowTextW(hEdit, sel); CoTaskMemFree(pidl);
                }
            } else if (id == IDC_BTN_SAVE) {
                wchar_t path[MAX_PATH] = {}; GetWindowTextW(hEdit, path, MAX_PATH);
                if (!fs::exists(path)) {
                    MessageBoxW(hDlg, L"Path does not exist.", L"Error", MB_ICONERROR);
                } else {
                    auto chk = [&](int ctrl){ return SendMessage(GetDlgItem(hDlg, ctrl), BM_GETCHECK, 0, 0) == BST_CHECKED; };
                    CfgSet(L"auto_analyze",               chk(IDC_CHK_AUTOANALYZE));
                    CfgSet(L"dark_mode",                  chk(IDC_CHK_DARKMODE));
                    CfgSet(L"clear_shader_on_quick_fix",  chk(IDC_CHK_SHADER));
                    CfgSet(L"clear_launcher_on_quick_fix",chk(IDC_CHK_LAUNCHER));
                    CfgSet(L"minecraft_path",             std::wstring(path));
                    SaveConfig(); ApplyTheme(g_hWnd);
                    done = true;
                }
            } else if (id == IDC_BTN_CANCEL) {
                done = true;
            } else if (id == IDC_BTN_RESETDEF) {
                if (MessageBoxW(hDlg, L"Reset all settings to defaults?",
                                L"Reset Defaults", MB_YESNO) == IDYES) {
                    InitDefaultConfig(); SaveConfig(); ApplyTheme(g_hWnd); done = true;
                }
            }
        } else if (m.message == WM_CLOSE) { done = true; }
        else { TranslateMessage(&m); DispatchMessageW(&m); }
    }
    DestroyWindow(hDlg);
}

// ── HELP WINDOW ───────────────────────────────────────────────────────────────
static void ShowHelpWindow(HWND hParent) {
    HWND hWin = CreateWindowExW(
        WS_EX_DLGMODALFRAME, L"STATIC", L"",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 660, 640,
        hParent, nullptr, GetModuleHandleW(nullptr), nullptr);
    SetWindowTextW(hWin, L"FixCraft v1.1 — Help & FAQ");

    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        6, 6, 630, 570, hWin, nullptr, nullptr, nullptr);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);

    SetWindowTextW(hEdit,
L"FixCraft v1.1 — Help & FAQ\r\n"
L"Not affiliated with Mojang. Use at your own risk.\r\n"
L"============================================================\r\n\r\n"
L"WHAT IS FIXCRAFT?\r\n"
L"  Diagnostic and repair tool for Minecraft on Windows.\r\n"
L"  Analyses RAM, disk, logs, mods, config files.\r\n"
L"  Does NOT modify your account or worlds.\r\n\r\n"
L"BUTTONS\r\n"
L"  System Analysis     Full diagnostic scan with recommendations\r\n"
L"  Quick Fix           Safe cleanup (logs, cache)\r\n"
L"  Full Fix            Deep clean (logs, crash reports, cache, config backup)\r\n"
L"  Increase Virt Mem   Guides paging file setup\r\n"
L"  System Info         CPU / RAM / GPU / disk snapshot\r\n"
L"  Analyse Crash Logs  Pattern detection in latest .log\r\n"
L"  Quick Scan          Fast resource check + crash probability\r\n"
L"  Full Scan           Forensic scan with stability score\r\n"
L"  Reset Config Files  Backs up + deletes options.txt & servers.dat\r\n"
L"  Check RAM Pressure  Live RAM usage + top process list\r\n"
L"  Open Crash Logs     Opens .minecraft\\crash-reports in Explorer\r\n"
L"  Clear Output        Clears the output box\r\n\r\n"
L"RECOMMENDED ORDER\r\n"
L"  1. System Analysis\r\n"
L"  2. RAM Pressure Check\r\n"
L"  3. Analyse Crash Logs\r\n"
L"  4. Quick Fix\r\n"
L"  5. Full Fix (if crashes persist)\r\n\r\n"
L"FAQ\r\n"
L"  Q: Will this delete my worlds?\r\n"
L"  A: No. Only cache/config files are modified.\r\n\r\n"
L"  Q: Antivirus is flagging this.\r\n"
L"  A: System tools that read process/RAM info are often false-flagged.\r\n\r\n"
L"  Q: Do I need internet?\r\n"
L"  A: No. Everything runs locally.\r\n\r\n"
L"COMMON CRASH CAUSES\r\n"
L"  - Low RAM (<4 GB available)\r\n"
L"  - Full disk\r\n"
L"  - Corrupted shader cache\r\n"
L"  - Outdated GPU drivers\r\n"
L"  - Broken config files\r\n"
L"  - Too many mods (>20)\r\n\r\n"
L"DISCLAIMER\r\n"
L"  FixCraft is NOT affiliated with Mojang.\r\n"
L"  Use at your own risk. No account data accessed.\r\n");

    CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        275, 584, 100, 28, hWin, (HMENU)IDCANCEL, nullptr, nullptr);

    ShowWindow(hWin, SW_SHOW); UpdateWindow(hWin);
    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        if ((m.message == WM_COMMAND && LOWORD(m.wParam) == IDCANCEL) || m.message == WM_CLOSE) break;
        TranslateMessage(&m); DispatchMessageW(&m);
    }
    DestroyWindow(hWin);
}

// ── CUSTOM BUTTON DRAWING ─────────────────────────────────────────────────────
struct BtnState { bool hover = false; };
static std::map<HWND, BtnState> g_btnStates;

static LRESULT CALLBACK BtnSubclass(HWND hBtn, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    auto& st = g_btnStates[hBtn];
    switch (msg) {
    case WM_MOUSEMOVE:
        if (!st.hover) {
            st.hover = true;
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hBtn };
            TrackMouseEvent(&tme);
            InvalidateRect(hBtn, nullptr, TRUE);
        }
        break;
    case WM_MOUSELEAVE:
        st.hover = false;
        InvalidateRect(hBtn, nullptr, TRUE);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hBtn, &ps);
        RECT rc; GetClientRect(hBtn, &rc);
        COLORREF bg = st.hover ? g_theme.selBg : g_theme.bg2;
        HBRUSH hBr = CreateSolidBrush(bg); FillRect(hdc, &rc, hBr); DeleteObject(hBr);
        // Subtle border
        HPEN hPen = CreatePen(PS_SOLID, 1, g_theme.selBg);
        HPEN hOp  = (HPEN)SelectObject(hdc, hPen);
        RECT border = rc; border.right--; border.bottom--;
        FrameRect(hdc, &border, CreateSolidBrush(g_theme.selBg));
        SelectObject(hdc, hOp); DeleteObject(hPen);
        // Text
        wchar_t txt[256] = {}; GetWindowTextW(hBtn, txt, 255);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, g_theme.fg);
        SelectObject(hdc, g_hFontNormal);
        DrawTextW(hdc, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hBtn, &ps);
        return 0;
    }
    case WM_NCDESTROY: g_btnStates.erase(hBtn); break;
    }
    return DefSubclassProc(hBtn, msg, wParam, lParam);
}

static HWND MakeBtn(HWND hParent, const wchar_t* txt, int id, int x, int y, int w, int h, HINSTANCE hInst) {
    HWND h = CreateWindowW(L"BUTTON", txt,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, w, h, hParent, (HMENU)(UINT_PTR)id, hInst, nullptr);
    SendMessage(h, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    g_btnStates[h] = {};
    SetWindowSubclass(h, BtnSubclass, 0, 0);
    return h;
}

// ── WINDOW PROCEDURE ──────────────────────────────────────────────────────────
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_LOG_MESSAGE: {
        wchar_t* buf = (wchar_t*)lParam;
        if (buf && g_hOutput) {
            int len = GetWindowTextLengthW(g_hOutput);
            SendMessage(g_hOutput, EM_SETSEL, len, len);
            SendMessage(g_hOutput, EM_REPLACESEL, FALSE,
                        (LPARAM)(std::wstring(buf) + L"\r\n").c_str());
            SendMessage(g_hOutput, EM_SCROLLCARET, 0, 0);
        }
        delete[] buf;
        return 0;
    }

    case WM_SET_STATUS: {
        struct PL { std::wstring msg; COLORREF col; };
        auto* p = (PL*)lParam;
        if (p && g_hStatus) {
            g_statusColor = p->col;
            SetWindowTextW(g_hStatus, p->msg.c_str());
            InvalidateRect(g_hStatus, nullptr, TRUE);
        }
        delete p;
        return 0;
    }

    case WM_TASK_DONE:
        g_isRunning = false;
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_EXIT)    { PostQuitMessage(0); return 0; }
        if (id == IDC_BTN_DARKMODE){ CfgSet(L"dark_mode", !CfgGetBool(L"dark_mode")); SaveConfig(); ApplyTheme(hWnd); return 0; }
        if (id == IDC_BTN_SETTINGS){ ShowSettingsWindow(hWnd); return 0; }
        if (id == IDC_BTN_HELP)    { std::thread([hWnd]{ ShowHelpWindow(hWnd); }).detach(); return 0; }
        if (id == IDC_BTN_CLEAROUTPUT) {
            SetWindowTextW(g_hOutput, L"");
            g_statusColor = RGB(128, 128, 128);
            SetWindowTextW(g_hStatus, L"Output cleared");
            InvalidateRect(g_hStatus, nullptr, TRUE);
            return 0;
        }
        if (id == IDC_BTN_OPENCRASHLOGS) {
            std::wstring p = CfgGet(L"minecraft_path") + L"\\crash-reports";
            fs::create_directories(p);
            ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
        if (g_isRunning) return 0;
        if      (id == IDC_BTN_ANALYZE)    RunTask(ThreadAnalyze);
        else if (id == IDC_BTN_QUICKFIX) {
            if (MessageBoxW(hWnd, L"Apply SAFE fixes only?\n\nContinue?",
                L"Confirm Quick Fix", MB_YESNO | MB_ICONQUESTION) == IDYES)
                RunTask(ThreadQuickFix);
        }
        else if (id == IDC_BTN_FULLFIX) {
            if (MessageBoxW(hWnd, L"Deep clean: logs, crash reports, cache, config backup.\n"
                L"Recommended only if crashes persist.\n\nContinue?",
                L"Confirm Full Fix", MB_YESNO | MB_ICONWARNING) == IDYES)
                RunTask(ThreadFullFix);
        }
        else if (id == IDC_BTN_VIRTMEM)    RunTask(ThreadVirtMem);
        else if (id == IDC_BTN_SYSINFO)    RunTask(ThreadSysInfo);
        else if (id == IDC_BTN_CRASHLOGS)  RunTask(ThreadCrashLogs);
        else if (id == IDC_BTN_QUICKSCAN)  RunTask(ThreadQuickScan);
        else if (id == IDC_BTN_FULLSCAN)   RunTask(ThreadFullScan);
        else if (id == IDC_BTN_RESETCFG) {
            if (MessageBoxW(hWnd, L"Reset Minecraft config files?\nA backup will be created.\n\nContinue?",
                L"Reset Config", MB_YESNO | MB_ICONQUESTION) == IDYES)
                RunTask(ThreadResetConfig);
        }
        else if (id == IDC_BTN_RAMPRESSURE) RunTask(ThreadRamPressure);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, hCtrl == g_hStatus ? g_statusColor : g_theme.fg);
        return (LRESULT)g_hBrushBg;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, g_theme.bg3);
        SetTextColor(hdc, g_theme.fg);
        return (LRESULT)g_hBrushBg3;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam; RECT rc; GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_hBrushBg); return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc; GetClientRect(hWnd, &rc);
        rc.bottom = 38;
        HBRUSH hBr = CreateSolidBrush(g_theme.barBg);
        FillRect(hdc, &rc, hBr); DeleteObject(hBr);
        EndPaint(hWnd, &ps); return 0;
    }

    case WM_DESTROY:
        SaveConfig();
        DeleteBrushes();
        if (g_hFontNormal) DeleteObject(g_hFontNormal);
        if (g_hFontBold)   DeleteObject(g_hFontBold);
        if (g_hFontSmall)  DeleteObject(g_hFontSmall);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ── BUILD UI ──────────────────────────────────────────────────────────────────
static void BuildUI(HWND hWnd, HINSTANCE hInst) {
    // Fonts
    auto mkFont = [](int size, bool bold) {
        return CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    };
    g_hFontNormal = mkFont(-13, false);
    g_hFontBold   = mkFont(-15, true);
    g_hFontSmall  = mkFont(-11, false);

    auto mkLabel = [&](const wchar_t* txt, int x, int y, int w, int h, bool bold=false) {
        HWND lbl = CreateWindowW(L"STATIC", txt, WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, w, h, hWnd, nullptr, hInst, nullptr);
        SendMessage(lbl, WM_SETFONT, (WPARAM)(bold ? g_hFontBold : g_hFontSmall), TRUE);
        return lbl;
    };

    // ── TOP BAR ────────────────────────────────────────────────────────────
    // Title label in bar
    HWND hTitle = CreateWindowW(L"STATIC", L"FixCraft v1.1",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 9, 160, 22, hWnd, nullptr, hInst, nullptr);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    // Bar buttons (right side)
    MakeBtn(hWnd, L"Exit",       IDC_BTN_EXIT,     700, 5, 80,  28, hInst);
    MakeBtn(hWnd, L"Help",       IDC_BTN_HELP,     604, 5, 90,  28, hInst);
    MakeBtn(hWnd, L"Dark Mode",  IDC_BTN_DARKMODE, 486, 5, 112, 28, hInst);
    MakeBtn(hWnd, L"Settings",   IDC_BTN_SETTINGS, 368, 5, 112, 28, hInst);

    // ── DISCLAIMER ─────────────────────────────────────────────────────────
    mkLabel(L"Not affiliated with Mojang. Use at your own risk.", 10, 44, 760, 16);

    // ── HEADING ────────────────────────────────────────────────────────────
    HWND hH = CreateWindowW(L"STATIC", L"FixCraft v1.1",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 64, 760, 26, hWnd, nullptr, hInst, nullptr);
    SendMessage(hH, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    // ── STATUS ─────────────────────────────────────────────────────────────
    g_hStatus = CreateWindowW(L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 96, 760, 20, hWnd, (HMENU)IDC_STATUS, hInst, nullptr);
    SendMessage(g_hStatus, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    g_statusColor = RGB(0, 140, 0);

    // ── FEATURE BUTTONS — two columns ──────────────────────────────────────
    struct BD { const wchar_t* txt; int id; };
    std::vector<BD> btns = {
        { L"System Analysis",         IDC_BTN_ANALYZE      },
        { L"Quick Fix",               IDC_BTN_QUICKFIX     },
        { L"Full Fix",                IDC_BTN_FULLFIX      },
        { L"Increase Virtual Memory", IDC_BTN_VIRTMEM      },
        { L"System Info",             IDC_BTN_SYSINFO      },
        { L"Analyse Crash Logs",      IDC_BTN_CRASHLOGS    },
        { L"Quick Scan",              IDC_BTN_QUICKSCAN    },
        { L"Full Scan",               IDC_BTN_FULLSCAN     },
        { L"Reset Config Files",      IDC_BTN_RESETCFG     },
        { L"Check RAM Pressure",      IDC_BTN_RAMPRESSURE  },
        { L"Open Crash Log Folder",   IDC_BTN_OPENCRASHLOGS},
    };

    const int bw = 378, bh = 32, gap = 5;
    const int cx0 = 10, cx1 = 396;
    int y0 = 124, y1 = 124;

    for (int i = 0; i < (int)btns.size(); i++) {
        int cx = (i % 2 == 0) ? cx0 : cx1;
        int& cy = (i % 2 == 0) ? y0 : y1;
        MakeBtn(hWnd, btns[i].txt, btns[i].id, cx, cy, bw, bh, hInst);
        cy += bh + gap;
    }

    int outputY = max(y0, y1) + 8;

    // ── OUTPUT HEADER + CLEAR BUTTON ───────────────────────────────────────
    HWND hOutLbl = CreateWindowW(L"STATIC", L"Output:",
        WS_CHILD | WS_VISIBLE, 10, outputY, 80, 20, hWnd, nullptr, hInst, nullptr);
    SendMessage(hOutLbl, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

    MakeBtn(hWnd, L"Clear Output", IDC_BTN_CLEAROUTPUT, 688, outputY - 1, 112, 24, hInst);

    // ── OUTPUT EDIT ────────────────────────────────────────────────────────
    g_hOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        10, outputY + 24, 780, 172,
        hWnd, (HMENU)IDC_OUTPUT, hInst, nullptr);
    SendMessage(g_hOutput, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

    // ── FOOTER ─────────────────────────────────────────────────────────────
    mkLabel(L"FixCraft v1.1  |  Not affiliated with Mojang",
            10, outputY + 202, 760, 16);
}

// ── ENTRY POINT ───────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);
    CoInitialize(nullptr);

    g_configDir  = GetAppDataPath() + L"\\FixCraft";
    g_configFile = g_configDir + L"\\settings.ini";
    fs::create_directories(g_configDir);
    LoadConfig();

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"FixCraftWnd";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);

    g_hWnd = CreateWindowExW(0, L"FixCraftWnd", L"FixCraft v1.1",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 820, 740,
        nullptr, nullptr, hInst, nullptr);

    if (!g_hWnd) return 1;

    BuildUI(g_hWnd, hInst);
    ApplyTheme(g_hWnd);
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    if (CfgGetBool(L"auto_analyze")) {
        SetTimer(g_hWnd, 1, 500, [](HWND h, UINT, UINT_PTR tid, DWORD) {
            KillTimer(h, tid);
            RunTask(ThreadAnalyze);
        });
    }

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}