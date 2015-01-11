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
    void (* FunctionToRun) (EFI_HANDLE); //tymaczasowe, tu wsadzimy informacje potrzebne do bootowania, miedzy innymi wskaznik na funkcję ktora ma byc wywolana (to jest to ale pewnie zmienia sie jej parametry)
} OPERATING_SYSTEM_ENTRY;


//global variables
EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_HANDLE IH;

//function declarations
EFI_STATUS ConsoleKeyRead(UINT64 *key, BOOLEAN wait);
void CallMenuEntry(OPERATING_SYSTEM_ENTRY * operatingSystems, int key);
void exitFun(EFI_HANDLE device);
void LoadWindows(EFI_HANDLE device);

int GetWindowsEntries(const CHAR16** menu, OPERATING_SYSTEM_ENTRY * operatingSystems, int firstKey);

EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable)
{
	IH=ImageHandle;
	ST = SystemTable;
    BS = ST->BootServices;
	const CHAR16 title[] = {L"\tEFI Bootloader 1.1\n\tAndrzej Podgórski\n\tBartosz Pollok\n"};
    CHAR16 const* menu[20] = {L"wroc do efi shell\n"};
    OPERATING_SYSTEM_ENTRY exit;
    exit.FunctionToRun = exitFun;
    OPERATING_SYSTEM_ENTRY operatingSystems[20] = {exit};
    int menuEntriesCount=1;
    
    Print(title);
    
    int numOfWindows = GetWindowsEntries(menu, operatingSystems, menuEntriesCount);
    menuEntriesCount+= numOfWindows;
//TODO: pobrac wpisy o systemach i wstawic do menu pod odpowiednie indeksy do jakiejś tablicy pod indeksy takie jak w menu
    int i;
    for (i=0; i< menuEntriesCount; i++)
    {
        Print(L"%d - %s\n", i, menu[i]);
    }
    UINT64 key;
    do 
    {
        ConsoleKeyRead(&key, 1);
    }while (!(key-48 >=0 && key-48 < menuEntriesCount));

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
    operatingSystems[key].FunctionToRun(operatingSystems[key].device);
}
void exitFun(EFI_HANDLE device)
{
    Print(L"Opuszczam program");
}

void LoadWindows(EFI_HANDLE device)
{
	EFI_HANDLE image;
	EFI_DEVICE_PATH *path;
	path = FileDevicePath(device, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi"); 	
	EFI_STATUS err;
	BS->LoadImage(FALSE, IH, path, NULL, 0, &image);
	err=BS->StartImage(image, NULL, NULL); 	
	if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) 
	{ 	
		Print(L"Blad dostepu do pliku loadera!\n"); 	
	}
}

int GetWindowsEntries(const CHAR16** menu, OPERATING_SYSTEM_ENTRY * operatingSystems, int firstKey)
{
	int num=0;
	EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_HANDLE devices[1000];
	UINTN l = 1000;
	BS->LocateHandle(2, &guid, 0, &l, devices);
	l = l/sizeof(EFI_HANDLE);
	int i;
	for (i=0; i<l;i++)
	{
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL * fs;
		EFI_FILE_PROTOCOL *root;
		BS->HandleProtocol(devices[i],&guid, (void **) &fs);
		fs->OpenVolume(fs, &root);
		EFI_FILE_PROTOCOL * file;
		EFI_STATUS err;
		err = root->Open(root,&file, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", EFI_FILE_MODE_READ, 0ULL);
		if (err == EFI_SUCCESS)
		{
			file->Close(file);
			OPERATING_SYSTEM_ENTRY sys;
			sys.FunctionToRun=LoadWindows;
			sys.device=devices[i];
			operatingSystems[firstKey+num]=sys;
			menu[firstKey+num]=L"Microsoft Windows loader";
			num++;
		}
		
	}
	return num;
}
