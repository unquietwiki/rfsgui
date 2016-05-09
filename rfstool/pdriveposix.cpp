#include "precomp.h"
#include "pdriveposix.h"
#include "unistd.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "sys/types.h"

#define USE_SPECIFIC_DEVICE -2
extern char* g_szUseSpecificDevice;

PPosixPhysicalDrive::PPosixPhysicalDrive()
{
    m_iDriveHandle = 0;
}

PPosixPhysicalDrive::~PPosixPhysicalDrive()
{
    if( m_iDriveHandle )
    {
        close( m_iDriveHandle );
        m_iDriveHandle = 0;
    }
}

BOOL PPosixPhysicalDrive::ReadPartitionInfoRecursive(DWORD dwSector,INT64 TotalOffset )
{
	BYTE mbr[512];
	INT64 OffsetInBytes = dwSector;
	OffsetInBytes *= 512;


	if( ReadAbsolute(mbr,sizeof(mbr),OffsetInBytes) )
	{
		MBR* pMBR = (MBR*) &(mbr[446]);
		if( pMBR->wSignature != 0xaa55 )
			return FALSE;

		INT64 ScheissOffset = 0;

		for( int i = 0; i < 4; i++ )
		{
			PARTITIONINFO* source = &(pMBR->pi[i]);

			if( !source->SIZE && !source->LBA )
				continue;

			PARTITION_INFORMATION pi;
			memset(&pi,0,sizeof(pi));

			pi.PartitionLength.QuadPart = source->SIZE;
			pi.PartitionLength.QuadPart *= 512L;
			pi.PartitionType = source->Type;

			if( i == 0 )
			{
				pi.StartingOffset.QuadPart = source->LBA;
				pi.StartingOffset.QuadPart *= 512L;
				pi.StartingOffset.QuadPart += TotalOffset;
				ScheissOffset = pi.StartingOffset.QuadPart;
			}
			else
			{
				pi.StartingOffset.QuadPart = ScheissOffset;
			}
			ScheissOffset += pi.PartitionLength.QuadPart;

			P9xPartitionInfo* p9pi = new P9xPartitionInfo(&pi);
			m_PartitionInfo.AddTail( p9pi );
			if( pi.PartitionType == PARTITION_TYPE_EXTENDED )
			{
				if( !ReadPartitionInfoRecursive(dwSector + source->LBA,pi.StartingOffset.QuadPart) )
				{
					p9pi->m_pi.StartingOffset.QuadPart += 63*512;
				}
			}
			
		}
	}
	return TRUE;
}

BOOL PPosixPhysicalDrive::Open( int iDrive )
{
    Close();
	m_iDriveNumber = iDrive;

    CHAR szFilenameBuffer[] = "/dev/hda";
	char* szFilename;
	if( iDrive == USE_SPECIFIC_DEVICE )
	{
		szFilename = g_szUseSpecificDevice;
	}
	else
	{
		szFilenameBuffer[7] += (char) iDrive;
		szFilename = szFilenameBuffer;
	}
    
    printf("Using device '%s'\n", szFilename );
    
    m_iDriveHandle = open(szFilename,0);
    if( m_iDriveHandle )
    {
		m_PartitionInfo.DeleteContents();
		ReadPartitionInfoRecursive(0,0);
		return TRUE;
	}
	return FALSE;
}

void PPosixPhysicalDrive::Close()
{
    if( m_iDriveHandle )
    {
        close(m_iDriveHandle);
        m_iDriveHandle = 0;
    }
}

BOOL PPosixPhysicalDrive::GetDriveGeometry( DISK_GEOMETRY* lpDG )
{
    memset(lpDG,0,sizeof(DISK_GEOMETRY));  
	lpDG->BytesPerSector = 512;
    return TRUE;
}

BOOL PPosixPhysicalDrive::ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 OffsetInBytes )
{
	lseek(m_iDriveHandle,OffsetInBytes,SEEK_SET);
    read( m_iDriveHandle, lpbMemory, dwSize );
    return TRUE;
}

BOOL PPosixPhysicalDrive::GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize )
{
	if( m_iDriveNumber == USE_SPECIFIC_DEVICE )
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

	DWORD dwBytesRequired = sizeof(DRIVE_LAYOUT_INFORMATION) + sizeof(PARTITION_INFORMATION)*(m_PartitionInfo.m_lCount-1);
	
	if( dwSize < dwBytesRequired )
		return FALSE;

	PDRIVE_LAYOUT_INFORMATION pli = (PDRIVE_LAYOUT_INFORMATION) lpbMemory;
	pli->PartitionCount = m_PartitionInfo.m_lCount;
	pli->Signature = 0;
	int index = 0;
	ENUMERATE( &m_PartitionInfo, P9xPartitionInfo, pI )
	{
		pli->PartitionEntry[index++] = pI->m_pi;
	}
	return TRUE;
}


