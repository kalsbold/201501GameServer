#include "stdafx.h"
#include "Exception.h"
#include "Log.h"
#include "PacketInterface.h"
#include "DummyClientSession.h"


#include "MyPacket.pb.h"


//@{ Handler Helper

typedef void(*HandlerFunc)(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream);

static HandlerFunc HandlerTable[MAX_PKT_TYPE];

static void DefaultHandler(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream)
{
	
	printf_s("Default Handler...PKT ID: %d\n", pktBase.mType);
}

struct InitializeHandlers
{
	InitializeHandlers()
	{
		for (int i = 0; i < MAX_PKT_TYPE; ++i)
			HandlerTable[i] = DefaultHandler;
	}
} _init_handlers_;

struct RegisterHandler
{
	RegisterHandler(int pktType, HandlerFunc handler)
	{
		HandlerTable[pktType] = handler;
	}
};

#define REGISTER_HANDLER(PKT_TYPE)	\
	static void Handler_##PKT_TYPE(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(DummyClientSession* session, PacketHeader& pktBase, protobuf::io::CodedInputStream& payloadStream)


//@}

void DummyClientSession::OnReceive(size_t len)
{
	
	/// ��Ŷ �Ľ��ϰ� ó��
	protobuf::io::ArrayInputStream arrayInputStream(mRecvBuffer.GetBufferStart(), mRecvBuffer.GetContiguiousBytes());
	protobuf::io::CodedInputStream codedInputStream(&arrayInputStream);

	PacketHeader packetheader;

	while (codedInputStream.ReadRaw(&packetheader, HEADER_SIZE))
	{
		const void* payloadPos = nullptr;
		int payloadSize = 0;

		codedInputStream.GetDirectBufferPointer(&payloadPos, &payloadSize);

		if ( payloadSize < packetheader.mSize ) ///< ��Ŷ ��ü ������ üũ
			break;

		if (packetheader.mType >= MAX_PKT_TYPE || packetheader.mType <= 0)
		{
			DisconnectRequest(DR_ACTIVE);
			break;;
		}

		/// payload �б�
		protobuf::io::ArrayInputStream payloadArrayStream(payloadPos, packetheader.mSize);
		protobuf::io::CodedInputStream payloadInputStream(&payloadArrayStream);

		/// packet dispatch...
		HandlerTable[packetheader.mType](this, packetheader, payloadInputStream);
	
		/// ���� ��ŭ ���� �� ���ۿ��� ����
		codedInputStream.Skip(packetheader.mSize); ///< readraw���� ��� ũ�⸸ŭ �̸� �����߱⶧��
		mRecvBuffer.Remove(HEADER_SIZE + packetheader.mSize);

	}
}

/////////////////////////////////////////////////////////////

using namespace MyPacket;

REGISTER_HANDLER(PKT_SC_LOGIN)
{
	LoginResult loginResult;
	if (false == loginResult.ParseFromCodedStream(&payloadStream))
	{
		session->DisconnectRequest(DR_ACTIVE);
		return;
	}

	const Position& pos = loginResult.playerpos();

	session->UpdatePlayer(loginResult.playerid(), loginResult.playername(), pos.x(), pos.y(), pos.z());

	printf_s("LOGIN SUCCESS: ID[%d], NAME[%s], POS[%f, %f, %f]\n", loginResult.playerid(), loginResult.playername().c_str(), pos.x(), pos.y(), pos.z());

	//test�� ���� move
	session->move();
}

REGISTER_HANDLER(PKT_SC_MOVE)
{
	MoveResult moveResult;
	if (false == moveResult.ParseFromCodedStream(&payloadStream))
	{
		session->DisconnectRequest(DR_ACTIVE);
		return;
	}

	printf_s("player [%d] move to POS[%f, %f, %f]\n", session->GetPlayerId(), session->GetX(), session->GetY(), session->GetZ());

	const Position& pos = moveResult.playerpos();

	session->UpdatePlayerPos(pos.x(), pos.y(), pos.z());

	//test�� ���� move, chat
	session->move();
	if (rand() % 10)
	{
		session->chat();
	}
}

REGISTER_HANDLER(PKT_SC_CHAT)
{
	ChatResult chatResult;

	if (false == chatResult.ParseFromCodedStream(&payloadStream))
	{
		session->DisconnectRequest(DR_ACTIVE);
		return;
	}

	printf_s("player [%s] says [%s]\n", chatResult.playername().c_str(), chatResult.playermessage().c_str());
}