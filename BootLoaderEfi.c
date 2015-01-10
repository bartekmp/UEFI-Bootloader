#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
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
    
    void (* FunctionToRun) (); //tymaczasowe, tu wsadzimy informacje potrzebne do bootowania, miedzy innymi wskaznik na funkcję ktora ma byc wywolana (to jest to ale pewnie zmienia sie jej parametry)
} OPERATING_SYSTEM_ENTRY;


//global variables
EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;

//function declarations
EFI_STATUS ConsoleKeyRead(UINT64 *key, BOOLEAN wait);
void CallMenuEntry(OPERATING_SYSTEM_ENTRY * operatingSystems, int key);
void exitFun();

EFI_STATUS EFIAPI UefiMain (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE  *SystemTable)
{
	ST = SystemTable;
    BS = ST->BootServices;
	const CHAR16 title[] = {L"\tEFI Bootloader 1.1\n\tAndrzej Podgórski\n\tBartosz Pollok\n"};
    CHAR16 const* menu[20] = {L"wroc do efi shell\n"};
    OPERATING_SYSTEM_ENTRY operatingSystems[20] = {{.FuntionToRun = exitFun}};
    int menuEntriesCount=1;
    
    Print(title);
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

/*
    Print(L"Key pressed: %d\n", key);
	if(key == KEYPRESS(0,0,'a'))
	{
		EFI_GUID guid;
		EFI_STATUS err;
		guid.Data1=0x5B1B31A1;
		guid.Data2=0x9562;guid.Data3=0x11d2;guid.Data4[0]=0x8E;guid.Data4[1]=0x3F;guid.Data4[2]=0x00;guid.Data4[3]=0xA0;guid.Data4[4]=0xC9;guid.Data4[5]=0x69;guid.Data4[6]=0x72;guid.Data4[7]=0x3B;
EFI_GUID guid2;
		guid2.Data1=0x09576e91;
		guid2.Data2=0x6d3f;guid2.Data3=0x11d2;guid2.Data4[0]=0x8e;guid2.Data4[1]=0x39;guid2.Data4[2]=0x00;guid2.Data4[3]=0xA0;guid2.Data4[4]=0xC9;guid2.Data4[5]=0x69;guid2.Data4[6]=0x72;guid2.Data4[7]=0x3B;
		EFI_LOADED_IMAGE *loaded_image;
		Print(L"openuje protokol\n");
		err=BS->OpenProtocol(ImageHandle, &guid, (void**)&loaded_image, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
		Print(L"%d\n", err);
		EFI_DEVICE_PATH* path;
		EFI_HANDLE image;
		EFI_HANDLE guwno[1000];
UINTN l = 1000;
		BS->LocateHandle(2, &guid2, 0, &l, guwno);
Print(L"%d\n", l);

		Print(L"\npat\n");
int i;
		for (i=0; i<l; i++)
		{
			path = FileDevicePath(guwno[i], L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi");
			err=BS->LoadImage(FALSE, ImageHandle, path, NULL, 0, &image);
			if (err==0)
			{
				Print(L"kurwa dziala jebany cwel");
break;
			
			}
		}

		Print(L"%X\n", path);
		Print(L"lod img\n");

		Print(L"%d\n", err);
		Print(L"gorontze\n");
		Print(L"NO KURWA BUTUJE\n");
		BS->Stall(4000000);
		err=BS->StartImage(image, NULL, NULL);
		Print(L"%d\n", err);
 if (err == EFI_ACCESS_DENIED || err == EFI_SECURITY_VIOLATION) {
                        Print(L"\nImage gives a security error\n");
                        Print(L"Please enrol the hash or signature of\n");

                }
	}	*/
/*
	switch(key)
	{
		case KEYPRESS(0,0,'1'):
		{	EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
			BS->OpenProtocol(ImageHandle, &LoadedImageProtocol, (void**)&loaded_image, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
			EFI_DEVICE_PATH path;
			path = FileDevicePath(loaded_image->device, L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi");
		}
		break;
		case KEYPRESS(0,0,'0'):
		default:
			Print(L"no elo");
			break;
		
	}*/

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
    operatingSystems[key].FunctionToRun();
}
void exitFun()
{
    Print(L"Opuszczam program");
}

