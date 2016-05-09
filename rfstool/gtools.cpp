//MPA 5-1-2005
//64bit
#include "precomp.h"
#include "gtools.h"

PList::PList()
{
    m_pHead = m_pTail = 0;
    m_lCount = 0;
}

PList::~PList()
{
    DeleteContents();
} 

bool PList::DeleteContents()
{
    while( m_pHead )
    {
        PNode* pNext = m_pHead->m_pNext;
        m_pHead->m_pNext = 0;
        delete m_pHead;
        m_pHead = pNext;
    }
    m_pHead = m_pTail = 0;
    m_lCount = 0;
    return true;
} 

bool PList::RemoveContents()
{
    while( m_pHead )
    {
        PNode* pNext = m_pHead->m_pNext;
        m_pHead->m_pNext = m_pHead->m_pPrevious = 0;
        m_pHead = pNext;
    }
    m_pHead = m_pTail = 0;
    m_lCount = 0;
    return  true;
} 

bool PList::Contains( PNode* p )
{
    bool bSuccess = false;
    if( p != 0 )
    {
        for( PNode* q = m_pHead; q; q = q->m_pNext )
        {
            if( q == p )
            {
                bSuccess = true;
                break;
            }
        }
    }
    return bSuccess;
}

bool PList::AddHead( PNode* p )
{
    p->m_pPrevious = 0;
    m_lCount++;

    if( !m_pHead )
    {
        m_pHead = m_pTail = p;
        p->m_pNext = 0;
    }
    else
    {
        p->m_pNext = m_pHead;
        m_pHead->m_pPrevious = p;
        m_pHead = p;
    }
    return true;
}

bool PList::AddTail( PNode* p )
{
    if( !m_pTail )
    {
        m_pHead = m_pTail = p;
        p->m_pNext = 0;
        m_lCount = 1;
    }
    else
    {
        m_lCount++;
        p->m_pNext = 0;
        p->m_pPrevious = m_pTail;
        m_pTail->m_pNext = p;
        m_pTail = p;
    }
    return true;
}

bool PList::InsertBefore( PNode* p, PNode* q )
{
    if( !q )
        return AddTail( p );
    
    if( !q->m_pPrevious )
        return AddHead( p );

    ++m_lCount;
    p->m_pPrevious = q->m_pPrevious;
    p->m_pPrevious->m_pNext = p;
    q->m_pPrevious = p;
    p->m_pNext = q;
    return true;
}

bool PList::InsertAfter( PNode* p, PNode* q )
{
    if( !q )
        return AddHead( p );

    if( !q->m_pNext )
        return AddTail( p );

    return InsertBefore( p, q->m_pNext );
}

void PList::Delete( PNode* p )
{
    Remove( p );
    delete p;
}

void PList::Remove( PNode* p )
{
    m_lCount--;

    if( m_pHead == p )
        m_pHead = p->m_pNext;

    if( m_pTail == p )
        m_pTail = p->m_pPrevious;

    if( p->m_pNext )
        p->m_pNext->m_pPrevious = p->m_pPrevious;

    if( p->m_pPrevious )
        p->m_pPrevious->m_pNext = p->m_pNext;

    p->m_pNext = p->m_pPrevious = 0;
}

bool PList::IsEmpty()
{
    return m_pHead ? true : false;
} 

PNode* PList::Find( long lZeroBasedIndex )
{
    long lIndex = 0;
    for( PNode* p = m_pHead; p; p = p->m_pNext )
        if( lIndex++ == lZeroBasedIndex )
            return p;

    return 0;
} 

bool PList::Merge( PList& s )
{
    while( s.m_pHead )
    {
        PNode* pNode = s.m_pHead;
        s.Remove( s.m_pHead );
        AddTail( pNode );
    }
    return true;
}

// reduce if your crap OS cannot handle tit - it ;(
#define BUFFERSIZE_FOR_SPRINTF    10480

PString::PString()
{
    m_iStringLength = 0;
    m_lpszData = 0;
}

inline void PString::CopyStringData(const char* lpszFrom)
{
    if( lpszFrom )
    {
        m_iStringLength = strlen( lpszFrom )+1;
        if( m_iStringLength < sizeof(m_szFixedBuffer) )
        {
            m_lpszData = m_szFixedBuffer;
            strcpy( m_lpszData, lpszFrom );
        }
        else if( m_iStringLength > 0 )
        {
            m_lpszData = new char[ m_iStringLength ];
            if( m_lpszData != 0 )
                strcpy( m_lpszData, lpszFrom );
            else m_iStringLength = 0;
        }
        else m_lpszData = 0;
    }
    else
    {
        m_lpszData = 0;
        m_iStringLength = 0;
    }
}

inline void PString::DeleteStringData()
{
    if( (m_lpszData != m_szFixedBuffer) && m_lpszData )
        delete m_lpszData;
    m_lpszData = 0;
    m_iStringLength = 0;
}


void PString::Append( const char * pString, ULONG32 dwSize )
{
    // *NOT* optimized

    SIZE_T dwCombinedLength = m_iStringLength + dwSize;
    if( dwCombinedLength && dwSize )
    {
        char * lpszData = new char[ dwCombinedLength + 2 ];
        if( m_lpszData && m_iStringLength )
            strcpy( lpszData, m_lpszData );
        strncpy( lpszData+m_iStringLength, pString, dwSize );
        lpszData[dwCombinedLength] = 0;
        DeleteStringData();
        CopyStringData(lpszData);
        delete lpszData;
         
    }
}


PString::PString( const char * lpszArgument )
{
    CopyStringData( lpszArgument );
}

PString::PString( const PString& objectSrc )
{
    CopyStringData( objectSrc.m_lpszData );
}

PString::PString( LONG_PTR, const char * szFormat, ... )
{
    if( szFormat )
    {
        char buffer[BUFFERSIZE_FOR_SPRINTF];
        va_list argptr;
        va_start( argptr, szFormat );
        ::vsprintf(buffer,szFormat,argptr);
        buffer[BUFFERSIZE_FOR_SPRINTF-1]=0;
        CopyStringData( buffer );
    }
    else
    {
        m_iStringLength = 0;
        m_lpszData = 0;
    }
}

PString::~PString()
{
    DeleteStringData();
}

void PString::sprintf( const char * szFormat, ... )
{
    DeleteStringData();
    m_lpszData = 0;
    m_iStringLength = 0;
    if( szFormat )
    {
        char buffer[BUFFERSIZE_FOR_SPRINTF];
        va_list argptr;
        va_start( argptr, szFormat );
        ::vsprintf(buffer,szFormat,argptr);
        buffer[BUFFERSIZE_FOR_SPRINTF-1]=0;
        CopyStringData( buffer );
    }
}

void PString::vsprintf( const char * szFormat, va_list args )
{
    DeleteStringData();
    m_lpszData = 0;
    m_iStringLength = 0;
    if( szFormat )
    {
        char buffer[BUFFERSIZE_FOR_SPRINTF];
        ::vsprintf(buffer,szFormat,args);
        buffer[BUFFERSIZE_FOR_SPRINTF-1]=0;
        CopyStringData( buffer );
    }
} 

PString& PString::operator=( const char * objectSrc )
{
    if( objectSrc != m_lpszData )
    {
        DeleteStringData();
        CopyStringData( objectSrc );
    }
    return *this;
}

PString& PString::operator=( PString& objectSrc )
{
    if( this != &objectSrc )
    {
        DeleteStringData();
        CopyStringData( objectSrc.m_lpszData );
    }
    return *this;
}

PString GetLastErrorString()
{
#ifdef _WIN32
    PString strResult;
    void* lpMsgBuf = NULL;

    if( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        GetLastError(),  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL ) && lpMsgBuf )
    {
        strResult = (LPCSTR) lpMsgBuf;
        LocalFree( lpMsgBuf );
        LPSTR p = strResult;
        while( isspace(p[strlen(p)-1]) )
            p[strlen(p)-1] = 0;
    }
    return strResult;
#else
    return PString(strerror(errno));
#endif
}

BOOL ChangeDirectory( LPCSTR lpszDirectory )
{
    if( !lpszDirectory )
        return FALSE;

#ifndef _WIN32
    if( chdir( lpszDirectory ) < 0 )
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
#else

    // determine if contains a drive letter
    LPCSTR lpszDrive = strchr( lpszDirectory, ':' );
    if( lpszDrive )
    {
        if( _chdrive( toupper(lpszDrive[-1]) - 'A' + 1 ) == -1 )
        {
            return FALSE;
        }
    }
    return SetCurrentDirectory( lpszDirectory );
#endif
} // ChangeDirectory()


PString GetCurrentDirectory()
{
    char szBuffer[1024];
    *szBuffer = 0;
#ifdef _WIN32
    ::GetCurrentDirectory( sizeof(szBuffer), szBuffer );
#elif defined(TARGET_IS_UNIX)
    getcwd (szBuffer,sizeof(szBuffer));
#endif
    return PString( szBuffer );
} // GetCurrentDirectory()

BOOL MakeSurePathExists( LPCSTR lpszFileName )
{
    char szBuffer[1024];
    strcpy( szBuffer, lpszFileName );

    // if a path is given, change the current drive
    PString strCurrentDirectory( GetCurrentDirectory() );

#ifdef _WIN32
    LPSTR lpszPathSeparator = strchr( lpszFileName, ':' );
    if( lpszPathSeparator )
    {
        CHAR szDrive[] = "?:\\";
        szDrive[0] = lpszPathSeparator[-1];

        // fail if unable to change to drive specified
        if( !ChangeDirectory( szDrive ) )
        {
            return FALSE;
        }
        if( lpszPathSeparator[1] == '\\' )
            lpszPathSeparator += 2;
        else
            lpszPathSeparator += 1;

        strcpy( szBuffer, lpszPathSeparator );
    }
    else
    {
        // check if networkpath is given
        if( lpszFileName[0] == '\\' && lpszFileName[1] == '\\')
        {
            // skip Rechnername
            LPSTR pPath = strchr( lpszFileName+2, '\\');
            if( pPath )
            {
                // skip Freigabename
                pPath = strchr( pPath+1, '\\');
                if( pPath)
                {
                    // this is the path we should check
                    strcpy( szBuffer, pPath);

                    // first change drive
                    *pPath = 0;
                    //PTrace( "PMakeSurePathExists sets current network drive %s", lpszFileName );
                    if( !ChangeDirectory( lpszFileName ) )
                    {
                        return FALSE;
                    }
                    *pPath = '\\';
                }
            }
        }
    }
#endif

    LPSTR lpszReadPosition = szBuffer;
    while( *lpszReadPosition )
    {
        CHAR c = *lpszReadPosition;
        if( (c == '\\') || (c == '/') )
        {
            // check if this partial path exists
            *lpszReadPosition = 0;

            if( *szBuffer )
            {
                #ifdef TARGET_IS_WIN32
                if( !CreateDirectory( szBuffer, 0 )
                {
                    DWORD dwLastError = GetLastError();
                    if( dwLastError != ERROR_ALREADY_EXISTS )
                    {
                        ChangeDirectory( strCurrentDirectory );
                        SetLastError(dwLastError);
                        return FALSE;
                    }
                }
                #elif defined(TARGET_IS_UNIX)
                INT32 result = mkdir( szBuffer, S_IRWXU|S_IRWXG|S_IRWXO );
                #endif
            }
            *lpszReadPosition = SLASH_CHAR;
        }
        lpszReadPosition++;
    }
    if( szBuffer[0] )
    {
        #ifdef _WIN32
        if( !CreateDirectory( szBuffer, 0 ) )
        {
            DWORD dwLastError = GetLastError();
            if( dwLastError != ERROR_ALREADY_EXISTS )
            {
                ChangeDirectory( strCurrentDirectory );
                SetLastError(dwLastError);
				return FALSE;
            }
        }
        #else
        INT32 result = mkdir( szBuffer, S_IRWXU|S_IRWXG|S_IRWXO );
        #endif
    }

    ChangeDirectory( strCurrentDirectory );
	#ifdef _WIN32
    SetLastError(S_OK);
	#endif
    return TRUE;
} // PMakeSurePathExists()

