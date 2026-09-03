#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <shlobj.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <ctime>

#pragma comment(lib, "Advapi32.lib")

namespace fs = std::filesystem;

// ANSI escape codes for colored CLI output
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string RED     = "\033[31m";
    const std::string GREEN   = "\033[32m";
    const std::string YELLOW  = "\033[33m";
    const std::string BLUE    = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN    = "\033[36m";
    const std::string WHITE   = "\033[37m";
    const std::string BOLD    = "\033[1m";
    const std::string DIM     = "\033[2m";
}

void printHeader() {
    std::cout << Color::CYAN << Color::BOLD
              << "\n=====================================\n"
              << "       Windows Cache Cleaner v1.1\n"
              << "=====================================\n"
              << Color::RESET << "\n";
}

void printStage(const std::string& name) {
    std::cout << Color::BLUE << Color::BOLD
              << "--- " << name << " ---\n"
              << Color::RESET;
}

void printError(const std::string& msg) {
    std::cout << "  " << Color::RED << "[ERROR]" << Color::RESET << " " << msg << "\n";
}

void printWarning(const std::string& msg) {
    std::cout << "  " << Color::YELLOW << "[WARN]" << Color::RESET << " " << msg << "\n";
}

void printCleaning(const std::string& name) {
    std::cout << "  " << Color::YELLOW << "[CLEANING]" << Color::RESET << " " << name << " ...\n";
}

void printOk() {
    std::cout << "    " << Color::GREEN << "[OK] done" << Color::RESET << "\n";
}

void printFreed(uintmax_t bytes) {
    if (bytes == 0) return;
    std::string unit = " bytes";
    double n = static_cast<double>(bytes);
    if (n >= (1024.0 * 1024.0 * 1024.0)) { n /= (1024.0 * 1024.0 * 1024.0); unit = " GB"; }
    else if (n >= 1024.0 * 1024.0) { n /= (1024.0 * 1024.0); unit = " MB"; }
    else if (n >= 1024.0) { n /= 1024.0; unit = " KB"; }
    std::cout << "      freed " << Color::GREEN << std::fixed << std::setprecision(2) << n << unit << Color::RESET << "\n";
}

// Get total size of all files in a path recursively
uintmax_t getFolderSize(const fs::path& path) {
    uintmax_t total = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                std::error_code ec;
                total += entry.file_size(ec);
            }
        }
    } catch (...) {}
    return total;
}

// Recursively delete contents of a folder, return freed bytes
uintmax_t clearFolder(const fs::path& folder) {
    uintmax_t freed = 0;
    if (!fs::exists(folder)) return 0;
    try {
        for (const auto& entry : fs::directory_iterator(folder, fs::directory_options::skip_permission_denied)) {
            std::error_code ec;
            uintmax_t sz = 0;
            if (entry.is_directory()) {
                sz = getFolderSize(entry.path());
                fs::remove_all(entry.path(), ec);
            } else {
                sz = entry.file_size(ec);
                fs::remove(entry.path(), ec);
            }
            freed += sz;
        }
    } catch (...) {}
    return freed;
}

std::string getHomeDir() {
    return std::getenv("USERPROFILE");
}

std::string getLocalAppData() {
    return std::getenv("LOCALAPPDATA");
}

std::string getAppData() {
    return std::getenv("APPDATA");
}

std::string getWindowsDir() {
    return std::getenv("WINDIR");
}

void runCommand(const std::string& command) {
    std::cout << "  [STOP SERVICE] " << command << " ...\n";
    // Note: This runs the command and captures output to null
    std::string cmd = command + " >nul 2>nul";
    system(cmd.c_str());
}

// --- Cleanup categories ---
uintmax_t cleanWindowsTemp() {
    printCleaning("Windows Temp folder");
    uintmax_t freed = clearFolder(fs::path(getWindowsDir()) / "Temp");
    printOk();
    return freed;
}

uintmax_t cleanUserTemp() {
    printCleaning("User Temp folder");
    uintmax_t freed = clearFolder(fs::path(getLocalAppData()) / "Temp");
    printOk();
    return freed;
}

uintmax_t cleanWindowsUpdateCache() {
    printCleaning("Windows Update download cache");
    runCommand("net stop wuauserv");
    uintmax_t freed = clearFolder(fs::path(getWindowsDir()) / "SoftwareDistribution" / "Download");
    runCommand("net start wuauserv");
    printOk();
    return freed;
}

uintmax_t cleanThumbnailCache() {
    printCleaning("Thumbnail caches");
    uintmax_t freed = 0;
    std::string explorer = getLocalAppData() + "\\Microsoft\\Windows\\Explorer";
    try {
        for (const auto& entry : fs::directory_iterator(explorer, fs::directory_options::skip_permission_denied)) {
            std::string name = entry.path().filename().string();
            if (name.find("thumbcache_") != std::string::npos && entry.path().extension() == ".db") {
                std::error_code ec;
                freed += entry.file_size(ec);
                fs::remove(entry.path(), ec);
            }
        }
    } catch (...) {}
    printOk();
    return freed;
}

uintmax_t cleanErrorReporting() {
    printCleaning("Windows Error Reporting (WER)");
    uintmax_t freed = 0;
    freed += clearFolder(fs::path(getLocalAppData()) / "Microsoft" / "Windows" / "WER");
    std::string progData = std::getenv("ProgramData");
    if (!progData.empty()) {
        freed += clearFolder(fs::path(progData) / "Microsoft" / "Windows" / "WER");
    }
    printOk();
    return freed;
}

uintmax_t cleanDeliveryOptimization() {
    printCleaning("Delivery Optimization cache");
    uintmax_t freed = 0;
    std::string winDir = getWindowsDir();
    // C:\Windows\ServiceProfiles\NetworkService\AppData\Local\...
    fs::path netCache = fs::path(winDir) / "ServiceProfiles" / "NetworkService" / "AppData" / "Local" / "Microsoft" / "Windows" / "DeliveryOptimization" / "Cache";
    fs::path locCache = fs::path(winDir) / "ServiceProfiles" / "LocalService" / "AppData" / "Local" / "Microsoft" / "Windows" / "DeliveryOptimization" / "Cache";
    freed += clearFolder(netCache);
    freed += clearFolder(locCache);
    // Also user-level
    freed += clearFolder(fs::path(getLocalAppData()) / "Microsoft" / "Windows" / "DeliveryOptimization");
    printOk();
    return freed;
}

uintmax_t cleanCrashDumps() {
    printCleaning("Crash dumps and minidumps");
    uintmax_t freed = 0;
    std::string winDir = getWindowsDir();
    freed += clearFolder(fs::path(winDir) / "Minidump");
    freed += clearFolder(fs::path(getLocalAppData()) / "CrashDumps");
    freed += clearFolder(fs::path(getLocalAppData()) / "D3DSCache");
    printOk();
    return freed;
}

uintmax_t cleanPrefetch() {
    printCleaning("Windows Prefetch");
    uintmax_t freed = clearFolder(fs::path(getWindowsDir()) / "Prefetch");
    printOk();
    return freed;
}

uintmax_t cleanBrowserCaches() {
    printCleaning("Browser caches (Chrome/Edge/Firefox)");
    uintmax_t freed = 0;
    std::string local = getLocalAppData();

    // Chrome
    std::vector<std::string> chromeCaches = {
        "\\Google\\Chrome\\User Data\\Default\\Cache",
        "\\Google\\Chrome\\User Data\\Default\\Code Cache",
        "\\Google\\Chrome\\User Data\\Default\\Service Worker\\CacheStorage",
        "\\Google\\Chrome\\User Data\\Default\\GPUCache"
    };
    for (auto& c : chromeCaches) {
        freed += clearFolder(fs::path(local + c));
    }

    // Edge
    std::vector<std::string> edgeCaches = {
        "\\Microsoft\\Edge\\User Data\\Default\\Cache",
        "\\Microsoft\\Edge\\User Data\\Default\\Code Cache",
        "\\Microsoft\\Edge\\User Data\\Default\\Service Worker\\CacheStorage",
        "\\Microsoft\\Edge\\User Data\\Default\\GPUCache"
    };
    for (auto& c : edgeCaches) {
        freed += clearFolder(fs::path(local + c));
    }

    // Firefox
    std::string appData = getAppData();
    std::vector<std::string> ffCaches = {
        "\\Mozilla\\Firefox\\Profiles\\*.default-release\\cache2",
        "\\Mozilla\\Firefox\\Profiles\\*.default\\cache2",
        "\\Mozilla\\Firefox\\Profiles\\*.default-release\\startupCache",
        "\\Mozilla\\Firefox\\Profiles\\*.default\\startupCache"
    };
    for (auto& f : ffCaches) {
        // Expand wildcard manually
        try {
            fs::path profPath(appData + "\\Mozilla\\Firefox\\Profiles");
            if (fs::exists(profPath)) {
                for (const auto& prof : fs::directory_iterator(profPath, fs::directory_options::skip_permission_denied)) {
                    if (prof.is_directory()) {
                        std::string profName = prof.path().filename().string();
                        if (profName.find("default") != std::string::npos) {
                            if (f.find("cache2") != std::string::npos) {
                                freed += clearFolder(prof.path() / "cache2");
                            } else if (f.find("startupCache") != std::string::npos) {
                                freed += clearFolder(prof.path() / "startupCache");
                            }
                        }
                    }
                }
            }
        } catch (...) {}
    }

    printOk();
    return freed;
}

uintmax_t cleanNpmCache() {
    printCleaning("npm cache");
    uintmax_t freed = clearFolder(fs::path(getLocalAppData()) / "npm-cache");
    printOk();
    return freed;
}

uintmax_t cleanPipCache() {
    printCleaning("pip cache");
    uintmax_t freed = clearFolder(fs::path(getLocalAppData()) / "pip" / "cache");
    // also try AppData dir
    freed += clearFolder(fs::path(std::string(std::getenv("USERPROFILE")) + "\\.cache\\pip"));
    printOk();
    return freed;
}

uintmax_t cleanNuGetCache() {
    printCleaning("NuGet cache");
    uintmax_t freed = 0;
    std::string home = getHomeDir();
    freed += clearFolder(fs::path(home) / ".nuget" / "packages");
    freed += clearFolder(fs::path(getLocalAppData()) / "NuGet" / "v3-cache");
    freed += clearFolder(fs::path(getLocalAppData()) / "NuGet" / "plugins-cache");
    freed += clearFolder(fs::path(getLocalAppData()) / "Temp" / "NuGetScratch");
    printOk();
    return freed;
}

uintmax_t cleanNvidiaCache() {
    printCleaning("NVIDIA shader caches");
    uintmax_t freed = 0;
    std::string local = getLocalAppData();
    freed += clearFolder(fs::path(local) / "NVIDIA" / "DXCache");
    freed += clearFolder(fs::path(local) / "NVIDIA" / "GLCache");
    freed += clearFolder(fs::path(local) / "NVIDIA" / "ComputeCache");
    // Also C:\ProgramData\NVIDIA Corporation\DownloadCache
    std::string progData = std::getenv("ProgramData");
    if (!progData.empty()) {
        freed += clearFolder(fs::path(progData) / "NVIDIA Corporation" / "DownloadCache");
    }
    printOk();
    return freed;
}

uintmax_t cleanDiscordCache() {
    printCleaning("Discord caches");
    uintmax_t freed = 0;
    std::string appData = getAppData();
    freed += clearFolder(fs::path(appData) / "discord" / "Cache");
    freed += clearFolder(fs::path(appData) / "discord" / "Code Cache");
    freed += clearFolder(fs::path(appData) / "discord" / "GPUCache");
    freed += clearFolder(fs::path(appData) / "discord" / "DawnCache");
    printOk();
    return freed;
}

uintmax_t cleanSteamCache() {
    printCleaning("Steam shader caches");
    uintmax_t freed = 0;
    std::string local = getLocalAppData();
    freed += clearFolder(fs::path(local) / "Steam" / "htmlcache");
    // Steam shader cache (non-app caches)
    std::string progFiles = std::getenv("ProgramFiles(x86)");
    if (!progFiles.empty()) {
        try {
            fs::path steamDir = fs::path(progFiles) / "Steam";
            if (fs::exists(steamDir)) {
                // Check steamapps for shadercache
                fs::path steamApps = steamDir / "steamapps";
                if (fs::exists(steamApps)) {
                    for (const auto& entry : fs::directory_iterator(steamApps, fs::directory_options::skip_permission_denied)) {
                        if (entry.is_directory() && entry.path().filename() == "shadercache") {
                            freed += clearFolder(entry.path());
                        }
                    }
                }
            }
        } catch (...) {}
    }
    printOk();
    return freed;
}

uintmax_t cleanRobloxCache() {
    printCleaning("Roblox caches");
    uintmax_t freed = 0;
    std::string local = getLocalAppData();
    freed += clearFolder(fs::path(local) / "Roblox");
    printOk();
    return freed;
}

uintmax_t cleanNuitkaCache() {
    printCleaning("Nuitka build cache");
    uintmax_t freed = clearFolder(fs::path(getLocalAppData()) / "Nuitka");
    printOk();
    return freed;
}

uintmax_t cleanWindowsInstallerCache() {
    printCleaning("Windows Installer patch cache");
    uintmax_t freed = 0;
    std::string winDir = getWindowsDir();
    freed += clearFolder(fs::path(winDir) / "Installer" / "$PatchCache$");
    printOk();
    return freed;
}

void runDiskCleanup() {
    printCleaning("Running built-in Windows Disk Cleanup");
    std::string cmd = std::string("cmd /c ") + getWindowsDir() + "\\System32\\cleanmgr.exe /d C /sagerun:1";
    system(cmd.c_str());
    printOk();
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    // Enable ANSI escape sequences on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD prev = 0;
    GetConsoleMode(hOut, &prev);
    SetConsoleMode(hOut, prev | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    printHeader();

    // Check admin
    bool isAdmin = false;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(token, TokenElevation, &elevation, size, &size)) {
            isAdmin = elevation.TokenIsElevated != 0;
        }
        CloseHandle(token);
    }
    if (!isAdmin) {
        printWarning("Running without admin rights. Some folders (Windows Update, Installer) may fail.");
    }

    fs::path home(getHomeDir());
    fs::path appData(getAppData());
    fs::path local(getLocalAppData());

    uintmax_t totalFreed = 0;
    uintmax_t partial = 0;

    std::cout << "\n";
    printStage("System Caches");
    partial = cleanWindowsTemp(); totalFreed += partial; printFreed(partial);
    partial = cleanUserTemp(); totalFreed += partial; printFreed(partial);
    partial = cleanWindowsUpdateCache(); totalFreed += partial; printFreed(partial);
    partial = cleanThumbnailCache(); totalFreed += partial; printFreed(partial);
    partial = cleanErrorReporting(); totalFreed += partial; printFreed(partial);
    partial = cleanDeliveryOptimization(); totalFreed += partial; printFreed(partial);
    partial = cleanCrashDumps(); totalFreed += partial; printFreed(partial);
    partial = cleanPrefetch(); totalFreed += partial; printFreed(partial);
    partial = cleanWindowsInstallerCache(); totalFreed += partial; printFreed(partial);

    std::cout << "\n";
    printStage("Browser Caches");
    partial = cleanBrowserCaches(); totalFreed += partial; printFreed(partial);

    std::cout << "\n";
    printStage("Package Manager Caches");
    partial = cleanNpmCache(); totalFreed += partial; printFreed(partial);
    partial = cleanPipCache(); totalFreed += partial; printFreed(partial);
    partial = cleanNuGetCache(); totalFreed += partial; printFreed(partial);

    std::cout << "\n";
    printStage("App Caches");
    partial = cleanNvidiaCache(); totalFreed += partial; printFreed(partial);
    partial = cleanDiscordCache(); totalFreed += partial; printFreed(partial);
    partial = cleanSteamCache(); totalFreed += partial; printFreed(partial);
    partial = cleanRobloxCache(); totalFreed += partial; printFreed(partial);
    partial = cleanNuitkaCache(); totalFreed += partial; printFreed(partial);

    std::cout << "\n";
    printStage("Windows Disk Cleanup");
    runDiskCleanup();

    // Final stats
    std::string freedStr;
    if (totalFreed >= (1024.0 * 1024.0 * 1024.0)) {
        freedStr = std::to_string(static_cast<double>(totalFreed) / (1024 * 1024 * 1024)) + " GB";
    } else {
        freedStr = std::to_string(static_cast<double>(totalFreed) / (1024 * 1024)) + " MB";
    }

    std::cout << "\n";
    std::cout << Color::CYAN << Color::BOLD
              << "=====================================\n"
              << "               SUMMARY\n"
              << "=====================================\n"
              << Color::RESET;
    std::cout << "  Total space cleaned: "
              << Color::GREEN << Color::BOLD << freedStr << Color::RESET << "\n";

    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    std::string driveRoot = "C:\\";
    GetDiskFreeSpaceExA(driveRoot.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes);
    double freeGB = static_cast<double>(totalFreeBytes.QuadPart) / (1024 * 1024 * 1024);
    double totalGB = static_cast<double>(totalBytes.QuadPart) / (1024 * 1024 * 1024);

    std::cout << "  C: drive free space: "
              << Color::GREEN << Color::BOLD << std::fixed << std::setprecision(1) << freeGB << " GB" << Color::RESET << "\n";
    std::cout << "  Total drive size: " << std::fixed << std::setprecision(1) << totalGB << " GB\n";

    std::cout << "\n  ";
    std::cout << Color::DIM << "Press Enter to exit..." << Color::RESET << "\n";
    std::cin.get();
    return 0;
}
