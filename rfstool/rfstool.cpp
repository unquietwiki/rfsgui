#include "precomp.h"
#include "reiserfs.h"
#include "time.h"

char* g_szUseSpecificDevice = NULL;

#define		S_IRUSR	0000400	/* read permission, owner */
#define		S_IWUSR	0000200	/* write permission, owner */
#define		S_IXUSR 0000100/* execute/search permission, owner */
#define		S_IRGRP	0000040	/* read permission, group */
#define		S_IWGRP	0000020	/* write permission, grougroup */
#define		S_IXGRP 0000010/* execute/search permission, group */
#define		S_IROTH	0000004	/* read permission, other */
#define		S_IWOTH	0000002	/* write permission, other */
#define		S_IXOTH 0000001/* execute/search permission, other */

void LinuxPermissionsAsString( LPSTR lpszBuffer, int mode )
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

#define ACTION_LISTDIR 1
#define ACTION_GETFILE 2
#define ACTION_SHOWINFO 3
#define ACTION_AUTODETECT 4
#define ACTION_DUMPTREE 5
#define ACTION_BACKUP 6
#define ACTION_RESTORE 7
#define ACTION_SHOW 7



void TRACE(const char* fmt, ...)
{
/*#ifdef _DEBUG
    va_list args;
    va_start( args, fmt );
    vprintf(fmt, args);
#endif*/
}

char* g_pszReadOnlyBackupFileName = NULL;

int comparefunc(const void *elem1, const void *elem2 )
{
    return strcmp( (*((ReiserFsFileInfo**)elem1))->m_strName, (*((ReiserFsFileInfo**)elem2))->m_strName );
}

#ifdef _WIN32

void AutodetectPartitionsFromRegistry( int* piPartition, int* piDrive )
{
	if( *piDrive == -1 )
	{
		HKEY hkSubkey;
		DWORD dwDisposition, dwType, dwSize;

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
					*piDrive = (int) dwDisposition;
				}
			}
			dwSize = sizeof(DWORD);
			if( RegQueryValueEx(hkSubkey, "Partition", 0, &dwType, (BYTE*) &dwDisposition, &dwSize ) == ERROR_SUCCESS )
			{
				if( dwType == REG_DWORD )
				{
					*piPartition = (int) dwDisposition;
				}
			}
			RegCloseKey( hkSubkey );
		}
		if( *piDrive == -1 || *piPartition == -1 )
		{
			ReiserFsPartition partition;
			partition.AutodetectFirstUsable( piPartition, piDrive );
		}
	}
}

void SaveLastUsedPartitionInRegistry( int iPartition, int iDrive )
{
	HKEY hkSubkey;
	DWORD dwDisposition;

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
		RegCloseKey( hkSubkey );
	}
}

#endif

class dumpfile : public ICreateFileInfo
    {
        virtual BOOL SetFileSize( INT64 Size )
        {
            return TRUE;
        }
        virtual void Write( LPBYTE lpbData, DWORD dwSize )
        {
            fwrite(lpbData,dwSize,1,stdout);
        }
    };

void showVersion()
{
    printf( "RFSTOOL - ReiserFS Read-support for Windows & Linux - Version 0.14\n"
            "Copyright (C) [2001..9999] by Gerson Kurz\n"
            "This is free software, licensed by the GPL.\n\n" );
}

int help()
{
    showVersion();
    printf("USAGE: rfstool [action] [options] directory-or-filename [target-filename]\n"
           "ACTIONS: ls | cp | show | info | autodetect | backup | dumptree.\n"
           "OPTIONS: \n"
		   "  -pdrive.partition .... select partition\n"
		   "  -x<backupfile> ....... restore from backup file\n"
		   "  -r ................... recurse subdirectories (cp only)\n"
           "  -v ................... show version and exit\n"
           "For more info, please go to http://p-nand-q.com/e/reiserfs.html");

    return 0;
}
int main( int argc, char* argv[] )
{
    int action = ACTION_LISTDIR;
	char* pszRestoreFile = NULL;
    int iPartition = -1, iDrive = -1;
	bool bRecurseSubdirectories = false;

    // I hate Cs cumbersome string ops, I really do. 
    char* p = getenv("REISERFS_PARTITION");
    if( p )
    {
        char* q = strchr(p,'.');
        if( q )
        {
            *(q++) = 0;
            iDrive = atoi(p);
            iPartition = atoi(q);
            printf("REISERFS_PARTITION selects drive %d, partition %d\n", iDrive, iPartition );
        }
        else printf("Warning, environment variable REISERFS_PARTITION exists, but is of invalid format\n" );
    }

    char* dirname = NULL;
    char* localfile = NULL;

    for( int i = 1; i < argc; i++ )
    {
        char* arg = argv[i];
        if( !stricmp(arg,"ls") )
        {
            action = ACTION_LISTDIR;
        }
        else if( !stricmp(arg,"cp") )
        {
            action = ACTION_GETFILE;
        }
		else if( !stricmp(arg,"backup") )
		{
			action = ACTION_BACKUP;
		}
        else if( !stricmp(arg,"info") )
        {
            action = ACTION_SHOWINFO;
        }
        else if( !stricmp(arg,"autodetect") || !stricmp(arg,"detect") )
        {
            action = ACTION_AUTODETECT;
        }
        else if( !stricmp(arg,"dumptree") )
        {
            action = ACTION_DUMPTREE;
        }
        else if( !stricmp(arg,"show") )
        {
            action = ACTION_SHOW;
        }
        else if( *arg == '-'  )
        {
            switch(toupper(arg[1]))
            {
			case 'X':
			{
				pszRestoreFile = arg+2;
				printf("Attempting to restore data from '%s'\n", pszRestoreFile );
				break;
			}
			case 'R':
			{
				bRecurseSubdirectories = true;
				break;
			}
            case 'V':
            {
                showVersion();
                return 0;
            }
            case 'P':
                {

					if( strncmp( arg+2, "backup:", 7 ) == 0 )
                    {
						iPartition = 0;
						iDrive = USE_BACKUP_FILENAME;
						g_szUseSpecificDevice = arg+2+7;
						break;
                    }
#ifndef _WIN32
					if( strncmp( arg+2, "/dev/", 5 ) == 0 )
					{
						
						iPartition = 0;
						iDrive = USE_SPECIFIC_DEVICE;
						g_szUseSpecificDevice = arg+2;
						break;
					}
#endif
                    char* q = strchr(arg+2,'.');
                    if( q )
                    {
                        *(q++) = 0;
                        iDrive = atoi(arg+2);
                        iPartition = atoi(q);
                    }
                    else 
                    {
                        printf("ERROR, must specify partition in the form /p<drive>.<partition>\n" );
                        return 0;
                    }
                } break;
             
            default:
                return help();
            }
        }
        else if( !dirname )
        {
            dirname = arg;
        }
        else
        {
            localfile = arg;
        }
    }

    if( action == ACTION_AUTODETECT )
    {
        ReiserFsPartition partition;
        partition.Autodetect();
        return 0;
    }

	if( action == ACTION_SHOWINFO )
	{
		IPhysicalDrive* drive = CreatePhysicalDriveInstance();
		CHAR szDriveName[256];

		if( iDrive == -1 || iPartition == -1 )
		{
			for( int i = 0; i < 8; i++ )
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
					printf("ERROR %s, unable to open drive %s\n", (LPCSTR) GetLastErrorString(), (LPCSTR)  szDriveName );
				}
			}
		}
		delete drive;
		return 0;
	}

#ifdef _WIN32
    if( iDrive == -1 || iPartition == -1 )
		AutodetectPartitionsFromRegistry( &iPartition, &iDrive );
#endif

    if( iDrive == -1 || iPartition == -1 )
    {
        printf("ERROR, must specify drive & partition.\n");
        return 10;
    }

#ifdef _WIN32
	SaveLastUsedPartitionInRegistry( iPartition, iDrive );
#endif
    ReiserFsPartition partition;

	if( pszRestoreFile )
	{
		if( action == ACTION_BACKUP )
		{
			printf("ERROR, cannot backup from restore!!!\n" );
			return 0;
		}
		if( !partition.PrepareForRestore(pszRestoreFile) )
		{
			return 0;
		}
	}

    if( (iDrive == USE_BACKUP_FILENAME) && (action != ACTION_LISTDIR) )
    {
        printf("ERROR, reading from backup is only supported with 'ls' option\n" );
        return 0;
    }

    if( partition.Open(iDrive,iPartition) )
    {
		if( action == ACTION_BACKUP )
		{
			partition.Backup(dirname);
		}
        else if( action == ACTION_LISTDIR )
        {
            if( !dirname )
                dirname = "/";

            PList directory;

            if( partition.ListDir(&directory,dirname) )
            {
                // create array of pointers 
                ReiserFsFileInfo** array = new ReiserFsFileInfo*[directory.m_lCount];
                if( array )
                {
                    int index = 0;
                    ENUMERATE( &directory, ReiserFsFileInfo, pFile )
                        array[index++] = pFile;

                    size_t sc = (size_t) (directory.m_lCount - 2);
                    if( sc > 1 )
                        qsort( &(array[2]), sc, sizeof(ReiserFsFileInfo*), comparefunc );

                    for( index = 0; index < directory.m_lCount; index++ )
                    {
                        ReiserFsFileInfo* pFile = array[index];

                        char szAttributes[256];
                        LinuxPermissionsAsString( szAttributes, pFile->m_stat.sd_mode );

                        PString strDate(asctime(localtime((const time_t*) &pFile->m_stat.sd_mtime)));
                        LPSTR p = strDate;
                        p[strlen(p)-1] = 0;
                        if( S_ISDIR(pFile->m_stat.sd_mode) )
                        {
                            printf("%s %12s %s %s \n", szAttributes, 
                            "<DIR>",
                            (LPCSTR) strDate,
                            (LPCSTR) pFile->m_strName );
                        }
                        else if( S_ISLNK(pFile->m_stat.sd_mode) )
                        {
                            printf("%s %12" FMT_QWORD " %s %s -> %s\n", szAttributes, 
                            pFile->m_stat.sd_size,
                            (LPCSTR) strDate,
                            (LPCSTR) pFile->m_strName,
                            (LPCSTR) partition.GetFileAsString(pFile) );
                        }
                        else
                        {
                            printf("%s %12" FMT_QWORD " %s %s\n", szAttributes, 
                            pFile->m_stat.sd_size,
                            (LPCSTR) strDate,
                            (LPCSTR) pFile->m_strName );
                        }
                        //printf("%d\n", pFile->m_stat.u.sd_generation );
                    }
                }
            }
        }
        else if( action == ACTION_SHOW )
        {
            if( !dirname )
            {
                printf("ERROR, must specify dirname\n" );
            }
            else
            {
                dumpfile info;
                partition.GetFileEx(dirname,&info);
            }
        }
        else if( action == ACTION_GETFILE )
        {
            if( !dirname || !localfile )
            {
                printf("ERROR, must specify both dirname and localfile\n" );
            }
            else
            {
                printf("about to copy %s -> %s...", dirname, localfile );
                if( partition.GetFile(dirname,localfile,bRecurseSubdirectories) )
                {
                    printf("done.\n");
                }
                else
                {
                    printf("failed\n");
                }
            }
        }
		else if( action == ACTION_DUMPTREE )
		{
			partition.DumpTree();
		}
    }
    return 0;
}

