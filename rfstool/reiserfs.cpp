#include "precomp.h"
#include "reiserfs.h"
#include "pdrivefile.h"

#define MAXLOOPTRY 10245

#ifndef ZeroMemory
#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
#endif

#define SIZE_OF_LAYOUT_BLOCK 20240


//MPA 11-9-2005: redefined MB converter parameter as function
double PBytesInMBytes(U64 bytes)
{
	return (double)(bytes / 1048576);
}


class LayoutBlock
    {
    public:

        LayoutBlock()
        {
            m_lpbData = new BYTE[SIZE_OF_LAYOUT_BLOCK];
        }
        bool isValid()
        {
            return (m_lpbData != NULL);
        }

        INT32 getSize()
        {
            return SIZE_OF_LAYOUT_BLOCK;
        }

        LPBYTE getData()
        {
            return m_lpbData;
        }

        virtual ~LayoutBlock()
        {
            delete [] m_lpbData;
        }
    protected:
        INT32 m_nSize;
        LPBYTE m_lpbData;
    };

DWORD dwTypeDirect = (DWORD) -1;
DWORD dwTypeIndirect = (DWORD) -2;

void FileTimeFromUnixTime(FILETIME* pf, U32 unixtime )
{
    struct tm* t = localtime((const time_t*) &unixtime );
    ZeroMemory( pf, sizeof(FILETIME) );
    if( t )
    {
        SYSTEMTIME st;
        st.wYear = (WORD) t->tm_year + 1900;
        st.wMonth = (WORD) t->tm_mon + 1;
        st.wDayOfWeek = (WORD) t->tm_wday;
        st.wDay = (WORD) t->tm_mday;
        st.wHour = (WORD) t->tm_hour;
        st.wMinute = (WORD) t->tm_min;
        st.wSecond = (WORD) t->tm_sec;
        st.wMilliseconds = (WORD) 0;
        SystemTimeToFileTime(&st, pf);
    }
}

//MPA: 5-2-2005
//64bit
void SetUnixFileTime( LPSTR lpszPath, ReiserFsFileInfo* pFile )
{
    FILETIME f_atime, f_mtime, f_ctime;

    FileTimeFromUnixTime(&f_atime, pFile->m_stat.sd_atime );
    FileTimeFromUnixTime(&f_mtime, pFile->m_stat.sd_mtime );
    FileTimeFromUnixTime(&f_ctime, pFile->m_stat.sd_ctime );

    HANDLE hFile = CreateFile( lpszPath, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
    if( hFile != INVALID_HANDLE_VALUE )
    {
        SetFileTime(hFile, &f_ctime, &f_atime, &f_mtime );
        CloseHandle(hFile);
    } 
}

ReiserFsFileInfo::ReiserFsFileInfo(REISERFS_DIRECTORY_HEAD* pDH, LPCSTR lpszName)
{
	DEBUGTRACE(("FOUND FILE %s\n", lpszName))

    m_deh = *pDH;
    m_strName = lpszName;
    memset(&m_stat,0,sizeof(m_stat));

    // rfs does not store file info for special dirs "." and "..", so simulate this here...
    if( (stricmp(lpszName, ".") == 0) ||
        (stricmp(lpszName, "..") == 0) )
    {
        m_stat.sd_mode |= S_IFDIR;
    }

}

INT32 comp_keys(REISERFS_CPU_KEY* a, REISERFS_CPU_KEY *b)
{
    if( a->k_dir_id < b->k_dir_id )
        return -1;
    if( a->k_dir_id > b->k_dir_id )
        return 1;
    if( a->k_objectid < b->k_objectid )
        return -1;
    if( a->k_objectid > b->k_objectid )
        return 1;
    if( a->k_offset < b->k_offset )
        return -1;
    if( a->k_offset > b->k_offset )
        return 1;
    if( a->k_type < b->k_type )
        return -1;
    if( a->k_type > b->k_type )
        return 1;
	return 0;
}

INT32 comp_keys_no_offset (REISERFS_CPU_KEY * le_key, REISERFS_CPU_KEY * cpu_key)
{
  if( le_key->k_dir_id < cpu_key->k_dir_id )
	return -1;
  if( le_key->k_dir_id > cpu_key->k_dir_id )
	return 1;
  if( le_key->k_objectid < cpu_key->k_objectid )
	return -1;
  if( le_key->k_objectid > cpu_key->k_objectid )
	return 1;
  if (le_key->k_type < cpu_key->k_type)
    return -1;
  if (le_key->k_type > cpu_key->k_type)
      return 1;
	return 0;
}

ReiserFsPartition::ReiserFsPartition()
{
    m_dwBlockSize = 4096;
	m_pDrive = CreatePhysicalDriveInstance();
} // ReiserFsPartition()

ReiserFsPartition::~ReiserFsPartition()
{
	delete m_pDrive;
} // ~ReiserFsPartition()

BOOL ReiserFsPartition::Read( LPBYTE lpbMemory, DWORD dwSize, INT64 BlockNumber )
{
	INT64 offset = m_sb.s_blocksize;
	offset *= BlockNumber;
	offset += m_PartitionStartingOffset;
	return m_pDrive->ReadAbsolute( lpbMemory, dwSize, offset );
} // Read()

bool ReiserFsPartition::CheckReiserFsPartition()
{
    INT64 diskoffset = (m_PartitionStartingOffset + REISERFS_DISK_OFFSET_IN_BYTES);

    BYTE* lpbMemory = new BYTE[ (DWORD)( m_BytesPerSector*2 ) ];
    if( !m_pDrive->ReadAbsolute(lpbMemory, (DWORD) m_BytesPerSector, diskoffset ) )
    {
        printf("ERROR %s, unable to read ReiserFS superblock\n", (LPCSTR) GetLastErrorString() ); 
        return false;
    }

    LPREISERFS_SUPER_BLOCK p = (LPREISERFS_SUPER_BLOCK) lpbMemory;
    if( stricmp(p->s_magic,REISERFS_SUPER_MAGIC_STRING) )
    {
        if( stricmp(p->s_magic,REISER2FS_SUPER_MAGIC_STRING) )
        {
            if( stricmp(p->s_magic,REISER3FS_SUPER_MAGIC_STRING) )
            {
                return false;
            }
        }
    }
    return true;
}

void ReiserFsPartition::AutodetectFirstUsable( LONG_PTR piPartition, LONG_PTR piDrive )
{
    DISK_GEOMETRY dg;
    LayoutBlock layout;

	printf("No drives specified, performing an autodetect...\n" );

    for( INT32 iDrive = 0; iDrive < 10; iDrive++ )
    {
        if( !m_pDrive->Open(iDrive) ) 
            continue;

        printf("Testing drive %d\n", iDrive );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        if( m_pDrive->GetDriveGeometryEx( (DISK_GEOMETRY_EX*) layout.getData(), layout.getSize() ) )
        {
            DISK_GEOMETRY_EX* pDG = (DISK_GEOMETRY_EX*) layout.getData();
            m_BytesPerSector = pDG->Geometry.BytesPerSector;
        }
        else 
#endif
        if( m_pDrive->GetDriveGeometry(&dg) )
        {
            m_BytesPerSector = dg.BytesPerSector;
        }
        else
        {
            printf("ERROR %s, unable to get drive geometry from this drive\n", (LPCSTR) GetLastErrorString() ); 
            continue;
        }
    
        memset( layout.getData(), 0, layout.getSize() );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        if( m_pDrive->GetDriveLayoutEx(layout.getData(), layout.getSize()) )
        {
            PDRIVE_LAYOUT_INFORMATION_EX pLI = (PDRIVE_LAYOUT_INFORMATION_EX)layout.getData();

            for( DWORD iPartition = 0; iPartition < pLI->PartitionCount; iPartition++ )
            {
                m_PartitionStartingOffset = pLI->PartitionEntry[iPartition].StartingOffset.QuadPart;
                if( CheckReiserFsPartition() )
                {
                    printf( "Drive %d, Partition %d is a ReiserFS\n",iDrive, iPartition );
                    printf( "Partition Size: %" FMT_QWORD " Bytes (%.2f MB)\n", 
                        pLI->PartitionEntry[iPartition].PartitionLength.QuadPart, 
                        PBytesInMBytes(pLI->PartitionEntry[iPartition].PartitionLength.QuadPart) );
                }
            }
        }
        else 
#endif
        if( m_pDrive->GetDriveLayout(layout.getData(), layout.getSize()) )
        {
            PDRIVE_LAYOUT_INFORMATION pLI = (PDRIVE_LAYOUT_INFORMATION)layout.getData();
            for( DWORD iPartition = 0; iPartition < pLI->PartitionCount; iPartition++ )
            {
                m_PartitionStartingOffset = pLI->PartitionEntry[iPartition].StartingOffset.QuadPart;
                if( CheckReiserFsPartition() )
                {
					piPartition = iPartition;
					piDrive = iDrive;
                    printf( "Drive %d, Partition %d is a ReiserFS -> using that\n",iDrive, iPartition );
                    printf( "Partition Size: %" FMT_QWORD " Bytes (%.2f MB)\n", 
                        pLI->PartitionEntry[iPartition].PartitionLength.QuadPart, 
                        PBytesInMBytes(pLI->PartitionEntry[iPartition].PartitionLength.QuadPart) );
					break;
                }
            }
        }
        else
        {
            printf("ERROR %s, unable to get drive layout from this drive\n", (LPCSTR) GetLastErrorString() ); 
            continue;
        }
    }
}

void ReiserFsPartition::Autodetect( LONG_PTR iMaxDrive, LPFNFoundPartition lpCallback, LPVOID lpContext )
{
    DISK_GEOMETRY dg;
    LayoutBlock layout;

    for( LONG_PTR iDrive = 0; iDrive < 10; iDrive++ )
    {
        if( !m_pDrive->Open(iDrive) ) 
            continue;

        //printf("Testing drive %d\n", iDrive );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        if( m_pDrive->GetDriveGeometryEx( (DISK_GEOMETRY_EX*) layout.getData(), layout.getSize() ) )
        {
            DISK_GEOMETRY_EX* pDG = (DISK_GEOMETRY_EX*) layout.getData();
            m_BytesPerSector = pDG->Geometry.BytesPerSector;
        }
        else 
#endif
        if( m_pDrive->GetDriveGeometry(&dg) )
        {
            m_BytesPerSector = dg.BytesPerSector;
        }
        else
        {
            printf("ERROR %s, unable to get drive geometry from this drive\n", (LPCSTR) GetLastErrorString() ); 
            continue;
        }
    
        memset( layout.getData(), 0, layout.getSize() );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        if( m_pDrive->GetDriveLayoutEx(layout.getData(), layout.getSize()) )
        {
            PDRIVE_LAYOUT_INFORMATION_EX pLI = (PDRIVE_LAYOUT_INFORMATION_EX)layout.getData();

            for( DWORD iPartition = 0; iPartition < pLI->PartitionCount; iPartition++ )
            {
                m_PartitionStartingOffset = pLI->PartitionEntry[iPartition].StartingOffset.QuadPart;
                if( CheckReiserFsPartition() )
                {
                    if( lpCallback )
                    {
                        static char szDriveLetters[] = "abcdefghijklmnopqrstuvwxyz";

                        lpCallback( PString( 0, "/dev/hd%c%d", szDriveLetters[iDrive], iPartition ), lpContext ); 
                    }
                    else
                    {
                        printf( "Drive %d, Partition %d is a ReiserFS\n",iDrive, iPartition );
                        printf( "Partition Size: %" FMT_QWORD " Bytes (%.2f MB)\n", 
                            pLI->PartitionEntry[iPartition].PartitionLength.QuadPart, 
                            PBytesInMBytes(pLI->PartitionEntry[iPartition].PartitionLength.QuadPart) );
                    }
                }
            }
        }
        else 
#endif
        if( m_pDrive->GetDriveLayout(layout.getData(), layout.getSize()) )
        {
            PDRIVE_LAYOUT_INFORMATION pLI = (PDRIVE_LAYOUT_INFORMATION)layout.getData();
            for( DWORD iPartition = 0; iPartition < pLI->PartitionCount; iPartition++ )
            {
                m_PartitionStartingOffset = pLI->PartitionEntry[iPartition].StartingOffset.QuadPart;
                if( CheckReiserFsPartition() )
                {
                    if( lpCallback )
                    {
                        static char szDriveLetters[] = "abcdefghijklmnopqrstuvwxyz";

                        lpCallback( PString( 0, "/dev/hd%c%d", szDriveLetters[iDrive], iPartition ), lpContext ); 
                    }
                    else
                    {
                        printf( "Drive %d, Partition %d is a ReiserFS\n",iDrive, iPartition );
                        printf( "Partition Size: %" FMT_QWORD " Bytes (%.2f MB)\n", 
                            pLI->PartitionEntry[iPartition].PartitionLength.QuadPart, 
                            PBytesInMBytes(pLI->PartitionEntry[iPartition].PartitionLength.QuadPart) );
                    }
                }
            }
        }
        else
        {
            printf("ERROR %s, unable to get drive layout from this drive\n", (LPCSTR) GetLastErrorString() ); 
            continue;
        }
    }
}

bool ReiserFsPartition::Open( LONG_PTR iDrive, LONG_PTR iPartition )
{
    if( iDrive == USE_BACKUP_FILENAME )
    {
        delete m_pDrive;
        m_pDrive = new PSimulatedDriveFromBackupFile(g_szUseSpecificDevice);
    }
    if( !m_pDrive->Open(iDrive) ) 
    {
        printf("ERROR %s, unable to open drive %d\n", (LPCSTR) GetLastErrorString(), iDrive ); 
        return false;
    }

    DISK_GEOMETRY dg;
    LayoutBlock layout;
    
#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
    if( m_pDrive->GetDriveGeometryEx( (DISK_GEOMETRY_EX*) layout.getData(), layout.getSize()) )
    {
        DISK_GEOMETRY_EX* pDG = (DISK_GEOMETRY_EX*) layout.getData();
        m_BytesPerSector = pDG->Geometry.BytesPerSector;
    }
    else 
#endif
    if( m_pDrive->GetDriveGeometry(&dg) )
    {
        m_BytesPerSector = dg.BytesPerSector;
    }
    else
    {
        printf("ERROR %s, unable to get drive geometry from this drive\n", (LPCSTR) GetLastErrorString()  ); 
        return false;
    }
    
    memset( layout.getData(), 0, layout.getSize() );

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
    if( m_pDrive->GetDriveLayoutEx(layout.getData(), layout.getSize()) )
    {
        printf("Got newstyle partition information, assuming Windows XP\n" );

        PDRIVE_LAYOUT_INFORMATION_EX pLI = (PDRIVE_LAYOUT_INFORMATION_EX)layout.getData();
        if( iPartition < 0 || iPartition >= (INT32) pLI->PartitionCount )
        {
            printf("ERROR, drive only has partitions [0..%d]\n", pLI->PartitionCount ); 
            return false;
        }
        m_PartitionStartingOffset = pLI->PartitionEntry[iPartition].StartingOffset.QuadPart;
    }
    else 
#endif
    if( m_pDrive->GetDriveLayout(layout.getData(), layout.getSize()) )
    {
        PDRIVE_LAYOUT_INFORMATION pLI = (PDRIVE_LAYOUT_INFORMATION)layout.getData();
        if( iPartition < 0 || iPartition >= (INT32) pLI->PartitionCount )
        {
            printf("ERROR, drive only has partitions [0..%d]\n", pLI->PartitionCount ); 
            return false;
        }
        m_PartitionStartingOffset = pLI->PartitionEntry[iPartition].StartingOffset.QuadPart;
    }
    else
    {
        printf("ERROR %s, unable to get drive layout from this drive\n", (LPCSTR) GetLastErrorString() ); 
        return false;
    }
    INT64 diskoffset = (m_PartitionStartingOffset + REISERFS_DISK_OFFSET_IN_BYTES);
    
    BYTE* lpbMemory = new BYTE[ (DWORD)( m_BytesPerSector*2 ) ];
    if( !m_pDrive->ReadAbsolute(lpbMemory, (DWORD) m_BytesPerSector, diskoffset ) )
    {
        printf("ERROR %s, unable to read ReiserFS superblock\n", (LPCSTR) GetLastErrorString() ); 
        return false;
    }
    LPREISERFS_SUPER_BLOCK p = (LPREISERFS_SUPER_BLOCK) lpbMemory;

    if( stricmp(p->s_magic,REISERFS_SUPER_MAGIC_STRING) )
    {
        if( stricmp(p->s_magic,REISER2FS_SUPER_MAGIC_STRING) &&
            stricmp(p->s_magic,REISER3FS_SUPER_MAGIC_STRING) )
        {
            printf("ERROR, doesn't seem to be a ReiserFS partition (magic string invalid)\n" ); 
            return false;
        }
		else
		{
			dwTypeDirect = (DWORD) 2;
			dwTypeIndirect = (DWORD) 1;
		}
    }
	else
	{
		dwTypeDirect = (DWORD) -1;
		dwTypeIndirect = (DWORD) -2;
	}
    memcpy(&m_sb,p,sizeof(REISERFS_SUPER_BLOCK));
    return true;
}

void WINAPI ListDirCallback(ReiserFsPartition* partition, 
    REISERFS_CPU_KEY* lpKey, LPBYTE lpbMemory, 
    INT32 nSize, void* lpContext )
{
    PList* pDirectory = (PList*) lpContext;
	INT32 dh_offset = 0;
	INT32 dh_strpos = nSize;
	INT32 i = 0, dh_strlen;
                               
    CHAR szTempBuffer[512];
	while( dh_offset < dh_strpos )
	{
		REISERFS_DIRECTORY_HEAD* pDH = (REISERFS_DIRECTORY_HEAD*) (lpbMemory+dh_offset);

        dh_strlen = dh_strpos-pDH->deh_location;
        memcpy(szTempBuffer, lpbMemory+pDH->deh_location, dh_strlen);
        szTempBuffer[dh_strlen] = 0;
		pDirectory->AddTail( new ReiserFsFileInfo(pDH,szTempBuffer) );
		dh_strpos = pDH->deh_location;
		dh_offset += sizeof(REISERFS_DIRECTORY_HEAD);
	}
}


void WINAPI GetFileStat(ReiserFsPartition* partition, REISERFS_CPU_KEY* lpKey, LPBYTE lpbMemory, INT32 nSize, void* lpContext )
{
    ReiserFsFileInfo* pFile = (ReiserFsFileInfo*) lpContext;

	if( nSize == sizeof(REISERFS_STAT2) )
	{
        pFile->m_stat = *(REISERFS_STAT2*) lpbMemory;
	}
	else if( nSize == sizeof(REISERFS_STAT1) )
	{
		REISERFS_STAT1* v1 = (REISERFS_STAT1*) lpbMemory;
        pFile->m_stat.sd_mode = v1->sd_mode;
        pFile->m_stat.sd_nlink = v1->sd_nlink;
        pFile->m_stat.sd_uid = v1->sd_uid;
        pFile->m_stat.sd_gid = v1->sd_gid;
        pFile->m_stat.sd_size = v1->sd_size;
        pFile->m_stat.sd_atime = v1->sd_atime;
        pFile->m_stat.sd_mtime = v1->sd_mtime;
        pFile->m_stat.sd_ctime = v1->sd_ctime;
	    pFile->m_stat.u.sd_rdev = v1->u.sd_rdev;
	}
    else assert(false);
}

typedef struct
{
    FILE* fp;
    PString* pString;
    ICreateFileInfo* pCFI;
    INT64 FileSize;
} GETFILEINDIRECTCONTEXT;

void WINAPI GetFileIndirect(ReiserFsPartition* partition, REISERFS_CPU_KEY* lpKey, LPBYTE lpbMemory, INT32 nSize, void* lpContext )
{
    GETFILEINDIRECTCONTEXT* pc = (GETFILEINDIRECTCONTEXT*) lpContext;

    DWORD dwBlockSize = partition->m_dwBlockSize;

    LPBYTE bMemory = new BYTE[dwBlockSize];
    assert(bMemory);

    long* pBlocks = (long*) lpbMemory;
    INT32 count = nSize / 4;
    INT64 SizeLeft = pc->FileSize;

    for( INT32 index = 0; index < count; index++ )
    {
        long block2 = pBlocks[index];
	    if(partition->Read(bMemory,dwBlockSize,block2) )
        {   
            if( SizeLeft >= dwBlockSize )
            {
                //printf("found %d bytes in indirect item at block %d\n", dwBlockSize, block2 );
                //putchar('.');
                if( pc->fp )
                {
                    fwrite(bMemory,1,dwBlockSize,pc->fp);
                }
                else if( pc->pCFI )
                {
                    pc->pCFI->Write(bMemory, dwBlockSize);
                }
                else
                {
                    pc->pString->Append((LPSTR)bMemory,dwBlockSize);
                }
                SizeLeft -= dwBlockSize;
            }
            else
            {
                //printf("found %d bytes in indirect item at block %d\n", (INT32)SizeLeft, block2 );
                //putchar('.');
                if( pc->fp )
                {
                    fwrite(bMemory,1,(INT32)SizeLeft,pc->fp);
                }
                else if( pc->pCFI )
                {
                    pc->pCFI->Write(bMemory, (DWORD) SizeLeft);
                }
                else
                {
                    pc->pString->Append((LPSTR)bMemory,(INT32)SizeLeft);
                }
                break;
            }
                
        }
    }
    delete bMemory;
    pc->FileSize=SizeLeft;
}

void WINAPI GetFileDirect(ReiserFsPartition* partition, REISERFS_CPU_KEY* lpKey, LPBYTE lpbMemory, INT32 nSize, void* lpContext )
{
    GETFILEINDIRECTCONTEXT* pc = (GETFILEINDIRECTCONTEXT*) lpContext;
    INT32 toWrite=pc->FileSize>nSize?nSize:(INT32)pc->FileSize;
    pc->FileSize-=toWrite;

    //putchar('.');

    if( pc->fp )
    {
        fwrite(lpbMemory,1,toWrite,pc->fp);
    }
    else if( pc->pCFI )
    {
        pc->pCFI->Write(lpbMemory, (DWORD) toWrite);
    }
    else
    {
        pc->pString->Append((LPSTR)lpbMemory,toWrite);
    }
}

PString ReiserFsPartition::GetFileAsString(ReiserFsFileInfo* pFile) 
{
    GETFILEINDIRECTCONTEXT context;
    ZeroMemory(&context, sizeof(context));
    REISERFS_CPU_KEY key;

    PString strResult;
    BOOL bSuccess = FALSE;

    // get indirect parts
    context.fp = NULL;
    context.pString = &strResult;
    context.FileSize = pFile->m_stat.sd_size;
    if( pFile->m_stat.sd_size > 500 )
    {
        key.k_dir_id = pFile->m_deh.deh_dir_id;
        key.k_objectid = pFile->m_deh.deh_objectid;
        key.k_offset = 1;
        key.k_type = dwTypeIndirect;
        ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileIndirect, &context );
    }
    // get direct parts
    context.fp = NULL;
    context.pString = &strResult;
    // context.FileSize = pFile->m_stat.sd_size;  // do not set file size again
    key.k_dir_id = pFile->m_deh.deh_dir_id;
    key.k_objectid = pFile->m_deh.deh_objectid;
    key.k_offset = 1;
    key.k_type = -1;

    DEBUGTRACE(("READ SYMBOLIC LINK !!!! %s", (LPCSTR) pFile->m_strName ))
    ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileDirect, &context );

    return strResult;
}

//MPA 6-26-2003: GetFile makes calls to this for file processing: recursion control is effective here
//Modified the original routine to support recursion control.
bool ReiserFsPartition::CopyFilesRecursive( PList* Directory, LPSTR lpszName, bool bRecurseSubdirectories)
{
    REISERFS_CPU_KEY key = { 1, 2, 1, 500 };
	BOOL bSuccess;
    ENUMERATE( Directory, ReiserFsFileInfo, pFile )
    {
		if( !strcmp(pFile->m_strName,".") || 
			!strcmp(pFile->m_strName,"..") )
			continue;


        key.k_dir_id = pFile->m_deh.deh_dir_id;
        key.k_objectid = pFile->m_deh.deh_objectid;
        key.k_offset = 0;
        key.k_type = 0;

        ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileStat, pFile );
//		if( !bSuccess ) 
//		{
//			printf("ERROR, unable to stat %s, ignoring\n", (char*) pFile->m_strName );
//			continue;
//		}
		PString strLocalPath( 0, "%s" SLASH_STRING "%s", lpszName, (char*) pFile->m_strName );

        if( S_ISDIR(pFile->m_stat.sd_mode) )
        {
			//Test for recursion
			if(bRecurseSubdirectories == true)
			{
			printf("Directory %s\n", (char*) strLocalPath );
			if( !MakeSurePathExists(strLocalPath) )
				continue;

			key.k_dir_id = pFile->m_deh.deh_dir_id;
			key.k_objectid = pFile->m_deh.deh_objectid;
			key.k_offset = 1;
			key.k_type = 500;


			PList Subdir;
			bSuccess = FALSE;
			ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::ListDirCallback, &Subdir );
			CopyFilesRecursive( &Subdir, strLocalPath, bRecurseSubdirectories);
			}
		}
        else if( pFile->isSymlink() )
        {
			printf("Warning, link '%s' ignored\n", (const char*) strLocalPath );
        }
        else
        {
			printf("File %s (%"FMT_QWORD" Bytes)\n", (const char*) strLocalPath, pFile->m_stat.sd_size );

            FILE* fp = fopen( strLocalPath,"wb");
            if( fp != NULL )
            {
                GETFILEINDIRECTCONTEXT context;
                ZeroMemory(&context, sizeof(context));

                // get indirect parts
                context.fp = fp;
                context.FileSize = pFile->m_stat.sd_size;
                key.k_dir_id = pFile->m_deh.deh_dir_id;
                key.k_objectid = pFile->m_deh.deh_objectid;

                key.k_offset = 1; // perhaps, file offset ???
                key.k_type = dwTypeIndirect;
                ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileIndirect, &context );

                // get direct parts
                context.fp = fp;
                // context.FileSize = pFile->m_stat.sd_size;  // do not set file size again
                key.k_dir_id = pFile->m_deh.deh_dir_id;
                key.k_objectid = pFile->m_deh.deh_objectid;
                key.k_offset = 1;
                key.k_type = dwTypeDirect;
                ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileDirect, &context );

                fclose(fp);
                SetUnixFileTime(strLocalPath, pFile );

            }
            else
				printf("ERROR, unable to open local file %s\n", (char*) strLocalPath );
        }
    }
	
	return true;
}

bool ReiserFsPartition::GetFile( LPSTR lpszReiserFsName, LPSTR lpszLocalName, bool bRecurseSubdirectories )
{
    // get array of tokens
    PList Tokens;
    PString strTemp(lpszReiserFsName);
    char* token = strtok( strTemp, "/" );
    while( token != NULL )
    {
        Tokens.AddTail( new PString(token) );
        token = strtok( NULL, "/" );
    }
    if( !Tokens.m_lCount )
    {
        printf("ERROR, you must specify a filename\n" );
        return false;
    }

    // find root directory first
    REISERFS_CPU_KEY key = { 1, 2, 1, 500 };

    // find root directory
    BOOL bSuccess = FALSE;
    PList Directory;
    ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::ListDirCallback, &Directory );

    ENUMERATE( &Tokens, PString, pToken )
    {
        INT32 found = 0;

        ENUMERATE( &Directory, ReiserFsFileInfo, pFile )
        {
            if( strcmp( pFile->m_strName, *pToken ) == 0 )
            {
                // read file|dir stat
                bSuccess = FALSE;

                key.k_dir_id = pFile->m_deh.deh_dir_id;
                key.k_objectid = pFile->m_deh.deh_objectid;
                key.k_offset = 0;
                key.k_type = 0;

                DEBUGTRACE(("**** BEGIN GETFILESTAT\n" ))
				ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileStat, pFile );
				DEBUGTRACE(("**** END GETFILESTAT\n" ))

                if( S_ISDIR(pFile->m_stat.sd_mode) )
                {
					key.k_dir_id = pFile->m_deh.deh_dir_id;
					key.k_objectid = pFile->m_deh.deh_objectid;
					key.k_offset = 1;
					key.k_type = 500;

					Directory.DeleteContents();
					bSuccess = FALSE;
					ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::ListDirCallback, &Directory );
					found = 1;

					if( pToken->m_pNext == 0 )
					{
						MakeSurePathExists(lpszLocalName);
						return CopyFilesRecursive( &Directory, lpszLocalName, bRecurseSubdirectories);
					}
					break;
                }
				//Sym-link
                else if( pFile->isSymlink() )
                {
                    PString strData(GetFileAsString(pFile) );
                    if( !IsEmptyString(strData) )
                    {
                        token = strtok( strData, "/" );
                        PString* pInsertPos = pToken;
                        while( token != NULL )
                        {
                            PString* pNewToken = new PString(token);
                            Tokens.InsertAfter( pNewToken, pInsertPos );
                            pInsertPos = pNewToken; 
                            token = strtok( NULL, "/" );
                        }
                        found = 1;
                        break;
                    }
                    else
                    {
                        return false;
                    }
                }
                else if( pToken->m_pNext )
                {
                    printf("ERROR, directory '%s' contains file or link\n", lpszReiserFsName );
                    return false;    
                }
                else
                {
                    printf("found\nRetrieving a total of %"FMT_QWORD" bytes\n", pFile->m_stat.sd_size );

                    FILE* fp = fopen( lpszLocalName,"wb");
                    if( fp != NULL )
                    {
                        GETFILEINDIRECTCONTEXT context;
                        ZeroMemory(&context, sizeof(context));

                        // get indirect parts
                        context.fp = fp;
                        context.FileSize = pFile->m_stat.sd_size;
                        key.k_dir_id = pFile->m_deh.deh_dir_id;
                        key.k_objectid = pFile->m_deh.deh_objectid;

                        key.k_offset = 1; // perhaps, file offset ???
                        key.k_type = dwTypeIndirect;
                        ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileIndirect, &context );

                        // get direct parts
                        context.fp = fp;
                        // context.FileSize = pFile->m_stat.sd_size;  // do not set file size again
                        key.k_dir_id = pFile->m_deh.deh_dir_id;
                        key.k_objectid = pFile->m_deh.deh_objectid;
                        key.k_offset = 1;
                        key.k_type = dwTypeDirect;
                        ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileDirect, &context );

                        fclose(fp);

                        SetUnixFileTime(lpszLocalName, pFile );
                        return true;
                    }
                    printf("ERROR, unable to open local file %s\n", lpszLocalName );
                    return false;
                }
            }
        }
        if( !found )
        {
            printf("ERROR, directory '%s' not found\n", lpszReiserFsName );
            return false;    
        }
        token = strtok( NULL, "/" );
    }
    return false;    
}

bool ReiserFsPartition::GetFileEx( LPSTR lpszReiserFsName, ICreateFileInfo* pCFI )
{
    // get array of tokens
    PList Tokens;
    PString strTemp(lpszReiserFsName);
    char* token = strtok( strTemp, "/" );
    while( token != NULL )
    {
        Tokens.AddTail( new PString(token) );
        token = strtok( NULL, "/" );
    }
    if( !Tokens.m_lCount )
    {
        printf("ERROR, you must specify a filename\n" );
        return false;
    }

    // find root directory first
    REISERFS_CPU_KEY key = { 1, 2, 1, 500 };

    // find root directory
    BOOL bSuccess = FALSE;
    PList Directory;
    ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::ListDirCallback, &Directory );

    ENUMERATE( &Tokens, PString, pToken )
    {
        INT32 found = 0;

        ENUMERATE( &Directory, ReiserFsFileInfo, pFile )
        {
            if( strcmp( pFile->m_strName, *pToken ) == 0 )
            {
                // read file|dir stat
                bSuccess = FALSE;

                key.k_dir_id = pFile->m_deh.deh_dir_id;
                key.k_objectid = pFile->m_deh.deh_objectid;
                key.k_offset = 0;
                key.k_type = 0;

                DEBUGTRACE(("**** BEGIN GETFILESTAT\n" ))
				ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileStat, pFile );
				DEBUGTRACE(("**** END GETFILESTAT\n" ))

                if( S_ISDIR(pFile->m_stat.sd_mode) )
                {
					key.k_dir_id = pFile->m_deh.deh_dir_id;
					key.k_objectid = pFile->m_deh.deh_objectid;
					key.k_offset = 1;
					key.k_type = 500;

					Directory.DeleteContents();
					bSuccess = FALSE;
					ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::ListDirCallback, &Directory );
					found = 1;

					if( pToken->m_pNext == 0 )
					{
                        return false;
					}
					break;
                }
                else if( pFile->isSymlink() ) 
                {
                    PString strData(GetFileAsString(pFile) );
                    if( !IsEmptyString(strData) )
                    {
                        token = strtok( strData, "/" );
                        PString* pInsertPos = pToken;
                        while( token != NULL )
                        {
                            PString* pNewToken = new PString(token);
                            Tokens.InsertAfter( pNewToken, pInsertPos );
                            pInsertPos = pNewToken; 
                            token = strtok( NULL, "/" );
                        }
                        found = 1;
                        break;
                    }
                    else
                    {
                        // this is an empty link
                        pCFI->SetFileSize(0);
                        return true;
                    }
                }
                else if( pToken->m_pNext )
                {
                    printf("ERROR, directory '%s' contains file or link\n", lpszReiserFsName );
                    return false;    
                }
                else
                {
                    GETFILEINDIRECTCONTEXT context;
                    ZeroMemory(&context, sizeof(context));

                    // get indirect parts
                    context.pCFI = pCFI;
                    context.FileSize = pFile->m_stat.sd_size;
                    key.k_dir_id = pFile->m_deh.deh_dir_id;
                    key.k_objectid = pFile->m_deh.deh_objectid;

                    pCFI->SetFileSize(context.FileSize);

                    key.k_offset = 1; // perhaps, file offset ???
                    key.k_type = dwTypeIndirect;
                    ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileIndirect, &context );

                    // get direct parts
                    context.pCFI = pCFI;
                    //context.FileSize = pFile->m_stat.sd_size;
                    key.k_dir_id = pFile->m_deh.deh_dir_id;
                    key.k_objectid = pFile->m_deh.deh_objectid;
                    key.k_offset = 1;
                    key.k_type = dwTypeDirect;
                    ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileDirect, &context );

                    return true;
                }
            }
        }
        if( !found )
        {
            printf("ERROR, directory '%s' not found\n", lpszReiserFsName );
            return false;    
        }
        token = strtok( NULL, "/" );
    }
    return false;    
}


bool ReiserFsPartition::ListDir( PList* pDirectory, LPSTR lpszDirectory )
{                                        
    PList Tokens;
    PString strTemp(lpszDirectory);
    char* token = strtok( strTemp, "/" );

    while( token != NULL )
    {
        Tokens.AddTail( new PString(token) );
        token = strtok( NULL, "/" );
		
    }

    // find root directory first
    REISERFS_CPU_KEY key = { 1, 2, 1, 500 };

    // find root directory
    BOOL bSuccess = FALSE;

    ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::ListDirCallback, pDirectory );

    ENUMERATE( &Tokens, PString, pToken )
    {
        INT32 found = 0;
        char* token = *pToken;

        ENUMERATE( pDirectory, ReiserFsFileInfo, pFile )
        {
            if( strcmp( pFile->m_strName, token ) == 0 )
            {
                // read file|dir stat

                key.k_dir_id = pFile->m_deh.deh_dir_id;
                key.k_objectid = pFile->m_deh.deh_objectid;
                key.k_offset = 0;
                key.k_type = 0;

                bSuccess = FALSE;	 
                ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileStat, pFile );

                if( S_ISDIR(pFile->m_stat.sd_mode) )
                {
                    key.k_dir_id = pFile->m_deh.deh_dir_id;
                    key.k_objectid = pFile->m_deh.deh_objectid;
                    key.k_offset = 1;
                    key.k_type = 500;

                    pDirectory->DeleteContents();
                    bSuccess = FALSE;

					
                    ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::ListDirCallback, pDirectory );
                    found = 1;
                    break;
                }
                else if( pFile->isSymlink() )
                {
                    PString strData(GetFileAsString(pFile) );
                    if( !IsEmptyString(strData) )
                    {
                        token = strtok( strData, "/" );
                        PString* pInsertPos = pToken;
                        while( token != NULL )
                        {
                            PString* pNewToken = new PString(token);
                            Tokens.InsertAfter( pNewToken, pInsertPos );
                            pInsertPos = pNewToken; 
                            token = strtok( NULL, "/" );
                        }
                        found = 1;
                        break;
                    }
                    else
                    {
                        return false;
                    }
                }
                else 
                {
                    printf("ERROR, Directory '%s' contains file or link '%s'\n", lpszDirectory, token );
                    pDirectory->DeleteContents();
                    return false;    
                }
            }
        }
        if( !found )
        {
            printf("ERROR, directory '%s' not found\n", lpszDirectory );
            pDirectory->DeleteContents();
            return false;    
        }
    }
    if( pDirectory->m_lCount )
    {
        // populate statistics
        INT32 count = 0;
        ENUMERATE( pDirectory, ReiserFsFileInfo, pFile )
        {
            if( count++ < 2 )
                continue;

            BOOL bSuccess = FALSE;
            REISERFS_CPU_KEY key;

            key.k_dir_id = pFile->m_deh.deh_dir_id;
            key.k_objectid = pFile->m_deh.deh_objectid;
            key.k_offset = 0;
            key.k_type = 0;

            ParseTreeRecursive( &bSuccess, &key, m_sb.s_root_block, &::GetFileStat, pFile );
        }
        return true;
    }
    return false;
}

LPBYTE ReiserFsPartition::GetBlock( DWORD BlockNumber )
{
    ENUMERATE(&m_BlockCache,ReiserFsBlock,p)
    {
        if( p->m_dwBlockNumber == BlockNumber ) 
            return p->m_lpbMemory;
    }
    LPBYTE lpbMemory = new BYTE[m_dwBlockSize];
	BOOL bSuccess;
	if( m_Metafile.m_pDataFile )
	{
		bSuccess = m_Metafile.Read( lpbMemory, m_dwBlockSize, BlockNumber );
	}
	else
	{
	    bSuccess = Read( lpbMemory, m_dwBlockSize, BlockNumber );
	}
	if( bSuccess )
	{
		m_BlockCache.AddHead(new ReiserFsBlock(BlockNumber, lpbMemory));
		return lpbMemory;
	}
	printf("*** FATAL ERROR, unable to read block %d\n", BlockNumber );
	return NULL;
}

LPBYTE ReiserFsPartition::GetBlockUncached( DWORD BlockNumber )
{
    ENUMERATE(&m_BlockCache,ReiserFsBlock,p)
    {
        if( p->m_dwBlockNumber == BlockNumber ) 
            return NULL;
    }
    LPBYTE lpbMemory = new BYTE[m_dwBlockSize];
    Read( lpbMemory, m_dwBlockSize, BlockNumber );
    m_BlockCache.AddHead(new ReiserFsBlock(BlockNumber, lpbMemory));
    return lpbMemory;
}

BOOL ReiserFsPartition::PrepareForRestore( const char* pszFilename )
{
	return m_Metafile.Open( this, pszFilename );
}

void ReiserFsPartition::Backup( const char* pszFilename )
{
	FILE* fpData = fopen(pszFilename,"wb");
	if( fpData != NULL )
	{
		PString strIndexName( 0, "%s.index", pszFilename );
		FILE* fpIndex = fopen(strIndexName,"wb");
		if( fpIndex != NULL )
		{
			fwrite(&m_sb,sizeof(REISERFS_SUPER_BLOCK),1,fpIndex);
			BackupTreeRecursive(fpData,fpIndex,m_sb.s_root_block);
			printf("done.\n");
			fclose(fpIndex);
		}
		else printf("ERROR, unable to open file '%s' for writing\n", (char*)strIndexName );

		printf("done.\n");
		fclose(fpData);
	}
	else printf("ERROR, unable to open file '%s' for writing\n", pszFilename );
}

void ReiserFsPartition::BackupTreeRecursive( FILE* fpData, FILE* fpIndex, INT32 nBlock )
{
    BYTE* bMemory = GetBlockUncached( nBlock );
	if( !bMemory )
	{
		// assume this block has already been written
		return;
	}

	// write block header
	fwrite(&nBlock,sizeof(INT32),1,fpIndex);

	// write block data
	fwrite(bMemory,m_dwBlockSize,1,fpData);
	//putchar('.');

    LPREISERFS_BLOCK_HEAD pH = (LPREISERFS_BLOCK_HEAD)bMemory;
	if( pH->blk_level != 1 )
	{
		LPBYTE lpbHeaderData = bMemory+sizeof(REISERFS_BLOCK_HEAD);
		LPBYTE lpbPointerData = bMemory+sizeof(REISERFS_BLOCK_HEAD)+(pH->blk_nr_item*sizeof(REISERFS_KEY));
		INT32 i;

		for( i = 0; i < pH->blk_nr_item; i++ )
		{
			REISERFS_KEY* key = (REISERFS_KEY*)(lpbHeaderData+i*sizeof(REISERFS_KEY));

            REISERFS_CPU_KEY cpukey;
            cpukey.k_dir_id = key->k_dir_id;
            cpukey.k_objectid = key->k_objectid;
		    cpukey.k_type = (INT32) key->u.k_offset_v1.k_uniqueness; // WAS: v2
		    cpukey.k_offset = key->u.k_offset_v1.k_offset;

			REISERFS_DISK_KEY* pointer = (REISERFS_DISK_KEY*) (lpbPointerData+i*sizeof(REISERFS_DISK_KEY));
			BackupTreeRecursive( fpData, fpIndex, pointer->dc_block_number );
		}
		REISERFS_DISK_KEY* pointer = (REISERFS_DISK_KEY*) (lpbPointerData+i*sizeof(REISERFS_DISK_KEY));
		BackupTreeRecursive( fpData, fpIndex, pointer->dc_block_number );
	}
}


void ReiserFsPartition::DumpTree()
{
	DumpTreeRecursive(m_sb.s_root_block,0);
}

void ReiserFsPartition::DumpTreeRecursive( INT32 nBlock, INT32 nIndent )
{
	CHAR szIndent[64];
	if( nIndent )
		memset(szIndent,'\t',nIndent);
	szIndent[nIndent] = 0;

    BYTE* bMemory = GetBlock( nBlock );
	if( !bMemory ) return;

	printf("%s---------------- BEGIN BLOCK %d ---------------\n", szIndent, nBlock );

    LPREISERFS_BLOCK_HEAD pH = (LPREISERFS_BLOCK_HEAD)bMemory;
	if( pH->blk_level == 1 )
	{
		printf("%sIS LEAF BLOCK (%d ITEMS)\n", szIndent, pH->blk_nr_item );

		LPBYTE lpbHeaderData = bMemory+sizeof(REISERFS_BLOCK_HEAD);
        bool result = false;
		for( INT32 i = 0; i < pH->blk_nr_item; i++ )
		{
			LPREISERFS_ITEM_HEAD iH = (LPREISERFS_ITEM_HEAD)lpbHeaderData;
            REISERFS_KEY* key = &(iH->ih_key);
            REISERFS_CPU_KEY cpukey;
            cpukey.k_dir_id = key->k_dir_id;
            cpukey.k_objectid = key->k_objectid;
	        if( iH->ih_version == ITEM_VERSION_1 )
	        {
		        cpukey.k_type = iH->ih_key.u.k_offset_v1.k_uniqueness;
		        cpukey.k_offset = iH->ih_key.u.k_offset_v1.k_offset;
	        }
	        else if ( iH->ih_version == ITEM_VERSION_2 )
	        {
		        cpukey.k_type = (INT32) iH->ih_key.u.k_offset_v2.k_type;
		        cpukey.k_offset = iH->ih_key.u.k_offset_v2.k_offset;
	        }
            else assert(false);


            if( cpukey.k_type == 500 )
            {
			    printf("%s(%d,%d,%"FMT_QWORD",%d) DIRECTORY:\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type );
                // given: 
                // SIZE_OF_BLOCK size of the item on disk.
                // DATA_OF_BLOCK data of the directory item on disk. You should allocate one 
                // byte more and make sure the buffer is zero-terminated for the code below to work.

	            INT32 dh_offset = 0;
	            INT32 dh_strpos = iH->ih_item_len;
	            INT32 i = 0, dh_strlen;
                LPBYTE lpbMemory = bMemory + iH->ih_item_location; 
                               
                CHAR szTempBuffer[512];
	            while( dh_offset < dh_strpos )
	            {
		            REISERFS_DIRECTORY_HEAD* pDH = (REISERFS_DIRECTORY_HEAD*) (lpbMemory+dh_offset);

                    dh_strlen = dh_strpos-pDH->deh_location;
                    memcpy(szTempBuffer, lpbMemory+pDH->deh_location, dh_strlen);
                    szTempBuffer[dh_strlen] = 0;
                    printf("%s\t(%d,%d,%d,%d,%d)=\"%s\"\n", szIndent, 
                        pDH->deh_offset,
                        pDH->deh_dir_id,
                        pDH->deh_objectid,
                        pDH->deh_location,
                        (INT32) pDH->deh_state,
                        szTempBuffer );
		            dh_strpos = pDH->deh_location;
		            dh_offset += sizeof(REISERFS_DIRECTORY_HEAD);
	            }
            }
            else if( cpukey.k_type == 0 )
            {
                INT32 nSize = iH->ih_item_len;
                LPBYTE lpbMemory = bMemory + iH->ih_item_location; 
                REISERFS_STAT2 m_stat;
	            if( nSize == sizeof(REISERFS_STAT2) )
	            {
                    m_stat = *(REISERFS_STAT2*) lpbMemory;
			        printf("%s(%d,%d,%"FMT_QWORD",%d) STAT2 (%d)\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type, m_stat.sd_size );

	            }
	            else if( nSize == sizeof(REISERFS_STAT1) )
	            {
			        printf("%s(%d,%d,%"FMT_QWORD",%d) STAT1 (%d)\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type, m_stat.sd_size );
		            REISERFS_STAT1* v1 = (REISERFS_STAT1*) lpbMemory;
                    m_stat.sd_mode = v1->sd_mode;
                    m_stat.sd_nlink = v1->sd_nlink;
                    m_stat.sd_uid = v1->sd_uid;
                    m_stat.sd_gid = v1->sd_gid;
                    m_stat.sd_size = v1->sd_size;
                    m_stat.sd_atime = v1->sd_atime;
                    m_stat.sd_mtime = v1->sd_mtime;
                    m_stat.sd_ctime = v1->sd_ctime;
	                m_stat.u.sd_rdev = v1->u.sd_rdev;
	            }
                else
                {
			        printf("%s(%d,%d,%"FMT_QWORD",%d) UNKNOWN ???\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type );
                }

            }
            else if( cpukey.k_type == -1 )
            {
			    printf("%s(%d,%d,%"FMT_QWORD",%d) DIRECT\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type );
            }
            else if( cpukey.k_type == -2 )
            {
			    printf("%s(%d,%d,%"FMT_QWORD",%d) INDIRECT\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type );
            }
            else
            {
			    printf("%s(%d,%d,%"FMT_QWORD",%d) UNKNOWN ???\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type );
            }
			lpbHeaderData += sizeof(REISERFS_ITEM_HEAD);
		}
	}
	else
	{
		INT32 i;
		printf("%sIS DATA BLOCK (%d ITEMS)\n", szIndent, pH->blk_nr_item );

		LPBYTE lpbHeaderData = bMemory+sizeof(REISERFS_BLOCK_HEAD);
		LPBYTE lpbPointerData = bMemory+sizeof(REISERFS_BLOCK_HEAD)+(pH->blk_nr_item*sizeof(REISERFS_KEY));

		for( i = 0; i < pH->blk_nr_item; i++ )
		{
			REISERFS_KEY* key = (REISERFS_KEY*)(lpbHeaderData+i*sizeof(REISERFS_KEY));

            REISERFS_CPU_KEY cpukey;
            cpukey.k_dir_id = key->k_dir_id;
            cpukey.k_objectid = key->k_objectid;
		    cpukey.k_type = (INT32) key->u.k_offset_v1.k_uniqueness; // WAS: v2
		    cpukey.k_offset = key->u.k_offset_v1.k_offset;

			REISERFS_DISK_KEY* pointer = (REISERFS_DISK_KEY*) (lpbPointerData+i*sizeof(REISERFS_DISK_KEY));
			printf("%s(%d,%d,%"FMT_QWORD",%d)->%d\n", szIndent, cpukey.k_dir_id, cpukey.k_objectid, cpukey.k_offset, cpukey.k_type, pointer->dc_block_number );
			DumpTreeRecursive( pointer->dc_block_number, nIndent+1 );

		}
		REISERFS_DISK_KEY* pointer = (REISERFS_DISK_KEY*) (lpbPointerData+i*sizeof(REISERFS_DISK_KEY));
		printf("%s(right)->%d\n", szIndent, pointer->dc_block_number );
		DumpTreeRecursive( pointer->dc_block_number, nIndent+1 );
	}

	printf("%s---------------- END BLOCK %d ---------------\n", szIndent, nBlock );
}


void ReiserFsPartition::ParseTreeRecursive( BOOL* lpbSuccess, REISERFS_CPU_KEY* lpKeyToFind, INT32 nBlock, LPFNReiserFsSearchCallback lpCallback, void* lpContext )
{
	m_lpKeyToFind = lpKeyToFind;
	m_lpCallback = lpCallback;
	m_lpContext = lpContext;
	m_nIndent = 0;
	DEBUGTRACE(("\nBEGIN ParseTreeRecursive (%ld,%ld,%" FMT_QWORD ",%ld)\n",
    lpKeyToFind->k_dir_id,
    lpKeyToFind->k_objectid,
    lpKeyToFind->k_offset,
    lpKeyToFind->k_type ))

	*lpbSuccess = IParseTreeRecursive( nBlock );
	DEBUGTRACE(("END ParseTreeRecursive\n" ))

}

BOOL ReiserFsPartition::IParseTreeRecursive( INT32 nBlock )
{
	BOOL bResult = FALSE;
#ifdef _DEBUG
	char szIndent[60];
	INT32 nMax = sizeof(szIndent)-1;
	nMax = m_nIndent>nMax?nMax:m_nIndent;
	memset( szIndent, '\t', nMax );
	szIndent[nMax] = 0;
	m_nIndent++;
#endif

	DEBUGTRACE(("%sBEGIN BLOCK %d -----------------------------\n", szIndent, nBlock ))

    if( nBlock == 526541 )
    {
    DEBUGTRACE(("%sBEGIN BLOCK %d -----------------------------\n", szIndent, nBlock ))
    }


    BYTE* bMemory = GetBlock(nBlock);
	if( !bMemory ) 
	{
        bResult = FALSE;
	}
	else
	{
		LPREISERFS_BLOCK_HEAD pH = (LPREISERFS_BLOCK_HEAD)bMemory;
		if( pH->blk_level == 1 )
		{
			DEBUGTRACE(("%sANALYZING LEAF BLOCK\n", szIndent ))
			LPBYTE lpbHeaderData = bMemory+sizeof(REISERFS_BLOCK_HEAD);

			for( INT32 i = 0; i < pH->blk_nr_item; i++ )
			{
				LPREISERFS_ITEM_HEAD iH = (LPREISERFS_ITEM_HEAD)lpbHeaderData;
				REISERFS_KEY* key = &(iH->ih_key);
				REISERFS_CPU_KEY cpukey;
				cpukey.k_dir_id = key->k_dir_id;
				cpukey.k_objectid = key->k_objectid;
				if( iH->ih_version == ITEM_VERSION_1 )
				{
					cpukey.k_type = iH->ih_key.u.k_offset_v1.k_uniqueness;
					cpukey.k_offset = iH->ih_key.u.k_offset_v1.k_offset;
				}
				else if ( iH->ih_version == ITEM_VERSION_2 )
				{
					cpukey.k_type = (INT32) iH->ih_key.u.k_offset_v2.k_type;
					cpukey.k_offset = iH->ih_key.u.k_offset_v2.k_offset;
				}
				else assert(false);

				INT32 cres = comp_keys_no_offset(m_lpKeyToFind,&cpukey);
				if( cres == 0  )
				{
					DEBUGTRACE(("%s**** CALLBACK ****\n", szIndent ))
					m_lpCallback(this, &cpukey, bMemory + iH->ih_item_location, iH->ih_item_len, m_lpContext);
					bResult = TRUE;
					//goto bailout;
				}
				else if( cres < 0 )
				{
					//DEBUGTRACE(("%s**** QUICK TO BAILOUT ****\n", szIndent ))
					//goto bailout;
				}
				lpbHeaderData += sizeof(REISERFS_ITEM_HEAD);
			}
		}
		else
		{
			DEBUGTRACE(("%sANALYZING TREE BLOCK\n", szIndent ))
			LPBYTE lpbHeaderData = bMemory+sizeof(REISERFS_BLOCK_HEAD);
			LPBYTE lpbPointerData = bMemory+sizeof(REISERFS_BLOCK_HEAD)+(pH->blk_nr_item*sizeof(REISERFS_KEY));

			INT32 i, nLastCres = -1;
			bool oldFound = false;

			for( i = 0; i < pH->blk_nr_item; i++ )
			{
				REISERFS_KEY* key = (REISERFS_KEY*)(lpbHeaderData+i*sizeof(REISERFS_KEY));

				REISERFS_CPU_KEY cpukey;
				cpukey.k_dir_id = key->k_dir_id;
				cpukey.k_objectid = key->k_objectid;
				cpukey.k_type = (INT32) key->u.k_offset_v2.k_type;
				cpukey.k_offset = (U64) key->u.k_offset_v2.k_offset;

				INT32 cres = comp_keys(&cpukey,m_lpKeyToFind);
				if( cres == 1 || cres == 0 )
				{
					REISERFS_DISK_KEY* pointer = (REISERFS_DISK_KEY*) (lpbPointerData+i*sizeof(REISERFS_DISK_KEY));
					if( IParseTreeRecursive( pointer->dc_block_number ) )
					{
						oldFound = true;
					}
					else
					{
						if( oldFound )
						{
							bResult = TRUE;
							goto bailout;
						}
						else if( nLastCres == 1 && cres == 1 )
						{
							goto bailout;
						}
					}
				}
				nLastCres = cres;
			}
			REISERFS_DISK_KEY* pointer = (REISERFS_DISK_KEY*) (lpbPointerData+i*sizeof(REISERFS_DISK_KEY));
			if( IParseTreeRecursive( pointer->dc_block_number ) )
			{
				bResult = TRUE;
			}
		}
	}

bailout:
	DEBUGTRACE(("%sEND BLOCK %d WITH %s -----------------------------\n", szIndent, nBlock, bResult ? "TRUE" : "FALSE" ))
#ifdef _DEBUG
	m_nIndent--;
#endif
	return bResult;
}

// potential code for bitmap dumiong
//    BYTE* pBitmap = m_lpbBitmap;
//    
//    INT32 BitmapSize = 0x9000;
//    INT32 BitmapOffset = 0;
//    INT32 iLastBitUsed = -1;
//    BYTE bitmap_map[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
//    INT32 iBit = 0;
//    while( iBit < BitmapSize )
//    {
//        if( pBitmap[iBit/8] & bitmap_map[iBit%8] )
//        {
//            if( iLastBitUsed == -1 )
//                iLastBitUsed = iBit;
//        }
//        else
//        {
//            // bit is not set
//            if( iLastBitUsed != -1 )
//            {
//                fprintf(fp,"used: %d..%d\r\n", BitmapOffset+iLastBitUsed, BitmapOffset+iBit-1 );
//                iLastBitUsed = -1;
//            }
//        }
//        iBit++;
//    }
//    fclose(fp);

ReiserFsMetafile::ReiserFsMetafile()
{
	m_pBlockIndices = NULL;
	m_pDataFile = NULL;
}

ReiserFsMetafile::~ReiserFsMetafile()
{
	delete m_pBlockIndices;
	if( m_pDataFile )
		fclose( m_pDataFile );

}

BOOL ReiserFsMetafile::Open( ReiserFsPartition* partition, const char* pszFilename )
{
	// read index file first
	PString strIndexName( 0, "%s.index", pszFilename );
	FILE* fpIndex = fopen(strIndexName,"rb");
	if( fpIndex != NULL )
	{
		// determine file size
		fseek(fpIndex,0,SEEK_END);
		INT64 filesize = ftell(fpIndex);
		fseek(fpIndex,0,SEEK_SET);

		if( filesize < sizeof(REISERFS_SUPER_BLOCK) )
		{
			printf("ERROR, file '%s' too small to be a valid index file, aborting.\n", (char*) strIndexName );
			fclose(fpIndex);
			return FALSE;
		}
		fread(&m_Superblock,sizeof(REISERFS_SUPER_BLOCK),1,fpIndex);
		m_dwBlocksize = m_Superblock.s_blocksize;
        if( partition )
        {
		    partition->m_sb = m_Superblock;
        }
		INT64 indexsize = filesize - sizeof(REISERFS_SUPER_BLOCK);
		m_iNumberOfIndices = (INT32)(indexsize / sizeof(INT32));
		m_pBlockIndices = new LONG_PTR[m_iNumberOfIndices];
		if( !m_pBlockIndices )
		{
			printf("ERROR, not enough memory to allocate %d bytes for indices, aborting\n", indexsize );
			fclose(fpIndex);
			return FALSE;
		}
		//64bit watch
		fread((void*)m_pBlockIndices,indexsize,1,fpIndex);
		fclose(fpIndex);

		printf("Read %d indices successfully.\n", m_iNumberOfIndices );
	}
	else 
	{
		printf("ERROR, unable to open file '%s' for reading\n", (char*)strIndexName );
		return FALSE;
	}
	m_pDataFile = fopen(pszFilename,"rb");
	if( m_pDataFile == NULL )
	{
		printf("ERROR, unable to open file '%s' for reading\n", pszFilename );
		return FALSE;
	}
	return TRUE;
}

BOOL ReiserFsMetafile::Read( LPBYTE lpbMemory, DWORD dwSize, INT64 BlockNumber )
{
	if( dwSize != m_dwBlocksize )
	{
		printf("ERROR, expected block size %d, got %d\n", m_dwBlocksize, dwSize );
		return FALSE;
	}
	INT32 index, blocknr = (INT32) BlockNumber;
	for( index = 0; index < m_iNumberOfIndices; index++ )
	{
		if( m_pBlockIndices[index] == blocknr )
		{
			fseek(m_pDataFile,index*m_dwBlocksize,SEEK_SET);
			fread(lpbMemory,m_dwBlocksize,1,m_pDataFile);
			return TRUE;
		}
	}
	return FALSE;
} // Read()
