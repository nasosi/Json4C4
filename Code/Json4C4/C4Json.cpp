// Json4C4
// A C++ JSON library
//
// MIT License
//
// Copyright(c) 2024 Athanasios Iliopoulos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// This software uses third party libraries. Their licenses can be found in the
// "Licenses" subfolder of the source code repository.

#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ ) || defined( __NT__ )
#
#    define JSON4C4_WINDOWS
#
#elif ( defined __linux__ ) || ( defined __linux )
#
#    define JSON4C4_LINUX
#
#else
#
#    error "Unsupported Operating System"
#
#endif

#ifdef JSON4C4_WINDOWS

#    include "windows.h"

#elif defined( JSON4C4_LINUX )

#    include <cstdio>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>

#endif

#include "C4Json.h"

// Without system headers, Terathon declares its own SSE types, which clash with the <emmintrin.h> that NumberTextThree
// includes for its SIMD digits.
#if defined( TERATHON_NO_SYSTEM ) && !defined( NUMBERTEXTTHREE_NO_SIMD )

#    define NUMBERTEXTTHREE_NO_SIMD

#endif

#include "NumberTextThree.h"

#if defined( _MSC_VER )

#    pragma warning( push )

#endif

#include "TSText.h"

#if defined( _MSC_VER )

#    pragma warning( pop )
#    pragma warning( disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled

#endif

#ifdef C4_ENGINE_MODULE

#    include "C4Files.h"

#endif

#define JSON4C4VERSIONMAJOR 3
#define JSON4C4VERSIONMINOR 0
#define JSON4C4VERSIONPATCH 1

#define STRN( s ) #s
#define STR( s ) STRN( s )

#define JSON4C4VERSION STR( JSON4C4VERSIONMAJOR ) "." STR( JSON4C4VERSIONMINOR ) "." STR( JSON4C4VERSIONPATCH )

#ifndef JSON4C4_MAX_FILE_SIZE

#    define JSON4C4_MAX_FILE_SIZE 0x7FFFFF00

#endif

static_assert( ( JSON4C4_MAX_FILE_SIZE > 0 ) && ( JSON4C4_MAX_FILE_SIZE <= 0x7FFFFF00 ),
               "JSON4C4_MAX_FILE_SIZE must be positive and leave room for the 32-bit sizes of a text and its strings" );

namespace C4
{

#ifndef C4_ENGINE_MODULE

#    if defined JSON4C4_WINDOWS

    class WCharBuffer
    {
    private:
        wchar_t* buffer = nullptr;

    public:
        WCharBuffer( Terathon::uint32 maxLength )
        {
            wchar_t* c = buffer = new wchar_t[ maxLength + 1 ];
            wchar_t* end        = buffer + maxLength;

            while ( c != end )
            {
                *( c++ ) = 0;
            }

            *end = 0;
        }

        WCharBuffer( const wchar_t* str, Terathon::uint32 maxLength )
        {
            wchar_t* c = buffer = new wchar_t[ maxLength + 1 ];
            wchar_t* end        = buffer + maxLength;

            while ( *str != L'\0' && c != end )
            {
                *( c++ ) = *( str++ );
            }

            *c   = 0;
            *end = 0;
        }

        WCharBuffer( const Terathon::String<>& str )
        {
            using namespace Terathon;

            int32 length = Text::GetWideTextCharCount( str );
            buffer       = new wchar_t[ length + 1 ];

            Text::ConvertStringToWideText( str, reinterpret_cast<uint16*>( buffer ), length );
        }

        ~WCharBuffer()
        {
            if ( buffer != nullptr )
            {
                delete[] buffer;
            }
        }

        operator wchar_t*()
        {
            return buffer;
        }

        operator const wchar_t*() const
        {
            return buffer;
        }

        operator Terathon::String<>()
        {
            using namespace Terathon;

            int32 charArrayLength = Text::GetUnicodeStringLength( reinterpret_cast<uint16*>( buffer ) );

            String<> string;
            string.SetStringLength( charArrayLength );

            Text::ConvertWideTextToString( reinterpret_cast<uint16*>( buffer ), string, charArrayLength );

            return string;
        }
    };

#    endif

    enum FileOpenMode
    {
        kFileReadOnly,
        kFileCreate
    };

    enum FileStatus
    {
        kFileOkay,
        kFileOpenFailed,
        kFileNotOpen,
        kFileWriteError,
        kFileReadError
    };

    class File
    {
    private:
        bool IsReadOnly = false;

#    if defined( JSON4C4_WINDOWS )

        HANDLE fileHandle = nullptr;

#    elif defined( JSON4C4_LINUX )

        int fileDescriptor = -1;

#    endif

    public:
        ~File()
        {
            CloseFile();
        }

        FileStatus OpenFile( const char* fileName, FileOpenMode fileOpenMode )
        {
            CloseFile();

#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideFileName( Terathon::String<> { fileName } );

            if ( fileOpenMode == kFileReadOnly )
            {
                HANDLE handle =
                    CreateFileW( wideFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );

                if ( handle == INVALID_HANDLE_VALUE )
                {
                    return kFileOpenFailed;
                }

                this->fileHandle = handle;
                this->IsReadOnly = true;

                return kFileOkay;
            }

            HANDLE handle = CreateFileW( wideFileName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );

            if ( handle == INVALID_HANDLE_VALUE )
            {
                return kFileOpenFailed;
            }

            this->fileHandle = handle;
            this->IsReadOnly = false;

            return kFileOkay;

#    elif defined( JSON4C4_LINUX )

            if ( fileOpenMode == kFileReadOnly )
            {
                int fd = open( fileName, O_RDONLY );

                if ( fd == -1 )
                {
                    return kFileOpenFailed;
                }

                this->fileDescriptor = fd;
                this->IsReadOnly     = true;

                return kFileOkay;
            }

            int fd = open( fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644 );

            if ( fd == -1 )
            {
                return kFileOpenFailed;
            }

            this->fileDescriptor = fd;
            this->IsReadOnly     = false;

            return kFileOkay;

#    endif
        }

        void CloseFile()
        {
#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle != nullptr )
            {
                CloseHandle( fileHandle );

                fileHandle = nullptr;
            }

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor != -1 )
            {
                close( this->fileDescriptor );

                fileDescriptor = -1;
            }

#    endif
        }

        Terathon::uint64 GetFileSize()
        {

#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle == nullptr )
            {
                return 0;
            }

            FILE_STANDARD_INFO fileInfo { 0 };

            bool result = GetFileInformationByHandleEx( fileHandle, FileStandardInfo, &fileInfo, sizeof( FILE_STANDARD_INFO ) );

            if ( !result )
            {
                return 0;
            }

            return fileInfo.EndOfFile.QuadPart;

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor == -1 )
            {
                return 0;
            }

            struct stat statBuffer;

            int result = fstat( this->fileDescriptor, &statBuffer );

            if ( result != 0 )
            {
                return 0;
            }

            return statBuffer.st_size;

#    endif
        }

        FileStatus ReadFile( char* buffer, Terathon::uint64 size )
        {

#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle == nullptr )
            {
                return kFileNotOpen;
            }

            if ( size > JSON4C4_MAX_FILE_SIZE )
            {
                return kFileReadError;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            char*       p     = buffer;

            while ( p != end )
            {
                DWORD numberOfBytesRead = 0;

                BOOL ok = ::ReadFile( fileHandle, p, DWORD( end - p ), &numberOfBytesRead, nullptr );

                if ( !ok || numberOfBytesRead == 0 )
                {
                    return kFileReadError;
                }

                p += numberOfBytesRead;
            }

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor == -1 )
            {
                return kFileNotOpen;
            }

            if ( size > JSON4C4_MAX_FILE_SIZE )
            {
                return kFileReadError;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            char*       p     = buffer;

            while ( p != end )
            {
                ssize_t numberOfBytesRead = read( this->fileDescriptor, p, size_t( end - p ) );

                if ( numberOfBytesRead <= 0 )
                {
                    return kFileReadError;
                }

                p += numberOfBytesRead;
            }

#    endif

            return kFileOkay;
        }

        static bool CanReplaceFile( const char* fileName )
        {
#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideFileName( Terathon::String<> { fileName } );

            DWORD attributes = GetFileAttributesW( wideFileName );

            return ( attributes == INVALID_FILE_ATTRIBUTES ) || ( ( attributes & ( FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT ) ) == 0 );

#    elif defined( JSON4C4_LINUX )

            struct stat statBuffer;

            if ( lstat( fileName, &statBuffer ) != 0 )
            {
                return true;
            }

            return S_ISREG( statBuffer.st_mode ) && ( statBuffer.st_nlink == 1 );

#    endif
        }

        void CopyPermissionsFrom( const char* fileName )
        {
#    if defined( JSON4C4_LINUX )

            struct stat statBuffer;

            if ( this->fileDescriptor != -1 && stat( fileName, &statBuffer ) == 0 )
            {
                fchmod( this->fileDescriptor, statBuffer.st_mode & 07777 );
            }

#    else

            static_cast<void>( fileName );

#    endif
        }

        static bool MoveOverFile( const char* source, const char* destination )
        {
#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideSource( Terathon::String<> { source } );
            const WCharBuffer wideDestination( Terathon::String<> { destination } );

            if ( GetFileAttributesW( wideDestination ) != INVALID_FILE_ATTRIBUTES )
            {
                return ReplaceFileW(
                           wideDestination, wideSource, nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, nullptr, nullptr ) != 0;
            }

            return MoveFileExW( wideSource, wideDestination, MOVEFILE_WRITE_THROUGH ) != 0;

#    elif defined( JSON4C4_LINUX )

            return rename( source, destination ) == 0;

#    endif
        }

        static void RemoveFile( const char* fileName )
        {
#    if defined( JSON4C4_WINDOWS )

            const WCharBuffer wideFileName( Terathon::String<> { fileName } );

            DeleteFileW( wideFileName );

#    elif defined( JSON4C4_LINUX )

            unlink( fileName );

#    endif
        }

        FileStatus WriteFile( const char* buffer, Terathon::uint64 size )
        {
            if ( IsReadOnly )
            {
                return kFileWriteError;
            }

#    if defined( JSON4C4_WINDOWS )

            if ( fileHandle == nullptr )
            {
                return kFileNotOpen;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            const char* p     = begin;

            while ( p != end )
            {
                constexpr Terathon::uint64 maxChunkSize = 0x40000000;

                Terathon::uint64 remaining            = Terathon::uint64( end - p );
                DWORD            numberOfBytesWritten = 0;

                BOOL ok = ::WriteFile( fileHandle, p, DWORD( remaining < maxChunkSize ? remaining : maxChunkSize ), &numberOfBytesWritten, nullptr );

                if ( !ok || numberOfBytesWritten == 0 )
                {
                    return kFileWriteError;
                }

                p += numberOfBytesWritten;
            }

#    elif defined( JSON4C4_LINUX )

            if ( this->fileDescriptor == -1 )
            {
                return kFileNotOpen;
            }

            const char* begin = buffer;
            const char* end   = begin + size;
            const char* p     = begin;

            while ( p != end )
            {
                ssize_t numberOfBytesWritten = write( this->fileDescriptor, p, size_t( end - p ) );

                if ( numberOfBytesWritten <= 0 )
                {
                    return kFileWriteError;
                }

                p += numberOfBytesWritten;
            }

#    endif

            return kFileOkay;
        }
    };

#endif

    namespace Json
    {
        const char* GetVersion()
        {
            return JSON4C4VERSION;
        }

        static const char* const StatusString[] = { "Ok",
                                                    "Could not open file",
                                                    "Could not read file",
                                                    "File is too large",
                                                    "Invalid JSON text",
                                                    "Unterminated text buffer",
                                                    "Expected opening curly braces",
                                                    "Expected closing curly braces or comma",
                                                    "Expected opening square brackets",
                                                    "Expected closing square brackets or comma",
                                                    "Illegal character",
                                                    "Illegal escape",
                                                    "Expected colon",
                                                    "Expected beginning double quotes",
                                                    "Expected ending double quotes",
                                                    "Invalid number",
                                                    "Expected 'true' or 'false'",
                                                    "Expected 'null'",
                                                    "Expected a JSON value (object, array, string, number, 'true', 'false', or 'null')",
                                                    "Expected end of file",
                                                    "File ends prematurely",
                                                    "Numbers cannot have leading zeros",
                                                    "JSON structure is invalid",
                                                    "An exception was caught",
                                                    "JSON structured data contains invalid value type",
                                                    "Could not find requested name of name/value pair",
                                                    "Maximum nesting depth exceeded",
                                                    "Duplicate name in object",
                                                    "Could not write file",
                                                    "Maximum value count exceeded",
                                                    "Maximum string byte count exceeded",
                                                    "A block source refused an allocation",
                                                    "String holds a zero character",
                                                    "Array index out of range",
                                                    "Edit handle taken before the document changed",
                                                    "Document is being built" };

        constexpr unsigned int statusStringCount = sizeof( StatusString ) / sizeof( StatusString[ 0 ] );

        static_assert( statusStringCount == (unsigned int)( Status::kBuildInProgress ) + 1, "StatusString must have one entry per Status" );

        TERATHON_API String<256> StatusToString( const Status& status ) noexcept
        {
            if ( (unsigned int)( status ) >= statusStringCount )
            {
                return "Unknown status";
            }

            return StatusString[ (unsigned int)( status ) ];
        }

        alignas( 64 ) const int8 hexadecimalCharValue[ 55 ] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, 10, 11,
                                                                12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15 };

        constexpr uint8 space          = 0x20;
        constexpr uint8 tab            = 0x09;
        constexpr uint8 newLine        = 0x0A;
        constexpr uint8 carriageReturn = 0x0D;

        constexpr double maxDouble = 1.7976931348623157e308;

        Status ExpandArray( Array<char>& array, const char c, const uint32 n )
        {
            int32 count = array.GetArrayElementCount();

            if ( n > uint32( JSON4C4_MAX_FILE_SIZE - count ) )
            {
                return Status::kFileTooLarge;
            }

            array.SetArrayElementCount( count + int32( n ), c );

            return Status::kOk;
        }

        inline void CopyText( char* restrict destination, const char* restrict source, int32 length ) noexcept
        {
            for ( int32 a = 0; a < length; a++ )
            {
                destination[ a ] = source[ a ];
            }
        }

        Status AppendText( Array<char>& output, const char* text, uint32 length )
        {
            int32 count = output.GetArrayElementCount();

            if ( uint64( count ) + length > JSON4C4_MAX_FILE_SIZE )
            {
                return Status::kFileTooLarge;
            }

            output.SetArrayElementCount( count + int32( length ) );
            CopyText( output.begin() + count, text, int32( length ) );

            return Status::kOk;
        }

        Status AppendText( Array<char>& output, const char* text )
        {
            return AppendText( output, text, uint32( Text::GetTextLength( text ) ) );
        }

        inline bool IsContinuationByte( uint32 c ) noexcept
        {
            return ( c & 0xC0U ) == 0x80U;
        }

        // The length of the character at byte when a string literal holds it unescaped, or 0 when it has to be escaped or is
        // not valid UTF-8.
        inline int32 GetPlainCharLength( const uint8* byte ) noexcept
        {
            uint32 c = byte[ 0 ];

            if ( c < 0x80U )
            {
                return ( ( c >= 0x20U ) && ( c != '"' ) && ( c != '\\' ) );
            }

            if ( ( c - 0xC2U < 0x33U ) && IsContinuationByte( byte[ 1 ] ) )
            {
                if ( c < 0xE0U )
                {
                    return 2;
                }

                if ( IsContinuationByte( byte[ 2 ] ) )
                {
                    if ( c < 0xF0U )
                    {
                        uint32 code = ( ( c & 0x0FU ) << 12 ) | ( ( byte[ 1 ] & 0x3FU ) << 6 ) | ( byte[ 2 ] & 0x3FU );

                        return ( ( code >= 0x0800U ) && ( code - 0xD800U >= 0x0800U ) ) ? 3 : 0;
                    }

                    if ( IsContinuationByte( byte[ 3 ] ) )
                    {
                        uint32 code = ( ( c & 0x07U ) << 18 ) | ( ( byte[ 1 ] & 0x3FU ) << 12 ) | ( ( byte[ 2 ] & 0x3FU ) << 6 ) | ( byte[ 3 ] & 0x3FU );

                        return ( code - 0x010000U < 0x100000U ) ? 4 : 0;
                    }
                }
            }

            return 0;
        }

        // Whether eight bytes are all ASCII characters that a string literal holds unescaped. The top bit of a byte is set
        // by subtracting 0x20 from a control character, or 1 from a quote or a backslash once the bytes are XORed with
        // it, and a byte from 0x80 up has it already. Other bytes subtract without a borrow, so a borrow only changes
        // the bytes after one that is found.
        inline bool IsPlainAscii( uint64 bytes ) noexcept
        {
            constexpr uint64 ones = 0x0101010101010101;

            uint64 quotes      = bytes ^ ( ones * '"' );
            uint64 backslashes = bytes ^ ( ones * '\\' );

            return ( ( ( bytes - ones * 0x20 ) | ( quotes - ones ) | ( backslashes - ones ) | bytes ) & ( ones * 0x80 ) ) == 0;
        }

        // A character that may run into the end of the text is read from a copy followed by zeros.
        inline int32 GetPlainCharLength( const uint8* byte, const uint8* end ) noexcept
        {
            if ( ( end - byte >= 4 ) || ( ( byte != end ) && ( byte[ 0 ] < 0x80U ) ) )
            {
                return GetPlainCharLength( byte );
            }

            char tail[ 4 ] = {};
            CopyText( tail, reinterpret_cast<const char*>( byte ), int32( end - byte ) );

            return GetPlainCharLength( reinterpret_cast<const uint8*>( tail ) );
        }

        Status AppendEscapedString( Array<char>& output, const char* text, uint32 length )
        {
            static const char hexadecimalDigit[] = "0123456789abcdef";

            const uint8* byte      = reinterpret_cast<const uint8*>( text );
            const uint8* end       = byte + length;
            const uint8* plainStop = ( length >= 4 ) ? end - 3 : byte;
            Status       status    = AppendText( output, "\"", 1 );

            while ( status == Status::kOk )
            {
                const uint8* run = byte;

                while ( byte < plainStop )
                {
                    int32 charLength = GetPlainCharLength( byte );
                    if ( charLength == 0 )
                    {
                        break;
                    }

                    byte += charLength;
                }

                while ( int32 charLength = GetPlainCharLength( byte, end ) )
                {
                    byte += charLength;
                }

                status = AppendText( output, reinterpret_cast<const char*>( run ), uint32( byte - run ) );

                if ( ( status != Status::kOk ) || ( byte == end ) )
                {
                    break;
                }

                uint8 c = byte[ 0 ];

                if ( c >= 0x80U )
                {
                    return Status::kStringIllegalChar;
                }

                char  escape[ 6 ]  = { '\\', char( c ), 0, 0, 0, 0 };
                int32 escapeLength = 2;

                if ( c == '\b' )
                {
                    escape[ 1 ] = 'b';
                }
                else if ( c == '\f' )
                {
                    escape[ 1 ] = 'f';
                }
                else if ( c == '\n' )
                {
                    escape[ 1 ] = 'n';
                }
                else if ( c == '\r' )
                {
                    escape[ 1 ] = 'r';
                }
                else if ( c == '\t' )
                {
                    escape[ 1 ] = 't';
                }
                else if ( c < 0x20 )
                {
                    escape[ 1 ]  = 'u';
                    escape[ 2 ]  = '0';
                    escape[ 3 ]  = '0';
                    escape[ 4 ]  = hexadecimalDigit[ c >> 4 ];
                    escape[ 5 ]  = hexadecimalDigit[ c & 0x0F ];
                    escapeLength = 6;
                }

                status = AppendText( output, escape, escapeLength );
                byte++;
            }

            return ( status == Status::kOk ) ? AppendText( output, "\"", 1 ) : status;
        }

        Status AppendEscapedString( Array<char>& output, const char* text )
        {
            return AppendEscapedString( output, text, uint32( Text::GetTextLength( text ) ) );
        }

        inline bool IsWhitespace( uint32 c ) noexcept
        {
            return ( c == space ) || ( c == tab ) || ( c == newLine ) || ( c == carriageReturn );
        }

        inline uint64 ReadEightBytes( const void* bytes ) noexcept
        {
            uint64 word;
            memcpy( &word, bytes, 8 );

            return word;
        }

        constexpr uint64 eightSpaces = 0x2020202020202020;

        inline bool IsDigit( uint32 c ) noexcept
        {
            return ( c - '0' ) < 10U;
        }

        int32 ReadEscapeChar( const char* text, uint32* value ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );
            uint32       c    = byte[ 0 ];

            if ( ( c == '\"' ) || ( c == '\\' ) || ( c == '/' ) )
            {
                *value = c;
                return ( 1 );
            }
            else if ( c == 'b' )
            {
                *value = '\b';
                return ( 1 );
            }
            else if ( c == 'f' )
            {
                *value = '\f';
                return ( 1 );
            }
            else if ( c == 'n' )
            {
                *value = '\n';
                return ( 1 );
            }
            else if ( c == 'r' )
            {
                *value = '\r';
                return ( 1 );
            }
            else if ( c == 't' )
            {
                *value = '\t';
                return ( 1 );
            }

            return ( 0 );
        }

        bool ReadHexadecimalCode( const uint8* byte, uint32* code ) noexcept
        {
            uint32 value = 0;

            for ( machine a = 0; a < 4; a++ )
            {
                uint32 c = byte[ a ] - '0';
                if ( c >= 55U )
                {
                    return ( false );
                }

                int32 x = hexadecimalCharValue[ c ];
                if ( x < 0 )
                {
                    return ( false );
                }

                value = ( value << 4 ) | uint32( x );
            }

            *code = value;

            return ( true );
        }

        int32 ReadStringEscapeChar( const char* text, int32* stringLength, char* restrict string, bool zeroAllowed ) noexcept
        {
            const uint8* byte = reinterpret_cast<const uint8*>( text );

            if ( byte[ 0 ] == 'u' )
            {
                uint32 code = 0;

                if ( !ReadHexadecimalCode( byte + 1, &code ) || ( ( code == 0 ) && !zeroAllowed ) || ( code - 0xDC00U < 0x0400U ) )
                {
                    return ( 0 );
                }

                int32 textLength = 5;

                if ( code - 0xD800U < 0x0400U )
                {
                    uint32 lowCode = 0;

                    if ( ( byte[ 5 ] != '\\' ) || ( byte[ 6 ] != 'u' ) || !ReadHexadecimalCode( byte + 7, &lowCode ) || ( lowCode - 0xDC00U >= 0x0400U ) )
                    {
                        return ( 0 );
                    }

                    code       = 0x010000 + ( ( code - 0xD800 ) << 10 ) + ( lowCode - 0xDC00 );
                    textLength = 11;
                }

                if ( string )
                {
                    *stringLength = Text::WriteUnicodeChar( string, code );
                }
                else
                {
                    *stringLength = 1 + ( code >= 0x000080 ) + ( code >= 0x000800 ) + ( code >= 0x010000 );
                }

                return ( textLength );
            }

            uint32 value;

            int32 textLength = ReadEscapeChar( text, &value );
            if ( textLength != 0 )
            {
                if ( string )
                {
                    *string = char( value );
                }

                *stringLength = 1;
            }

            return ( textLength );
        }

        // Objects with more members than this find a name through an index of member positions sorted by name, smaller
        // ones by comparing every name.
        constexpr int32 indexedMemberCount = 16;

        // How far back from the end the parser looks for a character that ends every string run.
        constexpr int32 stringStopSearchLength = 4096;

        // Reads the tokens of a text, each read bounded by the end of the text, and counts values and string bytes against
        // the limits of the options.
        class TextReader
        {
        public:
            // Runs of whitespace and of string characters test the end of the text only once. Everything from spaceStop
            // on is whitespace, so a whitespace run that starts before it stops there at the latest. stringStop follows a
            // quote, a backslash or a control character, none of which a string run passes or a UTF-8 character
            // spans, so a string run that starts before it stops there at the latest.
            TextReader( const char* begin, const char* textEnd, const ParseOptions& parseOptions, bool zeroCharsAllowed ) noexcept :
                end( textEnd ),
                spaceStop( textEnd ),
                stringStop( begin ),
                options( parseOptions ),
                valuesLeft( parseOptions.maxValueCount ),
                stringBytesLeft( parseOptions.maxStringByteCount ),
                zeroAllowed( zeroCharsAllowed )
            {
                while ( ( spaceStop != begin ) && IsWhitespace( uint8( spaceStop[ -1 ] ) ) )
                {
                    spaceStop--;
                }

                const char* searchStart = ( end - begin > stringStopSearchLength ) ? end - stringStopSearchLength : begin;

                for ( const char* c = end; c != searchStart; c-- )
                {
                    uint32 byte = uint8( c[ -1 ] );

                    if ( ( byte == '"' ) || ( byte == '\\' ) || ( byte < 0x20U ) )
                    {
                        stringStop = c;
                        break;
                    }
                }
            }

            const char* SkipWhitespace( const char* text ) const noexcept
            {
                if ( text >= spaceStop )
                {
                    return end;
                }

                while ( IsWhitespace( uint8( text[ 0 ] ) ) )
                {
                    text++;

                    while ( ( spaceStop - text >= 8 ) && ( ReadEightBytes( text ) == eightSpaces ) )
                    {
                        text += 8;
                    }
                }

                return text;
            }

        protected:
            const char*         end;
            const char*         spaceStop;
            const char*         stringStop;
            const ParseOptions& options;
            int64               valuesLeft;
            int64               stringBytesLeft;
            bool                zeroAllowed;

            uint32 Peek( const char* text ) const noexcept
            {
                return ( text != end ) ? uint8( text[ 0 ] ) : 0;
            }

            Status CountValue() noexcept
            {
                if ( valuesLeft <= 0 )
                {
                    return Status::kMaximumValueCountExceeded;
                }

                valuesLeft--;

                return Status::kOk;
            }

            Status CountStringBytes( int32 stringLength ) noexcept
            {
                if ( stringLength > stringBytesLeft )
                {
                    return Status::kMaximumStringByteCountExceeded;
                }

                stringBytesLeft -= stringLength;

                return Status::kOk;
            }

            Status ReadStringLiteral( const char* text, int32* textLength, int32* stringLength, char* restrict string = nullptr ) const noexcept
            {
                const uint8* byte    = reinterpret_cast<const uint8*>( text );
                const uint8* byteEnd = reinterpret_cast<const uint8*>( end );
                const uint8* stop    = reinterpret_cast<const uint8*>( stringStop );
                int32        count   = 0;
                Status       status  = Status::kOk;

                for ( ;; )
                {
                    const uint8* run = byte;

                    if ( byte < stop )
                    {
                        while ( ( stop - byte >= 8 ) && IsPlainAscii( ReadEightBytes( byte ) ) )
                        {
                            byte += 8;
                        }

                        while ( int32 length = GetPlainCharLength( byte ) )
                        {
                            byte += length;
                        }
                    }
                    else
                    {
                        while ( int32 length = GetPlainCharLength( byte, byteEnd ) )
                        {
                            byte += length;
                        }
                    }

                    int32 runLength = int32( byte - run );

                    if ( string )
                    {
                        CopyText( string, reinterpret_cast<const char*>( run ), runLength );
                        string += runLength;
                    }

                    count += runLength;

                    uint32 c = ( byte != byteEnd ) ? byte[ 0 ] : 0;

                    if ( c != '\\' )
                    {
                        if ( c != '\"' )
                        {
                            status = ( c == 0 ) ? Status::kPrematureNullTerminator : Status::kStringIllegalChar;
                        }

                        break;
                    }

                    // The longest escape, a surrogate pair, is 11 characters after the backslash.
                    const char* escape = reinterpret_cast<const char*>( byte + 1 );
                    char        tail[ 11 ];

                    if ( byteEnd - byte < 12 )
                    {
                        int32 left = int32( byteEnd - byte - 1 );

                        for ( int32 a = 0; a < 11; a++ )
                        {
                            tail[ a ] = ( a < left ) ? escape[ a ] : 0;
                        }

                        escape = tail;
                    }

                    int32 stringLen;

                    int32 textLen = ReadStringEscapeChar( escape, &stringLen, string, zeroAllowed );
                    if ( textLen == 0 )
                    {
                        status = Status::kStringIllegalEscape;
                        break;
                    }

                    if ( string )
                    {
                        string += stringLen;
                    }

                    byte += textLen + 1;
                    count += stringLen;
                }

                *textLength   = int32( reinterpret_cast<const char*>( byte ) - text );
                *stringLength = count;

                return ( status );
            }

            Status ReadNumber( double& number, const char*& text ) const noexcept
            {
                const char* stop;

                // A number out of range keeps the value NumberTextThree gives it, and Write refuses an infinity. The only
                // invalid number NumberTextThree stops at a digit is one with a leading zero.
                if ( ntt_parse_json_f64( text, end, &number, &stop ) == NUMBERTEXTTHREE_INVALID )
                {
                    return IsDigit( Peek( stop ) ) ? Status::kNumbersCannotHaveLeadingZeros : Status::kFloatInvalid;
                }

                text = stop;

                return Status::kOk;
            }

            Status ReadBool( bool& value, const char*& text ) const noexcept
            {
                if ( end - text >= 4 && text[ 0 ] == 't' && text[ 1 ] == 'r' && text[ 2 ] == 'u' && text[ 3 ] == 'e' )
                {
                    value = true;
                    text += 4;

                    return Status::kOk;
                }

                if ( end - text >= 5 && text[ 0 ] == 'f' && text[ 1 ] == 'a' && text[ 2 ] == 'l' && text[ 3 ] == 's' && text[ 4 ] == 'e' )
                {
                    value = false;
                    text += 5;

                    return Status::kOk;
                }

                return Status::kExpectedTrueOrFalse;
            }

            Status ReadNull( const char*& text ) const noexcept
            {
                if ( end - text >= 4 && text[ 0 ] == 'n' && text[ 1 ] == 'u' && text[ 2 ] == 'l' && text[ 3 ] == 'l' )
                {
                    text += 4;

                    return Status::kOk;
                }

                return Status::kExpectedNull;
            }
        };

        void FindLineAndColumn( const char* begin, const char* end, const char* position, int32* line, int32* column ) noexcept
        {
            int32       lineNumber = 1;
            const char* lineStart  = begin;

            for ( const char* c = begin; c != position; c++ )
            {
                if ( ( c[ 0 ] == '\n' ) || ( ( c[ 0 ] == '\r' ) && ( ( c + 1 == end ) || ( c[ 1 ] != '\n' ) ) ) )
                {
                    lineNumber++;
                    lineStart = c + 1;
                }
            }

            *line   = lineNumber;
            *column = int32( position - lineStart ) + 1;
        }

        template <class ParserType, class RootType>
        Status ParseRoot( ParserType& parser, RootType& root, const char* begin, const char* end, int32* errorLine, int32* errorColumn ) noexcept
        {
            const char* text   = parser.SkipWhitespace( begin );
            Status      status = parser.ParseValue( root, text, 0 );

            if ( status == Status::kOk )
            {
                text = parser.SkipWhitespace( text );

                if ( text != end )
                {
                    status = Status::kExpectedEndOfFile;
                }
            }
            else if ( status == Status::kPrematureNullTerminator && text != end )
            {
                status = Status::kStringIllegalChar;
            }

            if ( status != Status::kOk )
            {
                FindLineAndColumn( begin, end, text, errorLine, errorColumn );
            }

            return status;
        }

        Status WriteTextFile( const char* fileName, const Array<char>& output ) noexcept
        {
#ifdef C4_ENGINE_MODULE

            File file;
            if ( file.OpenFile( fileName, kFileCreate ) != kFileOkay )
            {
                return Status::kFileOpenError;
            }

            if ( file.WriteFile( output.begin(), output.GetArrayElementCount() ) != kFileOkay )
            {
                return Status::kFileWriteError;
            }

            file.CloseFile();

            return Status::kOk;

#else

            String<>   temporaryName;
            File       file;
            FileStatus fileStatus = kFileOpenFailed;

            Status status = Detail::MayThrow(
                [ & ]()
                {
                    if ( File::CanReplaceFile( fileName ) )
                    {
                        String<> name( fileName );
                        name += ".json4c4.tmp";

                        temporaryName = static_cast<String<>&&>( name );
                        fileStatus    = file.OpenFile( temporaryName, kFileCreate );
                    }

                    if ( fileStatus != kFileOkay )
                    {
                        temporaryName = "";
                        fileStatus    = file.OpenFile( fileName, kFileCreate );
                    }

                    return Status::kOk;
                } );

            if ( status != Status::kOk || fileStatus != kFileOkay )
            {
                return ( status != Status::kOk ) ? status : Status::kFileOpenError;
            }

            bool written = ( file.WriteFile( output.begin(), output.GetArrayElementCount() ) == kFileOkay );

            if ( temporaryName.GetStringLength() == 0 )
            {
                file.CloseFile();

                return written ? Status::kOk : Status::kFileWriteError;
            }

            file.CopyPermissionsFrom( fileName );
            file.CloseFile();

            bool replaced = false;

            Detail::MayThrow(
                [ & ]()
                {
                    replaced = written && File::MoveOverFile( temporaryName, fileName );

                    if ( !replaced )
                    {
                        File::RemoveFile( temporaryName );
                    }

                    return Status::kOk;
                } );

            return replaced ? Status::kOk : Status::kFileWriteError;

#endif
        }

        String<256> ParseResultToString( const ParseResult& parseResult ) noexcept
        {
            String<256> string = StatusToString( parseResult.status );

            if ( parseResult.errorLine != 0 && parseResult.status != Status::kException )
            {
                string += ". Line: ";
                string += String<16>( parseResult.errorLine );
                string += ", column: ";
                string += String<16>( parseResult.errorColumn );
            }

            return string;
        }

        static_assert( sizeof( Member ) == 32, "A member takes 32 bytes" );

        class DefaultBlockSource final : public BlockSource
        {
        public:
            void* AllocateBlock( uint64 size ) noexcept override
            {
                uint64* block = nullptr;

                if ( size <= ( machine_address( -1 ) >> 1 ) )
                {
                    Detail::MayThrow(
                        [ & ]()
                        {
                            block = new uint64[ machine( size / 8 ) ];
                            return Status::kOk;
                        } );
                }

                return block;
            }

            void FreeBlock( void* block, uint64 ) noexcept override
            {
                delete[] static_cast<uint64*>( block );
            }
        };

        DefaultBlockSource defaultBlockSource;

        BlockSource& GetDefaultBlockSource() noexcept
        {
            return defaultBlockSource;
        }

        constexpr uint64 smallestBlockSize       = 1024;
        constexpr uint64 largestGrowingBlockSize = 0x10000000;
        constexpr uint64 smallestStackSize       = 4096;

        inline uint64 RoundUpToEight( uint64 size ) noexcept
        {
            return ( size + 7 ) & ~uint64( 7 );
        }

        // Copies a count of bytes that is a multiple of 8 between blocks aligned to 8 bytes.
        inline void CopyWords( void* restrict destination, const void* restrict source, uint64 byteCount ) noexcept
        {
            uint64*       to   = static_cast<uint64*>( destination );
            const uint64* from = static_cast<const uint64*>( source );

            for ( uint64 a = 0; a < byteCount / 8; a++ )
            {
                to[ a ] = from[ a ];
            }
        }

        inline bool SameName( const char* first, uint32 firstLength, const char* second, uint32 secondLength ) noexcept
        {
            if ( firstLength != secondLength )
            {
                return false;
            }

            for ( uint32 a = 0; a < firstLength; a++ )
            {
                if ( first[ a ] != second[ a ] )
                {
                    return false;
                }
            }

            return true;
        }

        inline bool SameName( const StringView& first, const StringView& second ) noexcept
        {
            return SameName( first.GetText(), first.GetLength(), second.GetText(), second.GetLength() );
        }

        // Whether the text of a number, which ends at stop, has neither a fraction nor an exponent.
        inline bool IsIntegerText( const char* text, const char* stop ) noexcept
        {
            text += ( text != stop ) && ( text[ 0 ] == '-' );

            while ( ( text != stop ) && IsDigit( uint8( text[ 0 ] ) ) )
            {
                text++;
            }

            return text == stop;
        }

        // Orders names by their bytes, read as unsigned values, and a name before the longer names it begins.
        int32 CompareNames( const char* first, uint32 firstLength, const char* second, uint32 secondLength ) noexcept
        {
            const uint8* a      = reinterpret_cast<const uint8*>( first );
            const uint8* b      = reinterpret_cast<const uint8*>( second );
            uint32       length = ( firstLength < secondLength ) ? firstLength : secondLength;

            for ( uint32 i = 0; i < length; i++ )
            {
                if ( a[ i ] != b[ i ] )
                {
                    return int32( a[ i ] ) - int32( b[ i ] );
                }
            }

            return ( firstLength < secondLength ) ? -1 : int32( firstLength > secondLength );
        }

        bool StringView::operator==( const StringView& other ) const noexcept
        {
            if ( !text || !other.text )
            {
                return !text && !other.text;
            }

            return SameName( *this, other );
        }

        bool StringView::operator==( const char* other ) const noexcept
        {
            if ( !text || !other )
            {
                return !text && !other;
            }

            for ( uint32 a = 0; a < length; a++ )
            {
                if ( ( other[ a ] == 0 ) || ( other[ a ] != text[ a ] ) )
                {
                    return false;
                }
            }

            return other[ length ] == 0;
        }

        // The place in the index of a large object where a name is, or would go.
        uint32 FindIndexSlot( const Member* members, const uint32* index, uint32 count, const char* name, uint32 length ) noexcept
        {
            uint32 low  = 0;
            uint32 high = count;

            while ( low < high )
            {
                uint32            middle     = low + ( high - low ) / 2;
                const StringView& memberName = members[ index[ middle ] ].GetName();

                if ( CompareNames( memberName.GetText(), memberName.GetLength(), name, length ) < 0 )
                {
                    low = middle + 1;
                }
                else
                {
                    high = middle;
                }
            }

            return low;
        }

        const Value* ObjectView::Find( const char* name, uint32 length ) const noexcept
        {
            if ( !object || !name )
            {
                return nullptr;
            }

            const Member* members = object->data.members;
            uint32        count   = object->count;

            if ( count <= uint32( indexedMemberCount ) )
            {
                for ( uint32 a = 0; a < count; a++ )
                {
                    if ( SameName( members[ a ].name.GetText(), members[ a ].name.GetLength(), name, length ) )
                    {
                        return &members[ a ].value;
                    }
                }

                return nullptr;
            }

            const uint32* index = reinterpret_cast<const uint32*>( members + object->GetCapacity() );
            uint32        slot  = FindIndexSlot( members, index, count, name, length );

            if ( slot == count )
            {
                return nullptr;
            }

            const Member& member = members[ index[ slot ] ];

            return SameName( member.name.GetText(), member.name.GetLength(), name, length ) ? &member.value : nullptr;
        }

        const Value* ObjectView::Find( const char* name ) const noexcept
        {
            return name ? Find( name, uint32( Text::GetTextLength( name ) ) ) : nullptr;
        }

        StructuredData::StructuredData( BlockSource& source ) noexcept : blockSource( &source ), nextBlockSize( smallestBlockSize )
        {
        }

        StructuredData::~StructuredData() noexcept
        {
            Clear();
        }

        // A Serialize that clears the document it is building leaves it as it is, so that the build keeps its blocks.
        void StructuredData::Clear() noexcept
        {
            if ( !builder )
            {
                FreeBlocks( TakeBlocks() );
            }
        }

        // Empties the document and gives its blocks to the caller, to free once nothing reads them.
        StructuredData::Block* StructuredData::TakeBlocks() noexcept
        {
            Block* taken = blocks;

            blocks        = nullptr;
            blockTop      = nullptr;
            blockEnd      = nullptr;
            nextBlockSize = smallestBlockSize;
            root          = nullptr;
            editCount++;

            return taken;
        }

        void StructuredData::FreeBlocks( Block* block ) noexcept
        {
            while ( block )
            {
                Block* next = block->next;
                blockSource->FreeBlock( block, block->size );
                block = next;
            }
        }

        // Whether an address lies in one of the document's blocks.
        bool StructuredData::Owns( const void* address ) const noexcept
        {
            machine_address at = reinterpret_cast<machine_address>( address );

            for ( const Block* block = blocks; block; block = block->next )
            {
                machine_address start = reinterpret_cast<machine_address>( block + 1 );
                machine_address end   = reinterpret_cast<machine_address>( block ) + machine_address( block->size );

                if ( ( at >= start ) && ( at < end ) )
                {
                    return true;
                }
            }

            return false;
        }

        // Sizes are multiples of 8, and values, arrays and members are taken from the bottom of the free space of the
        // current block, so they stay aligned, and string bytes from its top.
        void* StructuredData::AllocateAligned( uint64 size ) noexcept
        {
            if ( uint64( blockEnd - blockTop ) >= size )
            {
                char* memory = blockTop;
                blockTop += size;

                return memory;
            }

            return AllocateInNewBlock( size, true );
        }

        char* StructuredData::AllocateBytes( uint64 size ) noexcept
        {
            if ( uint64( blockEnd - blockTop ) >= size )
            {
                blockEnd -= size;

                return blockEnd;
            }

            return static_cast<char*>( AllocateInNewBlock( size, false ) );
        }

        // A request larger than half the next block gets a block of its own, and the current block keeps its free
        // space. When the source refuses a block, smaller ones are asked for, and blocks stop growing.
        void* StructuredData::AllocateInNewBlock( uint64 size, bool aligned ) noexcept
        {
            uint64 ownSize   = sizeof( Block ) + RoundUpToEight( size );
            uint64 blockSize = ( ownSize > nextBlockSize / 2 ) ? ownSize : nextBlockSize;
            Block* block     = static_cast<Block*>( blockSource->AllocateBlock( blockSize ) );
            bool   refused   = !block;

            while ( !block && ( blockSize > ownSize ) )
            {
                blockSize = ( blockSize / 2 > ownSize ) ? RoundUpToEight( blockSize / 2 ) : ownSize;
                block     = static_cast<Block*>( blockSource->AllocateBlock( blockSize ) );
            }

            if ( !block )
            {
                return nullptr;
            }

            block->next = blocks;
            block->size = blockSize;
            blocks      = block;

            char* memory = reinterpret_cast<char*>( block + 1 );

            if ( refused )
            {
                nextBlockSize = ( blockSize > smallestBlockSize ) ? blockSize : smallestBlockSize;
            }

            if ( blockSize == ownSize )
            {
                return memory;
            }

            blockTop = memory;
            blockEnd = reinterpret_cast<char*>( block ) + blockSize;

            if ( !refused && ( nextBlockSize < largestGrowingBlockSize ) )
            {
                nextBlockSize *= 2;
            }

            return aligned ? AllocateAligned( size ) : AllocateBytes( size );
        }

        // The parser's stack of the elements and members of the arrays and objects being read, in blocks from the
        // document's block source, handed back when the parse ends. Everything on it is a multiple of 8 bytes.
        class ScratchStack
        {
        public:
            explicit ScratchStack( BlockSource& source ) noexcept : blockSource( source )
            {
            }

            ~ScratchStack() noexcept
            {
                if ( base )
                {
                    blockSource.FreeBlock( base, capacity );
                }
            }

            ScratchStack( const ScratchStack& )            = delete;
            ScratchStack& operator=( const ScratchStack& ) = delete;

            uint64 GetSize() const noexcept
            {
                return size;
            }

            void SetSize( uint64 stackSize ) noexcept
            {
                size = stackSize;
            }

            template <class T>
            T* At( uint64 offset ) noexcept
            {
                return reinterpret_cast<T*>( base + offset );
            }

            bool Reserve( uint64 extra ) noexcept
            {
                if ( capacity - size >= extra )
                {
                    return true;
                }

                uint64 needed = RoundUpToEight( size + extra );
                uint64 grown  = ( capacity != 0 ) ? capacity * 2 : smallestStackSize;

                while ( grown < needed )
                {
                    grown *= 2;
                }

                char* block = static_cast<char*>( blockSource.AllocateBlock( grown ) );

                while ( !block && ( grown > needed ) )
                {
                    grown = ( grown / 2 > needed ) ? RoundUpToEight( grown / 2 ) : needed;
                    block = static_cast<char*>( blockSource.AllocateBlock( grown ) );
                }

                if ( !block )
                {
                    return false;
                }

                if ( base )
                {
                    CopyWords( block, base, size );
                    blockSource.FreeBlock( base, capacity );
                }

                base     = block;
                capacity = grown;

                return true;
            }

            template <class T>
            bool Push( const T& item ) noexcept
            {
                if ( !Reserve( sizeof( T ) ) )
                {
                    return false;
                }

                *reinterpret_cast<T*>( base + size ) = item;
                size += sizeof( T );

                return true;
            }

        private:
            BlockSource& blockSource;
            char*        base     = nullptr;
            uint64       capacity = 0;
            uint64       size     = 0;
        };

        struct PendingMember
        {
            Member      member;
            const char* nameStart;
        };

        // The elements and members of the arrays and objects being parsed or built. Each array and object collects them
        // on a stack in blocks from the document's block source and copies them into the document once, at their exact
        // count, when it closes.
        class PendingValues
        {
        public:
            explicit PendingValues( StructuredData& data ) noexcept : document( data ), stack( *data.blockSource )
            {
            }

            uint64 GetSize() const noexcept
            {
                return stack.GetSize();
            }

            void SetSize( uint64 size ) noexcept
            {
                stack.SetSize( size );
            }

            uint32 GetMemberCount( uint64 base ) const noexcept
            {
                return uint32( ( stack.GetSize() - base ) / sizeof( PendingMember ) );
            }

            template <class T>
            T* At( uint64 offset ) noexcept
            {
                return stack.At<T>( offset );
            }

            template <class T>
            bool Push( const T& item ) noexcept
            {
                return stack.Push( item );
            }

            int32 FindName( uint64 base, const StringView& name ) noexcept
            {
                const PendingMember* pending = stack.At<PendingMember>( base );
                uint32               count   = GetMemberCount( base );

                for ( uint32 a = 0; a < count; a++ )
                {
                    if ( SameName( pending[ a ].member.name, name ) )
                    {
                        return int32( a );
                    }
                }

                return -1;
            }

            Status CloseArray( Value& array, uint64 base ) noexcept
            {
                uint32 count    = uint32( ( stack.GetSize() - base ) / sizeof( Value ) );
                Value* elements = nullptr;

                if ( count != 0 )
                {
                    elements = static_cast<Value*>( document.AllocateAligned( uint64( count ) * sizeof( Value ) ) );

                    if ( !elements )
                    {
                        return Status::kAllocationRefused;
                    }

                    CopyWords( elements, stack.At<Value>( base ), uint64( count ) * sizeof( Value ) );
                }

                array.type          = Type::kArray;
                array.count         = count;
                array.data.elements = elements;

                stack.SetSize( base );

                return Status::kOk;
            }

            // With kReject a repeated name fails, and repeated is set to where the name of the earliest repeat begins.
            Status CloseObject( Value& object, uint64 base, DuplicateNames duplicateNames, const char*& repeated ) noexcept
            {
                uint32        count  = GetMemberCount( base );
                const uint32* index  = nullptr;
                Status        status = Status::kOk;

                if ( count > uint32( indexedMemberCount ) )
                {
                    status = SettleNames( base, count, index, duplicateNames, repeated );
                }

                if ( status == Status::kOk )
                {
                    status = CopyMembers( object, base, count, index );
                }

                stack.SetSize( base );

                return status;
            }

        private:
            struct NamedPosition
            {
                const char* name;
                uint32      length;
                uint32      position;
            };

            StructuredData& document;
            ScratchStack    stack;

            // A merge sort, so that members with the same name stay in document order. Returns the array that holds
            // the sorted names.
            static NamedPosition* SortByName( NamedPosition* names, NamedPosition* scratch, uint32 count ) noexcept
            {
                NamedPosition* from = names;
                NamedPosition* to   = scratch;

                for ( uint32 width = 1; width < count; width *= 2 )
                {
                    for ( uint32 left = 0; left < count; left += 2 * width )
                    {
                        uint32 middle = ( count - left > width ) ? left + width : count;
                        uint32 right  = ( count - middle > width ) ? middle + width : count;
                        uint32 i      = left;
                        uint32 j      = middle;

                        for ( uint32 k = left; k < right; k++ )
                        {
                            bool takeLeft = ( j == right ) ||
                                            ( ( i < middle ) && ( CompareNames( from[ i ].name, from[ i ].length, from[ j ].name, from[ j ].length ) <= 0 ) );
                            to[ k ] = takeLeft ? from[ i++ ] : from[ j++ ];
                        }
                    }

                    NamedPosition* swap = from;
                    from                = to;
                    to                  = swap;
                }

                return from;
            }

            // Sorts the members of a large object by name and settles the duplicate names among them. With kReject, the
            // duplicate reported is the earliest member whose name came before, as if each member were checked when
            // read. The positions of the members in name order are left in index.
            Status SettleNames( uint64 base, uint32& count, const uint32*& index, DuplicateNames duplicateNames, const char*& repeated ) noexcept
            {
                uint64 namesOffset = base + uint64( count ) * sizeof( PendingMember );

                if ( !stack.Reserve( uint64( count ) * 2 * sizeof( NamedPosition ) ) )
                {
                    return Status::kAllocationRefused;
                }

                PendingMember* pending = stack.At<PendingMember>( base );
                NamedPosition* names   = stack.At<NamedPosition>( namesOffset );
                NamedPosition* scratch = names + count;

                for ( uint32 a = 0; a < count; a++ )
                {
                    names[ a ] = NamedPosition { pending[ a ].member.name.GetText(), pending[ a ].member.name.GetLength(), a };
                }

                const NamedPosition* sorted    = SortByName( names, scratch, count );
                uint32*              positions = reinterpret_cast<uint32*>( ( sorted == names ) ? scratch : names );
                uint32               repeat    = count;
                bool                 removed   = false;

                for ( uint32 first = 0, next = 1; first < count; first = next++ )
                {
                    while ( ( next < count ) && SameName( sorted[ next ].name, sorted[ next ].length, sorted[ first ].name, sorted[ first ].length ) )
                    {
                        next++;
                    }

                    if ( next - first == 1 )
                    {
                        continue;
                    }

                    if ( duplicateNames == DuplicateNames::kReject )
                    {
                        repeat = ( sorted[ first + 1 ].position < repeat ) ? sorted[ first + 1 ].position : repeat;
                        continue;
                    }

                    uint32 kept = sorted[ ( duplicateNames == DuplicateNames::kKeepLast ) ? next - 1 : first ].position;

                    pending[ sorted[ first ].position ].member.value = pending[ kept ].member.value;

                    for ( uint32 a = first + 1; a < next; a++ )
                    {
                        pending[ sorted[ a ].position ].nameStart = nullptr;
                    }

                    removed = true;
                }

                if ( repeat < count )
                {
                    repeated = pending[ repeat ].nameStart;
                    return Status::kDuplicateName;
                }

                uint32* sortedPositions = positions + count;

                if ( removed )
                {
                    uint32* newPosition = positions;
                    uint32  keptCount   = 0;

                    for ( uint32 a = 0; a < count; a++ )
                    {
                        newPosition[ a ] = pending[ a ].nameStart ? keptCount : ~uint32( 0 );

                        if ( pending[ a ].nameStart )
                        {
                            pending[ keptCount++ ] = pending[ a ];
                        }
                    }

                    uint32 k = 0;

                    for ( uint32 a = 0; a < count; a++ )
                    {
                        uint32 position = newPosition[ sorted[ a ].position ];

                        if ( position != ~uint32( 0 ) )
                        {
                            sortedPositions[ k++ ] = position;
                        }
                    }

                    count = keptCount;
                }
                else
                {
                    for ( uint32 a = 0; a < count; a++ )
                    {
                        sortedPositions[ a ] = sorted[ a ].position;
                    }
                }

                index = sortedPositions;

                return Status::kOk;
            }

            Status CopyMembers( Value& object, uint64 base, uint32 count, const uint32* index ) noexcept
            {
                Member* members = nullptr;

                if ( count != 0 )
                {
                    bool   indexed    = count > uint32( indexedMemberCount );
                    uint64 memberSize = uint64( count ) * sizeof( Member );
                    uint64 indexSize  = indexed ? RoundUpToEight( uint64( count ) * sizeof( uint32 ) ) : 0;

                    members = static_cast<Member*>( document.AllocateAligned( memberSize + indexSize ) );
                    if ( !members )
                    {
                        return Status::kAllocationRefused;
                    }

                    const PendingMember* pending = stack.At<PendingMember>( base );

                    for ( uint32 a = 0; a < count; a++ )
                    {
                        members[ a ] = pending[ a ].member;
                    }

                    if ( indexed )
                    {
                        uint32* memberIndex = reinterpret_cast<uint32*>( members + count );

                        for ( uint32 a = 0; a < count; a++ )
                        {
                            memberIndex[ a ] = index[ a ];
                        }
                    }
                }

                object.type         = Type::kObject;
                object.count        = count;
                object.data.members = members;

                return Status::kOk;
            }
        };

        // Reads a text into a document.
        class DocumentParser : public TextReader
        {
        public:
            DocumentParser( StructuredData& data, const char* begin, const char* textEnd, const ParseOptions& parseOptions ) noexcept :
                TextReader( begin, textEnd, parseOptions, true ),
                document( data ),
                children( data )
            {
            }

            Status ParseValue( Value& value, const char*& text, int32 depth ) noexcept
            {
                uint32 c = Peek( text );

                if ( ( c == '[' ) || ( c == '{' ) )
                {
                    if ( depth >= options.maxNestingDepth )
                    {
                        return Status::kMaximumNestingDepthExceeded;
                    }
                }
                else if ( ( c != '"' ) && ( c != 't' ) && ( c != 'f' ) && ( c != 'n' ) && ( c != '-' ) && !IsDigit( c ) )
                {
                    return Status::kExpectedJsonValue;
                }

                Status status = CountValue();
                if ( status != Status::kOk )
                {
                    return status;
                }

                switch ( c )
                {
                    case '"':
                    {
                        StringView string;

                        status          = ParseString( string, text );
                        value.type      = Type::kString;
                        value.count     = string.GetLength();
                        value.data.text = string.GetText();

                        return status;
                    }

                    case 't':
                    case 'f':
                        value.type = Type::kBool;
                        return ReadBool( value.data.boolean, text );

                    case 'n':
                        value.type = Type::kNull;
                        return ReadNull( text );

                    case '[':
                        return ParseArray( value, text, depth + 1 );

                    case '{':
                        return ParseObject( value, text, depth + 1 );

                    default:
                    {
                        const char* number = text;

                        status        = ReadNumber( value.data.number, text );
                        value.type    = Type::kNumber;
                        value.integer = IsIntegerText( number, text );

                        return status;
                    }
                }
            }

        private:
            StructuredData& document;
            PendingValues   children;

            Status ParseString( StringView& string, const char*& text ) noexcept
            {
                if ( Peek( text ) != '"' )
                {
                    return Status::kExpectedBeginningDoubleQuotes;
                }

                int32 textLength;
                int32 stringLength;

                Status status = ReadStringLiteral( text + 1, &textLength, &stringLength );
                if ( status != Status::kOk )
                {
                    text += textLength + 1;
                    return status;
                }

                status = CountStringBytes( stringLength );
                if ( status != Status::kOk )
                {
                    return status;
                }

                text++;

                const char* bytes = "";

                if ( stringLength != 0 )
                {
                    char* stored = document.AllocateBytes( uint64( stringLength ) );
                    if ( !stored )
                    {
                        return Status::kAllocationRefused;
                    }

                    if ( stringLength == textLength )
                    {
                        CopyText( stored, text, stringLength );
                    }
                    else
                    {
                        ReadStringLiteral( text, &textLength, &stringLength, stored );
                    }

                    bytes = stored;
                }

                text += textLength + 1;
                string = StringView( bytes, uint32( stringLength ) );

                return Status::kOk;
            }

            Status ParseArray( Value& array, const char*& text, int32 depth ) noexcept
            {
                uint64 base   = children.GetSize();
                Status status = ParseElements( text, depth );

                return ( status == Status::kOk ) ? children.CloseArray( array, base ) : status;
            }

            Status ParseElements( const char*& text, int32 depth ) noexcept
            {
                bool emptyArray = true;

                for ( ;; )
                {
                    text++;
                    text = SkipWhitespace( text );

                    if ( Peek( text ) == ']' )
                    {
                        if ( !emptyArray )
                        {
                            return Status::kExpectedJsonValue;
                        }

                        text++;

                        return Status::kOk;
                    }

                    Value  element;
                    Status status = ParseValue( element, text, depth );
                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    if ( !children.Push( element ) )
                    {
                        return Status::kAllocationRefused;
                    }

                    emptyArray = false;

                    text = SkipWhitespace( text );

                    if ( Peek( text ) != ',' )
                    {
                        break;
                    }
                }

                if ( Peek( text ) != ']' )
                {
                    return Status::kExpectedClosingSquareBracketsOrComma;
                }

                text++;

                return Status::kOk;
            }

            Status ParseObject( Value& object, const char*& text, int32 depth ) noexcept
            {
                uint64 base   = children.GetSize();
                Status status = ParseMembers( base, text, depth );

                return ( status == Status::kOk ) ? children.CloseObject( object, base, options.duplicateNames, text ) : status;
            }

            Status ParseMembers( uint64 base, const char*& text, int32 depth ) noexcept
            {
                bool emptyObject = true;

                for ( ;; )
                {
                    text++;
                    text = SkipWhitespace( text );

                    if ( Peek( text ) == '}' )
                    {
                        if ( !emptyObject )
                        {
                            return Status::kExpectedJsonValue;
                        }

                        text++;

                        return Status::kOk;
                    }

                    PendingMember pending;
                    pending.nameStart = text;

                    Status status = ParseString( pending.member.name, text );
                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    text = SkipWhitespace( text );

                    if ( Peek( text ) != ':' )
                    {
                        return Status::kExpectedColon;
                    }

                    text++;
                    text = SkipWhitespace( text );

                    status = ParseValue( pending.member.value, text, depth );
                    if ( status != Status::kOk )
                    {
                        return status;
                    }

                    // Past the first members, SettleNames finds the duplicate names once the object ends.
                    int32 position = ( children.GetMemberCount( base ) >= uint32( indexedMemberCount ) ) ? -1 : children.FindName( base, pending.member.name );

                    if ( position < 0 )
                    {
                        if ( !children.Push( pending ) )
                        {
                            return Status::kAllocationRefused;
                        }
                    }
                    else if ( options.duplicateNames == DuplicateNames::kKeepLast )
                    {
                        children.At<PendingMember>( base )[ position ].member.value = pending.member.value;
                    }
                    else if ( options.duplicateNames == DuplicateNames::kReject )
                    {
                        text = pending.nameStart;
                        return Status::kDuplicateName;
                    }

                    emptyObject = false;

                    text = SkipWhitespace( text );

                    if ( Peek( text ) != ',' )
                    {
                        break;
                    }
                }

                if ( Peek( text ) != '}' )
                {
                    return Status::kExpectedClosingCurlyBracesOrComma;
                }

                text++;

                return Status::kOk;
            }
        };

        // A text in the document's own blocks, such as one of its strings, is parsed before they are handed back.
        ParseResult StructuredData::ParseText( const char* text, uint64 length, const ParseOptions& options ) noexcept
        {
            if ( builder )
            {
                return ParseResult { Status::kBuildInProgress, 0, 0 };
            }

            Block*      replaced = Owns( text ) ? TakeBlocks() : nullptr;
            ParseResult result   = ClearAndParse( text, length, options );

            FreeBlocks( replaced );

            return result;
        }

        ParseResult StructuredData::ClearAndParse( const char* text, uint64 length, const ParseOptions& options ) noexcept
        {
            Clear();

            if ( ( text == nullptr ) || ( length == 0 ) )
            {
                return ParseResult { Status::kTextBufferContentsInvalid, 0, 0 };
            }

            if ( length > JSON4C4_MAX_FILE_SIZE )
            {
                return ParseResult { Status::kFileTooLarge, 0, 0 };
            }

            uint64 firstBlockSize = RoundUpToEight( length * 2 );

            nextBlockSize = ( firstBlockSize < smallestBlockSize )         ? smallestBlockSize
                            : ( firstBlockSize > largestGrowingBlockSize ) ? largestGrowingBlockSize
                                                                           : firstBlockSize;

            ParseResult result;
            Value*      parsedRoot = static_cast<Value*>( AllocateAligned( sizeof( Value ) ) );

            if ( !parsedRoot )
            {
                result.status = Status::kAllocationRefused;
                return result;
            }

            *parsedRoot = Value();

            {
                DocumentParser parser( *this, text, text + length, options );
                result.status = ParseRoot( parser, *parsedRoot, text, text + length, &result.errorLine, &result.errorColumn );
            }

            if ( result.status == Status::kOk )
            {
                root = parsedRoot;
            }
            else
            {
                Clear();
            }

            return result;
        }

        ParseResult StructuredData::Parse( const Array<char>& nullTerminatedTextBuffer, const ParseOptions& options ) noexcept
        {
            int32 count = nullTerminatedTextBuffer.GetArrayElementCount();

            if ( builder )
            {
                return ParseResult { Status::kBuildInProgress, 0, 0 };
            }

            if ( ( count >= 2 ) && ( nullTerminatedTextBuffer[ count - 1 ] != 0 ) )
            {
                Clear();

                return ParseResult { Status::kUnterminatedTextBuffer, 0, 0 };
            }

            return ParseText( nullTerminatedTextBuffer.begin(), uint64( ( count > 0 ) ? count - 1 : 0 ), options );
        }

        // The file is opened before the document is cleared, so that its name may be a string of the document. Its text
        // is read into a block from the block source, handed back when the parse ends.
        ParseResult StructuredData::Parse( const char* fileName, const ParseOptions& options ) noexcept
        {
            if ( builder )
            {
                return ParseResult { Status::kBuildInProgress, 0, 0 };
            }

            File       file;
            FileStatus fileStatus = kFileOpenFailed;

            Detail::MayThrow(
                [ & ]()
                {
                    fileStatus = file.OpenFile( fileName, kFileReadOnly );
                    return Status::kOk;
                } );

            Clear();

            if ( fileStatus != kFileOkay )
            {
                return ParseResult { Status::kFileOpenError, 0, 0 };
            }

            uint64 fileSize = file.GetFileSize();

            if ( fileSize > JSON4C4_MAX_FILE_SIZE )
            {
                return ParseResult { Status::kFileTooLarge, 0, 0 };
            }

            if ( fileSize == 0 )
            {
                return ParseResult { Status::kTextBufferContentsInvalid, 0, 0 };
            }

            uint64 bufferSize = RoundUpToEight( fileSize );
            char*  buffer     = static_cast<char*>( blockSource->AllocateBlock( bufferSize ) );

            if ( !buffer )
            {
                return ParseResult { Status::kAllocationRefused, 0, 0 };
            }

            ParseResult result { Status::kFileReadError, 0, 0 };

            if ( file.ReadFile( buffer, fileSize ) == kFileOkay )
            {
                file.CloseFile();
                result = ParseText( buffer, fileSize, options );
            }

            blockSource->FreeBlock( buffer, bufferSize );

            return result;
        }

        // Writes a float in its shortest form, unless that form, read as a double as the parser reads it, narrows to
        // another float. The float is then written as a double. Of all floats, only 7.038531e-26 and its negative need
        // it.
        inline ntt_i32 WriteFloat( float number, char* text ) noexcept
        {
            ntt_i32     length = ntt_write_f32( number, text );
            double      read   = 0;
            const char* stop   = nullptr;

            ntt_parse_json_f64( text, text + length, &read, &stop );

            return ( float( read ) == number ) ? length : ntt_write_f64( number, text );
        }

        // Writes what 2.3.0 wrote for the same values.
        class Writer
        {
        public:
            Writer( uint32 indentation, char indentationCharacter ) noexcept : indentationLength( indentation ), indentationChar( indentationCharacter )
            {
            }

            Status Write( Array<char>& output, const Value& value, uint32 depth )
            {
                switch ( value.GetType() )
                {
                    case Type::kNull:
                        return AppendText( output, "null" );

                    case Type::kBool:
                        return AppendText( output, *value.AsBool() ? "true" : "false" );

                    case Type::kNumber:
                        return WriteNumber( output, *value.AsNumber(), value.IsFloat() );

                    case Type::kString:
                    {
                        StringView string = value.AsString();
                        return AppendEscapedString( output, string.GetText(), string.GetLength() );
                    }

                    case Type::kArray:
                        return WriteArray( output, value.AsArray(), depth );

                    case Type::kObject:
                        return WriteObject( output, value.AsObject(), depth );
                }

                return Status::kInvalidValueType;
            }

        private:
            uint32 indentationLength;
            char   indentationChar;

            Status Indent( Array<char>& output, uint32 depth )
            {
                uint64 length = uint64( depth ) * indentationLength;

                return ( length > JSON4C4_MAX_FILE_SIZE ) ? Status::kFileTooLarge : ExpandArray( output, indentationChar, uint32( length ) );
            }

            Status WriteNumber( Array<char>& output, double number, bool single )
            {
                if ( !( number >= -maxDouble && number <= maxDouble ) )
                {
                    return Status::kFloatInvalid;
                }

                char text[ NUMBERTEXTTHREE_F64_MAX_CHARS ];

                return AppendText( output, text, single ? WriteFloat( float( number ), text ) : ntt_write_f64( number, text ) );
            }

            Status WriteArray( Array<char>& output, const ArrayView& array, uint32 depth )
            {
                uint32 count  = array.GetElementCount();
                Status status = AppendText( output, "[\n" );

                for ( uint32 a = 0; ( status == Status::kOk ) && ( a != count ); a++ )
                {
                    status = Indent( output, depth + 1 );

                    if ( status == Status::kOk )
                    {
                        status = Write( output, array[ a ], depth + 1 );
                    }

                    if ( status == Status::kOk )
                    {
                        status = AppendText( output, ( a + 1 < count ) ? ",\n" : "\n" );
                    }
                }

                if ( status == Status::kOk )
                {
                    status = Indent( output, depth );
                }

                return ( status == Status::kOk ) ? AppendText( output, "]", 1 ) : status;
            }

            Status WriteObject( Array<char>& output, const ObjectView& object, uint32 depth )
            {
                uint32 count  = object.GetMemberCount();
                Status status = AppendText( output, "{\n" );

                for ( uint32 a = 0; ( status == Status::kOk ) && ( a != count ); a++ )
                {
                    const Member& member = object.begin()[ a ];

                    status = Indent( output, depth + 1 );

                    if ( status == Status::kOk )
                    {
                        status = AppendEscapedString( output, member.GetName().GetText(), member.GetName().GetLength() );
                    }

                    if ( status == Status::kOk )
                    {
                        status = AppendText( output, " : " );
                    }

                    if ( status == Status::kOk )
                    {
                        status = Write( output, member.GetValue(), depth + 1 );
                    }

                    if ( status == Status::kOk )
                    {
                        status = AppendText( output, ( a + 1 < count ) ? ",\n" : "\n" );
                    }
                }

                if ( status == Status::kOk )
                {
                    status = Indent( output, depth );
                }

                return ( status == Status::kOk ) ? AppendText( output, "}", 1 ) : status;
            }
        };

        Status StructuredData::Write( Array<char>& output, uint32 indentationLength, char indentationChar ) const noexcept
        {
            if ( !root )
            {
                return Status::kInvalidStructuredData;
            }

            return Detail::MayThrow(
                [ & ]()
                {
                    Writer writer( indentationLength, indentationChar );
                    return writer.Write( output, *root, 0 );
                } );
        }

        Status StructuredData::Write( const char* fileName, uint32 indentationLength, char indentationChar ) const noexcept
        {
            Array<char> output;
            Status      status = Write( output, indentationLength, indentationChar );

            return ( status == Status::kOk ) ? WriteTextFile( fileName, output ) : status;
        }

        Status ValidateTraced( const Value& value, const String<>&, ReadPath& ) noexcept
        {
            StringView string = value.AsString();

            if ( !string )
            {
                return Status::kInvalidValueType;
            }

            return Detail::HoldsZero( string ) ? Status::kStringHoldsZero : Status::kOk;
        }

        Status DeserializeTraced( const Value& value, String<>& data, ReadPath& path ) noexcept
        {
            Status status = ValidateTraced( value, data, path );

            if ( status != Status::kOk )
            {
                return status;
            }

            StringView string = value.AsString();

            return Detail::MayThrow(
                [ & ]()
                {
                    String<> copy( string.GetText(), int32( string.GetLength() ) );
                    data = static_cast<String<>&&>( copy );

                    return Status::kOk;
                } );
        }

#ifdef JSON4C4_ENABLE_STD_SUPPORT_INTERNAL

        Status ValidateTraced( const Value& value, const std::string&, ReadPath& ) noexcept
        {
            return value.AsString() ? Status::kOk : Status::kInvalidValueType;
        }

        Status DeserializeTraced( const Value& value, std::string& data, ReadPath& ) noexcept
        {
            StringView string = value.AsString();

            if ( !string )
            {
                return Status::kInvalidValueType;
            }

            return Detail::MayThrow(
                [ & ]()
                {
                    std::string copy( string.GetText(), string.GetLength() );
                    data.swap( copy );

                    return Status::kOk;
                } );
        }

#endif

        Status ReadTextFile( const char* fileName, Array<char>& text ) noexcept
        {
            File       file;
            FileStatus fileStatus = kFileOpenFailed;

            Detail::MayThrow(
                [ & ]()
                {
                    fileStatus = file.OpenFile( fileName, kFileReadOnly );
                    return Status::kOk;
                } );

            if ( fileStatus != kFileOkay )
            {
                return Status::kFileOpenError;
            }

            uint64 fileSize = file.GetFileSize();

            if ( fileSize > JSON4C4_MAX_FILE_SIZE )
            {
                return Status::kFileTooLarge;
            }

            Status status = Detail::MayThrow(
                [ & ]()
                {
                    text.SetArrayElementCount( int32( fileSize ) );
                    return Status::kOk;
                } );

            if ( status != Status::kOk )
            {
                return status;
            }

            return ( ( fileSize == 0 ) || ( file.ReadFile( text.begin(), fileSize ) == kFileOkay ) ) ? Status::kOk : Status::kFileReadError;
        }

        // Finds where the value that a read path leads to begins in a text that parsed.
        class TextLocator : public TextReader
        {
        public:
            TextLocator( const char* begin, const char* textEnd, const ParseOptions& parseOptions ) noexcept :
                TextReader( begin, textEnd, parseOptions, true ),
                textBegin( begin )
            {
            }

            // The object that misses a member, when the path ends with one.
            const char* Find( const ReadPath& path ) noexcept
            {
                const char* text = SkipWhitespace( textBegin );

                for ( int32 a = path.GetSegmentCount() - 1; ( a >= 0 ) && text; a-- )
                {
                    const ReadPath::Segment& segment = path.GetSegment( a );

                    if ( segment.missing )
                    {
                        break;
                    }

                    text = segment.name ? FindMember( text, segment ) : FindElement( text, segment.index );
                }

                return text;
            }

        private:
            const char* textBegin;

            const char* SkipString( const char* text ) const noexcept
            {
                int32 textLength;
                int32 stringLength;

                if ( ( Peek( text ) != '"' ) || ( ReadStringLiteral( text + 1, &textLength, &stringLength ) != Status::kOk ) )
                {
                    return nullptr;
                }

                return text + textLength + 2;
            }

            const char* SkipValue( const char* text ) const noexcept
            {
                switch ( Peek( text ) )
                {
                    case '"':
                        return SkipString( text );

                    case '[':
                    case '{':
                        return SkipContainer( text );

                    case 't':
                    case 'f':
                    {
                        bool boolean;
                        return ( ReadBool( boolean, text ) == Status::kOk ) ? text : nullptr;
                    }

                    case 'n':
                        return ( ReadNull( text ) == Status::kOk ) ? text : nullptr;

                    default:
                    {
                        double number;
                        return ( ReadNumber( number, text ) == Status::kOk ) ? text : nullptr;
                    }
                }
            }

            const char* SkipContainer( const char* text ) const noexcept
            {
                uint32 close = ( text[ 0 ] == '[' ) ? ']' : '}';

                text = SkipWhitespace( text + 1 );

                while ( text && ( Peek( text ) != close ) )
                {
                    if ( close == '}' )
                    {
                        text = SkipString( text );
                        text = text ? SkipWhitespace( text ) : nullptr;
                        text = ( text && ( Peek( text ) == ':' ) ) ? SkipWhitespace( text + 1 ) : nullptr;
                    }

                    text = text ? SkipValue( text ) : nullptr;
                    text = text ? SkipWhitespace( text ) : nullptr;

                    if ( text && ( Peek( text ) == ',' ) )
                    {
                        text = SkipWhitespace( text + 1 );
                    }
                    else if ( text && ( Peek( text ) != close ) )
                    {
                        text = nullptr;
                    }
                }

                return text ? text + 1 : nullptr;
            }

            const char* FindElement( const char* text, uint32 index ) const noexcept
            {
                if ( Peek( text ) != '[' )
                {
                    return nullptr;
                }

                text = SkipWhitespace( text + 1 );

                for ( uint32 a = 0; ( a < index ) && text; a++ )
                {
                    text = SkipValue( text );
                    text = text ? SkipWhitespace( text ) : nullptr;
                    text = ( text && ( Peek( text ) == ',' ) ) ? SkipWhitespace( text + 1 ) : nullptr;
                }

                return text;
            }

            // With kKeepLast a repeated name keeps the value of its last occurrence, otherwise of its first.
            const char* FindMember( const char* text, const ReadPath::Segment& segment ) noexcept
            {
                if ( Peek( text ) != '{' )
                {
                    return nullptr;
                }

                const char* found = nullptr;

                text = SkipWhitespace( text + 1 );

                while ( text && ( Peek( text ) == '"' ) )
                {
                    bool same = IsName( text, segment );

                    text = SkipString( text );
                    text = text ? SkipWhitespace( text ) : nullptr;
                    text = ( text && ( Peek( text ) == ':' ) ) ? SkipWhitespace( text + 1 ) : nullptr;

                    if ( text && same )
                    {
                        found = text;

                        if ( options.duplicateNames != DuplicateNames::kKeepLast )
                        {
                            break;
                        }
                    }

                    text = text ? SkipValue( text ) : nullptr;
                    text = text ? SkipWhitespace( text ) : nullptr;
                    text = ( text && ( Peek( text ) == ',' ) ) ? SkipWhitespace( text + 1 ) : nullptr;
                }

                return found;
            }

            bool IsName( const char* text, const ReadPath::Segment& segment ) const noexcept
            {
                int32 textLength;
                int32 stringLength;

                if ( ( ReadStringLiteral( text + 1, &textLength, &stringLength ) != Status::kOk ) || ( uint32( stringLength ) != segment.length ) )
                {
                    return false;
                }

                if ( stringLength == textLength )
                {
                    return SameName( text + 1, uint32( stringLength ), segment.name, segment.length );
                }

                Array<char> name;
                bool        same = false;

                Detail::MayThrow(
                    [ & ]()
                    {
                        name.SetArrayElementCount( stringLength );
                        ReadStringLiteral( text + 1, &textLength, &stringLength, name.begin() );
                        same = SameName( name.begin(), uint32( stringLength ), segment.name, segment.length );

                        return Status::kOk;
                    } );

                return same;
            }
        };

        void LocateReadFailure( const char* text, uint64 length, const ParseOptions& options, const ReadPath& path, ReadResult& result ) noexcept
        {
            Detail::MayThrow(
                [ & ]()
                {
                    String<> pathText;

                    for ( int32 a = path.GetSegmentCount() - 1; a >= 0; a-- )
                    {
                        const ReadPath::Segment& segment = path.GetSegment( a );

                        if ( !segment.name )
                        {
                            pathText += "[";
                            pathText += String<>( segment.index );
                            pathText += "]";

                            continue;
                        }

                        if ( pathText.GetStringLength() != 0 )
                        {
                            pathText += ".";
                        }

                        for ( uint32 b = 0; b < segment.length; b++ )
                        {
                            char c[ 2 ] = { segment.name[ b ], 0 };
                            pathText += ( c[ 0 ] != 0 ) ? c : "\\u0000";
                        }
                    }

                    result.path = static_cast<String<>&&>( pathText );

                    return Status::kOk;
                } );

            TextLocator locator( text, text + length, options );
            const char* position = locator.Find( path );

            if ( position )
            {
                FindLineAndColumn( text, text + length, position, &result.errorLine, &result.errorColumn );
            }
        }

        // Whether the writer writes a number with neither a fraction nor an exponent, as a double or as a float.
        inline bool IsIntegerNumber( double number ) noexcept
        {
            if ( !( ( number > -1e21 ) && ( number < 1e21 ) ) )
            {
                return false;
            }

            return ( number >= 9.0e18 ) || ( number <= -9.0e18 ) || ( double( int64( number ) ) == number );
        }

        // Builds a document as Serialize writes it. The elements and members of each array and object wait on the stack
        // until the Serialize call that opened it returns, so that it is copied into the document once, at its exact
        // count, as a parsed one is.
        class Builder
        {
        public:
            struct OpenContainer
            {
                uint64 value;
                uint64 base;
            };

            StructuredData&        document;
            PendingValues          children;
            ScratchStack           open;
            Status                 failure = Status::kOk;
            StructuredData::Block* replacedBlocks;

            Builder( StructuredData& data, StructuredData::Block* replaced ) noexcept :
                document( data ),
                children( data ),
                open( *data.blockSource ),
                replacedBlocks( replaced )
            {
            }

            // The first call that fails fails the build, even when the Serialize that made it goes on.
            Status Record( Status status ) noexcept
            {
                failure = ( failure == Status::kOk ) ? status : failure;

                return status;
            }

            Value& At( uint64 offset ) noexcept
            {
                return *children.At<Value>( offset );
            }

            uint32 GetOpenCount() const noexcept
            {
                return uint32( open.GetSize() / sizeof( OpenContainer ) );
            }

            OpenContainer* GetLastOpen() noexcept
            {
                return ( open.GetSize() != 0 ) ? open.At<OpenContainer>( open.GetSize() - sizeof( OpenContainer ) ) : nullptr;
            }

            bool IsOpen( uint64 value ) noexcept
            {
                const OpenContainer* containers = open.At<OpenContainer>( 0 );

                for ( uint32 a = 0; a < GetOpenCount(); a++ )
                {
                    if ( containers[ a ].value == value )
                    {
                        return true;
                    }
                }

                return false;
            }

            // Elements and members go only to the array or object opened last.
            bool IsLastOpen( uint64 value, Type type ) noexcept
            {
                const OpenContainer* last = GetLastOpen();

                return last && ( last->value == value ) && ( At( value ).type == type );
            }

            // A node that is an open array or object takes no other value.
            Status SetScalar( uint64 value, const Value& scalar ) noexcept
            {
                if ( IsOpen( value ) )
                {
                    return Status::kInvalidValueType;
                }

                At( value ) = scalar;

                return Status::kOk;
            }

            Status SetBool( uint64 value, bool boolean ) noexcept
            {
                Value scalar;

                scalar.type         = Type::kBool;
                scalar.data.boolean = boolean;

                return SetScalar( value, scalar );
            }

            // A number that cannot be written is refused, so that a built document can always be written.
            Status SetNumber( uint64 value, double number, bool single ) noexcept
            {
                if ( !( number >= -maxDouble && number <= maxDouble ) )
                {
                    return Status::kFloatInvalid;
                }

                Value scalar;

                scalar.type        = Type::kNumber;
                scalar.integer     = IsIntegerNumber( number );
                scalar.single      = single;
                scalar.data.number = number;

                return SetScalar( value, scalar );
            }

            Status SetString( uint64 value, const char* text, uint32 length ) noexcept
            {
                Value  scalar;
                Status status = CopyText( text, length, scalar.data.text );

                scalar.type  = Type::kString;
                scalar.count = length;

                return ( status == Status::kOk ) ? SetScalar( value, scalar ) : status;
            }

            Status Open( uint64 value, Type type ) noexcept
            {
                if ( IsOpen( value ) )
                {
                    return IsLastOpen( value, type ) ? Status::kOk : Status::kInvalidValueType;
                }

                if ( !open.Push( OpenContainer { value, children.GetSize() } ) )
                {
                    return Status::kAllocationRefused;
                }

                Value& container = At( value );
                container        = Value();
                container.type   = type;

                return Status::kOk;
            }

            Status AppendElement( uint64 array, uint64& element ) noexcept
            {
                if ( !IsLastOpen( array, Type::kArray ) )
                {
                    return Status::kInvalidValueType;
                }

                if ( !children.Push( Value() ) )
                {
                    return Status::kAllocationRefused;
                }

                element = children.GetSize() - sizeof( Value );

                return Status::kOk;
            }

            Status AddMember( uint64 object, const char* name, uint32 length, uint64& member ) noexcept
            {
                if ( !IsLastOpen( object, Type::kObject ) )
                {
                    return Status::kInvalidValueType;
                }

                if ( !name )
                {
                    return Status::kNameNotPresent;
                }

                const char* copy;
                Status      status = CopyText( name, length, copy );

                if ( status != Status::kOk )
                {
                    return status;
                }

                PendingMember pending;

                pending.member.name = StringView( copy, length );
                pending.nameStart   = nullptr;

                if ( !children.Push( pending ) )
                {
                    return Status::kAllocationRefused;
                }

                uint64         pendingOffset = children.GetSize() - sizeof( PendingMember );
                PendingMember* pushed        = children.At<PendingMember>( pendingOffset );

                member = pendingOffset + uint64( reinterpret_cast<char*>( &pushed->member.value ) - reinterpret_cast<char*>( pushed ) );

                return Status::kOk;
            }

            // An array or object that fails to close leaves nothing it held on the stack, so that a Serialize that goes
            // on adds to the one around it.
            Status CloseSince( uint32 openCount ) noexcept
            {
                while ( GetOpenCount() > openCount )
                {
                    OpenContainer container = *GetLastOpen();
                    Value         closed;
                    Status        status;

                    open.SetSize( open.GetSize() - sizeof( OpenContainer ) );

                    if ( At( container.value ).type == Type::kArray )
                    {
                        status = children.CloseArray( closed, container.base );
                    }
                    else
                    {
                        status = CloseObject( closed, container.base );
                    }

                    if ( status != Status::kOk )
                    {
                        children.SetSize( container.base );
                        return status;
                    }

                    At( container.value ) = closed;
                }

                return Status::kOk;
            }

        private:
            Status CopyText( const char* text, uint32 length, const char*& copy ) noexcept
            {
                copy = "";

                if ( length != 0 )
                {
                    char* stored = document.AllocateBytes( length );

                    if ( !stored )
                    {
                        return Status::kAllocationRefused;
                    }

                    for ( uint32 a = 0; a < length; a++ )
                    {
                        stored[ a ] = text[ a ];
                    }

                    copy = stored;
                }

                return Status::kOk;
            }

            // The parser finds a repeated name among the first members of an object as it reads them, and the builder
            // when the object closes.
            Status CloseObject( Value& object, uint64 base ) noexcept
            {
                uint32 count = children.GetMemberCount( base );

                if ( count <= uint32( indexedMemberCount ) )
                {
                    const PendingMember* pending = children.At<PendingMember>( base );

                    for ( uint32 a = 1; a < count; a++ )
                    {
                        for ( uint32 b = 0; b < a; b++ )
                        {
                            if ( SameName( pending[ a ].member.name, pending[ b ].member.name ) )
                            {
                                return Status::kDuplicateName;
                            }
                        }
                    }
                }

                const char* repeated = nullptr;

                return children.CloseObject( object, base, DuplicateNames::kReject, repeated );
            }
        };

        // A node that was never given, or that AppendElement or AddMember failed to give, has no builder.
        Status Node::SetNull() noexcept
        {
            return builder ? builder->Record( builder->SetScalar( offset, Value() ) ) : Status::kInvalidValueType;
        }

        Status Node::SetBool( bool value ) noexcept
        {
            return builder ? builder->Record( builder->SetBool( offset, value ) ) : Status::kInvalidValueType;
        }

        Status Node::SetNumber( double value ) noexcept
        {
            return builder ? builder->Record( builder->SetNumber( offset, value, false ) ) : Status::kInvalidValueType;
        }

        Status Node::SetFloat( float value ) noexcept
        {
            return builder ? builder->Record( builder->SetNumber( offset, value, true ) ) : Status::kInvalidValueType;
        }

        Status Node::SetString( const char* text, uint32 length ) noexcept
        {
            return builder ? builder->Record( builder->SetString( offset, text, length ) ) : Status::kInvalidValueType;
        }

        Status Node::SetArray() noexcept
        {
            return builder ? builder->Record( builder->Open( offset, Type::kArray ) ) : Status::kInvalidValueType;
        }

        Status Node::SetObject() noexcept
        {
            return builder ? builder->Record( builder->Open( offset, Type::kObject ) ) : Status::kInvalidValueType;
        }

        Status Node::AppendElement( Node& element ) noexcept
        {
            uint64 at     = 0;
            Status status = builder ? builder->Record( builder->AppendElement( offset, at ) ) : Status::kInvalidValueType;

            element.builder = ( status == Status::kOk ) ? builder : nullptr;
            element.offset  = at;

            return status;
        }

        Status Node::AddMember( const char* name, uint32 length, Node& member ) noexcept
        {
            uint64 at     = 0;
            Status status = builder ? builder->Record( builder->AddMember( offset, name, length, at ) ) : Status::kInvalidValueType;

            member.builder = ( status == Status::kOk ) ? builder : nullptr;
            member.offset  = at;

            return status;
        }

        uint32 Node::GetOpenCount() const noexcept
        {
            return builder ? builder->GetOpenCount() : 0;
        }

        Status Node::CloseSince( uint32 openCount ) noexcept
        {
            return builder ? builder->Record( builder->CloseSince( openCount ) ) : Status::kInvalidValueType;
        }

        Status Serialize( Node node, const Value& value ) noexcept
        {
            switch ( value.GetType() )
            {
                case Type::kNull:
                    return node.SetNull();

                case Type::kBool:
                    return node.SetBool( *value.AsBool() );

                case Type::kNumber:
                    return value.IsFloat() ? node.SetFloat( float( *value.AsNumber() ) ) : node.SetNumber( *value.AsNumber() );

                case Type::kString:
                    return node.SetString( value.AsString().GetText(), value.AsString().GetLength() );

                case Type::kArray:
                {
                    Status status = node.SetArray();

                    for ( const Value& element : value.AsArray() )
                    {
                        Node copy;

                        status = ( status == Status::kOk ) ? node.AppendElement( copy ) : status;
                        status = ( status == Status::kOk ) ? SerializeValue( copy, element ) : status;
                    }

                    return status;
                }

                case Type::kObject:
                {
                    Status status = node.SetObject();

                    for ( const Member& member : value.AsObject() )
                    {
                        Node copy;

                        status = ( status == Status::kOk ) ? node.AddMember( member.GetName().GetText(), member.GetName().GetLength(), copy ) : status;
                        status = ( status == Status::kOk ) ? SerializeValue( copy, member.GetValue() ) : status;
                    }

                    return status;
                }
            }

            return Status::kInvalidValueType;
        }

        // Puts the position of a member into the index of a large object, which holds the positions of count members.
        void InsertIntoIndex( const Member* members, uint32* index, uint32 count, uint32 position ) noexcept
        {
            const StringView& name = members[ position ].GetName();
            uint32            slot = FindIndexSlot( members, index, count, name.GetText(), name.GetLength() );

            for ( uint32 a = count; a > slot; a-- )
            {
                index[ a ] = index[ a - 1 ];
            }

            index[ slot ] = position;
        }

        // A value in none of this document's blocks, such as a value of another document, gives an empty handle.
        EditHandle StructuredData::Edit( const Value& value ) noexcept
        {
            EditHandle handle;

            if ( Owns( &value ) )
            {
                handle.document = this;
                handle.value    = &value;
                handle.edit     = editCount;
            }

            return handle;
        }

        // An edit is refused while a build is open, for a handle of another document or an empty one, for a stale
        // handle, and for a value that is not of the type it needs.
        Status StructuredData::CheckEdit( const EditHandle& handle ) const noexcept
        {
            if ( builder )
            {
                return Status::kBuildInProgress;
            }

            if ( handle.document != this )
            {
                return Status::kInvalidValueType;
            }

            return ( handle.edit == editCount ) ? Status::kOk : Status::kStaleEditHandle;
        }

        Status StructuredData::CheckEdit( const EditHandle& handle, Type type ) const noexcept
        {
            Status status = CheckEdit( handle );

            return ( ( status == Status::kOk ) && ( handle.value->type != type ) ) ? Status::kInvalidValueType : status;
        }

        // The builder lives in a block of its own, handed back when the build ends, and a SerializeFrom keeps the
        // blocks of the document it replaces until then, so that the document may be built from its own values.
        Status StructuredData::BeginBuild( const Change& change, Node& rootNode ) noexcept
        {
            Status status = builder ? Status::kBuildInProgress : Status::kOk;

            switch ( change.kind )
            {
                case ChangeKind::kDocument:
                    break;

                case ChangeKind::kReplace:
                    status = CheckEdit( *change.handle );
                    break;

                case ChangeKind::kAppend:
                    status = CheckEdit( *change.handle, Type::kArray );
                    break;

                case ChangeKind::kAddMember:
                    status = CheckEdit( *change.handle, Type::kObject );
                    status = ( ( status == Status::kOk ) && !change.name ) ? Status::kNameNotPresent : status;
                    status =
                        ( ( status == Status::kOk ) && change.handle->value->AsObject().Find( change.name, change.length ) ) ? Status::kDuplicateName : status;
                    break;
            }

            if ( status != Status::kOk )
            {
                return status;
            }

            Block* replaced = ( change.kind == ChangeKind::kDocument ) ? TakeBlocks() : nullptr;
            void*  memory   = blockSource->AllocateBlock( RoundUpToEight( sizeof( Builder ) ) );

            if ( !memory )
            {
                FreeBlocks( replaced );
                return Status::kAllocationRefused;
            }

            builder          = new ( memory ) Builder( *this, replaced );
            rootNode.builder = builder;
            rootNode.offset  = 0;

            return builder->children.Push( Value() ) ? Status::kOk : Status::kAllocationRefused;
        }

        // The edit is made only when its value is built, so that a failed edit leaves the document as it was. A failed
        // SerializeFrom leaves it empty.
        Status StructuredData::EndBuild( const Change& change, const Node& rootNode, Status status ) noexcept
        {
            if ( !rootNode.builder )
            {
                return status;
            }

            status          = ( status == Status::kOk ) ? builder->failure : status;
            Value  built    = ( status == Status::kOk ) ? builder->At( 0 ) : Value();
            Block* replaced = builder->replacedBlocks;

            builder->~Builder();
            blockSource->FreeBlock( builder, RoundUpToEight( sizeof( Builder ) ) );
            builder = nullptr;

            status = ( status == Status::kOk ) ? Splice( change, built ) : status;

            FreeBlocks( replaced );

            if ( ( status != Status::kOk ) && ( change.kind == ChangeKind::kDocument ) )
            {
                Clear();
            }

            if ( ( status == Status::kOk ) && ( change.kind != ChangeKind::kDocument ) )
            {
                change.handle->edit = ++editCount;
            }

            return status;
        }

        Status StructuredData::Splice( const Change& change, const Value& built ) noexcept
        {
            if ( change.kind == ChangeKind::kDocument )
            {
                Value* value = static_cast<Value*>( AllocateAligned( sizeof( Value ) ) );

                if ( !value )
                {
                    return Status::kAllocationRefused;
                }

                *value = built;
                root   = value;

                return Status::kOk;
            }

            Value& target = const_cast<Value&>( *change.handle->value );

            if ( change.kind == ChangeKind::kReplace )
            {
                target = built;

                return Status::kOk;
            }

            const char* name = "";

            if ( change.length != 0 )
            {
                char* copy = AllocateBytes( change.length );

                if ( !copy )
                {
                    return Status::kAllocationRefused;
                }

                for ( uint32 a = 0; a < change.length; a++ )
                {
                    copy[ a ] = change.name[ a ];
                }

                name = copy;
            }

            if ( !MakeRoom( target ) )
            {
                return Status::kAllocationRefused;
            }

            if ( change.kind == ChangeKind::kAppend )
            {
                const_cast<Value*>( target.data.elements )[ target.count++ ] = built;

                return Status::kOk;
            }

            Member* members = const_cast<Member*>( target.data.members );
            uint32  room    = target.GetCapacity();

            members[ target.count ].name  = StringView( name, change.length );
            members[ target.count ].value = built;

            if ( room > uint32( indexedMemberCount ) )
            {
                InsertIntoIndex( members, reinterpret_cast<uint32*>( members + room ), target.count, target.count );
            }

            target.count++;

            return Status::kOk;
        }

        // Gives an array or object room for one more element or member. The first edit that adds to one moves it into
        // room for a power of two of them, at least 4, which doubles when it is full. An object with room for more
        // than 16 members keeps its index after that room, even while it has fewer members.
        bool StructuredData::MakeRoom( Value& container ) noexcept
        {
            uint32 count    = container.count;
            uint32 capacity = container.GetCapacity();

            if ( ( container.capacityShift != 0 ) && ( count < capacity ) )
            {
                return true;
            }

            uint8 shift = 2;

            while ( ( shift < 31 ) && ( ( uint32( 1 ) << shift ) <= count ) )
            {
                shift++;
            }

            uint32 room = uint32( 1 ) << shift;

            if ( room <= count )
            {
                return false;
            }

            bool   isObject  = container.type == Type::kObject;
            uint64 itemSize  = isObject ? sizeof( Member ) : sizeof( Value );
            bool   indexed   = isObject && ( room > uint32( indexedMemberCount ) );
            uint64 indexSize = indexed ? uint64( room ) * sizeof( uint32 ) : 0;
            char*  storage   = static_cast<char*>( AllocateAligned( uint64( room ) * itemSize + indexSize ) );

            if ( !storage )
            {
                return false;
            }

            if ( count != 0 )
            {
                CopyWords( storage, isObject ? static_cast<const void*>( container.data.members ) : container.data.elements, uint64( count ) * itemSize );
            }

            if ( indexed )
            {
                const Member* members = reinterpret_cast<const Member*>( storage );
                uint32*       index   = reinterpret_cast<uint32*>( storage + uint64( room ) * itemSize );

                if ( capacity > uint32( indexedMemberCount ) )
                {
                    const uint32* oldIndex = reinterpret_cast<const uint32*>( container.data.members + capacity );

                    for ( uint32 a = 0; a < count; a++ )
                    {
                        index[ a ] = oldIndex[ a ];
                    }
                }
                else
                {
                    for ( uint32 a = 0; a < count; a++ )
                    {
                        InsertIntoIndex( members, index, a, a );
                    }
                }
            }

            if ( isObject )
            {
                container.data.members = reinterpret_cast<const Member*>( storage );
            }
            else
            {
                container.data.elements = reinterpret_cast<const Value*>( storage );
            }

            container.capacityShift = shift;

            return true;
        }

        Status StructuredData::RemoveElement( EditHandle& array, uint32 index ) noexcept
        {
            Status status = CheckEdit( array, Type::kArray );

            if ( status != Status::kOk )
            {
                return status;
            }

            Value& edited = const_cast<Value&>( *array.value );

            if ( index >= edited.count )
            {
                return Status::kIndexOutOfRange;
            }

            Value* elements = const_cast<Value*>( edited.data.elements );

            for ( uint32 a = index + 1; a < edited.count; a++ )
            {
                elements[ a - 1 ] = elements[ a ];
            }

            edited.count--;
            array.edit = ++editCount;

            return Status::kOk;
        }

        // An object keeps its room, and its index stays valid for no members.
        Status StructuredData::EmptyObject( EditHandle& object ) noexcept
        {
            Status status = CheckEdit( object, Type::kObject );

            if ( status == Status::kOk )
            {
                const_cast<Value&>( *object.value ).count = 0;
                object.edit                               = ++editCount;
            }

            return status;
        }

        // The members after the one removed move down, so that the others keep their order. An object without room to
        // spare keeps its index right after its members, so the index moves down with them.
        Status StructuredData::RemoveMember( EditHandle& object, const char* name, uint32 length ) noexcept
        {
            Status status = CheckEdit( object, Type::kObject );

            if ( status != Status::kOk )
            {
                return status;
            }

            if ( !name )
            {
                return Status::kNameNotPresent;
            }

            Value&  edited   = const_cast<Value&>( *object.value );
            Member* members  = const_cast<Member*>( edited.data.members );
            uint32  count    = edited.count;
            uint32  capacity = edited.GetCapacity();
            uint32* index    = ( capacity > uint32( indexedMemberCount ) ) ? reinterpret_cast<uint32*>( members + capacity ) : nullptr;
            uint32  slot     = 0;
            uint32  position = 0;

            if ( index )
            {
                slot     = FindIndexSlot( members, index, count, name, length );
                position = ( slot < count ) ? index[ slot ] : count;
            }
            else
            {
                while ( ( position < count ) && !SameName( members[ position ].name.GetText(), members[ position ].name.GetLength(), name, length ) )
                {
                    position++;
                }
            }

            if ( ( position == count ) || !SameName( members[ position ].name.GetText(), members[ position ].name.GetLength(), name, length ) )
            {
                return Status::kNameNotPresent;
            }

            for ( uint32 a = position + 1; a < count; a++ )
            {
                members[ a - 1 ] = members[ a ];
            }

            uint32 room = ( edited.capacityShift != 0 ) ? capacity : count - 1;

            if ( room > uint32( indexedMemberCount ) )
            {
                uint32* movedIndex = reinterpret_cast<uint32*>( members + room );
                uint32  kept       = 0;

                for ( uint32 a = 0; a < count; a++ )
                {
                    if ( a != slot )
                    {
                        movedIndex[ kept++ ] = index[ a ] - uint32( index[ a ] > position );
                    }
                }
            }

            edited.count = count - 1;
            object.edit  = ++editCount;

            return Status::kOk;
        }

    } // namespace Json
} // namespace C4
