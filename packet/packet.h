#define PATCH_REQUEST_PACKET_TYPE 1

class packet
{
public:
    uint16_t packetTypeID;

    void serializePacket(char buffer)
    {
        uint16_t ptid = htons(packetTypeID);
        memcpy(&buffer, &ptid, 2);
    }
    void unserializePacket(char* buffer)
    {
    }
};
