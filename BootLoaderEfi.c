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
    OPERATING_SYSTEM_ENTRY exit;
    exit.FunctionToRun = exitFun;
    OPERATING_SYSTEM_ENTRY operatingSystems[20] = {exit};
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

