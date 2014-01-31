/*  MFiWrapper
 *  Copyright (C) 2014 - Jason Fetters
 * 
 *  MFiWrapper is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  MFiWrapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with MFiWrapper.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"

namespace MFiWrapperCommon {

static const uint32_t PACKET_HEADER = 12;

Connection::Connection(int aDescriptor) :
    Descriptor(aDescriptor),
    Socket(0),
    Source(0),
    Data((uint8_t*)&Packet),
    Position(0)
{
    // Make socket non-blocking
    fcntl(aDescriptor, F_SETFL, fcntl(aDescriptor, F_GETFL, 0) | O_NONBLOCK);

    //
    memset(&Packet, 0, sizeof(Packet));

    CFSocketContext ctx = { 0, this, 0, 0, 0 };
    Socket = CFSocketCreateWithNative(0, Descriptor, kCFSocketReadCallBack,
                                      Callback, &ctx);
    Source = CFSocketCreateRunLoopSource(0, Socket, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), Source, kCFRunLoopCommonModes);
}

Connection::~Connection()
{
    // TODO
    
    CFRunLoopSourceInvalidate(Source);
    CFRelease(Source);
    CFRelease(Socket);
}

void Connection::SendConnect(uint32_t aHandle, const MFiWConnectPacket* aData)
{
    MFiWDataPacket pkt;
    pkt.Size = PACKET_HEADER + sizeof(MFiWConnectPacket);
    pkt.Type = MFiWPacketConnect;
    pkt.Handle = aHandle;
    pkt.Connect = *aData;
    write(Descriptor, &pkt, pkt.Size);
}

void Connection::SendDisconnect(uint32_t aHandle)
{
    MFiWDataPacket pkt;
    pkt.Size = PACKET_HEADER;
    pkt.Type = MFiWPacketDisconnect;
    pkt.Handle = aHandle;
    write(Descriptor, &pkt, pkt.Size);
}

void Connection::SendInputState(uint32_t aHandle, const MFiWInputStatePacket* aData)
{
    MFiWDataPacket pkt;
    pkt.Size = PACKET_HEADER + sizeof(MFiWInputStatePacket);
    pkt.Type = MFiWPacketInputState;
    pkt.Handle = aHandle;
    pkt.State = *aData;
    write(Descriptor, &pkt, pkt.Size);
}

void Connection::SendStartDiscovery()
{
    MFiWDataPacket pkt;
    pkt.Size = PACKET_HEADER;
    pkt.Type = MFiWPacketStartDiscovery;
    write(Descriptor, &pkt, pkt.Size);
}

void Connection::SendStopDiscovery()
{
    MFiWDataPacket pkt;
    pkt.Size = PACKET_HEADER;
    pkt.Type = MFiWPacketStopDiscovery;
    write(Descriptor, &pkt, pkt.Size);
}

void Connection::SendSetPlayerIndex(uint32_t aHandle, int32_t aIndex)
{
    MFiWDataPacket pkt;
    pkt.Size = PACKET_HEADER + sizeof(MFiWPlayerIndexPacket);
    pkt.Type = MFiWPacketSetPlayerIndex;
    pkt.Handle = aHandle;
    pkt.PlayerIndex.Value = aIndex;
    write(Descriptor, &pkt, pkt.Size);
}

bool Connection::Read()
{
    unsigned targetSize = (Position < 4) ? 4 : Packet.Size;

    ssize_t result = read(Descriptor, &Data[Position], targetSize - Position);

    if (result <= 0)
        return false;
                          
    Position += result;
    assert(Position <= targetSize);
    
    return true;
}

void Connection::Parse()
{
    while (Read())
    {
        if (Position >= Packet.Size)
        {
            HandlePacket(&Packet);

            Position = 0;
            memset(&Packet, 0, sizeof(Packet));
        }
    }
}

void Connection::Callback(CFSocketRef s, CFSocketCallBackType callbackType,
                          CFDataRef address, const void *data, void *info)
{
    using namespace MFiWrapperCommon;

    Connection* connection = (Connection*)info;    
    connection->Parse();
}

}