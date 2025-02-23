/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/engine.h>

#include "config.h"
#include "console.h"
#include "network.h"
#include "huffman.h"


static void ConchainDbgLognetwork(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	((CNetBase *)pUserData)->UpdateLogHandles();
}

static int GetHeaderSize(CNetConnection *pConnection)
{
	return (pConnection && (pConnection->Protocol() == NETPROTOCOL_SIX)) ? 4 : 6;
}

void CNetRecvUnpacker::Clear()
{
	m_Valid = false;
}

void CNetRecvUnpacker::Start(const NETADDR *pAddr, CNetConnection *pConnection, int ClientID)
{
	m_Addr = *pAddr;
	m_pConnection = pConnection;
	m_ClientID = ClientID;
	m_CurrentChunk = 0;
	m_Valid = true;
}

// TODO: rename this function
int CNetRecvUnpacker::FetchChunk(CNetChunk *pChunk)
{
	// Don't bother with connections that already went offline
	if(m_pConnection && m_pConnection->State() != NET_CONNSTATE_ONLINE)
	{
		Clear();
		return 0;
	}

	CNetChunkHeader Header;
	unsigned char *pEnd = m_Data.m_aChunkData + m_Data.m_DataSize;

	while(1)
	{
		unsigned char *pData = m_Data.m_aChunkData;

		// check for old data to unpack
		if(!m_Valid || m_CurrentChunk >= m_Data.m_NumChunks)
		{
			Clear();
			return 0;
		}

		// TODO: add checking here so we don't read too far
		for(int i = 0; i < m_CurrentChunk; i++)
		{
			pData = Header.Unpack(pData, GetHeaderSize(m_pConnection));
			pData += Header.m_Size;
		}

		// unpack the header
		pData = Header.Unpack(pData, GetHeaderSize(m_pConnection));
		m_CurrentChunk++;

		if(pData+Header.m_Size > pEnd)
		{
			Clear();
			return 0;
		}

		// handle sequence stuff
		if(m_pConnection && (Header.m_Flags&NET_CHUNKFLAG_VITAL))
		{
			if(Header.m_Sequence == (m_pConnection->m_Ack+1)%NET_MAX_SEQUENCE)
			{
				// in sequence
				m_pConnection->m_Ack = (m_pConnection->m_Ack+1)%NET_MAX_SEQUENCE;
			}
			else
			{
				// old packet that we already got
				if(m_pConnection->IsSeqInBackroom(Header.m_Sequence, m_pConnection->m_Ack))
					continue;

				// out of sequence, request resend
				if(m_pConnection->Config()->m_Debug)
					dbg_msg("conn", "asking for resend %d %d", Header.m_Sequence, (m_pConnection->m_Ack+1)%NET_MAX_SEQUENCE);
				m_pConnection->SignalResend();
				continue; // take the next chunk in the packet
			}
		}

		// fill in the info
		pChunk->m_ClientID = m_ClientID;
		pChunk->m_Address = m_Addr;
		pChunk->m_Flags = (Header.m_Flags&NET_CHUNKFLAG_VITAL) ? NETSENDFLAG_VITAL : 0;
		pChunk->m_DataSize = Header.m_Size;
		pChunk->m_pData = pData;
		return 1;
	}
}
CNetBase::CNetInitializer CNetBase::m_NetInitializer;

CNetBase::CNetBase()
{
	net_invalidate_socket(&m_Socket);
	m_pConfig = 0;
	m_pEngine = 0;
	m_DataLogSent = 0;
	m_DataLogRecv = 0;
}

CNetBase::~CNetBase()
{
	if(m_Socket.type != NETTYPE_INVALID)
		Shutdown();
}

void CNetBase::Init(NETSOCKET Socket, CConfig *pConfig, IConsole *pConsole, IEngine *pEngine)
{
	m_Socket = Socket;
	m_pConfig = pConfig;
	m_pEngine = pEngine;
	m_Huffman.Init();
	mem_zero(m_aRequestTokenBuf, sizeof(m_aRequestTokenBuf));
	if(pEngine)
		pConsole->Chain("dbg_lognetwork", ConchainDbgLognetwork, this);
}

void CNetBase::Shutdown()
{
	net_udp_close(m_Socket);
	net_invalidate_socket(&m_Socket);
}

void CNetBase::Wait(int Time)
{
	net_socket_read_wait(m_Socket, Time);
}

// packs the data tight and sends it
void CNetBase::SendPacketConnless(const NETADDR *pAddr, TOKEN Token, TOKEN ResponseToken, const void *pData, int DataSize)
{
	unsigned char aBuffer[NET_MAX_PACKETSIZE];

	dbg_assert(DataSize <= NET_MAX_PAYLOAD, "packet data size too high");
	dbg_assert((Token&~NET_TOKEN_MASK) == 0, "token out of range");
	dbg_assert((ResponseToken&~NET_TOKEN_MASK) == 0, "resp token out of range");

	int i = 0;
	aBuffer[i++] = ((NET_PACKETFLAG_CONNLESS<<2)&0xfc) | (NET_PACKETVERSION&0x03); // connless flag and version
	aBuffer[i++] = (Token>>24)&0xff; // token
	aBuffer[i++] = (Token>>16)&0xff;
	aBuffer[i++] = (Token>>8)&0xff;
	aBuffer[i++] = (Token)&0xff;
	aBuffer[i++] = (ResponseToken>>24)&0xff; // response token
	aBuffer[i++] = (ResponseToken>>16)&0xff;
	aBuffer[i++] = (ResponseToken>>8)&0xff;
	aBuffer[i++] = (ResponseToken)&0xff;

	dbg_assert(i == NET_PACKETHEADERSIZE_CONNLESS, "inconsistency");

	mem_copy(&aBuffer[i], pData, DataSize);
	net_udp_send(m_Socket, pAddr, aBuffer, i+DataSize);
}

void CNetBase::SendPacket(const NETADDR *pAddr, CNetPacketConstruct *pPacket, int Protocol)
{
	unsigned char aBuffer[NET_MAX_PACKETSIZE];
	int CompressedSize = -1;
	int FinalSize = -1;

	// log the data
	if(m_DataLogSent)
	{
		int Type = 1;
		io_write(m_DataLogSent, &Type, sizeof(Type));
		io_write(m_DataLogSent, &pPacket->m_DataSize, sizeof(pPacket->m_DataSize));
		io_write(m_DataLogSent, &pPacket->m_aChunkData, pPacket->m_DataSize);
		io_flush(m_DataLogSent);
	}

	dbg_assert((pPacket->m_Token&~NET_TOKEN_MASK) == 0, "token out of range");

	int HeaderSize = (Protocol == NETPROTOCOL_SIX) ? 3 : NET_PACKETHEADERSIZE;
	// compress if not ctrl msg
	if(!(pPacket->m_Flags&NET_PACKETFLAG_CONTROL))
		CompressedSize = m_Huffman.Compress(pPacket->m_aChunkData, pPacket->m_DataSize, &aBuffer[HeaderSize], NET_MAX_PAYLOAD);

	// check if the compression was enabled, successful and good enough
	if(CompressedSize > 0 && CompressedSize < pPacket->m_DataSize)
	{
		FinalSize = CompressedSize;
		pPacket->m_Flags |= NET_PACKETFLAG_COMPRESSION;
	}
	else
	{
		// use uncompressed data
		FinalSize = pPacket->m_DataSize;
		mem_copy(&aBuffer[HeaderSize], pPacket->m_aChunkData, pPacket->m_DataSize);
		pPacket->m_Flags &= ~NET_PACKETFLAG_COMPRESSION;
	}
		
	if(Protocol == NETPROTOCOL_SIX)
	{
		unsigned Flags = 0;
		if(pPacket->m_Flags & NET_PACKETFLAG_CONTROL)
			Flags |= 4;
		if(pPacket->m_Flags & NET_PACKETFLAG_CONNLESS)
			Flags |= 8;
		if(pPacket->m_Flags & NET_PACKETFLAG_RESEND)
			Flags |= 16;
		if(pPacket->m_Flags & NET_PACKETFLAG_COMPRESSION)
			Flags |= 32;
		pPacket->m_Flags = Flags;
	}

	// set header and send the packet if all things are good
	if(FinalSize >= 0)
	{
		FinalSize += HeaderSize;

		int i = 0;
		aBuffer[i++] = ((pPacket->m_Flags<<2)&0xfc) | ((pPacket->m_Ack>>8)&0x03); // flags and ack
		aBuffer[i++] = (pPacket->m_Ack)&0xff; // ack
		if(Protocol == NETPROTOCOL_SEVEN)
		{
			aBuffer[i++] = (pPacket->m_NumChunks)&0xff; // num chunks
			aBuffer[i++] = (pPacket->m_Token>>24)&0xff; // token
			aBuffer[i++] = (pPacket->m_Token>>16)&0xff;
			aBuffer[i++] = (pPacket->m_Token>>8)&0xff;
			aBuffer[i++] = (pPacket->m_Token)&0xff;
		}
		else
		{
			aBuffer[i++] = pPacket->m_NumChunks; // num chunks
		}
		
		dbg_assert(i == HeaderSize, "inconsistency");

		net_udp_send(m_Socket, pAddr, aBuffer, FinalSize);

		// log raw socket data
		if(m_DataLogSent)
		{
			int Type = 0;
			io_write(m_DataLogSent, &Type, sizeof(Type));
			io_write(m_DataLogSent, &FinalSize, sizeof(FinalSize));
			io_write(m_DataLogSent, aBuffer, FinalSize);
			io_flush(m_DataLogSent);
		}
	}
}

static const unsigned char NET_HEADER_EXTENDED[] = {'x', 'e'};
// TODO: rename this function
int CNetBase::UnpackPacket(NETADDR *pAddr, unsigned char *pBuffer, CNetPacketConstruct *pPacket, int& Protocol, int *pSize)
{
	int Size = net_udp_recv(m_Socket, pAddr, pBuffer, NET_MAX_PACKETSIZE);
	
	// no more packets for now
	if(Size <= 0)
		return 1;

	int Success = UnpackPacket(pBuffer, Size, pPacket, Protocol);
	if(pSize)
	{
		*pSize = Size;
	}
	// return success
	return Success;
}

int CNetBase::UnpackPacket(unsigned char *pBuffer, int Size, CNetPacketConstruct *pPacket, int& Protocol)
{
	// log the data
	if(m_DataLogRecv)
	{
		int Type = 0;
		io_write(m_DataLogRecv, &Type, sizeof(Type));
		io_write(m_DataLogRecv, &Size, sizeof(Size));
		io_write(m_DataLogRecv, pBuffer, Size);
		io_flush(m_DataLogRecv);
	}

	// check the size
	if(Size < 3 || Size > NET_MAX_PACKETSIZE)
	{
		if(m_pConfig->m_Debug)
			dbg_msg("network", "packet too small, size=%d", Size);
		return -1;
	}

	// read the packet
	pPacket->m_Flags = (pBuffer[0]&0xfc)>>2;
		// FFFFFFxx
	
	if(pPacket->m_Flags&NET_PACKETFLAG_CONNLESS)
	{
		if(Protocol == NETPROTOCOL_UNKNOWN)
			Protocol = ((pBuffer[0] & 0x3) == 1) ? NETPROTOCOL_SEVEN : NETPROTOCOL_SIX;
		int Offset = (Protocol == NETPROTOCOL_SIX) ? 6 : NET_PACKETHEADERSIZE_CONNLESS;
		if(Size < Offset)
		{
			if(m_pConfig->m_Debug)
				dbg_msg("net", "connless packet too small, size=%d", Size);
			return -1;
		}

		pPacket->m_Flags = NET_PACKETFLAG_CONNLESS;
		pPacket->m_Ack = 0;
		pPacket->m_NumChunks = 0;
		int Version = pBuffer[0]&0x3;
			// xxxxxxVV

		if((Protocol == NETPROTOCOL_SEVEN) && Version != NET_PACKETVERSION)
			return -1;

		pPacket->m_DataSize = Size - Offset;
		if(Protocol == NETPROTOCOL_SEVEN)
		{
			pPacket->m_Token = (pBuffer[1] << 24) | (pBuffer[2] << 16) | (pBuffer[3] << 8) | pBuffer[4];
				// TTTTTTTT TTTTTTTT TTTTTTTT TTTTTTTT
			pPacket->m_ResponseToken = (pBuffer[5]<<24) | (pBuffer[6]<<16) | (pBuffer[7]<<8) | pBuffer[8];
				// RRRRRRRR RRRRRRRR RRRRRRRR RRRRRRRR
		}
		else
		{
			pPacket->m_ResponseToken = NET_TOKEN_NONE;
		}
		mem_copy(pPacket->m_aChunkData, &pBuffer[Offset], pPacket->m_DataSize);
		
		if((Protocol == NETPROTOCOL_SIX) && mem_comp(pBuffer, NET_HEADER_EXTENDED, sizeof(NET_HEADER_EXTENDED)) == 0)
		{
			pPacket->m_Flags |= NET_PACKETFLAG_EXTENDED;
			mem_copy(pPacket->m_aExtraData, pBuffer + sizeof(NET_HEADER_EXTENDED), sizeof(pPacket->m_aExtraData));
		}
	}
	else
	{
		if(Protocol == NETPROTOCOL_UNKNOWN)
		{
			if(pPacket->m_Flags&NET_PACKETFLAG_CONTROL)
				Protocol = NETPROTOCOL_SEVEN;
			else
				Protocol = NETPROTOCOL_SIX;
		}

		int DataStart = (Protocol == NETPROTOCOL_SIX) ? 3 : NET_PACKETHEADERSIZE;
		if((Protocol == NETPROTOCOL_SIX) && Size < DataStart)
		{
			return -1;
		}
		
		if(Size - NET_PACKETHEADERSIZE > NET_MAX_PAYLOAD)
		{
			if(m_pConfig->m_Debug)
				dbg_msg("network", "packet payload too big, size=%d", Size);
			return -1;
		}
		
		if(Protocol == NETPROTOCOL_SIX)
		{
			unsigned Flags = 0;
			if(pPacket->m_Flags & 4)
				Flags |= NET_PACKETFLAG_CONTROL;
			if(pPacket->m_Flags & 8)
				Flags |= NET_PACKETFLAG_CONNLESS;
			if(pPacket->m_Flags & 16)
				Flags |= NET_PACKETFLAG_RESEND;
			if(pPacket->m_Flags & 32)
				Flags |= NET_PACKETFLAG_COMPRESSION;
			pPacket->m_Flags = Flags;
			pPacket->m_Token = NET_TOKEN_NONE;
		}

		pPacket->m_Ack = ((pBuffer[0]&0x3)<<8) | pBuffer[1];
			// xxxxxxAA AAAAAAAA
		pPacket->m_NumChunks = pBuffer[2];
			// NNNNNNNN

		pPacket->m_DataSize = Size - DataStart;

		if(Protocol == NETPROTOCOL_SEVEN)
			pPacket->m_Token = (pBuffer[3] << 24) | (pBuffer[4] << 16) | (pBuffer[5] << 8) | pBuffer[6];
				// TTTTTTTT TTTTTTTT TTTTTTTT TTTTTTTT
		pPacket->m_ResponseToken = NET_TOKEN_NONE;

		if(pPacket->m_Flags&NET_PACKETFLAG_COMPRESSION)
			pPacket->m_DataSize = m_Huffman.Decompress(&pBuffer[DataStart], pPacket->m_DataSize, pPacket->m_aChunkData, sizeof(pPacket->m_aChunkData));
		else
			mem_copy(pPacket->m_aChunkData, &pBuffer[DataStart], pPacket->m_DataSize);
	}

	// check for errors
	if(pPacket->m_DataSize < 0)
	{
		if(m_pConfig->m_Debug)
			dbg_msg("network", "error during packet decoding");
		return -1;
	}

	// set the response token (a bit hacky because this function shouldn't know about control packets)
	if((Protocol == NETPROTOCOL_SEVEN) && pPacket->m_Flags&NET_PACKETFLAG_CONTROL)
	{
		if(pPacket->m_DataSize >= 5) // control byte + token
		{
			if(pPacket->m_aChunkData[0] == NET_CTRLMSG_CONNECT
				|| pPacket->m_aChunkData[0] == NET_CTRLMSG_TOKEN)
			{
				pPacket->m_ResponseToken = (pPacket->m_aChunkData[1]<<24) | (pPacket->m_aChunkData[2]<<16)
					| (pPacket->m_aChunkData[3]<<8) | pPacket->m_aChunkData[4];
			}
		}
	}

	// log the data
	if(m_DataLogRecv)
	{
		int Type = 1;
		io_write(m_DataLogRecv, &Type, sizeof(Type));
		io_write(m_DataLogRecv, &pPacket->m_DataSize, sizeof(pPacket->m_DataSize));
		io_write(m_DataLogRecv, pPacket->m_aChunkData, pPacket->m_DataSize);
		io_flush(m_DataLogRecv);
	}

	return 0;
}

void CNetBase::SendControlMsg(const NETADDR *pAddr, TOKEN Token, int Ack, int ControlMsg, const void *pExtra, int ExtraSize, int Protocol)
{
	CNetPacketConstruct Construct;
	Construct.m_Token = Token;
	Construct.m_Flags = NET_PACKETFLAG_CONTROL;
	Construct.m_Ack = Ack;
	Construct.m_NumChunks = 0;
	Construct.m_DataSize = 1+ExtraSize;
	Construct.m_aChunkData[0] = ControlMsg;
	if(ExtraSize > 0)
		mem_copy(&Construct.m_aChunkData[1], pExtra, ExtraSize);

	// send the control message
	SendPacket(pAddr, &Construct, Protocol);
}


void CNetBase::SendControlMsgWithToken(const NETADDR *pAddr, TOKEN Token, int Ack, int ControlMsg, TOKEN MyToken, bool Extended)
{
	dbg_assert((Token&~NET_TOKEN_MASK) == 0, "token out of range");
	dbg_assert((MyToken&~NET_TOKEN_MASK) == 0, "resp token out of range");

	m_aRequestTokenBuf[0] = (MyToken>>24)&0xff;
	m_aRequestTokenBuf[1] = (MyToken>>16)&0xff;
	m_aRequestTokenBuf[2] = (MyToken>>8)&0xff;
	m_aRequestTokenBuf[3] = (MyToken)&0xff;
	SendControlMsg(pAddr, Token, 0, ControlMsg, m_aRequestTokenBuf, Extended ? sizeof(m_aRequestTokenBuf) : 4, NETPROTOCOL_SEVEN);
}

unsigned char *CNetChunkHeader::Pack(unsigned char *pData, int Split)
{
	pData[0] = ((m_Flags&0x03)<<6) | ((m_Size>>Split)&0x3F);
	pData[1] = (m_Size&((1<<Split)-1));
	if(m_Flags&NET_CHUNKFLAG_VITAL)
	{
		pData[1] |= (m_Sequence>>2)&(~((1<<Split)-1));
		pData[2] = m_Sequence&0xFF;
		return pData + 3;
	}
	return pData + 2;
}

unsigned char *CNetChunkHeader::Unpack(unsigned char *pData, int Split)
{
	m_Flags = (pData[0]>>6)&0x03;
	m_Size = ((pData[0]&0x3F)<<Split) | (pData[1]&((1<<Split)-1));
	m_Sequence = -1;
	if(m_Flags&NET_CHUNKFLAG_VITAL)
	{
		m_Sequence = ((pData[1]&(~((1<<Split)-1)))<<2) | pData[2];
		return pData + 3;
	}
	return pData + 2;
}

void CNetBase::UpdateLogHandles()
{
	if(Engine())
		Engine()->QueryNetLogHandles(&m_DataLogSent, &m_DataLogRecv);
}
