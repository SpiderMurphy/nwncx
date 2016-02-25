#ifdef _MSC_VER

#pragma once

#pragma warning(push)
#pragma warning(disable:4290) // warning C4290: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)

#endif

#ifndef _NWNCONNLIB_PROTOCOL_PROTOCOL_H
#define _NWNCONNLIB_PROTOCOL_PROTOCOL_H

#endif

#include "Skywing/Precomp.h"

namespace NWN
{
	class ProtocolMessage
	{

	public:

		// Protocol enumerations removed.
		struct CMD
		{
			enum MAJORENUM
			{
				Chat                      = 0x09,
				ClientSideMessage         = 0x12,
				MaximumMajorFunction	  = 0x37
			};
		};

		struct CLS
		{
			enum CLSENUM
			{
				PlayerToServer = 0x70,
				ServerToPlayer = 0x50,
				AdminToServer  = 0x73,
				ServerToAdmin  = 0x53,

				MaximumClass   = 0xFF
			};
		};

		//
		// Receive constructor.  Note that the message header must have already
		// been validated by a call to ValidateProtocolHeader.
		//

		explicit ProtocolMessage(
			__in const void *Data,
			__in size_t Length
			) throw( std::exception );

		//
		// Send constructor.
		//

		explicit ProtocolMessage(
			__in CLS::CLSENUM Cls,
			__in CMD::MAJORENUM MajorFunction,
			__in unsigned char MinorFunction,
			__in size_t ByteStreamSizeHint = 32,
			__in size_t BitStreamSizeHint = 1
			) throw( std::exception );

		~ProtocolMessage( );

		//
		// Header field access.
		//

		inline		
		ProtocolMessage::CLS::CLSENUM
		ProtocolMessage::GetClass( ) const
		{
			switch (m_Header.Cls)
			{

			case CLS::PlayerToServer:
			case CLS::ServerToPlayer:
			case CLS::AdminToServer:
			case CLS::ServerToAdmin:
				return static_cast< CLS::CLSENUM >( m_Header.Cls );

			}

			return CLS::MaximumClass;
		}

		inline
		unsigned char
		GetMajorFunction( ) const
		{
			return m_Header.MajorFunction;
		}

		inline
		unsigned char
		GetMinorFunction( ) const
		{
			return m_Header.MinorFunction;
		}

		inline
		unsigned long
		GetBitStreamOffset( ) const
		{
			return m_Header.FragmentsOffset;
		}

		//
		// Builder / Parser access.  A send message has a builder, and a
		// receive message has a parser.
		//

		inline NWN::ExoBuildBuffer *GetBuilder( ) { return m_Builder; }
		inline NWN::ExoParseBuffer *GetParser( ) { return m_Parser; }

		//
		// Retrieve individual component buffers for a message.   The
		// components are assembled for transmission over the wire.  Note that
		// each component is a discrete buffer.
		//

		inline
		void
		GetSendStreamPointers(
			__out const void *&Header,
			__out size_t &HeaderLength,
			__out const void *&ByteStream,
			__out size_t &ByteStreamLength,
			__out const void *&BitStream,
			__out size_t &BitStreamLength
			)
		{
			unsigned char *ByteStreamPtr;
			unsigned char *BitStreamPtr;

			Header       = &m_Header;
			HeaderLength = sizeof( ProtocolHeader );

			m_Builder->GetBuffer(
				ByteStreamPtr,
				ByteStreamLength,
				BitStreamPtr,
				BitStreamLength
				);

			ByteStream = reinterpret_cast< const void * >( ByteStreamPtr );
			BitStream  = reinterpret_cast< const void * >( BitStreamPtr );

			//
			// Update the bit stream offset in the header.
			//

			if (HeaderLength > MinimumHeaderLength( ))
				m_Header.FragmentsOffset = (ULONG) (HeaderLength + ByteStreamLength);
		}

		//
		// Get stream offsets for a received message.
		//

		inline
		void
		GetRecvStreamPointers(
			__out size_t &ByteStreamOffset,
			__out size_t &ByteStreamLength,
			__out size_t &BitStreamOffset,
			__out size_t &BitStreamLength
			)
		{
			ByteStreamOffset = m_ByteStreamOffset;
			ByteStreamLength = m_ByteStreamLength;
			BitStreamOffset  = m_BitStreamOffset;
			BitStreamLength  = m_BitStreamLength;
		}

		//
		// Size of a raw message header.
		//

		static
		size_t
		MinimumHeaderLength( )
		{
			return 3;
		}

		//
		// Size of a raw message header an an (unused) fragments offset field.
		//

		static
		size_t
		EmptyMessageLength( )
		{
			return sizeof( ProtocolHeader );
		}

		static
		bool
		ValidateProtocolMessage(
			__in_bcount( MessageLength ) const void *Message,
			__in size_t MessageLength
			)
		{
			if (MessageLength < MinimumHeaderLength())
				return false;

			if ((MessageLength <  EmptyMessageLength()) &&
			    (MessageLength != MinimumHeaderLength()))
				return false;

			const ProtocolHeader *ProtoHeader;
			size_t                FragsOffset;

			ProtoHeader = reinterpret_cast< const ProtocolHeader *>( Message );
			FragsOffset = static_cast< size_t >( ProtoHeader->FragmentsOffset );

			//
			// If we are a message with no payload, then return here and now.
			// In this case, we leave the FragsOffset field uninitialized and
			// send heap contents from the server to the client (ugh).  We
			// need to account for this so that we can process these
			// questionably formed messages.
			//

			if ((MessageLength == MinimumHeaderLength()) ||
			    (MessageLength == EmptyMessageLength()))
				return true;

			//
			// The fragment data offset must not extend beyond the length of
			// the message.
			//

			if (FragsOffset > MessageLength)
				return false;

			//
			// The fragment data offset must not point to a data region that is
			// inside of the header.
			//

			if (FragsOffset < sizeof( ProtocolHeader ))
				return false;

			//
			// If we have one byte of fragment data, then ensure that the
			// minimum bit length is enough to contain the bit length of the
			// minimum bit length field itself.
			//

			unsigned char MaxBitPos = *(reinterpret_cast< const unsigned char *>( Message ) + FragsOffset) >> 5;

			if (MaxBitPos == 0)
				MaxBitPos = 8;

			if ((MessageLength - FragsOffset) == 1)
			{
				if (MaxBitPos < 3)
					return false;
			}

			return true;
		}

#include <pshpack1.h>
		struct ProtocolHeader
		{
			unsigned char Cls;
			unsigned char MajorFunction;
			unsigned char MinorFunction;
			unsigned long FragmentsOffset;
		};
#include <poppack.h>

	private:

		NWN::ExoBuildBuffer *m_Builder;
		NWN::ExoParseBuffer *m_Parser;

		ProtocolHeader       m_Header;
		size_t               m_ByteStreamOffset;
		size_t               m_ByteStreamLength;
		size_t               m_BitStreamOffset;
		size_t               m_BitStreamLength;

	};
}


#ifdef _MSC_VER

#pragma warning(pop)

#endif
