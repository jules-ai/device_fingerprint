#include "fingerprint.h"
#include <string>
#include <iomanip>
#include <functional>
#include <algorithm>

// Helper function to trim strings
static std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first)
    {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

#ifdef _WIN32
#define _WIN32_DCOM
#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <winioctl.h>
#include <lmcons.h> // For UNLEN
#include <vector>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

std::string get_baseboard_serial_wmi()
{
    std::string serial = "";
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
        return "";

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres))
    {
        CoUninitialize();
        return "";
    }

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if (FAILED(hres))
    {
        CoUninitialize();
        return "";
    }

    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres))
    {
        pLoc->Release();
        CoUninitialize();
        return "";
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "";
    }

    IEnumWbemClassObject *pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT SerialNumber FROM Win32_BaseBoard"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "";
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn)
            break;

        VARIANT vtProp;
        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
        {
            _bstr_t bstr(vtProp.bstrVal, false);
            serial = (LPCSTR)bstr;
        }
        VariantClear(&vtProp);
        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return trim(serial);
}

std::string get_hdd_serial_windows()
{
    std::string serial = "";
    HANDLE hDevice = CreateFileA("\\\\.\\PhysicalDrive0", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
        return "";

    STORAGE_PROPERTY_QUERY query;
    ZeroMemory(&query, sizeof(query));
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    DWORD dwBytesReturned = 0;
    char buffer[10000];
    if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &buffer, sizeof(buffer), &dwBytesReturned, NULL))
    {
        STORAGE_DEVICE_DESCRIPTOR *desc = (STORAGE_DEVICE_DESCRIPTOR *)buffer;
        if (desc->SerialNumberOffset)
        {
            serial = trim(std::string(buffer + desc->SerialNumberOffset));
        }
    }
    CloseHandle(hDevice);
    return serial;
}

std::string get_username_win()
{
    wchar_t username_utf16[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (!GetUserNameW(username_utf16, &username_len))
    {
        return "";
    }

    int buffer_size = WideCharToMultiByte(CP_UTF8, 0, username_utf16, -1, NULL, 0, NULL, NULL);
    if (buffer_size == 0)
    {
        return "";
    }
    std::vector<char> username_utf8(buffer_size);
    WideCharToMultiByte(CP_UTF8, 0, username_utf16, -1, &username_utf8[0], buffer_size, NULL, NULL);

    return trim(std::string(username_utf8.begin(), username_utf8.end() - 1)); // -1 to remove null terminator
}
#elif __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/IOBSD.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>

// Helper function to convert CFStringRef to std::string
static std::string cfstring_to_stdstring(CFStringRef cfStr)
{
    if (cfStr == NULL)
    {
        return "";
    }
    const char *c_str = CFStringGetCStringPtr(cfStr, kCFStringEncodingUTF8);
    if (c_str)
    {
        return std::string(c_str);
    }
    CFIndex length = CFStringGetLength(cfStr);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    char buffer[maxSize];
    if (CFStringGetCString(cfStr, buffer, maxSize, kCFStringEncodingUTF8))
    {
        return std::string(buffer);
    }
    return "";
}

std::string get_platform_serial_mac()
{
    std::string serial = "";
    io_service_t platformExpert = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
    if (platformExpert)
    {
        CFStringRef serialNumberRef = (CFStringRef)IORegistryEntryCreateCFProperty(platformExpert, CFSTR(kIOPlatformSerialNumberKey), kCFAllocatorDefault, 0);
        serial = cfstring_to_stdstring(serialNumberRef);
        if (serialNumberRef)
        {
            CFRelease(serialNumberRef);
        }
        IOObjectRelease(platformExpert);
    }
    return trim(serial);
}

std::string get_hdd_serial_mac()
{
    std::string serial_number_str = "";

    struct stat root_stat;
    if (stat("/", &root_stat) != 0)
    {
        return "";
    }
    char *dev_name_c = devname(root_stat.st_dev, S_IFBLK);
    if (dev_name_c == NULL)
    {
        return "";
    }
    std::string root_bsd_name(dev_name_c);

    io_iterator_t iter;
    if (IOServiceGetMatchingServices(kIOMainPortDefault, IOServiceMatching(kIOMediaClass), &iter) != KERN_SUCCESS)
    {
        return "";
    }

    io_registry_entry_t service;
    io_registry_entry_t root_partition_service = 0;
    while ((service = IOIteratorNext(iter)) != 0)
    {
        CFStringRef bsdNameRef = (CFStringRef)IORegistryEntryCreateCFProperty(service, CFSTR("BSD Name"), kCFAllocatorDefault, 0);
        std::string bsd_name_str = cfstring_to_stdstring(bsdNameRef);
        if (bsdNameRef)
        {
            CFRelease(bsdNameRef);
        }

        if (root_bsd_name == bsd_name_str)
        {
            root_partition_service = service;
            break;
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iter);

    if (root_partition_service == 0)
    {
        return "";
    }

    io_registry_entry_t current_node = root_partition_service;
    while (true)
    {
        CFStringRef serialRef = (CFStringRef)IORegistryEntryCreateCFProperty(current_node, CFSTR("Serial Number"), kCFAllocatorDefault, 0);
        if (serialRef)
        {
            serial_number_str = cfstring_to_stdstring(serialRef);
            CFRelease(serialRef);
            if (!serial_number_str.empty())
            {
                break;
            }
        }

        io_registry_entry_t parent_node;
        kern_return_t kr = IORegistryEntryGetParentEntry(current_node, kIOServicePlane, &parent_node);

        if (current_node != root_partition_service)
        {
            IOObjectRelease(current_node);
        }

        if (kr != KERN_SUCCESS)
        {
            current_node = 0;
            break;
        }
        current_node = parent_node;
    }

    if (current_node != 0 && current_node != root_partition_service)
    {
        IOObjectRelease(current_node);
    }
    IOObjectRelease(root_partition_service);

    return trim(serial_number_str);
}

std::string get_username_mac()
{
    struct passwd *pw = getpwuid(getuid());
    if (pw)
    {
        return trim(std::string(pw->pw_name));
    }
    return "";
}
#endif

std::string get_board_serial()
{
#ifdef _WIN32
    return get_baseboard_serial_wmi();
#elif __APPLE__
    return get_platform_serial_mac();
#endif
    return "unknown_board";
}

std::string get_hdd_serial()
{
#ifdef _WIN32
    return get_hdd_serial_windows();
#elif __APPLE__
    return get_hdd_serial_mac();
#endif
    return "unknown_hdd";
}

std::string get_username()
{
#ifdef _WIN32
    return get_username_win();
#elif __APPLE__
    return get_username_mac();
#endif
    return "unknown_username";
}

std::string get_machine_fingerprint()
{
    return get_board_serial() + "#" + get_hdd_serial() + "#" + get_username();
}
