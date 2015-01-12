#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DevicePathLib.h>

#define KEYPRESS(keys, scan, uni) ((((UINT64)keys) << 32) | ((scan) << 16) | (uni))

// structures
typedef struct 
{
	EFI_HANDLE device;
	CHAR16* path;
	CHAR16* name;
} OPERATING_SYSTEM_ENTRY;

typedef struct
{
	CHAR16* path;
	CHAR16* label;
} LOADER_ENTRY;

//global variables
EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_HANDLE IH;

//constants
const unsigned char LoadersCount = 6;
const LOADER_ENTRY Loaders[] = 
		{{ .path=L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", .label=L"Windows Loader" }, 
		{ .path=L"\\EFI\\ubuntu\\grubx64.efi", .label=L"Ubuntu Loader" }, 
		{ .path=L"\\EFI\\debian\\grubx64.efi", .label=L"Debian Loader" }, 
		{ .path=L"\\EFI\\fedora\\grubx64.efi", .label=L"Fedora Loader" },
		{ .path=L"\\System\\Library\\CoreServices\\boot.efi", .label=L"Mac OSX Loader" },
		{ .path=L"\\shellx64.efi", .label=L"EFI Shell" }};
		
//function declarations
unsigned int GetEntries(const CHAR16** menu, OPERATING_SYSTEM_ENTRY * operatingSystems, unsigned char firstKey);
EFI_STATUS ConsoleKeyRead(UINT64 *key, BOOLEAN wait);
EFI_STATUS CallMenuEntry(OPERATING_SYSTEM_ENTRY * operatingSystems, unsigned int key);
EFI_STATUS LoadSystem(OPERATING_SYSTEM_ENTRY sys);
EFI_STATUS exitFun();

EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable)
{
	const CHAR16 asciiLogo[][72]=
		{{L" ____                 _    _                        _             \n"}, 
		{L"|  _ \\               | |  | |                      | |            \n"},
		{L"| |_) |  ___    ___  | |_ | |      ___    __ _   __| |  ___  _ __ \n"},
		{L"|  _ <  / _ \\  / _ \\ | __|| |     / _ \\  / _` | / _` | / _ \\| '__|\n"},
		{L"| |_) || (_) || (_) || |_ | |____| (_) || (_| || (_| ||  __/| |   \n"},
		{L"|____/  \\___/  \\___/  \\__||______|\\___/  \\__,_| \\__,_| \\___||_|   \n"}};
    unsigned char h;
    for(h=0; h < 6; ++h)
    {
		Print(asciiLogo[h]);
	}
	Print(L"\tEFI Bootloader 1.2\n\tAndrzej Podgórski\n\tBartosz Pollok\n");
	
	IH=ImageHandle;
	ST = SystemTable;
    BS = ST->BootServices;
    EFI_STATUS err;
    	
    CHAR16 const* menu[20] = {L"exit"};
    OPERATING_SYSTEM_ENTRY operatingSystems[20];
    unsigned int menuEntriesCount=1;
    
    unsigned int numOfLoaders = GetEntries(menu, operatingSystems, menuEntriesCount);
    menuEntriesCount+= numOfLoaders;
    
    unsigned short int i;
    for (i=0; i< menuEntriesCount; i++)
    {
        Print(L"%d - %s\n", i, menu[i]);
    }
    
    UINT64 key;
    
    do 
    {
        err=ConsoleKeyRead(&key, 1);
        if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
		{ 	
			Print(L"Key read error!\n"); 	
		}
		
    } while (!(key-48 >=0 && key-48 < menuEntriesCount));
    
	if(key-48==0)
		err=exitFun();
	else
		err=CallMenuEntry(operatingSystems, key-48);
	
	if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
	{ 	
		Print(L"Loader file access error!\n"); 	
	}

	return EFI_SUCCESS;

}

EFI_STATUS ConsoleKeyRead(UINT64 *key, BOOLEAN wait) {

        UINTN index;
        EFI_INPUT_KEY k;
        EFI_STATUS err;
        BS->WaitForEvent( 1, &ST->ConIn->WaitForKey, &index);

        err  = ST->ConIn->ReadKeyStroke(ST->ConIn, &k);
        if (EFI_ERROR(err))
                return err;

        *key = KEYPRESS(0, k.ScanCode, k.UnicodeChar);
        return EFI_SUCCESS;
}

EFI_STATUS CallMenuEntry(OPERATING_SYSTEM_ENTRY * operatingSystems, unsigned int key)
{
    return LoadSystem(operatingSystems[key]);
}
EFI_STATUS exitFun()
{
    Print(L"Exiting program");
    return EFI_SUCCESS;
}

EFI_STATUS LoadSystem(OPERATING_SYSTEM_ENTRY sys)
{
	EFI_HANDLE image;
	EFI_DEVICE_PATH *path;
	path = FileDevicePath(sys.device, sys.path); 	
	EFI_STATUS err;
	BS->LoadImage(FALSE, IH, path, NULL, 0, &image);
	err=BS->StartImage(image, NULL, NULL); 	

	return err;
}

unsigned int GetEntries(const CHAR16** menu, OPERATING_SYSTEM_ENTRY * operatingSystems, unsigned char firstKey)
{
	unsigned int num = 0;
	EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	UINTN length = 1000;
	EFI_HANDLE devices[length];
	BS->LocateHandle(2, &guid, 0, &length, devices);
	length = length/sizeof(EFI_HANDLE);
	unsigned int i;
	for (i = 0; i < length; ++i)
	{
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL * fs;
		EFI_FILE_PROTOCOL *root;
		BS->HandleProtocol(devices[i],&guid, (void **) &fs);
		fs->OpenVolume(fs, &root);
		EFI_FILE_PROTOCOL * file;
		unsigned int j;
		for(j = 0; j < LoadersCount; ++j)
		{
			if(root->Open(root,&file, Loaders[j].path, EFI_FILE_MODE_READ, 0ULL) == EFI_SUCCESS)
			{
				file->Close(file);
				OPERATING_SYSTEM_ENTRY sys={.device=devices[i], .path = Loaders[j].path, .name = Loaders[j].label};
				operatingSystems[firstKey+num]=sys;
				menu[firstKey+num]=Loaders[j].label;
				num++;
			}
		}
	}
	return num;
}
