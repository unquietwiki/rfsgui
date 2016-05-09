#include "precomp.h"
#include "pdrivefile.h"

class KnownDriveSector : public PNode
    {
    public:
        KnownDriveSector( INT64 OffsetInBytes, DWORD dwFileOffset );
        
        INT64 m_OffsetInBytes; // offset on disk
        DWORD m_dwFileOffset; // offset in file
    };

PSimulatedDriveFromBackupFile::PSimulatedDriveFromBackupFile(LPCSTR lpszFilename)
    :   m_strFilename( lpszFilename )
{
}

PSimulatedDriveFromBackupFile::~PSimulatedDriveFromBackupFile()
{
}

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
BOOL PSimulatedDriveFromBackupFile::GetDriveGeometryEx( DISK_GEOMETRY_EX* lpDG, DWORD dwSize )
{
    return FALSE;
}
BOOL PSimulatedDriveFromBackupFile::GetDriveLayoutEx( LPBYTE lpbMemory, DWORD dwSize )
{
    return FALSE;
}
#endif

BOOL PSimulatedDriveFromBackupFile::Open(LONG_PTR iDrive)
{
    return m_Metafile.Open( NULL, m_strFilename );
}

void PSimulatedDriveFromBackupFile::Close()
{
}

BOOL PSimulatedDriveFromBackupFile::GetDriveGeometry( DISK_GEOMETRY* lpDG )
{
    memset(lpDG,0,sizeof(DISK_GEOMETRY));  
	lpDG->BytesPerSector = m_Metafile.m_dwBlocksize;
    return TRUE;
}

BOOL PSimulatedDriveFromBackupFile::ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 OffsetInBytes )
{
    if( OffsetInBytes == REISERFS_DISK_OFFSET_IN_BYTES )
    {
        memcpy(lpbMemory, &m_Metafile.m_Superblock, sizeof(m_Metafile.m_Superblock) );
        return TRUE;
    }
    else
    {
        INT64 OffsetInBlocks = OffsetInBytes / m_Metafile.m_dwBlocksize;
        return m_Metafile.Read( lpbMemory, dwSize, OffsetInBlocks );
    }
}

BOOL PSimulatedDriveFromBackupFile::GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize )
{
	DWORD dwBytesRequired = sizeof(DRIVE_LAYOUT_INFORMATION) + sizeof(PARTITION_INFORMATION);
	
	if( dwSize < dwBytesRequired )
		return FALSE;

	PDRIVE_LAYOUT_INFORMATION pli = (PDRIVE_LAYOUT_INFORMATION) lpbMemory;
	memset(lpbMemory, 0, dwSize);
	pli->PartitionCount = 1;
	pli->Signature = 0;
	return TRUE;
}


