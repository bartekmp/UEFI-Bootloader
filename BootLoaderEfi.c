#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileSystemInfo.h>
#include <Library/DevicePathLib.h>

#define KEYPRESS(keys, scan, uni) ((((UINT64)keys) << 32) | ((scan) << 16) | (uni))
#define EFI_TIMER_PERIOD_SECONDS(Seconds) MultU64x32((UINT64)(Seconds), 10000000)

// structures
typedef struct 
{
    EFI_HANDLE device;
    CHAR16* path;
    CHAR16* name;
    CHAR16* label;
    UINT64 size;
} OPERATING_SYSTEM_ENTRY;

typedef struct
{
    CHAR16* path;
    CHAR16* label;
} LOADER_ENTRY;

typedef struct
{
    char timeout;
    unsigned int entries;
    OPERATING_SYSTEM_ENTRY* systems;
} CALLBACK_CONTEXT;

//global variables
EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_HANDLE IH;

//constants
const char timeout = 5;
const unsigned char LoadersCount = 6;
const LOADER_ENTRY Loaders[] = // well-known system loaders to find
        {{ .path=L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", .label=L"Windows Loader" }, 
        { .path=L"\\EFI\\ubuntu\\grubx64.efi", .label=L"Ubuntu Loader" }, 
        { .path=L"\\EFI\\debian\\grubx64.efi", .label=L"Debian Loader" }, 
        { .path=L"\\EFI\\fedora\\grubx64.efi", .label=L"Fedora Loader" },
        { .path=L"\\System\\Library\\CoreServices\\boot.efi", .label=L"Mac OSX Loader" },
        { .path=L"\\shellx64.efi", .label=L"EFI Shell" }};
        
//function declarations
unsigned int GetEntries(OPERATING_SYSTEM_ENTRY* operatingSystems, unsigned char firstKey);

EFI_STATUS ConsoleKeyRead(UINT64* key, BOOLEAN wait); // reads one key pressed by user
EFI_STATUS CallMenuEntry(OPERATING_SYSTEM_ENTRY* operatingSystems, unsigned int key); // calls LoadSystem
EFI_STATUS LoadSystem(OPERATING_SYSTEM_ENTRY sys); // loads system selected by user
EFI_STATUS ExitBootloader();
EFI_STATUS ClearScreen();

VOID EFIAPI TimerCallback(EFI_EVENT event, void* context); // timer callback procedure

//MAIN
EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
    IH = ImageHandle;
    ST = SystemTable;
    BS = ST->BootServices;
    
    EFI_STATUS err;
    
    err = ClearScreen();
    if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
    {   
        Print(L"Unable to clear screen!\n");
    }
    
    const CHAR16 asciiLogo[][72] =
        {{L" ____                 _    _                        _             \n"}, 
        {L"|  _ \\               | |  | |                      | |            \n"},
        {L"| |_) |  ___    ___  | |_ | |      ___    __ _   __| |  ___  _ __ \n"},
        {L"|  _ <  / _ \\  / _ \\ | __|| |     / _ \\  / _` | / _` | / _ \\| '__|\n"},
        {L"| |_) || (_) || (_) || |_ | |____| (_) || (_| || (_| ||  __/| |   \n"},
        {L"|____/  \\___/  \\___/  \\__||______|\\___/  \\__,_| \\__,_| \\___||_|   \n"}};
    unsigned char h;
    for(h = 0; h < 6; ++h)
    {
        Print(asciiLogo[h]);
    }
    Print(L"\tEFI Bootloader 1.2\n");
            
    OPERATING_SYSTEM_ENTRY operatingSystems[20];
    unsigned int menuEntriesCount = 1;
    
    unsigned int numOfLoaders = GetEntries(operatingSystems, menuEntriesCount);
    menuEntriesCount += numOfLoaders;
    
    unsigned short int i;
    Print(L"0 - exit\n");
    for (i = 1; i< menuEntriesCount; i++)
    {
        Print(L"%d - %s on filesystem with label \"%s\" and size %dMB \n", i, operatingSystems[i].name, operatingSystems[i].label, operatingSystems[i].size>>20);
    }
    
    EFI_EVENT timer;
    if(menuEntriesCount > 1)
    {
        CALLBACK_CONTEXT context = {timeout, menuEntriesCount, operatingSystems};
        
        err = BS->CreateEventEx(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, &TimerCallback, &context, NULL, &timer);
        if(err == EFI_INVALID_PARAMETER)    
            Print(L"Invalid parameters for timer creation\n");        
         
        err = BS->SetTimer(timer, TimerPeriodic, EFI_TIMER_PERIOD_SECONDS(1));
        if (err==EFI_INVALID_PARAMETER)
            Print(L"Invalid parameters for SetTimer\n");
    }
    else
    {
        Print(L"No operating systems found!\n");
    }
    
    UINT64 key;
    do 
    {
        err = ConsoleKeyRead(&key, 1);
        BS->CloseEvent(timer);
        if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
        {   
            Print(L"Key read error!\n");    
        }
        
    } while (!(key-48 >=0 && key-48 < menuEntriesCount));
    
    if(key-48 == 0)
        err = ExitBootloader();
    else
        err = CallMenuEntry(operatingSystems, key-48);
    
    if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
    {   
        Print(L"Loader file access error!\n");  
    }

    return EFI_SUCCESS;

}

EFI_STATUS ClearScreen()
{
    return ST->ConOut->ClearScreen(ST->ConOut);
}

EFI_STATUS ConsoleKeyRead(UINT64* key, BOOLEAN wait)
{
    UINTN index;
    EFI_INPUT_KEY k;
    EFI_STATUS err;
    BS->WaitForEvent(1, &ST->ConIn->WaitForKey, &index);

    err = ST->ConIn->ReadKeyStroke(ST->ConIn, &k);
    if (EFI_ERROR(err))
        return err;

    *key = KEYPRESS(0, k.ScanCode, k.UnicodeChar);
    return EFI_SUCCESS;
}

VOID EFIAPI TimerCallback(EFI_EVENT event, void* context)
{
    CALLBACK_CONTEXT* ctx = (CALLBACK_CONTEXT*)context;
    Print(L"%d...", ctx->timeout--);
    if(ctx->timeout < 0)
    {
        EFI_STATUS err;
        Print(L"\n");
        if(ctx->entries > 1)
        {
            err = CallMenuEntry(ctx->systems, 1);
            if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
            {   
                Print(L"Loader file access error!\n");  
            }
        }
        else
        {
            err = ExitBootloader();
        }
    }
}

EFI_STATUS CallMenuEntry(OPERATING_SYSTEM_ENTRY* operatingSystems, unsigned int key)
{
    Print(L"\nLoading %s", operatingSystems[key].name);
    return LoadSystem(operatingSystems[key]);
}
EFI_STATUS ExitBootloader()
{
    Print(L"Exiting EFI BootLoader");
    return EFI_SUCCESS;
}

EFI_STATUS LoadSystem(OPERATING_SYSTEM_ENTRY sys)
{
    EFI_HANDLE image;
    EFI_DEVICE_PATH* path;
    EFI_STATUS err;
    path = FileDevicePath(sys.device, sys.path);    
    BS->LoadImage(FALSE, IH, path, NULL, 0, &image);
    
    err = ClearScreen();
    if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
    {   
        Print(L"Unable to clear screen!\n");
    }
    
    err = BS->StartImage(image, NULL, NULL);  

    return err;
}

unsigned int GetEntries(OPERATING_SYSTEM_ENTRY* operatingSystems, unsigned char firstKey)
{
    unsigned int num = 0;
    EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID fsi_guid = EFI_FILE_SYSTEM_INFO_ID;
    UINTN length = 1000;
    EFI_HANDLE devices[length];
    BS->LocateHandle(2, &guid, 0, &length, devices);
    length = length/sizeof(EFI_HANDLE);
    unsigned int i;
    for (i = 0; i < length; ++i)
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
        EFI_FILE_PROTOCOL* root;
	UINTN buffer_size = 200;
	UINTN * buffer[200];
        BS->HandleProtocol(devices[i], &guid, (void **) &fs);
        fs->OpenVolume(fs, &root);
	EFI_STATUS deviceInfoTaken = root->GetInfo(root,&fsi_guid, &buffer_size,(void *) buffer);
	EFI_FILE_PROTOCOL* file;
        unsigned int j;
        for(j = 0; j < LoadersCount; ++j)
        {
            if(root->Open(root,&file, Loaders[j].path, EFI_FILE_MODE_READ, 0ULL) == EFI_SUCCESS)
            {
                file->Close(file);
		CHAR16 * label=L"<no label>";
		UINT64 size=0;
		if (deviceInfoTaken == EFI_SUCCESS)
		{
			EFI_FILE_SYSTEM_INFO * fi = (EFI_FILE_SYSTEM_INFO *) buffer;
			size = fi->VolumeSize;
			label = fi->VolumeLabel;
		}
                OPERATING_SYSTEM_ENTRY sys = {.device=devices[i], .path = Loaders[j].path, .name = Loaders[j].label, .label=label, .size=size};
                operatingSystems[firstKey+num] = sys;
                num++;
            }
        }
    }
    return num;
}
