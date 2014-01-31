#import <CoreFoundation/CoreFoundation.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <map>

#include "HIDManager.h"

#include "backend.h"
#include "protocol.h"


static pthread_t thread;
static CFRunLoopRef runLoop;
static CFSocketRef _socket;
static CFRunLoopSourceRef _socketSource;
static int sockets[2] = { -1, -1 };

static void HandleSocketEvent(CFSocketRef s, CFSocketCallBackType callbackType,
                              CFDataRef address, const void *data, void *info)
{
    printf("WHATA\n");
}


static void* ManagerThread(void* aUnused)
{
    runLoop = CFRunLoopGetCurrent();

    _socket = CFSocketCreateWithNative(0, sockets[0], kCFSocketDataCallBack, HandleSocketEvent, 0);        
    _socketSource = CFSocketCreateRunLoopSource(0, _socket, 0);

    HIDManager::StartUp();    
    CFRunLoopRun();
    HIDManager::ShutDown();
    
    return 0;
}

int HACKStart()
{
    if (!thread)
    {
        socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets);
        pthread_create(&thread, 0, ManagerThread, 0);
    }
    
    return sockets[1];
}

void SendProtocolMessage(const void* aData, uint32_t aSize)
{
    write(sockets[0], aData, aSize);
}

namespace MFiWrapperBackend {

uint32_t nextHandle = 1;
std::map<HIDPad::Interface*, uint32_t> devices;

void AttachController(HIDPad::Interface* aInterface)
{
    if (devices.find(aInterface) == devices.end())
    {
        uint32_t handle = nextHandle ++;
        devices[aInterface] = handle;
        
        MFiWDataPacket pkt;
        pkt.Size = sizeof(MFiWDataPacket);
        pkt.Type = MFiWPacketConnect;
        pkt.Handle = handle;
        strlcpy(pkt.Connect.VendorName, "Test", sizeof(pkt.Connect.VendorName));
        pkt.Connect.PresentControls = 0xFFFFFFFF;
        pkt.Connect.AnalogControls = 0;
        write(sockets[0], &pkt, pkt.Size);
    }
}

void DetachController(HIDPad::Interface* aInterface)
{
    std::map<HIDPad::Interface*, uint32_t>::iterator device;
    if ((device = devices.find(aInterface)) != devices.end())
    {
        uint32_t handle = device->second;
        devices.erase(device);
        
        MFiWDataPacket pkt;
        pkt.Size = sizeof(MFiWDataPacket);
        pkt.Type = MFiWPacketDisconnect;
        pkt.Handle = handle;
        write(sockets[0], &pkt, pkt.Size);
    }
}

void SendControllerState(HIDPad::Interface* aInterface, const float aData[32])
{
    std::map<HIDPad::Interface*, uint32_t>::iterator device;
    if ((device = devices.find(aInterface)) != devices.end())
    {
        uint32_t handle = device->second;
        
        MFiWDataPacket pkt;
        pkt.Size = sizeof(MFiWDataPacket);
        pkt.Type = MFiWPacketInputState;
        pkt.Handle = handle;
        memcpy(pkt.State.Data, aData, sizeof(pkt.State.Data));
        write(sockets[0], &pkt, pkt.Size);
    }
}

}
