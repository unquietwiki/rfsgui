#include "precomp.h"
#include "physicaldrive.h"
#include "reiserfs.h"
#include "pdrivent.h"

BOOL PNtPhysicalDrive::GetDriveLayout( LPBYTE lpbMemory, DWORD dwSize )
{
    DWORD junk;

    return DeviceIoControl(m_hDevice,  // the device we are querying
                        IOCTL_DISK_GET_DRIVE_LAYOUT,  // operation to perform
                        NULL, 0, // no input buffer, so we pass zero
                        lpbMemory, dwSize,  // the output buffer
                        &junk, // discard the count of bytes returned
                        NULL);
} // GetDriveLayout()

#ifdef SUPPORT_WINDOWS_XP_PARTITIONS

BOOL PNtPhysicalDrive::GetDriveLayoutEx( LPBYTE lpbMemory, DWORD dwSize )
{
    DWORD junk;

    return DeviceIoControl(m_hDevice,  // the device we are querying
                        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,  // operation to perform
                        NULL, 0, // no input buffer, so we pass zero
                        lpbMemory, dwSize,  // the output buffer
                        &junk, // discard the count of bytes returned
                        NULL);
} // GetDriveLayout()

BOOL PNtPhysicalDrive::GetDriveGeometryEx( DISK_GEOMETRY_EX* lpDG, DWORD dwSize )
{
    DWORD junk;

    return DeviceIoControl(m_hDevice,  // the device we are querying
                        IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,  // operation to perform
                        NULL, 0, // no input buffer, so we pass zero
                        lpDG, dwSize,  // the output buffer
                        &junk, // discard the count of bytes returned
                        NULL);
} // GetDriveGeometry()
#endif
                                 
BOOL PNtPhysicalDrive::GetDriveGeometry( DISK_GEOMETRY* lpDG )
{
    DWORD junk;

    return DeviceIoControl(m_hDevice,  // the device we are querying
                        IOCTL_DISK_GET_DRIVE_GEOMETRY,  // operation to perform
                        NULL, 0, // no input buffer, so we pass zero
                        lpDG, sizeof(DISK_GEOMETRY),  // the output buffer
                        &junk, // discard the count of bytes returned
                        NULL);
} // GetDriveGeometry()


BOOL PNtPhysicalDrive::Open( LONG_PTR iDrive )
{
    if(m_hDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle( m_hDevice );
		m_hDevice = INVALID_HANDLE_VALUE;
	}

	CHAR szPath[256];
	sprintf( szPath, "\\\\.\\PhysicalDrive%d", iDrive );

	m_hDevice = CreateFile( szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );
	return (m_hDevice != INVALID_HANDLE_VALUE);
}

void PNtPhysicalDrive::Close()
{
    if(m_hDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle( m_hDevice );
		m_hDevice = INVALID_HANDLE_VALUE;
	}
}

BOOL PNtPhysicalDrive::ReadAbsolute( LPBYTE lpbMemory, DWORD dwSize, INT64 Sector )
{
    LARGE_INTEGER li;
    li.QuadPart = Sector;
    SetFilePointer(m_hDevice, li.LowPart, &li.HighPart, FILE_BEGIN);
    return ReadFile(m_hDevice, lpbMemory, dwSize, &li.LowPart, 0 );
} // ReadAbsolute()

PNtPhysicalDrive::PNtPhysicalDrive()
{
    m_hDevice = INVALID_HANDLE_VALUE;
} // PNtPhysicalDrive()

PNtPhysicalDrive::~PNtPhysicalDrive()
{
    if( m_hDevice != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_hDevice );
    }
} // ~PNtPhysicalDrive()

