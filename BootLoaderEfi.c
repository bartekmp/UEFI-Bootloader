#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DevicePathLib.h>

#define KEYPRESS(keys, scan, uni) ((((UINT64)keys) << 32) | ((scan) << 16) | (uni))

/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
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
LOADER_ENTRY Loaders[] = {{.path=L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", .label=L"Windows Loader"}, 
		{.path=L"\\EFI\\ubuntu\\grubx64.efi", .label=L"Ubuntu Loader"}, 
		{.path=L"\\EFI\\debian\\grubx64.efi", .label=L"Debian Loader"}, 
		{.path=L"\\EFI\\fedora\\grubx64.efi", .label=L"Fedora Loader"},
		{.path=L"\\System\\Library\\CoreServices\\boot.efi", .label=L"Mac OSX Loader"},
		{.path=L"\\shellx64.efi", .label=L"EFI Shell"}};

//constants
const unsigned int LoadersCount = 6;

//function declarations
EFI_STATUS ConsoleKeyRead(UINT64 *key, BOOLEAN wait);
void CallMenuEntry(OPERATING_SYSTEM_ENTRY * operatingSystems, int key);
void exitFun();
void LoadSystem(OPERATING_SYSTEM_ENTRY sys);

int GetEntries(const CHAR16** menu, OPERATING_SYSTEM_ENTRY * operatingSystems, int firstKey);

EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable)
{
	IH=ImageHandle;
	ST = SystemTable;
    BS = ST->BootServices;
	const CHAR16 title[] = {L"\tEFI Bootloader 1.1\n\tAndrzej Podgórski\n\tBartosz Pollok\n"};
	
    CHAR16 const* menu[20] = {L"0 - wyjscie\n"};
    OPERATING_SYSTEM_ENTRY operatingSystems[20];
    int menuEntriesCount=0;
    
    Print(title);
    int numOfLoaders = GetEntries(menu, operatingSystems, menuEntriesCount);
    menuEntriesCount+= numOfLoaders;
    
    int i;
    for (i=0; i< menuEntriesCount; i++)
    {
        Print(L"%d - %s\n", i+1, menu[i]);
    }
    UINT64 key;
    do 
    {
        ConsoleKeyRead(&key, 1);
    }while (!(key-48 >=0 && key-48 < menuEntriesCount));
    
	if(key-48==0)
		exitFun();
	else
		CallMenuEntry(operatingSystems, key-48);

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
        return 0;
}

void CallMenuEntry(OPERATING_SYSTEM_ENTRY * operatingSystems, int key)
{
    LoadSystem(operatingSystems[key]);
}
void exitFun()
{
    Print(L"Opuszczam program");
}

void LoadSystem(OPERATING_SYSTEM_ENTRY sys)
{
	EFI_HANDLE image;
	EFI_DEVICE_PATH *path;
	path = FileDevicePath(sys.device, sys.path); 	
	EFI_STATUS err;
	BS->LoadImage(FALSE, IH, path, NULL, 0, &image);
	err=BS->StartImage(image, NULL, NULL); 	
	if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
	{ 	
		Print(L"Blad dostepu do pliku loadera!\n"); 	
	}
}

int GetEntries(const CHAR16** menu, OPERATING_SYSTEM_ENTRY * operatingSystems, int firstKey)
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
