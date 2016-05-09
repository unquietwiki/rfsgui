#ifndef PDriveNT_H
#define PDriveNT_H

#include "ntdiskspec.h"

class PNtPhysicalDrive : public IPhysicalDrive
    {
    public:
        PNtPhysicalDrive();
        virtual ~PNtPhysicalDrive();
        virtual BOOL Open( LONG_PTR iDrive );
		virtual void Close();
        virtual BOOL GetDriveGeometry( DISK_GEOMETRY* lpDG );
        virtual BOOL GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize );
        virtual BOOL ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 Sector );
#ifdef SUPPORT_WINDOWS_XP_PARTITIONS
        // shit for Windows XP
        virtual BOOL GetDriveGeometryEx( DISK_GEOMETRY_EX* lpDG, DWORD dwSize );
        virtual BOOL GetDriveLayoutEx( LPBYTE lpbMemory, DWORD dwSize );
#endif

        HANDLE m_hDevice;
    };

#endif // PDriveNT_H


