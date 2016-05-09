#ifndef pdriveposix_H
#define pdriveposix_H

#include "physicaldrive.h"

class PPosixPhysicalDrive : public IPhysicalDrive
    {
    public:
        PPosixPhysicalDrive();
        virtual ~PPosixPhysicalDrive();
        virtual BOOL Open( LONG_PTR iDrive );
		virtual void Close();
        virtual BOOL GetDriveGeometry( DISK_GEOMETRY* lpDG );
        virtual BOOL GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize );
        virtual BOOL ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 OffsetInBytes );

		virtual BOOL ReadPartitionInfoRecursive(DWORD dwSector,INT64 TotalOffset);
    
		PList m_PartitionInfo;
        INT32 m_iDriveHandle;
        INT32 m_iDriveNumber;
    };

#endif // pdriveposix_H
