#include "../Precomp.h"
#include "NetBuffer.h"

#ifdef _MSC_VER

#pragma warning(push)
#pragma warning(disable:4290) // warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)

#endif

using NWN::ExoParseBuffer;
using NWN::ExoBuildBuffer;

/*
 * ExoParseBuffer
 */

ExoParseBuffer::ExoParseBuffer(
	__in_bcount( ByteDataLength ) const void *ByteData,
	__in size_t ByteDataLength,
	__in_bcount_opt( BitDataLength ) const void *BitData,
	__in size_t BitDataLength
	)
	: m_ByteStream(
		ByteData,
		ByteDataLength,
		swutil::BufferParser::BitOrderHighToLow
		),
	m_BitStream(
		BitData,
		BitDataLength,
		swutil::BufferParser::BitOrderHighToLow
		)
{
}

ExoParseBuffer::~ExoParseBuffer( )
{
}

bool
ExoParseBuffer::ReadCExoString(
	__out std::string &String,
	__in size_t NumBits /* = 32 */
	)
{
	const void *Data;

	if (NumBits == 32)
	{
		unsigned Length;

		if (!m_ByteStream.GetField( Length ))
			return false;

		if (!m_ByteStream.GetDataPtr( Length, &Data ))
			return false;

		try
		{
			String.assign( reinterpret_cast< const char * >( Data ), Length );
		}
		catch (std::bad_alloc)
		{
			return false;
		}

		return true;
	}

	unsigned __int64 Length;

	if (!ReadSigned( Length, NumBits ))
		return false;

	if (!m_ByteStream.GetDataPtr( static_cast< size_t >( Length ), &Data ))
		return false;

	try
	{
		String.assign(
			reinterpret_cast< const char * >( Data ),
			static_cast< size_t >( Length )
			);
	}
	catch (std::bad_alloc)
	{
		return false;
	}

	return true;
}

bool
ExoParseBuffer::ReadCExoLocString(
	NWN::ExoLocString &String
	)
{
	if (!ReadBOOL( String.IsStrRef ))
		return false;

	if (String.IsStrRef)
	{
		unsigned char Flag;

		if (!ReadBYTE( Flag, 1 ))
			return false;

		String.Flag = Flag ? true : false;

		if (!ReadDWORD( String.StrRef ))
			return false;
	}
	else
	{
		if (!ReadCExoString( String.String ))
			return false;
	}

	return true;
}

bool
ExoParseBuffer::ReadSmallString(
	__out std::string &String,
	__in size_t NumBits /* = 8 */
	)
{
	unsigned __int64   Length;
	const void       * Data;

	if (!ReadUnsigned( Length, NumBits ))
		return false;

	if (!m_ByteStream.GetDataPtr( (size_t) Length, &Data ))
		return false;

	try
	{
		String.assign( reinterpret_cast< const char * >( Data ), (size_t) Length );
	}
	catch (std::bad_alloc)
	{
		return false;
	}

	return true;
}

bool
ExoParseBuffer::ReadSigned(
	__out unsigned __int64 &FieldBits,
	__in size_t NumBits
	)
{
	bool SignBit;

	if (!m_BitStream.GetFieldBit( SignBit ))
		return false;

	if (!ReadUnsigned( FieldBits, NumBits - 1))
		return false;

	if (!SignBit)
		return true;

	int Low   = -(static_cast< int >( static_cast< unsigned >( (FieldBits >> 0 ) & 0xFFFFFFFF ) )             );
	int High  = -(static_cast< int >( static_cast< unsigned >( (FieldBits >> 32) & 0xFFFFFFFF ) ) + (Low != 0));

	if (!((unsigned)Low | (unsigned)High))
		FieldBits = 0x8000000000000000;
	else
	{
		FieldBits = (static_cast< unsigned         >( Low  ) << 0 ) |
		            (static_cast< unsigned __int64 >( High ) << 32);
	}

	return true;
}

bool
ExoParseBuffer::ReadUnsigned(
	__out unsigned __int64 &FieldBits,
	__in size_t NumBits
	)
{
	size_t Offset = 0;

	if (NumBits == 64)
		return m_ByteStream.GetField( FieldBits );

	FieldBits = 0;

	if (NumBits - Offset >= 32)
	{
		unsigned long Bits;

		if (!m_ByteStream.GetField( Bits ))
			return false;

		FieldBits <<= 32;
		FieldBits |= (static_cast< unsigned __int64 >( Bits ));
		Offset    += 32;
	}

	if (NumBits - Offset >= 16)
	{
		unsigned short Bits;

		if (!m_ByteStream.GetField( Bits ))
			return false;

		FieldBits <<= 16;
		FieldBits |= (static_cast< unsigned __int64 >( Bits ));
		Offset    += 16;
	}

	if (NumBits - Offset >= 8)
	{
		unsigned char Bits;

		if (!m_ByteStream.GetField( Bits ))
			return false;

		FieldBits <<= 8;
		FieldBits |= (static_cast< unsigned __int64 >( Bits ));
		Offset    += 8;
	}

	if (NumBits - Offset)
	{
		unsigned char Bits;

		if (!m_BitStream.GetFieldBits( NumBits - Offset, Bits ))
			return false;

		FieldBits <<= NumBits - Offset;
		FieldBits |= (static_cast< unsigned __int64 >( Bits ));
		Offset    += (NumBits - Offset);
	}

	return true;
}

/*
 * ExoBuildBuffer
 */

ExoBuildBuffer::ExoBuildBuffer(
	__in size_t ByteDataSizeHint /* = 32 */,
	__in size_t BitDataSizeHint /* = 1 */
	) throw( std::exception )
	: m_ByteStream(
		ByteDataSizeHint,
		swutil::BufferBuilder::BitOrderHighToLow
		),
	  m_BitStream(
		BitDataSizeHint,
		swutil::BufferBuilder::BitOrderHighToLow
		)
{
}

ExoBuildBuffer::~ExoBuildBuffer( )
{
}

void
ExoBuildBuffer::WriteCExoString(
	__in const std::string &String,
	__in size_t NumBits /* = 32 */
	) throw( std::exception )
{
	if (NumBits == 32)
	{
		m_ByteStream.AddField( static_cast< unsigned long >( String.length() ) );
	}
	else
	{
		WriteSigned(
			static_cast< unsigned __int64 >( String.length() ),
			NumBits
			);
	}

	if (String.empty())
		return;

	m_ByteStream.AddData( String.length(), String.data() );
}

void
ExoBuildBuffer::WriteCExoLocString(
	__in const std::string &String
	)
{
	//
	// We don't support STRREFs directly, we'll just always send a string.
	//

	WriteBOOL( false );
	WriteCExoString( String );
}

void
ExoBuildBuffer::WriteCExoLocString(
	__in const NWN::ExoLocString &String
	) throw( std::exception )
{
	WriteBOOL( String.IsStrRef );

	if (String.IsStrRef)
	{
		WriteBYTE( String.Flag, 1 );
		WriteDWORD( String.StrRef );
	}
	else
	{
		WriteCExoString( String.String );
	}
}


void
ExoBuildBuffer::WriteSmallString(
	__in const std::string &String,
	__in size_t NumBits /* = 8 */
	) throw( std::exception )
{
	if (String.length() > (1ui64 << NumBits))
		throw std::runtime_error( "String too long." );

	WriteUnsigned( String.length(), NumBits );

	if (String.empty())
		return;

	m_ByteStream.AddData( String.length(), String.data() );
}

void
ExoBuildBuffer::WriteSigned(
	__in signed __int64 FieldBits,
	__in size_t NumBits
	) throw( std::exception )
{
	m_BitStream.AddFieldBit( (FieldBits < 0) );
	WriteUnsigned( static_cast< unsigned __int64 >( FieldBits ), NumBits - 1 );
}

void
ExoBuildBuffer::WriteUnsigned(
	__in unsigned __int64 FieldBits,
	__in size_t NumBits
	) throw( std::exception )
{
	size_t Offset;

	if (NumBits == 64)
		return m_ByteStream.AddField( FieldBits );

	Offset    = NumBits;

	if (Offset >= 32)
	{
		unsigned long Bits = static_cast< unsigned long >( FieldBits >> (Offset - 32) );

		m_ByteStream.AddField( Bits );

		Offset -= 32;
	}

	if (Offset >= 16)
	{
		unsigned short Bits = static_cast< unsigned short >( FieldBits >> (Offset - 16) );

		m_ByteStream.AddField( Bits );

		Offset -= 16;
	}

	if (Offset >= 8)
	{
		unsigned char Bits = static_cast< unsigned char >( FieldBits >> (Offset - 8) );

		m_ByteStream.AddField( Bits );

		Offset -= 8;
	}

	if (Offset)
	{
		unsigned char Bits = static_cast< unsigned char >( FieldBits >> (         0) );

		m_BitStream.AddFieldBits( Offset, Bits );
	}
}

#ifdef _MSC_VER

#pragma warning(pop)

#endif
