/*
 *  REACDataStream.h
 *  REAC
 *
 *  Created by Per Eckerdal on 15/03/2011.
 *  Copyright 2011 Per Eckerdal. All rights reserved.
 *
 */

#ifndef _REACDATASTREAM_H
#define _REACDATASTREAM_H

#include <libkern/OSTypes.h>
#include <libkern/c++/OSObject.h>
#include <libkern/c++/OSArray.h>
#include <IOKit/IOReturn.h>

#include "REACConstants.h"

#define REACPacketHeader        com_pereckerdal_driver_REACPacketHeader
#define REACSplitUnit           com_pereckerdal_driver_REACSplitUnit
#define REACDataStream          com_pereckerdal_driver_REACDataStream
#define REACDeviceInfo          com_pereckerdal_driver_REACDeviceInfo

class  com_pereckerdal_driver_REACConnection;
struct com_pereckerdal_driver_EthernetHeader;

struct REACDeviceInfo {
    UInt8 addr[ETHER_ADDR_LEN];
    UInt32 in_channels;
    UInt32 out_channels;
};

/* REAC packet header */
struct REACPacketHeader {
    UInt8 counter[2];
    UInt8 type[2];
    UInt8 data[32];
    
    UInt16 getCounter() {
        UInt16 ret = counter[0];
        ret += ((UInt16) counter[1]) << 8;
        return ret;
    }
    void setCounter(UInt16 c) {
        counter[0] = c;
        counter[1] = c >> 8;
    }
};

// Represents one connected REAC_SPLIT device
class REACSplitUnit : public OSObject {
    OSDeclareFinalStructors(REACSplitUnit);
    
public:
    virtual bool initAddress(UInt64 lastHeardFrom, UInt8 identifier, UInt32 addrLen, const UInt8 *addr);
    static REACSplitUnit *withAddress(UInt64 lastHeardFrom, UInt8 identifier, UInt32 addrLen, const UInt8 *addr);
    
    UInt64 lastHeardFrom;
    UInt8  identifier;
    UInt8  address[ETHER_ADDR_LEN];
};

// Handles the data stream part of a REAC stream (both input and output).
// Each REAC connection is supposed to have one of these objects.
//
// This class is not thread safe.
//
// TODO Private constructor/assignment operator/destructor?
class REACDataStream : public OSObject {
    OSDeclareDefaultStructors(REACDataStream)
    
    struct MasterAnnouncePacket {
        UInt8 unknown1[9];
        UInt8 address[ETHER_ADDR_LEN];
        UInt8 inChannels;
        UInt8 outChannels;
        UInt8 unknown2[4];
    };
    
    struct SplitAnnounceResponsePacket {
        UInt8 unknown1[9];
        UInt8 address[ETHER_ADDR_LEN];
        UInt8 unknown2;
        UInt8 identifierAssignment;
    };
    
    enum REACStreamType {
        REAC_STREAM_FILLER = 0,
        REAC_STREAM_CONTROL = 1,
        REAC_STREAM_MASTER_ANNOUNCE = 2,
        REAC_STREAM_SPLIT_ANNOUNCE = 3
    };
    
    static const UInt8 REAC_SPLIT_ANNOUNCE_FIRST[];
    static const UInt8 STREAM_TYPE_IDENTIFIERS[][2];
    
public:
    
    virtual bool initConnection(com_pereckerdal_driver_REACConnection *conn);
    static REACDataStream *withConnection(com_pereckerdal_driver_REACConnection *conn);
    
protected:
    
    // Object destruction method that is used by free, and init on failure.
    virtual void deinit();
    virtual void free();
    
public:

    void gotPacket(const REACPacketHeader *packet, const com_pereckerdal_driver_EthernetHeader *header);
    // Return kIOReturnSuccess on success, kIOReturnAborted if no packet should be sent, and anything else on error.
    IOReturn processPacket(REACPacketHeader *packet, UInt32 dhostLen, UInt8 *dhost);
    
    // Returns true if a packet should be sent
    bool prepareSplitAnnounce(REACPacketHeader *packet);
    
protected:
    
    com_pereckerdal_driver_REACConnection *connection;
    UInt64    lastAnnouncePacket; // The counter of the last announce counter packet
    UInt64    recievedPacketCounter;
    UInt64    counter;
    
    // Cdea state (used by both REAC_SLAVE and REAC_MASTER)
    UInt8     lastCdeaTwoBytes[2];
    SInt32    packetsUntilNextCdea;
    SInt32    cdeaState;
    SInt32    cdeaPacketsSinceStateChange;
    SInt32    cdeaAtChannel;     // Used when writing the cdea channel info packets
    SInt32    cdeaCurrentOffset; // Used when writing the cdea filler packets
    
    // REAC_SPLIT state
    enum SplitHandshakeState {
        HANDSHAKE_NOT_INITIATED,
        HANDSHAKE_GOT_MASTER_ANNOUNCE,
        HANDSHAKE_SENT_FIRST_ANNOUNCE,
        HANDSHAKE_GOT_SECOND_MASTER_ANNOUNCE,
        HANDSHAKE_CONNECTED
    };
    SplitHandshakeState splitHandshakeState;
    REACDeviceInfo      splitMasterDevice;
    UInt8               splitIdentifier;
    UInt64              counterAtLastSplitAnnounce;
    
    // REAC_MASTER state
    enum GotSplitAnnounceState {
        GOT_SPLIT_NOT_INITIATED,
        GOT_SPLIT_ANNOUNCE,
        GOT_SPLIT_SENT_SPLIT_ANNOUNCE_RESPONSE
    };
    OSArray               *splitUnits;
    GotSplitAnnounceState  masterGotSplitAnnounceState;
    UInt8                  masterSplitAnnounceAddr[ETHER_ADDR_LEN];
    
    bool updateLastHeardFromSplitUnit(const com_pereckerdal_driver_EthernetHeader *header, UInt32 addrLen, const UInt8 *addr);
    IOReturn splitUnitConnected(UInt8 identifier, UInt32 addrLen, const UInt8 *addr);
    void disconnectObsoleteSplitUnits();
    
    static bool checkChecksum(const REACPacketHeader *packet);
    static UInt8 applyChecksum(REACPacketHeader *packet);
};


#endif
