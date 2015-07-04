﻿#include "HelloMessage.h"

// 请求：2版本 + S类型 + S名称 + 8本地时间 + 本地IP端口 + S支持加密算法列表
// 响应：2版本 + S类型 + S名称 + 8对方时间 + 对方IP端口 + S加密算法 + N密钥

// 初始化消息，各字段为0
HelloMessage::HelloMessage() : Ciphers(1), Key(16)
{
	Version		= Sys.Version;

	ushort code = __REV16(Sys.Code);
	ByteArray bs((byte*)&code, 2);
	Type = bs.ToHex('\0');
	//Name.Set(Sys.Company); 	// Sys.company 是一个字符串   在flash里面   Name.Clear() 会出错
	LocalTime	= Time.Current();
	Ciphers[0]	= 1;

	Reply		= false;
}

HelloMessage::HelloMessage(HelloMessage& msg) : Ciphers(1), Key(16)
{
	Version		= msg.Version;
	Type		= msg.Type;
	Name		= msg.Name;
	LocalTime	= msg.LocalTime;
	EndPoint	= msg.EndPoint;
	Ciphers		= msg.Ciphers;
	Key			= msg.Key;
	Reply		= msg.Reply;
}

// 从数据流中读取消息
bool HelloMessage::Read(Stream& ms)
{
	Version		= ms.Read<ushort>();

	ms.ReadString(Type.Clear());
	ms.ReadString(Name.Clear());

	LocalTime	= ms.Read<ulong>() / 10;

	EndPoint.Address = ms.ReadBytes(4);
	EndPoint.Port = ms.Read<ushort>();

	if(!Reply)
	{
		ms.ReadArray(Ciphers);
	}
	else
	{
		Ciphers[0]	= ms.Read<byte>();
		ms.ReadArray(Key);
	}

	return false;
}

// 把消息写入数据流中
void HelloMessage::Write(Stream& ms) const
{
	ms.Write(Version);

	ms.WriteString(Type);
	if(Name.Length() != 0)
		ms.WriteString(Name);
	else
	{
		String _name;
		_name.Set(Sys.Company);
		ms.WriteString(_name);
	}

	ms.Write(LocalTime * 10);

	ms.Write(EndPoint.Address.ToArray(), 0, 4);
	ms.Write((ushort)EndPoint.Port);

	if(!Reply)
	{
		ms.WriteArray(Ciphers);
	}
	else
	{
		ms.Write(Ciphers[0]);
		ms.WriteArray(Key);
	}
}

bool HelloMessage::Read(Message& msg)
{
	Stream ms(msg.Data, msg.Length);
	return Read(ms);
}

void HelloMessage::Write(Message& msg) const
{
	//Stream ms(msg.Data, ArrayLength(msg._Data));
	Stream ms(msg.Data, 256);

	Write(ms);

	msg.Length = ms.Position();
}

// 显示消息内容
String& HelloMessage::ToStr(String& str) const
{
	str += "握手";
	if(Reply) str += "#";
	str.Format(" Ver=%d.%d Type=%s Name=%s ", Version >> 8, Version & 0xFF, Type.GetBuffer(), Name.GetBuffer());
	DateTime dt;
	dt.Parse(LocalTime);
	//debug_printf("%s ", dt.ToString());
	str += dt.ToString();

	str += " ";
	str + EndPoint;

	str.Format(" Ciphers[%d]=", Ciphers.Length());
	for(int i=0; i<Ciphers.Length(); i++)
	{
		str.Format("%d ", Ciphers[i]);
	}

	if(Reply)
	{
		str.Format(" Key=");
		str += Key;
	}
	//debug_printf("\r\n");
	
	return str;
}
