#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned long __u32;
#ifdef __GNUC__
typedef long long __u64;
typedef void* LPVOID;
typedef void* HANDLE;
#else
typedef unsigned __int64 __u64;
#endif

typedef char __s8;
typedef short __s16;
typedef long __s32;

#ifdef __GNUC__
#ifndef WINAPI
#define WINAPI
#endif
#endif

#ifndef FMT_QWORD
#ifdef _MSC_VER
#define FMT_QWORD "I64d"
#else
#define FMT_QWORD "qd"
#endif
#endif


#ifndef  S_ISCHR
#define S_IFMT       0170000     /* type of file (mask for following) */
#define S_IFIFO      0010000     /* first-in/first-out (pipe) */
#define S_IFCHR      0020000     /* character-special file */
#define S_IFDIR      0040000     /* directory */
#define S_IFBLK      0060000     /* blocking device (not used on NetWare) */
#define S_IFREG      0100000     /* regular */
#define S_IFLNK      0120000     /* symbolic link (not used on NetWare) */
#define S_IFSOCK     0140000     /* Berkeley socket */

#define S_ISFIFO(m)  (((m) & S_IFMT) == S_IFIFO)   /* (e.g.: pipe) */
#define S_ISCHR(m)   (((m) & S_IFMT) == S_IFCHR)   /* (e.g.: console) */
#define S_ISDIR(m)   (((m) & S_IFMT) == S_IFDIR)
#define S_ISBLK(m)   (((m) & S_IFMT) == S_IFBLK)   /* (e.g.: pipe) */
#define S_ISREG(m)   (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)   (((m) & S_IFMT) == S_IFLNK)   /* should be FALSE */
#define S_ISSOCK(m)  (((m) & S_IFMT) == S_IFSOCK)  /* (e.g.: socket) */
#endif

#define UNIX_FILENAME_LENGTH 255

struct UNIX_FILEINFO
{
	__u16	i_mode;		/* File mode (permissions)*/
	__u16	i_uid;		/* Low 16 bits of Owner Uid */
	__u16	i_gid;		/* Low 16 bits of Group Id */
	__u64	i_size;		/* Size in bytes */
	__s32	i_atime;	/* Access time */
	__s32	i_ctime;	/* Creation time */
	__s32	i_mtime;	/* Modification time */
	char szFileName[UNIX_FILENAME_LENGTH];
};

typedef BOOL (WINAPI* LPFNCopyFileCallback)(LPBYTE lpbData, DWORD dwSize, LPVOID lpContext);

class ICreateFileInfo
    {
    public:
        virtual BOOL SetFileSize( INT64 Size ) = 0;
        virtual void Write( LPBYTE lpbData, DWORD dwSize ) = 0;
    };

class IFilesystem
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

	    virtual HANDLE WINAPI OpenFile(LPCSTR szFilename) const = 0;
	    virtual void WINAPI ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD NumberOfBytesToRead, DWORD& NumberOfBytesRead) const = 0;
	    virtual void WINAPI CloseFile(HANDLE hFile) const = 0;
	    
	    virtual HANDLE WINAPI FindFirstFile(LPCSTR szPath) const = 0;
	    virtual BOOL WINAPI FindNextFile(HANDLE hFind, UNIX_FILEINFO& FileInfo) const = 0;
	    virtual void WINAPI CloseFind(HANDLE hFind) const = 0;

        virtual DWORD WINAPI GetVersion() const = 0;
	    virtual BOOL WINAPI CopyFile(LPCSTR szPath, ICreateFileInfo* cfi ) const = 0;
    };

#define RFSHELL_API_VERSION ((DWORD)2)

#ifdef __cplusplus
extern "C" {
#endif

typedef void (WINAPI* LPFNFoundPartition)(LPCSTR lpszName, LPVOID lpContext);

void WINAPI FS_Autodetect( LPFNFoundPartition lpCallback, LPVOID lpContext );
IFilesystem* WINAPI FS_CreateInstance( LPCSTR lpszName );
void WINAPI FS_DestroyInstance( IFilesystem* pFilesystem );

#ifdef __cplusplus
}
#endif

#endif // IFilesystem_H


