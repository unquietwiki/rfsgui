#include "precomp.h"

#ifndef REISER4WIN_H
#define REISER4WIN_H

#define REISERFS_DISK_OFFSET_IN_BYTES (64 * 1024)
#define REISERFS_SUPER_MAGIC_STRING "ReIsErFs"
#define REISER2FS_SUPER_MAGIC_STRING "ReIsEr2Fs"
#define REISER3FS_SUPER_MAGIC_STRING "ReIsEr3Fs" // seems to work straight outta box

#define USE_SPECIFIC_DEVICE -2
#define USE_BACKUP_FILENAME -3

extern char* g_szUseSpecificDevice;

#pragma pack(1)

#define TYPE_STAT_DATA 0
#define TYPE_INDIRECT 1
#define TYPE_DIRECT 2
#define TYPE_DIRECTORY 3 
#define TYPE_ANY 15 // FIXME: comment is required

//MPA 5-2-2005
//64bit
//Removed already declared defines

#define S_IFBLK      0060000     /* blocking device (not used on NetWare) */
#define S_IFLNK      0120000     /* symbolic link (not used on NetWare) */
#define S_IFSOCK     0140000     /* Berkeley socket */
#define S_IFIFO      0010000     /* first-in/first-out (pipe) */

#define S_ISFIFO(m)  (((m) & S_IFMT) == S_IFIFO)   // (e.g.: pipe)
#define S_ISCHR(m)   (((m) & S_IFMT) == S_IFCHR)   // (e.g.: console)
#define S_ISDIR(m)   (((m) & S_IFMT) == S_IFDIR)
#define S_ISBLK(m)   (((m) & S_IFMT) == S_IFBLK)   // (e.g.: pipe)
#define S_ISREG(m)   (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)   (((m) & S_IFMT) == S_IFLNK)   // should be FALSE
#define S_ISSOCK(m)  (((m) & S_IFMT) == S_IFSOCK)  // (e.g.: socket)

//MPA 5-2-2005
//64bit
//Redefined 32 & 64-bit types to new standard

typedef unsigned short U16;
typedef ULONG32 U32;
typedef ULONG64 U64;

//MPA 11-9-2005: redefined MB converter parameter as function
double PBytesInMBytes(U64 bytes);

typedef struct reiserfs_super_block
{
  U32 s_block_count;
  U32 s_free_blocks;                  /* free blocks count    */
  U32 s_root_block;           	/* root block number    */
  U32 s_journal_block;           	/* journal block number    */
  U32 s_journal_dev;           	/* journal device number  */

  /* Since journal size is currently a #define in a header file, if 
  ** someone creates a disk with a 16MB journal and moves it to a 
  ** system with 32MB journal default, they will overflow their journal 
  ** when they mount the disk.  s_orig_journal_size, plus some checks
  ** while mounting (inside journal_init) prevent that from happening
  */

				/* great comment Chris. Thanks.  -Hans */

  U32 s_orig_journal_size; 		
  U32 s_journal_trans_max ;           /* max number of blocks in a transaction.  */
  U32 s_journal_block_count ;         /* total size of the journal. can change over time  */
  U32 s_journal_max_batch ;           /* max number of blocks to batch into a trans */
  U32 s_journal_max_commit_age ;      /* in seconds, how old can an async commit be */
  U32 s_journal_max_trans_age ;       /* in seconds, how old can a transaction be */
  U16 s_blocksize;                   	/* block size           */
  U16 s_oid_maxsize;			/* max size of object id array, see get_objectid() commentary  */
  U16 s_oid_cursize;			/* current size of object id array */
  U16 s_state;                       	/* valid or error       */
  char s_magic[12];                     /* reiserfs magic string indicates that file system is reiserfs */
  U32 s_hash_function_code;		/* indicate, what hash function is being use to sort names in a directory*/
  U16 s_tree_height;                  /* height of disk tree */
  U16 s_bmap_nr;                      /* amount of bitmap blocks needed to address each block of file system */
  U16 s_version;		/* I'd prefer it if this was a string,
                                   something like "3.6.4", and maybe
                                   16 bytes long mostly unused. We
                                   don't need to save bytes in the
                                   superblock. -Hans */
  U16 s_reserved;
  U32 s_inode_generation;
  char s_unused[124] ;			/* zero filled by mkreiserfs */
} REISERFS_SUPER_BLOCK, *LPREISERFS_SUPER_BLOCK;

struct offset_v1 {
    U32 k_offset;
    U32 k_uniqueness;
};

struct offset_v2 {
	    U64 k_offset:60;
	    U64 k_type: 4;
};

typedef struct  {
    U32 k_dir_id;    /* packing locality: by default parent directory object id */
    U32 k_objectid;  /* object identifier */
    union {
	struct offset_v1 k_offset_v1;
	struct offset_v2 k_offset_v2;                                       
    } u;
} REISERFS_KEY, *LPREISERFS_KEY;

typedef struct
{
    U32 k_dir_id;    /* packing locality: by default parent directory object id */
    U32 k_objectid;  /* object identifier */
    U64 k_offset;
    U32 k_type;
} REISERFS_CPU_KEY, *LPREISERFS_CPU_KEY;

typedef struct {
  U16 blk_level;        /* Level of a block in the tree. */
  U16 blk_nr_item;      /* Number of keys/items in a block. */
  U16 blk_free_space;   /* Block free space in bytes. */
  U16 ignore;
  REISERFS_KEY key;
} REISERFS_BLOCK_HEAD, *LPREISERFS_BLOCK_HEAD;

#define ITEM_VERSION_1 0
#define ITEM_VERSION_2 1

typedef struct item_head
{
  REISERFS_KEY ih_key; 	/* Everything in the tree is found by searching for it based on its key.*/

  union {
    U16 ih_free_space_reserved; /* The free space in the last unformatted node of an indirect item if this
				     is an indirect item.  This equals 0xFFFF iff this is a direct item or
				     stat data item. Note that the key, not this field, is used to determine
				     the item type, and thus which field this union contains. */
    U16 ih_entry_count; /* Iff this is a directory item, this field equals the number of directory
				      entries in the directory item. */
  }u;
  U16 ih_item_len;           /* total size of the item body */
  U16 ih_item_location;      /* an offset to the item body within the block */
				/* I thought we were going to use this
                                   for having lots of item types? Why
                                   don't you use this for item type
                                   not item version.  That is how you
                                   talked me into this field a year
                                   ago, remember?  I am still not
                                   convinced it needs to be 16 bits
                                   (for at least many years), but at
                                   least I can sympathize with that
                                   hope. Change the name from version
                                   to type, and tell people not to use
                                   FFFF in case 16 bits is someday too
                                   small and needs to be extended:-). */
  U16 ih_version;	       /* 0 for all old items, 2 for new
                                  ones. Highest bit is set by fsck
                                  temporary, cleaned after all done */
} REISERFS_ITEM_HEAD, *LPREISERFS_ITEM_HEAD;

typedef struct 
{
  U32 deh_offset;		/* third component of the directory entry key */
  U32 deh_dir_id;		/* objectid of the parent directory of the object, that is referenced
					   by directory entry */
  U32 deh_objectid;		/* objectid of the object, that is referenced by directory entry */
  U16 deh_location;		/* offset of name in the whole item */
  U16 deh_state;		/* whether 1) entry contains stat data (for future), and 2) whether
					   entry is hidden (unlinked) */
} REISERFS_DIRECTORY_HEAD, *LPREISERFS_DIRECTORY_HEAD;

typedef struct stat_data_v1
{
    U16 sd_mode;	/* file type, permissions */
    U16 sd_nlink;	/* number of hard links */
    U16 sd_uid;		/* owner */
    U16 sd_gid;		/* group */
    U32 sd_size;	/* file size */
    U32 sd_atime;	/* time of last access */
    U32 sd_mtime;	/* time file was last modified  */
    U32 sd_ctime;	/* time inode (stat data) was last changed (except changes to sd_atime and sd_mtime) */
    union {
	U32 sd_rdev;
	U32 sd_blocks;	/* number of blocks file uses */
    } u;
    U32 sd_first_direct_byte; /* first byte of file which is stored
				   in a direct item: except that if it
				   equals 1 it is a symlink and if it
				   equals ~(__u32)0 there is no
				   direct item.  The existence of this
				   field really grates on me. Let's
				   replace it with a macro based on
				   sd_size and our tail suppression
				   policy.  Someday.  -Hans */
} REISERFS_STAT1, *LPREISERFS_STAT1;

typedef struct {
    U16 sd_mode;	/* file type, permissions */
    U16 sd_reserved;
    U32 sd_nlink;	/* number of hard links */
    U64 sd_size;	/* file size */
    U32 sd_uid;		/* owner */
    U32 sd_gid;		/* group */
    U32 sd_atime;	/* time of last access */
    U32 sd_mtime;	/* time file was last modified  */
    U32 sd_ctime;	/* time inode (stat data) was last changed (except changes to sd_atime and sd_mtime) */
    U32 sd_blocks;
    union {
	U32 sd_rdev;
	U32 sd_generation;
      //__u32 sd_first_direct_byte; 
      /* first byte of file which is stored in a
				       direct item: except that if it equals 1
				       it is a symlink and if it equals
				       ~(__u32)0 there is no direct item.  The
				       existence of this field really grates
				       on me. Let's replace it with a macro
				       based on sd_size and our tail
				       suppression policy? */
  } u;
} REISERFS_STAT2, *LPREISERFS_STAT2;


typedef struct { // (Pointer to disk block) Field Name Type Size (in bytes) Description 
U32 dc_block_number; //  ULONG32 4  Disk child's block number. 
U16 dc_size; // unsigned short  2  Disk child's used space. 
U16 dc_reserved;
} REISERFS_DISK_KEY, *LPREISERFS_DISK_KEY;

#pragma pack()


#include "physicaldrive.h"
#include "ifilesystem.h"

class ReiserFsFileData : public PNode
    {
    public:
        ReiserFsFileData( LPBYTE lpbMemory, DWORD dwSize );

        LPBYTE m_lpbMemory;
        DWORD m_dwSize;
    };

class ReiserFsFileInfo : public PNode
    {
    public:
        ReiserFsFileInfo(REISERFS_DIRECTORY_HEAD* pDH, LPCSTR lpszName);
        bool isSymlink()
        {
            return /*(m_stat.u.sd_generation == 1) &&*/ S_ISLNK(m_stat.sd_mode);
        }

        PList m_FileData;
        PString m_strName;
        REISERFS_STAT2 m_stat;
        REISERFS_DIRECTORY_HEAD m_deh; 
    };

class ReiserFsPartition;

typedef void (WINAPI* LPFNReiserFsSearchCallback)(ReiserFsPartition* pPartition, REISERFS_CPU_KEY* lpKey, LPBYTE lpbMemory, INT32 nSize, void* lpContext );

class ReiserFsBlock : public PNode
    {
    public:
        ReiserFsBlock(DWORD dwBlockNumber, LPBYTE lpbMemory)
            :   m_dwBlockNumber( dwBlockNumber ),
                m_lpbMemory( lpbMemory )
        {
        }

        ReiserFsBlock::~ReiserFsBlock()
        {
            delete m_lpbMemory;
        }

        DWORD m_dwBlockNumber;
        LPBYTE m_lpbMemory;
    };

class ReiserFsPartition;

class ReiserFsMetafile
	{
	public:
		ReiserFsMetafile();
		virtual ~ReiserFsMetafile();
		BOOL Open( ReiserFsPartition* pPartition, const char* pszFilename );
		BOOL Read( LPBYTE lpbMemory, DWORD dwSize, INT64 BlockNumber );
		
		INT32 m_iNumberOfIndices;
		LONG_PTR *m_pBlockIndices;
		FILE* m_pDataFile;
		DWORD m_dwBlocksize;
        REISERFS_SUPER_BLOCK m_Superblock;
	};

class CheckedBlock;



class ReiserFsPartition
    {
    public:
        ReiserFsPartition();
        virtual ~ReiserFsPartition();
        virtual bool Open( LONG_PTR iDrive, LONG_PTR iPartition );
        virtual void Autodetect( LONG_PTR iMaxDrive = 10, LPFNFoundPartition lpCallback = NULL, LPVOID lpContext = NULL );
		virtual void AutodetectFirstUsable( LONG_PTR piPartition, LONG_PTR piDrive );
        virtual BOOL Read( LPBYTE lpbMemory, DWORD dwSize, INT64 BlockNumber );
        virtual LPBYTE GetBlock( DWORD BlockNumber );
        virtual void ParseTreeRecursive( BOOL* lpbSuccess, REISERFS_CPU_KEY* lpKeyToFind, INT32 nBlock, LPFNReiserFsSearchCallback lpCallback, void* lpContext );
        virtual bool ListDir( PList* pDirectory, LPSTR lpszDirectory );
        virtual bool GetFile( LPSTR lpszReiserFsName, LPSTR lpszLocalName, bool bRecurseSubdirectories );
        virtual bool GetFileEx( LPSTR lpszReiserFsName, ICreateFileInfo* pCFI );
		bool CopyFilesRecursive( PList* Directory, LPSTR lpszName, bool bRecurseSubdirectories );
		void DumpTree();
		void Backup( const char* pszFilename );
		BOOL PrepareForRestore( const char* pszFilename );
        PString GetFileAsString( ReiserFsFileInfo* pFile );
        bool CheckReiserFsPartition();
		void DumpTreeRecursive(INT32 nBlock,INT32 nIndent);
		void BackupTreeRecursive(FILE* fpData, FILE* fpIndex, INT32 nBlock);
		LPBYTE GetBlockUncached( DWORD BlockNumber );

        PList m_BlockCache;
		DWORD m_dwBlockSize;            
        IPhysicalDrive* m_pDrive;
        INT64 m_PartitionStartingOffset;
        INT64 m_BytesPerSector;
        REISERFS_SUPER_BLOCK m_sb;
		ReiserFsMetafile m_Metafile;

		BOOL IParseTreeRecursive( INT32 nBlock );

		REISERFS_CPU_KEY* m_lpKeyToFind;
		LPFNReiserFsSearchCallback m_lpCallback;
		void* m_lpContext;
		INT32 m_nIndent;
    };

extern void TRACE(const char*, ...);

#ifdef _DEBUG
#define DEBUGTRACE(ARGUMENTS) TRACE ARGUMENTS;
#else
#define DEBUGTRACE(ARGUMENTS)
#endif

#endif // REISER4WIN_H
