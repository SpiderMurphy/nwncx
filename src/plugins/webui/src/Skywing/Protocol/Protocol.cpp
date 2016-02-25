#include "../Precomp.h"
#include "../NetBuffer/NetBuffer.h"
#include "Protocol.h"

#ifdef _MSC_VER

#pragma warning(push)
#pragma warning(disable:4290) // warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)

#endif

using NWN::ExoParseBuffer;
using NWN::ExoBuildBuffer;
using NWN::ProtocolMessage;

ProtocolMessage::ProtocolMessage(
	__in const void *Data,
	__in size_t Length
	) throw( std::exception )
	: m_Builder( NULL ),
	  m_Parser( NULL )
{
	size_t FragsOffset;
	size_t HeaderLength;

	//
	// If we have an empty message, then we'll not use the fragments offset
	// field (which may be absent, or worse, present and uninitialized).
	//

	if (Length <= EmptyMessageLength())
	{
		m_Header.FragmentsOffset = static_cast< unsigned long >( Length );

		FragsOffset  = static_cast< size_t >( m_Header.FragmentsOffset );
		HeaderLength = MinimumHeaderLength( );

		memcpy( &m_Header, Data, MinimumHeaderLength() );

		m_ByteStreamOffset = Length;
		m_ByteStreamLength = 0;
		m_BitStreamOffset  = Length;
		m_BitStreamLength  = 0;
	}
	else
	{
		memcpy( &m_Header, Data, sizeof( ProtocolHeader ) );

		FragsOffset  = static_cast< size_t >( m_Header.FragmentsOffset );
		HeaderLength = sizeof( ProtocolHeader );

		m_ByteStreamOffset = sizeof( ProtocolHeader );
		m_ByteStreamLength = FragsOffset - sizeof( ProtocolHeader );
		m_BitStreamOffset  = FragsOffset;
		m_BitStreamLength  = Length - FragsOffset;
	}

	m_Parser = new ExoParseBuffer(
		reinterpret_cast< const ProtocolHeader *>( Data ) + 1,
		FragsOffset - HeaderLength,
		reinterpret_cast< const unsigned char *>( Data ) + FragsOffset,
		Length - FragsOffset
		);

	//
	// If we have a fragment stream, then read the maximum bit offset.
	//

	if (Length - FragsOffset)
	{
		unsigned char MaxBitOffset;

		if (!m_Parser->ReadBYTE( MaxBitOffset, 3 ))
			throw std::runtime_error( "Failed to read last byte bit count." );

		//
		// Buggy server sends us 0 valid bits even though the first three are
		// always required if we have a byte of fragment stream.  Treat it as
		// though there were just those three bits.
		//

		//if (Length - FragsOffset == 1)
		//{
		//	if (MaxBitOffset < 3)
		//		MaxBitOffset = 3;
		//}

		//
		// If there are 8 valid bits in the last byte, that is masked to 0 as
		// there are only three bits in the field.  However, as there is never
		// legitimate cause for there to be trailing data following the
		// fragments stream, there is no ambiguity.
		//

		if (MaxBitOffset == 0)
			MaxBitOffset = 8;

		m_Parser->SetHighestValidBitPos( MaxBitOffset );
	}
}

ProtocolMessage::ProtocolMessage(
	__in CLS::CLSENUM Cls,
	__in CMD::MAJORENUM MajorFunction,
	__in unsigned char MinorFunction,
	__in size_t ByteStreamSizeHint /* = 32 */,
	__in size_t BitStreamSizeHint /* = 1 */
	) throw( std::exception )
	: m_Builder( NULL ),
	  m_Parser( NULL ),
	  m_ByteStreamOffset( 0 ),
	  m_ByteStreamLength( 0 ),
	  m_BitStreamOffset( 0 ),
	  m_BitStreamLength( 0 )
{
	//
	// Construct an ExoBuildBuffer to format the message data streams.
	//

	m_Builder = new ExoBuildBuffer( ByteStreamSizeHint, BitStreamSizeHint );

	//
	// Reserve space for the bit stream's bit offset.  It will be filled in
	// when we serialize the message.
	//

	m_Builder->WriteBYTE( 0, 3 );

	//
	// Initialize the message header.
	//

	m_Header.Cls             = static_cast< unsigned char >( Cls );
	m_Header.MajorFunction   = static_cast< unsigned char >( MajorFunction );
	m_Header.MinorFunction   = static_cast< unsigned char >( MinorFunction );
	m_Header.FragmentsOffset = sizeof( ProtocolHeader );
}

ProtocolMessage::~ProtocolMessage(
	)
{
	delete m_Builder;
}

#ifdef _MSC_VER

#pragma warning(pop)

#endif
