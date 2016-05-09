/*

****************************************
		RFSGUI 2.2: 11-09-2005
****************************************

----------------------------------------
				Author
----------------------------------------

	Michael Adams
	http://www.wolfsheep.com
	cwolfsheep@yahoo.com (email if site's down)

----------------------------------------
				History
----------------------------------------

0.1: 5-21-2003; 5-22-2003; 5-23-2003; 5-24-2003
A function a day keeps this program from going astray ;)

0.8: 5-29-2003; 5-30-2003
Interfaces & strings are the key!

0.9: 6-26-2003
Everything but partition-selection works.

1.0: 7-13-2003
Everything should be working!

1.1 10-1-2003
Let all the files be free!

1.2 11-13-2003
Took a month of scattered thought to match the mighty Kurz!

1.3 6-20-2004
Recompiled with VC6-SP6; fixed a syntactal error; listbox more fluid

1.4 1-10-2005
Back to VC6-SP5; user & self-suggested interface tweaks.

2.0 5-2-2005
Cleanup & port to AMD64 & Windows 2003 SP1 x64 SDK

2.1 5-8-2005
More cleanup

2.2 11-9-2005
Attempt to fix bugs

----------------------------------------
			Source Credits
----------------------------------------

rfstool.cpp -- RFSTool 0.13

1. Assmiliated most of the main() functions into new functions or GUI-message-procedures.
2. Retained the auxillary functions.
3. Globalized some of the function variables: repeated use of the same syntax;
!. The original files outside rfstool.cpp in this software are unchanged.
!. Later RFSTool versions are incorporated @ source level as able.

ENVIRON.C -- Environment List Box; (c) Charles Petzold, 1998

Petzold had an example listbox application in his "Programming Windows, 5th Edition" book.
I started building the GUI on top of that program & now only a fraction of that code
remains: the original listbox was buggy per the needs of this program, and Microsoft's
"Platform SDK" from October 2002 has useful detail on non-MFC / Windows SDK listboxes.

*/

//Includes
#include "..\rfstool\precomp.h"
#include "..\rfstool\reiserfs.h"
#include "..\rfstool\gtools.h"

//RFSTOOL: Unix/Linux permissions
#define	S_IRUSR	0000400	/* read permission, owner */
#define	S_IWUSR	0000200	/* write permission, owner */
#define	S_IXUSR 0000100	/* execute/search permission, owner */
#define	S_IRGRP	0000040	/* read permission, group */
#define	S_IWGRP	0000020	/* write permission, grougroup */
#define	S_IXGRP 0000010	/* execute/search permission, group */
#define	S_IROTH	0000004	/* read permission, other */
#define	S_IWOTH	0000002	/* write permission, other */
#define	S_IXOTH 0000001	/* execute/search permission, other */

//RFSGUI: GUI Element IDs
#define ID_LIST     1	//List box
#define ID_TEXT     2	//Text box
#define ID_BUTTON_COPY     3	//Copy button
#define ID_BUTTON_ABOUT     4	//About button
#define ID_BUTTON_DIRECTORY 5	//Change directory button
#define ID_EDIT_PATH 6	//Directory path
#define ID_EDIT_COPY 7	//Copy path
#define ID_STATIC_STATUS 8 //Status (static)
#define ID_BUTTON_RECURSIVE 9 //Button for recursive copying
#define ID_BUTTON_BUGS 10 //Bugs information button
#define ID_BUTTON_DUMPTREE 11 //Tree dumping button
#define ID_STATIC_TOTALS 12 //Total (static)
#define ID_BUTTON_DUMPINFO 13 //Part. info. dumping button
#define ID_STATIC_LABEL_PERMISSIONS 14 //Static label: permissions
#define ID_STATIC_LABEL_TIMEDATE 15 //Static label: time/date
#define ID_STATIC_LABEL_TYPESIZE 16 //Static label: type or size
#define ID_STATIC_PERMISSIONS 17 //Static: permissions
#define ID_STATIC_TIMEDATE 18 //Static: time/date
#define ID_STATIC_TYPESIZE 19 //Static: type or size
#define ID_STATIC_LABEL_PATH 20	//Static: directory-source label
#define ID_STATIC_LABEL_COPY 21	//Static: copy-target label
#define ID_EDIT_PARTITIONID 22	//Partition-ID string
#define ID_COMBOBOX_PARTITION 24	//Partition-ID string
#define ID_STATIC_LABEL_DRIVEID 25	//Drive-ID string
#define ID_ICON	100	//Attempt to pull saved icon

//RFSGUI: dump extra directory info into structures
struct dirStruct{
public:
	char permissionsStr[256];	//permissions
	char timedateStr[256];		//time & date
	char typesizeStr[256];		//DIR or filesize
	char symlinkStr[256];		//symlink property
};

//RFSTOOL: Globals
char* g_szUseSpecificDevice = NULL;
char* pszRestoreFile = NULL;
LONG_PTR iPartition = -1, iDrive = -1, iSelection = -1;
bool boolRecurseSubdirectories = false;
char* p = getenv("REISERFS_PARTITION");
char dirname[512] = "/";
char localfile[512] = "C:\\TEMP";
HKEY hkSubkey;
DWORD dwDisposition, dwType, dwSize;
RECT rcListbox, rcMain;
LONG_PTR drivelisting[99];
LONG_PTR partitioncount;
INT32 lbxSize, lbySize, mainxSize, mainySize;

//RFSGUI: Number of directory entries to monitor: use Win9x limit
dirStruct* arrayOfDirStructs = new dirStruct[32767];

//RFSTOOL: Backup Filename
char* g_pszReadOnlyBackupFileName = NULL;

//RFSGUI: Globals
INT32 cxChar = 0, cyChar = 0; //Global window parameters
char activedirname[512] = "/"; //Use this as a basis for some file/dir operations
char tempStr[512]; //Reuseable temporary string (strcat operations)

//RFSGUI: Count # used for dir listings
long count = -1; //Default is -1; listings will count++ to 0 & up

//RFSGUI: Setup a data array to refer the drive/partition ComboBox to
//Support 8 drives * 99 partitions * 2 pieces each
INT32 cbData[792][2];

//RFSGUI: Modified routine for checking directory existence
//        http://www.experts-exchange.com/Programming/Programming_Languages/Cplusplus/Q_20455711.html
bool TestPath(char* pathStr)
{
	//Check to see if we need a default filename
	HANDLE  FndHnd = NULL;   // Handle to find data.
	struct _stat fileinfo;
	WIN32_FIND_DATA FindDat;  // Info on file found.
	FndHnd = FindFirstFile(pathStr, &FindDat);
	_stat(pathStr, &fileinfo);
	
	if(S_ISDIR(fileinfo.st_mode)) return true;

	return false;
}


//RFSTOOL: Convert recovered string to Unix/Linux permissions display
//Was updated by Kurz in RFSTOOL 0.14; RFSGUI in 1.2
void LinuxPermissionsAsString( LPTSTR lpszBuffer, INT mode )
{
    *lpszBuffer++ =   (S_ISFIFO(mode))? 'f' : 
                    ( (S_ISDIR(mode)) ? 'd' : 
                    ( (S_ISCHR(mode)) ? 'c' : 
                    ( (S_ISBLK(mode)) ? 'b' :
                    ( (S_ISLNK(mode)) ? 'l' :
                    ( (S_ISREG(mode)) ? '-' :  '-' )))));
    *lpszBuffer++ = (mode & S_IRUSR) ? 'r' : '-';
    *lpszBuffer++ = (mode & S_IWUSR) ? 'w' : '-';
    *lpszBuffer++ = (mode & S_IXUSR) ? 'x' : '-';
    *lpszBuffer++ = (mode & S_IRGRP) ? 'r' : '-';
    *lpszBuffer++ = (mode & S_IWGRP) ? 'w' : '-';
    *lpszBuffer++ = (mode & S_IXGRP) ? 'x' : '-';
    *lpszBuffer++ = (mode & S_IROTH) ? 'r' : '-';
    *lpszBuffer++ = (mode & S_IWOTH) ? 'w' : '-';
    *lpszBuffer++ = (mode & S_IXOTH) ? 'x' : '-';

    *lpszBuffer = 0;
}

//RFSTOOL: Trace Function; required by auxillary files
void TRACE(const char* fmt, ...)
{
    va_list args;
    va_start( args, fmt );
    vprintf(fmt, args);
}

//RFSTOOL: Comparative Function
INT32 comparefunc(const void *elem1, const void *elem2 )
{
    return strcmp( (*((ReiserFsFileInfo**)elem1))->m_strName, (*((ReiserFsFileInfo**)elem2))->m_strName );
}

//RFSTOOL: Partition Auto-Detection & Registry Key
//RFSGUI: Modified to support loading the combobox selection
void AutodetectPartitionsFromRegistry( LONG_PTR piPartition, LONG_PTR piDrive, LONG_PTR piSelection )
{
	//Instantate a new partition action
	ReiserFsPartition partition;

	if( piDrive == -1 )
	{

		LONG lResult = RegCreateKeyEx( HKEY_CURRENT_USER,
										"SOFTWARE\\p-nand-q.com\\rfstool",
										0,
										"",
										REG_OPTION_NON_VOLATILE,
										KEY_READ,
										0,
										&hkSubkey,
										&dwDisposition );
		if( lResult == ERROR_SUCCESS )
		{
			dwSize = sizeof(DWORD);
			if( RegQueryValueEx(hkSubkey, "Drive", 0, &dwType, (BYTE*) &dwDisposition, &dwSize ) == ERROR_SUCCESS )
			{
				if( dwType == REG_DWORD )
				{
					piDrive = (INT32) dwDisposition;
					iDrive = piDrive; //RFSGUI: use this for starting value
				}
			}
			dwSize = sizeof(DWORD);
			if( RegQueryValueEx(hkSubkey, "Partition", 0, &dwType, (BYTE*) &dwDisposition, &dwSize ) == ERROR_SUCCESS )
			{
				if( dwType == REG_DWORD )
				{
					piPartition = (INT32) dwDisposition;
					iPartition = piPartition; //RFSGUI: use this for starting value
				}
			}
			dwSize = sizeof(DWORD);
			if( RegQueryValueEx(hkSubkey, "Selection", 0, &dwType, (BYTE*) &dwDisposition, &dwSize ) == ERROR_SUCCESS )
			{
				if( dwType == REG_DWORD )
				{
					piSelection = (INT32) dwDisposition;
					iSelection = piSelection; //RFSGUI: use this for starting value
				}
			}
			RegCloseKey( hkSubkey );
		}
		if( piDrive == -1 || piPartition == -1 )
		{
			partition.AutodetectFirstUsable( piPartition, piDrive );
		}
	}
}


//RFSTOOL: Save Autodetected or Last-Used Partition in Registry
//RFSGUI: Modified to support saving the combobox selection
void SaveLastUsedPartitionInRegistry(LONG_PTR iPartition,LONG_PTR iDrive,LONG_PTR iSelection)
{
	LONG lResult = RegCreateKeyEx( HKEY_CURRENT_USER,
									"SOFTWARE\\p-nand-q.com\\rfstool",
									0,
									"",
									REG_OPTION_NON_VOLATILE,
									KEY_WRITE,
									0,
									&hkSubkey,
									&dwDisposition );
	if( lResult == ERROR_SUCCESS )
	{
		if( iDrive >= 0 )
		{
			dwDisposition = (DWORD) iDrive;
			RegSetValueEx(hkSubkey, "Drive", 0, REG_DWORD, (CONST BYTE*) &dwDisposition, sizeof(DWORD) );
		}
		if( iPartition >= 0 )
		{
			dwDisposition = (DWORD) iPartition;
			RegSetValueEx(hkSubkey, "Partition", 0, REG_DWORD, (CONST BYTE*) &dwDisposition, sizeof(DWORD) );
		}
		if( iSelection >= 0 )
		{
			dwDisposition = (DWORD) iSelection;
			RegSetValueEx(hkSubkey, "Selection", 0, REG_DWORD, (CONST BYTE*) &dwDisposition, sizeof(DWORD) );
		}
		RegCloseKey( hkSubkey );
	}
}


//RFSGUI: WndProc declaration
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

//RFSGUI: Windows Elements
INT32 WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPTSTR szCmdLine, INT32 iCmdShow)
{
     static TCHAR szAppName[] = TEXT ("rfsgui-2.2") ;
     HWND         hwnd ;
     MSG          msg ;
     WNDCLASS     wndclass ;

     wndclass.style         = CS_HREDRAW | CS_VREDRAW;
     wndclass.lpfnWndProc   = WndProc ;
     wndclass.cbClsExtra    = 0 ;
     wndclass.cbWndExtra    = 0 ;
     wndclass.hInstance     = hInstance ;
     wndclass.hIcon         = LoadIcon(hInstance,MAKEINTRESOURCE(ID_ICON));
     wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
     wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
     wndclass.lpszMenuName  = NULL ;
     wndclass.lpszClassName = szAppName ;
     
	 //Error out: not a WinNT-based system
     if (!RegisterClass (&wndclass))
     {
          MessageBox (NULL, TEXT ("This program requires Windows NT/2000/XP/2003!"),
                      szAppName, MB_ICONERROR | MB_OK) ;
          return 0 ;
     }

	 //Generate character grid & parent window
     cxChar = LOWORD (GetDialogBaseUnits ()) ;
     cyChar = HIWORD (GetDialogBaseUnits ()) ;
     hwnd = CreateWindow (szAppName, TEXT (szAppName),
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          cxChar * 82,cyChar * 33,
                          NULL, NULL, hInstance, NULL) ;

	 //Load parent window
     ShowWindow (hwnd, iCmdShow) ;
     UpdateWindow (hwnd);
     
	 //Message loop
     while (GetMessage (&msg, NULL, 0, 0))
     {
          TranslateMessage (&msg) ;
          DispatchMessage (&msg) ;
     }
     return (long)msg.wParam ;
}


//RFSGUI: Previous directory ("..") string generation
void previousdir(char entry[], char product[])
{
	LONG_PTR pathnumber = 0;
	LONG_PTR counter = 0;
	LONG_PTR len = strlen(entry);

	//Scan for "/"
	for(counter = 0; counter < len; counter++)
	{
		if(entry[counter] == '/') pathnumber = counter;
	}

	//Build product: recopy up to last delimiter
	for(counter = 0; counter < pathnumber; counter++)
	{
		product[counter] = entry[counter];
	}

	//Build product: fill remainder of output string with NULLs
	for(counter = pathnumber; counter < len; counter++)
	{
		product[counter] = NULL;
	}

	//Check for rootdir (if so, correct blank that stripping leaves)
	if(product[0] == 0 || product[0] == NULL) product[0] = '/';
}

//RFSGUI: reuseable safety procedures
void safetydefault()
{
	if(!dirname) strcpy(dirname,"/");
	if(!localfile) strcpy(localfile,"C:\\TEMP");
}

//RFSGUI: empty the listbox
void VoidListBox (HWND hwndListBox) 
{
	//Delete the listbox entries onto itself
	while(SendMessage(hwndListBox,LB_DELETESTRING,0,0) != LB_ERR);
}

//RFSGUI: Directory Listing
void FillListBox (HWND hwndListBox) 
{
	//Create a timedate variable to reuse
	__time64_t temptime;
	//Instantate a new partition action
	ReiserFsPartition partition;
	//rfstool-based linked-list: recreate with every fill to prevent root-dir issues
	PList directory;
	//Reset file counter
	count = -1;
	//Safety default
	safetydefault();
	//Delete the previous listbox
	VoidListBox(hwndListBox);

	if( partition.Open(iDrive,iPartition) )
    {
            if( partition.ListDir(&directory,dirname) )
            {
                // create array of pointers 
                ReiserFsFileInfo** array = new ReiserFsFileInfo*[directory.m_lCount];
                if( array )
                {
                    INT32 index = 0;
                    ENUMERATE( &directory, ReiserFsFileInfo, pFile )
                        array[index++] = pFile;

                    SIZE_T sc = (size_t) (directory.m_lCount - 2);
                    if( sc > 1 )
                        qsort( &(array[2]), sc, sizeof(ReiserFsFileInfo*), comparefunc );

					//generate directory entries
                    for( index = 0; index < directory.m_lCount; index++ )
                    {
						//Increment count
						count++;

						//Get file info
                        ReiserFsFileInfo* pFile = array[index];

						//Generate display permissions
                        char szAttributes[256];
                        LinuxPermissionsAsString( szAttributes, pFile->m_stat.sd_mode );

						//Generate date & time of file
						temptime = pFile->m_stat.sd_mtime;
						LPTSTR strDate = _ctime64(&temptime);
						strcpy(arrayOfDirStructs[count].timedateStr,strDate);						

						//Generate information besides DIR/filesize property
						SendMessage (hwndListBox, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)(pFile->m_strName));
						strcpy(arrayOfDirStructs[count].permissionsStr,szAttributes);

						//It's a directory: give "DIR"
                        if( S_ISDIR(pFile->m_stat.sd_mode) )
                        {
						strcpy(arrayOfDirStructs[count].typesizeStr,"DIR");
                        }
						//It's a file: give filesize
                        else
                        {
						gcvt(DOUBLE(signed(pFile->m_stat.sd_size)),10,tempStr);
						strcpy(arrayOfDirStructs[count].typesizeStr,tempStr);
                        }
						//Its symlink
						if(pFile->isSymlink())						
						{
						PString strData(partition.GetFileAsString(pFile) );
						strcpy(arrayOfDirStructs[count].symlinkStr,strData);
						}
                    }
                }
            }
	}
}

//RFSGUI: Fill the partition list
void FillPartitionList(HWND hwndComboBox,HWND hwndMain)
{
	// Build list of partitions for listing
	IPhysicalDrive* drive = CreatePhysicalDriveInstance();
	char * partnumStr;
	char * drivenumStr;
	char * outputStr;
	INT32 i = 0;
	INT32 partnum = 0;
	INT32 cbIndex = 0;
	
	for(i = 0; i < 8; i++)
	{
	// Open logical drive
	if(drive->Open(i))
	{
		// Get partition data & close drive access
		partitioncount = drive->GetDriveList(drivelisting);
		drive->Close();

		// Don't try to list partitions if they can't be listed
		if(partitioncount > 0)
		{
			
			// Add to combobox
			for(partnum = 0; partnum < 99; partnum++)
			{
				// If drive ID is more than 0, its a ReiserFS drive
				if(drivelisting[partnum] > 0)
				{
				// Make new strings
				partnumStr = new char [3];
				drivenumStr = new char [3];
				outputStr = new char [25];

				// Retrieve number values
				_itoa(i,drivenumStr,10);
				_itoa(partnum,partnumStr,10);

				// Build string
				strcpy(outputStr,"Drive: ");
				strcat(outputStr,drivenumStr);
				strcat(outputStr,"  Partition: ");
				strcat(outputStr,partnumStr);

				// Update data for combo box
				cbData[cbIndex][0] = i;
				cbData[cbIndex][1] = partnum;

				// Add to ComboBox
				SendMessage(hwndComboBox,CB_ADDSTRING,0,(LPARAM)((LPTSTR)outputStr));
				cbIndex++;

				}
			}	
		}	
	}
	else 
	{
        #ifdef _WIN32
		DWORD dwLastError = GetLastError();
		if( dwLastError != 2 )
        #endif
		MessageBox(hwndMain,"Partition Info Is Unretrievable","rfsgui: Partition List",MB_OK | MB_ICONERROR);		
	}

	}// end for

	// Delete strings
	delete partnumStr;
	delete drivenumStr;
	delete outputStr;

	// Set ComboBox to 1st item & purge drive array
	SendMessage(hwndComboBox, CB_SETCURSEL, iSelection, 0);
	delete drive;
	
}

//RFSGUI: do some of the things RFSTOOL does @ startup
void StartUp()
{
	// Gather saved info on partitions
	AutodetectPartitionsFromRegistry(iPartition,iDrive,iSelection);
}

//Console redirection: trying to put console in a window
//Reference: http://support.microsoft.com:80/support/kb/articles/q105/3/05.asp&NoWebContent=1
//To close window without closing app, call FreeConsole();
void RedirectConsole()
{
	INT32 hCrt;
   FILE *hf;
   AllocConsole();
   //MPA: 5-2-2005
   //64bit
   hCrt = _open_osfhandle(
             (LONG_PTR) GetStdHandle(STD_OUTPUT_HANDLE),
             _O_TEXT
          );
   hf = _fdopen( hCrt, "w" );
   *stdout = *hf;
   setvbuf( stdout, NULL, _IONBF, 0 );
}

//RFSGUI: Console redirection to file: use with "dumptree" output
void DumpTreeToFile(char* filename, HWND hwnd)
{
	//Instantate a new partition action
	ReiserFsPartition partition;

	//"Prod" partition with an open-command to get dump to work
	if( partition.Open(iDrive,iPartition) )
	{
	FILE *stream;
	INT32  fh;
	// Modified from MSDN reference: Open a file handle.
	if( (fh = _open( filename,_O_RDWR | _O_CREAT | _O_TRUNC)) == -1 ){
		MessageBox(hwnd,"Incorrect Filename Specified!","rfsgui: dumptree",MB_OK | MB_ICONERROR);
		return;
	}
	AllocConsole();
	// Modified from MSDN reference: Change handle access to stream access.
	if( (stream = _fdopen( fh, "wt" )) == NULL ){
		MessageBox(hwnd,"Error: Cannot Dumptree to File!","rfsgui: dumptree",MB_OK | MB_ICONERROR);
		return;
	}
	// Redirect stream & perform task
	*stdout = *stream;
	setvbuf(stdout, NULL, _IONBF, 0 );
	partition.DumpTree();

	//Close & flush
	flushall(); //Flush all buffers
	fclose(stream); //Close stream
	fclose(stdout); //Close console (safety)
	FreeConsole(); //Disable console

	//Prevent "contamination" of copied files
	RedirectConsole(); //Reinitialize console
	FreeConsole(); //Disable new console

	//Done
	MessageBox(hwnd,"DumpInfo Completed!","rfsgui",MB_OK | MB_ICONEXCLAMATION);
	}
	else
	{
	MessageBox(hwnd,"Could Not Load Partition Tree!","rfsgui",MB_OK | MB_ICONERROR);
	}
}

//RFSGUI: Console redirection to file: use with partition information output
void DumpInfoToFile(char* filename, HWND hwnd)
{
	FILE *stream;
	INT32  fh;
	//Modified from MSDN reference: Open a file handle.
	if( (fh = _open( filename,_O_RDWR | _O_CREAT | _O_TRUNC)) == -1 ){
		MessageBox(hwnd,"Incorrect Filename Specified!","rfsgui: dumptree",MB_OK | MB_ICONERROR);
		return;
	}
	AllocConsole();
	//Modified from MSDN reference: Change handle access to stream access.
	if( (stream = _fdopen( fh, "wt" )) == NULL ){
		MessageBox(hwnd,"Error: Cannot DumpInfo!","rfsgui: dumptree",MB_OK | MB_ICONERROR);
		return;
	}
	//Redirect stream
	*stdout = *stream;
	setvbuf(stdout, NULL, _IONBF, 0 );
	//Task: copied from RFSTOOL
	IPhysicalDrive* drive = CreatePhysicalDriveInstance();
	CHAR szDriveName[256];
		for( INT32 i = 0; i < 8; i++ )
		{
			sprintf(szDriveName,"\\\\.\\PhysicalDrive%d", i );
			if( drive->Open(i) )
			{
				drive->DumpDriveInfo(szDriveName);
				drive->Close();
			}
			else 
			{
                #ifdef _WIN32
				DWORD dwLastError = GetLastError();
				if( dwLastError != 2 )
                #endif
				MessageBox(hwnd,"Partition Info Is Unretrievable","rfsgui: dumpinfo",MB_OK | MB_ICONERROR);
			}
		}
		delete drive;

	//Close & flush
	flushall(); //Flush all buffers
	fclose(stream); //Close stream
	fclose(stdout); //Close console (safety)
	FreeConsole(); //Disable console

	//Prevent "contamination" of copied files
	RedirectConsole(); //Reinitialize console
	FreeConsole(); //Disable new console

	//Done
	MessageBox(hwnd,"DumpInfo Completed!","rfsgui",MB_OK | MB_ICONEXCLAMATION);
}

//RFSGUI: Windows Event Processor
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	 //Window handler declaration
     static HWND  hwndListBox, hwndCopyButton, hwndAboutButton,
		          hwndCDButton, hwndPathEdit, hwndCopyEdit, hwndStatusStatic,
				  hwndRecursiveButton, hwndBugsButton, hwndDumpTreeButton,
				  hwndTotalsStatic, hwndConsole, hwndDumpInfoButton,
				  hwndPermissionsLabelStatic,hwndTimeDateLabelStatic,hwndTypeSizeLabelStatic,
				  hwndPermissionsStatic,hwndTimeDateStatic,hwndTypeSizeStatic,
				  hwndPathLabelStatic, hwndCopyLabelStatic,
				  hwndPartitionComboBox,hwndDriveIDStatic;

	 //Application message processor: respond to different Windows events
     switch (message)
     {
     
	 // Create listbox and static text windows.
     case WM_CREATE :

		  //Load rectangle parameters to measure against
		  GetWindowRect(hwnd,&rcMain);
		  GetWindowRect(hwndListBox,&rcListbox);

		  //Listbox: DO NOT MAKE LBS_STANDARD OR LBS_SORT! Dir listings are NOT alphabetical!
          hwndListBox = CreateWindow (TEXT ("listbox"), NULL,
                              WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_HASSTRINGS | LBS_NOTIFY | WS_TABSTOP,
                              cxChar, cyChar,
                              cxChar * 34 + GetSystemMetrics (SM_CXVSCROLL),
                              (rcMain.bottom - rcMain.top) - (cyChar * 3),
                              hwnd, (HMENU) ID_LIST,
                              (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
                              NULL) ;

  		  //Static Textbox Label: Directory-Source
          hwndPathLabelStatic = CreateWindow (TEXT("static"),"Partition Path",
							WS_CHILD | WS_VISIBLE | SS_LEFT,
							cxChar * 38, cyChar * 1,
							cxChar*11,cyChar*1,
							hwnd, (HMENU) ID_STATIC_LABEL_PATH,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //Directory-Source Textbox
          hwndPathEdit = CreateWindowEx (WS_EX_CLIENTEDGE,TEXT("edit"),dirname,
							WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
							cxChar * 50, cyChar * 1,
							cxChar*30,cyChar*2,
							hwnd, (HMENU) ID_EDIT_PATH,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);


		  //Enter Directory Button
          hwndCDButton = CreateWindow (TEXT("button"),"Change Directory/Drive/Partition",
							WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
							cxChar * 38, cyChar * 4,
							cxChar*30,cyChar*2,
							hwnd, (HMENU) ID_BUTTON_DIRECTORY,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Static Textbox Label: Copy-Target
          hwndCopyLabelStatic = CreateWindow (TEXT("static"),"Local Target",
							WS_CHILD | WS_VISIBLE | SS_LEFT,
							cxChar * 38, cyChar * 7,
							cxChar*11,cyChar*1,
							hwnd, (HMENU) ID_STATIC_LABEL_COPY,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Copy-Target Textbox
          hwndCopyEdit = CreateWindowEx (WS_EX_CLIENTEDGE,TEXT("edit"),"C:\\TEMP",
							WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
							cxChar * 50, cyChar * 7,
							cxChar*30,cyChar*2,
							hwnd, (HMENU) ID_EDIT_COPY,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //Copy Button
          hwndCopyButton = CreateWindow (TEXT("button"),"Copy",
							WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
							cxChar * 38, cyChar * 10,
							cxChar*10,cyChar*2,
							hwnd, (HMENU) ID_BUTTON_COPY,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //Recursive Button (Toggle)
          hwndRecursiveButton = CreateWindow (TEXT("button"),"Recursive Directory Copy is Off",
							WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
							cxChar * 50, cyChar * 10,
							cxChar*30,cyChar*2,
							hwnd, (HMENU) ID_BUTTON_RECURSIVE,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //Dump Tree Button
          hwndDumpTreeButton = CreateWindow (TEXT("button"),"Dump Tree to File",
							WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
							cxChar * 38, cyChar * 13,
							cxChar*18,cyChar*2,
							hwnd, (HMENU) ID_BUTTON_DUMPTREE,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //Dump Info Button
          hwndDumpInfoButton = CreateWindow (TEXT("button"),"Dump Partition Info to File",
							WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
							cxChar * 57, cyChar * 13,
							cxChar*23,cyChar*2,
							hwnd, (HMENU) ID_BUTTON_DUMPINFO,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //About Button
          hwndAboutButton = CreateWindow (TEXT("button"),"About",
							WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
							cxChar * 38, cyChar * 28,
							cxChar*10,cyChar*2,
							hwnd, (HMENU) ID_BUTTON_ABOUT,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Static Textbox Label: Permissions
          hwndPermissionsLabelStatic = CreateWindow (TEXT("static"),"Permissions",
							WS_CHILD | WS_VISIBLE | SS_LEFT,
							cxChar * 38, cyChar * 16,
							cxChar*13,cyChar*1,
							hwnd, (HMENU) ID_STATIC_LABEL_PERMISSIONS,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Static Textbox Label: Time & Date
          hwndTimeDateLabelStatic = CreateWindow (TEXT("static"),"Time / Date",
							WS_CHILD | WS_VISIBLE | SS_LEFT,
							cxChar * 38, cyChar * 18,
							cxChar*13,cyChar*1,
							hwnd, (HMENU) ID_STATIC_LABEL_TIMEDATE,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Static Textbox Label: Type (DIR) or Size (of file)
          hwndTypeSizeLabelStatic = CreateWindow (TEXT("static"),"DIR or Filesize",
							WS_CHILD | WS_VISIBLE | SS_LEFT,
							cxChar * 38, cyChar * 20,
							cxChar*13,cyChar*1,
							hwnd, (HMENU) ID_STATIC_LABEL_TYPESIZE,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Static Textbox: Permissions
          hwndPermissionsStatic = CreateWindow (TEXT("static"),NULL,
							WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
							cxChar * 52, cyChar * 16,
							cxChar*25,cyChar*1,
							hwnd, (HMENU) ID_STATIC_PERMISSIONS,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Static Textbox: Time & Date
          hwndTimeDateStatic = CreateWindow (TEXT("static"),NULL,
							WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
							cxChar * 52, cyChar * 18,
							cxChar*25,cyChar*1,
							hwnd, (HMENU) ID_STATIC_TIMEDATE,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Static Textbox: Type (DIR) or Size (of file)
          hwndTypeSizeStatic = CreateWindow (TEXT("static"),NULL,
							WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
							cxChar * 52, cyChar * 20,
							cxChar*25,cyChar*1,
							hwnd, (HMENU) ID_STATIC_TYPESIZE,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //Bugs Button
          hwndBugsButton = CreateWindow (TEXT("button"),"Bugs",
							WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
							cxChar * 50, cyChar * 28,
							cxChar*10,cyChar*2,
							hwnd, (HMENU) ID_BUTTON_BUGS,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Totals Static Textbox
          hwndTotalsStatic = CreateWindow (TEXT("static"),"Misc. Here",
							WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
							cxChar * 63, cyChar * 27,
							cxChar*17,cyChar*1,
							hwnd, (HMENU) ID_STATIC_TOTALS,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Status Static Textbox
          hwndStatusStatic = CreateWindow (TEXT("static"),"Ready",
							WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
							cxChar * 63, cyChar * 29,
							cxChar*17,cyChar*1,
							hwnd, (HMENU) ID_STATIC_STATUS,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

  		  //Drive-ID Static Textbox
          hwndDriveIDStatic = CreateWindow (TEXT("static"),"Logical Drive",
							WS_CHILD | WS_VISIBLE | SS_LEFT,
							cxChar * 38, cyChar * 23,
							cxChar*13,cyChar*1,
							hwnd, (HMENU) ID_STATIC_LABEL_DRIVEID,
                            (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							NULL);

		  //Partition Listing ComboBox
          hwndPartitionComboBox = CreateWindow ("COMBOBOX","Partition #",
							      WS_BORDER | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
							      cxChar * 52, cyChar * 23,
							      cxChar*25,cyChar*4,
							      hwnd, (HMENU) ID_COMBOBOX_PARTITION,
                                  (HINSTANCE) GetWindowLongPtr (hwnd, GWLP_HINSTANCE),
							      NULL);

          //Intialization

		  StartUp(); //System preparation
		  FillListBox(hwndListBox); //Fill directory list
		  FillPartitionList(hwndPartitionComboBox,hwnd); //Fill partition list


		  //Indicate the number of loaded partitions
		  _ultoa((ULONG32)partitioncount,tempStr,10);
		  strcat(tempStr," partitions");
		  SetWindowText(hwndTotalsStatic,tempStr);
          return 0 ;

	 //Focus on the list box
     case WM_SETFOCUS :
          SetFocus (hwndListBox) ;
          return 0 ;

	 //Command processing
     case WM_COMMAND :

		  //Combobox selection
          if (LOWORD (wParam) == ID_COMBOBOX_PARTITION && HIWORD(wParam) == CBN_SELENDOK)
          {
				//Get current combobox selection index
                LONG_PTR cbIndex = SendMessage(hwndPartitionComboBox,CB_GETCURSEL,0,0);
                if (cbIndex != -1)
                {

				//Change partition & drive
				iDrive = cbData[cbIndex][0];
				iPartition = cbData[cbIndex][1];

				//Set to root directory
				char selectedname[512] = ".";
				SendMessage(hwndListBox,LB_GETTEXT,0,(LPARAM)selectedname);
				strcpy(dirname,"/");
				strcpy(activedirname,dirname);
				
				//Update listbox
				FillListBox(hwndListBox);

				//Done: update status & current directory
				_ltoa(GetListBoxInfo(hwndListBox),tempStr,10);
				iSelection = cbIndex;
				if(atol(tempStr) > 1) SaveLastUsedPartitionInRegistry(iPartition,iDrive,iSelection);
				strcat(tempStr," entries listed");
				SetWindowText(hwndTotalsStatic,tempStr);
				SetWindowText(hwndStatusStatic,"Changed LD");
				SetWindowText(hwndPathEdit,"/");
                }
		  }

		  //Listbox selection
          if (LOWORD (wParam) == ID_LIST && HIWORD (wParam) == LBN_SELCHANGE)
          {
			//Get INT32 number of current selection
			LONG_PTR selectionID = SendMessage(hwndListBox, LB_GETCURSEL, 0, 0);

			//Get name of selected file / directory entry
			char selectedname[512];
			SendMessage(hwndListBox,LB_GETTEXT,selectionID,(LPARAM)selectedname);

			//Construct new Unix/Linux path-name for selection purposes
			strcpy(tempStr,activedirname); //Get last-known-good dirname
			if(strcmp(activedirname,"/") != 0) strcat(tempStr,"/"); //Add path addition
			strcat(tempStr,selectedname); //Add selected filename
			SetWindowText(hwndPathEdit,tempStr); //Update Unix/Linux path-name

			//Access proper array element with data pertaining to selection
			SetWindowText(hwndPermissionsStatic,arrayOfDirStructs[selectionID].permissionsStr);
			SetWindowText(hwndTimeDateStatic,arrayOfDirStructs[selectionID].timedateStr);
			SetWindowText(hwndTypeSizeStatic,arrayOfDirStructs[selectionID].typesizeStr);

			//Check to see if its a symlink
			if(arrayOfDirStructs[selectionID].permissionsStr[0] == 'l')
			{
			strcpy(tempStr,"");
			
				//Some symlinks use recursive markers
				INT32 tempInt; tempInt = 0;
				char newString[256];

					//Clone & truncate '..'
				for(tempInt=0;tempInt<256;tempInt++)
				{
					if(arrayOfDirStructs[selectionID].symlinkStr[tempInt] == '.' && arrayOfDirStructs[selectionID].symlinkStr[tempInt+1] == '.')
					{
					tempInt = tempInt + 3; //skip ahead to next group
					}
					else
					{
					tempStr[tempInt] = arrayOfDirStructs[selectionID].symlinkStr[tempInt];
					}
				}
					//Detect usage of truncation
				for(tempInt=0;tempInt<256;tempInt++)
				{
					if(arrayOfDirStructs[selectionID].symlinkStr[tempInt] == '.' && arrayOfDirStructs[selectionID].symlinkStr[tempInt+1] == '.')
					{
					strcpy(newString,activedirname); //Get last-known-good dirname
					strcat(newString,tempStr); //Append path modifications
					if(strcmp(activedirname,"/") != 0) strcat(tempStr,"/"); //Add path addition
					tempInt = 255; //exit loop
					}
				}				 
			
					//Detect mal-processed symlinks & notify
				if(tempStr[0] != '/' || tempStr[1] == 0){
					strcat(tempStr," :SYMLINK ERROR");
					SetWindowText(hwndPathEdit,tempStr); //bad symlink process
				}
			else SetWindowText(hwndPathEdit,tempStr); //update path
			SetWindowText(hwndTypeSizeStatic,"Symlink"); //symlink notification
			break;
			}

			//Indicate the # of entry selected
			_ultoa((ULONG32)selectionID,tempStr,10);
			SetWindowText(hwndTotalsStatic,tempStr);
          }

		  //Listbox double-click
          if (LOWORD (wParam) == ID_LIST && HIWORD (wParam) == LBN_DBLCLK)
          {
			//Get INT32 number of current selection
			LONG_PTR selectionID = SendMessage(hwndListBox, LB_GETCURSEL, 0, 0);

			//Get name of selected file / directory entry
			char selectedname[512];
			SendMessage(hwndListBox,LB_GETTEXT,selectionID,(LPARAM)selectedname);

			//Check for "." dir & abort change if so (. = current directory)
			if(strcmp(selectedname,".") == 0) break;

		  	//Check for ".." dir
			if(strcmp(selectedname,"..") == 0)
			{
				//Root is root!
				if(strcmp(activedirname,"/") == 0) break;

			char newpathname[512];
			previousdir(activedirname,newpathname);
			strcpy(dirname,newpathname);
			SetWindowText(hwndPathEdit,dirname); //Update Unix/Linux path-name
			}
			else
			{
				//Check to see if it's a DIR & break if not
			if(strcmp(arrayOfDirStructs[selectionID].typesizeStr,"DIR") != 0) break;

				//Construct new Unix/Linux path-name for selection purposes
			strcpy(tempStr,activedirname); //Get last-known-good dirname
			if(strcmp(activedirname,"/")) strcat(tempStr,"/"); //Add path addition
			strcat(tempStr,selectedname); //Add selected filename
			SetWindowText(hwndPathEdit,tempStr); //Update Unix/Linux path-name
			}

			//Change directory
			GetWindowText(hwndPathEdit,dirname,512);
			GetWindowText(hwndCopyEdit,localfile,512);

			//Update listbox
			FillListBox(hwndListBox);

			//Update active dirname
			strcpy(activedirname,dirname);

			//Done: update status
			_ltoa(GetListBoxInfo(hwndListBox),tempStr,10);
			strcat(tempStr," entries listed");
			SetWindowText(hwndTotalsStatic,tempStr);
			SetWindowText(hwndStatusStatic,"Changed Directory");
		  }

		  //Change Directory Button
          if (LOWORD (wParam) == ID_BUTTON_DIRECTORY && HIWORD(wParam) == BN_CLICKED)
          {
			//Change directory
			GetWindowText(hwndPathEdit,dirname,512);
			GetWindowText(hwndCopyEdit,localfile,512);

			//Get INT32 number of current selection
			LONG_PTR selectionID = SendMessage(hwndListBox, LB_GETCURSEL, 0, 0);

			//Get name of selected file / directory entry
			char selectedname[512];
			SendMessage(hwndListBox,LB_GETTEXT,selectionID,(LPARAM)selectedname);

			//Check for "." dir & abort change if so (. = current directory)
			if(strcmp(selectedname,".") == 0) break;

			//Check for ".." dir
			if(strcmp(selectedname,"..") == 0)
			{
				//Root is root!
				if(strcmp(activedirname,"/") == 0) break;

			char newpathname[512];
			previousdir(activedirname,newpathname);
			strcpy(dirname,newpathname);
			SetWindowText(hwndPathEdit,dirname); //Update Unix/Linux path-name
			}

			//Update listbox
			FillListBox(hwndListBox);

			//Update active dirname
			strcpy(activedirname,dirname);

			//Done: update status
			_ltoa(GetListBoxInfo(hwndListBox),tempStr,10);
			if(atol(tempStr) > 1) SaveLastUsedPartitionInRegistry(iPartition,iDrive,iSelection);
			strcat(tempStr," entries listed");
			SetWindowText(hwndTotalsStatic,tempStr);
			SetWindowText(hwndStatusStatic,"Changed Directory");
		  }

		  //Copy Button
          if (LOWORD (wParam) == ID_BUTTON_COPY && HIWORD(wParam) == BN_CLICKED)
          {

			//Instantate a new partition action
			ReiserFsPartition partition;
			  
			//Get filename to copy
			GetWindowText(hwndPathEdit,dirname,512);
			GetWindowText(hwndCopyEdit,localfile,512);

			//Safety default
			safetydefault();

			if(partition.Open(iDrive,iPartition))
			{
				//Flag copying
				SetWindowText(hwndStatusStatic,"Copying");
				//Copy file/directory recursively
				if(partition.GetFile(dirname,localfile,boolRecurseSubdirectories) )
				{
				_chmod(localfile,_S_IREAD | _S_IWRITE | _S_IEXEC);	//set file to be editable
				MessageBox(hwnd,"File copy completed!","rfsgui",MB_OK | MB_ICONEXCLAMATION);
				SetWindowText(hwndStatusStatic,"Copied Files");
				}
				//Try blind name copy of source to target directory; append path with "\" if necessary
				else
				{
				if(TestPath(tempStr)==true) strcat(localfile,"\\");
				strcpy(tempStr,localfile);
				strcat(tempStr,strrchr(dirname,'/'));
				strcpy(localfile,tempStr);
					// Attempt copy
					if(partition.GetFile(dirname,localfile,boolRecurseSubdirectories) )
					{
					_chmod(localfile,_S_IREAD | _S_IWRITE | _S_IEXEC);	//set file to be editable
					MessageBox(hwnd,"File copy completed!","rfsgui",MB_OK | MB_ICONEXCLAMATION);
					SetWindowText(hwndStatusStatic,"Copied Files");
					}
					// Failed to copy
					else
					{
					MessageBox(hwnd,"File copy failed!\n"
					"A file copy needs a filename to copy to.\n"
					"A directory copy needs a directory to copy into.\n"
					,"rfsgui",MB_OK | MB_ICONERROR);
					SetWindowText(hwndStatusStatic,"Failed Copy");
					}	
				}

			}
		  }

		  //Recursive Copy Button
          if (LOWORD (wParam) == ID_BUTTON_RECURSIVE && HIWORD(wParam) == BN_CLICKED)
          {
			  if(boolRecurseSubdirectories == true)
			  {
				boolRecurseSubdirectories = false;
				SetWindowText(hwndStatusStatic,"Recursive: Off");
				SetWindowText(hwndRecursiveButton,"Recursive Directory Copy is Off");
			  }
			  else
			  {
				boolRecurseSubdirectories = true;
				SetWindowText(hwndStatusStatic,"Recursive: On");
				SetWindowText(hwndRecursiveButton,"Recursive Directory Copy is On");
			  }
		  }

		  //Tree Dump Button
          if (LOWORD (wParam) == ID_BUTTON_DUMPTREE && HIWORD(wParam) == BN_CLICKED)
          {
			//Perform dump
			GetWindowText(hwndCopyEdit,tempStr,512);
			if(TestPath(tempStr)==true) strcat(tempStr,"\\dumptree.txt");
			
			//Dump new file & allow it to be editable
			DumpTreeToFile(tempStr,hwnd);
			_chmod(tempStr,_S_IREAD | _S_IWRITE | _S_IEXEC);
		  }

		  //Partition Information Dump Button
          if (LOWORD (wParam) == ID_BUTTON_DUMPINFO && HIWORD(wParam) == BN_CLICKED)
          {
			//Perform dump
			GetWindowText(hwndCopyEdit,tempStr,512);
			if(TestPath(tempStr)==true) strcat(tempStr,"\\dumpinfo.txt");
			
			//Dump new file & allow it to be editable
			DumpInfoToFile(tempStr,hwnd);
			_chmod(tempStr,_S_IREAD | _S_IWRITE | _S_IEXEC);
		  }

		  //About Button
          if (LOWORD (wParam) == ID_BUTTON_ABOUT && HIWORD(wParam) == BN_CLICKED)
          {
			MessageBox(hwnd,
			"RFSTOOL - ReiserFS Read-support for Windows & Linux - Version 0.14\n"
            "Copyright (C) [2001..9999] by Gerson Kurz\n"
            "This is free software, licensed by the GPL.\n"
			"http://p-nand-q.com/e/reiserfs.html\n"
			"\n"
			"RFSGUI - Win32 & Win-x64 GUI for RFSTOOL functions - Version 2.2 \n"
			"Michael Adams, 11-09-2005\n"
			"cwolfsheep@yahoo.com\n"
			"http://www.wolfsheep.com\n"
			,"About RFSGUI",MB_OK);
		  }

		  //Bug Documentation Button
          if (LOWORD (wParam) == ID_BUTTON_BUGS && HIWORD(wParam) == BN_CLICKED)
          {
			  MessageBox(hwnd,
				  "Known bugs from 1.4:\n"
				  "\n"
				  "*. If you don't specify a new dirname to copy to, \n"
				  "   the directory contents will copy to the target\n"
				  "   without first going into a new dir of the same \n"
				  "   name. If you copy to a new name, it all copies \n"
				  "   into that new dir. \n"
				  "\n"
				  "Known bugs from 1.3:\n"
				  "\n"
				  "*. Kurz added symlink support to his program.\n"
				  "   I've tried to implement it best as possible.\n"
				  "   The trade-off is that some symlinks work, and \n"
				  "   some get mangled. You also cannot double-click \n"
				  "   on a symlink.\n"
				  ,"Known Bugs in RFSGUI",MB_OK);
		  }

		//End of WM_COMMAND
		return 0 ;

     //Maximized window
     case WM_SIZE:

		//Load rectangle parameters to measure against
		GetWindowRect(hwnd,&rcMain);
		GetWindowRect(hwndListBox,&rcListbox);

		//Adjust listbox
		MoveWindow( hwndListBox, cxChar, cyChar,
			cxChar * 34 + GetSystemMetrics (SM_CXVSCROLL),
			(rcMain.bottom - rcMain.top) - (cyChar * 3),
			TRUE );
		return 0;
		

     //Killing application
     case WM_DESTROY :
		//Release dynamic memory
		delete [] arrayOfDirStructs;
		//Quit
        PostQuitMessage (0) ;
		return 0 ;

     //Control monitoring: use to disable resizing
	 case WM_NCHITTEST:
		 //switch modified from ExpertsExchange posting
         switch ( DefWindowProc ( hwnd, WM_NCHITTEST, 0, lParam ) ) {
		  //cases garnered from winuser.h
		  case HTLEFT:
		  case HTRIGHT:
		  //case HTTOP:
		  case HTTOPLEFT:
		  case HTTOPRIGHT:
		  //case HTBOTTOM:
		  case HTBOTTOMLEFT:
		  case HTBOTTOMRIGHT:
		  return 0;
         }
         break;

 }return DefWindowProc (hwnd, message, wParam, lParam) ;
}
