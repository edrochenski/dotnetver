#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma comment(lib, "Advapi32")
#pragma comment(lib, "Kernel32")
#include <stdio.h>
#include <string.h>
#include <Windows.h>

#define LBL_DNC_RUN         ".NET Core Runtime"
#define LBL_DNC_SDK         ".NET Core SDK"
#define LBL_DISPLAY_NAME    "Display Name"
#define LBL_INSTALL_VERSION "Installed Version"
#define MAX_KEY_SIZE        255
#define MAX_VAL_SIZE        255
#define DNF_PROFILE_COUNT   2

static const char* ARG_COMPACT      = "--compact";
static const char* INVALID_VERSION  = "<invalid>\n";
static const char* DNC_RUN_DIR      = "%PROGRAMFILES%\\dotnet\\shared\\Microsoft.NETCore.App\\*.*";
static const char* DNC_RUN_SDK      = "%PROGRAMFILES%\\dotnet\\sdk\\*.*";
static const char* DNF_PROFILES[]   = {"Full", "Client"};
static const char* REG_BASE_CURR    = "Software\\Microsoft\\NET Framework Setup\\NDP";
static const char* REG_BASE_NET1    = "Software\\Microsoft\\.NETFramework\\Policy\\v1.0\\3705";
static const char* REG_NAME_INSTALL = "Install";
static const char* REG_NAME_VERSION = "Version";

static BOOL compact = FALSE;
static BOOL prevVer = FALSE;

static void net_core_search(const char* directory, const char* label)
{
    WIN32_FIND_DATA findData;
    char runDir[MAX_PATH];
    ExpandEnvironmentStrings(directory, runDir, MAX_PATH);
    HANDLE ffResult = FindFirstFileEx(runDir, FindExInfoStandard, &findData, FindExSearchLimitToDirectories, NULL, 0);
    if (ffResult != INVALID_HANDLE_VALUE)
    {
        int i = 0;
        while (FindNextFile(ffResult, &findData))
        {
            if (findData.cFileName[0] == '.') { continue; }
            if (!compact)
            { printf_s("%-24s %-32s\n", label, findData.cFileName); }
            else
            {
                if (i == 0)
                {
                    if (prevVer) { printf_s(" | "); }
                    printf_s("%s: ", label);
                }
                
                if (i > 0) { printf_s(", "); }
                printf_s(findData.cFileName);
            }
            
            ++i;
            prevVer = TRUE;
        }
    }
}

int main (int argc, char** argv)
{
    HKEY ndpkey, dn1key;

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], ARG_COMPACT)) { compact = TRUE; }
    }

    if (!compact)
    {
        printf_s("%-24s %-32s\n", LBL_DISPLAY_NAME, LBL_INSTALL_VERSION);
        printf_s("------------------------ --------------------------------\n");
    }

    // .NET == v1.0
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_BASE_NET1, 0, KEY_READ, &dn1key) == ERROR_SUCCESS)
    {
        byte dn1data[MAX_VAL_SIZE] = {0}; 
        ULONG dn1len = MAX_VAL_SIZE;
        ULONG type = REG_SZ;

        if (!compact) { printf_s("%-24s ", "v1.0"); }
        else          { printf_s("v1.0: "); }
        if (RegQueryValueEx(dn1key, REG_NAME_INSTALL, 0, &type, dn1data, &dn1len) == ERROR_SUCCESS && dn1data[0] == '\x1')
        { 
            printf_s("1.0.0.0"); /* no Version value available to query */ 
        }
        else
        { printf_s(INVALID_VERSION); }
        
        RegCloseKey(dn1key);
        prevVer = TRUE;
    }

    // .NET > v1.0
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_BASE_CURR, 0, KEY_READ, &ndpkey) == ERROR_SUCCESS)
    {
        char keyName[MAX_KEY_SIZE];
        ULONG index = 0;
        ULONG nameLen = MAX_KEY_SIZE;
            
        while (RegEnumKey(ndpkey, index++, keyName, nameLen) == ERROR_SUCCESS)
        {
            if (keyName[0] != 'v') { continue; }

            if (keyName[1] == '4') // keyName could be 'v4' or 'v4.0'
            {
                HKEY dnverkey;
                if (RegOpenKeyEx(ndpkey, keyName, 0, KEY_READ, &dnverkey) != ERROR_SUCCESS) { continue; }

                for (int i = 0; i < DNF_PROFILE_COUNT; ++i)
                {
                    HKEY profileKey;
                    if (RegOpenKeyEx(dnverkey, DNF_PROFILES[i], 0, KEY_READ, &profileKey) != ERROR_SUCCESS) { continue; }

                    char profile[MAX_KEY_SIZE+32];
                    snprintf(profile, sizeof(profile), "%s (%s)", keyName, DNF_PROFILES[i]);
                    if (!compact)
                    { printf_s("%-24s ", profile); }
                    else if (i > 0)
                    {
                        if (prevVer) { printf_s(" | "); }
                        printf_s("%s: ", profile);
                    }

                    unsigned char verdata[MAX_VAL_SIZE];
                    ULONG verdatalen = MAX_VAL_SIZE;
                    ULONG type = REG_SZ;
                    
                    if (RegQueryValueEx(profileKey, REG_NAME_VERSION, 0, &type, verdata, &verdatalen) == ERROR_SUCCESS)
                    {
                        if (!compact)
                        { printf_s("%-32s\n", verdata); }
                        else
                        { printf_s("%s", verdata); }
                    }
                    else
                    { printf_s(INVALID_VERSION); }

                    RegCloseKey(profileKey);
                    prevVer = TRUE;
                }

                RegCloseKey(dnverkey);
            }
            else
            {
                if (!compact)
                { printf_s("%-24s ", keyName); }
                else
                {
                    if (prevVer) { printf_s(" | "); }
                    printf_s("%s: ", keyName);
                }

                HKEY dnverkey;
                if (RegOpenKeyEx(ndpkey, keyName, 0, KEY_READ, &dnverkey) != ERROR_SUCCESS) { continue; }

                unsigned char verdata[MAX_VAL_SIZE];
                ULONG verdatalen = MAX_VAL_SIZE;
                ULONG type = REG_SZ;
                if (RegQueryValueEx(dnverkey, REG_NAME_VERSION, 0, &type, verdata, &verdatalen) == ERROR_SUCCESS)
                {
                    if (!compact)
                    { printf_s("%-32s\n", verdata); }
                    else
                    { printf_s("%s", verdata); }
                }

                RegCloseKey(dnverkey);
                prevVer = TRUE;
            }
        }

        RegCloseKey(ndpkey);
    }

    // .NET Core (Runtime)
    net_core_search(DNC_RUN_DIR, LBL_DNC_RUN);

    // .NET Core (SDK)
    net_core_search(DNC_RUN_SDK, LBL_DNC_SDK);
}
