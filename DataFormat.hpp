/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 * Contributions from  Jonas Bardino <bardino@nbi.ku.dk>
 *
 * @file
 * @author Troels Blum <troels@blum.dk>
 * @section LICENSE
 * This program is free software: you can redistribute it and/or modify
 *        it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *         but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 * Data format for packages and HDF5 file dumps.
 *
 */

#ifndef MULTIBLADEDATAHANDLER_DATAFORMAT_HPP
#define MULTIBLADEDATAHANDLER_DATAFORMAT_HPP

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <iostream>

/* NOTE: Data format version - bump on changes in struct format */
#define VERSION {1, 1, 0}
#define VERSIONPARTS (3)

/* Default to 1 MB buffer */
#define MAXBUFSIZE (1024*1024)

/* Max length of digitizer model string */
#define MAXMODELSIZE (8)

/* TODO: is this limit big enough - maybe switch to var len anyway? */
/* NOTE: We can't bump to say 1024 - it causes message too big on send */
/* MAX waveform array elements */
#define MAXWAVESAMPLES (512)

/* TODO: enabling these breaks optimize aggregates setup and send waveform */
/*
#define INCLUDE_SAMPLE2
#define INCLUDE_DSAMPLE
*/

#define UDP_LISTEN_ALL "*"
#define DEFAULT_UDP_SEND_ADDRESS "127.0.0.1"
#define DEFAULT_UDP_PORT "12345"

/* A simple helper to get current time since epoch in milliseconds */
#define getTimeMsecs() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

/** Every Data set contains meta data with the digitizer and format
 * followed by the actual List and/or waveform data points. */
namespace Data {
    /* Shared meta data for the entire data package */
    struct Meta { // 32 bytes
        uint64_t globalTime;
        uint64_t runStartTime;
        uint16_t digitizerID;
        uint16_t version[VERSIONPARTS];
        char digitizerModel[MAXMODELSIZE];
       /* No padding required */
    };
    namespace List {
        struct Element // 72 bit
        {
            uint32_t localTime;
            uint32_t adcValue;
            uint16_t extendTime;
            uint16_t channel;
            /* Pad to force 8-byte boundary alignment */
            uint8_t __pad[4];
        };
    }; // namespace List
    namespace Waveform {
        struct Element // semi-variable size
        {
            uint32_t localTime;
            uint16_t extendTime;
            uint16_t channel;
            uint16_t waveformLength;
            /* Pad to force 8-byte boundary alignment */
            uint8_t __pad[2];
            /* TODO: consider going back to variable size or packing
             * for less waveform overhead */
            uint16_t waveformSample1[MAXWAVESAMPLES];
#ifdef INCLUDE_SAMPLE2
            uint16_t waveformSample2[MAXWAVESAMPLES];
#endif
#ifdef INCLUDE_DSAMPLE
            uint8_t waveformDSample1[MAXWAVESAMPLES];
            uint8_t waveformDSample2[MAXWAVESAMPLES];
            uint8_t waveformDSample3[MAXWAVESAMPLES];
            uint8_t waveformDSample4[MAXWAVESAMPLES];
#endif
        };
    }; // namespace Waveform
    /* Wrapped up data with multiple events */
    /* Actual metadata, listevent and waveformevent contents are
     * appended right after EventData during setup to keep it as one big
     * contiguous chunk for sending. */
    struct EventData // 24 bytes
    {
        Meta *metadata;
        /* Raw buffer size */
        uint32_t allocatedSize;
        uint32_t checksum;
        uint16_t listEventsLength;
        uint16_t waveformEventsLength;
        List::Element *listEvents;
        Waveform::Element *waveformEvents;
    };
    /* Actual package with metadata and payload of element(s) */
    struct PackedEvents
    {
        void* data;
        size_t size;      // Allocated size
        size_t dataSize;  // Size of current data
    };


    /** @brief
        Translate the basic version arrays to a string.
     */
    inline std::string makeVersion(uint16_t version[]) {
        std::stringstream ss;
        for (int i=0; i<VERSIONPARTS; i++) {
            if (i > 0)
                ss << '.';
            ss << version[i];
        }
        return ss.str();
    }

    /** @brief
        Takes a pre-allocated data buffer and prepares it for EventData
        use with given number of list events and waveform events.
        User can then manually fill metadata, listEvents and
        waveformEvents with actual contents afterwards.
        The waveformEvents array has variable sized entries so it is
        saved last to avoid interference and it is left to the user to
        do the slicing.
    */
    inline EventData *setupEventData(void *data, uint32_t dataSize, uint32_t listEntries, uint32_t waveformEntries) {
        EventData *eventData = (EventData *)data;
        eventData->allocatedSize = dataSize;

        /* NOTE: We must make sure padding and layout on receiver
         * matches the sender. We could use boost::serialize but it comes
         * with significant overhead so we just assume platform
         * similarity for now. */
        /* TODO: implement better checksum for validation after transfer */
        eventData->checksum = sizeof(EventData) + sizeof(Meta) + sizeof(List::Element) * listEntries + sizeof(Waveform::Element) * waveformEntries;

        eventData->listEventsLength = listEntries;
        eventData->waveformEventsLength = waveformEntries;

        eventData->metadata = (Meta *)(eventData+1);
        /* Init version in probably zeroed-out metadata */
        uint16_t version[VERSIONPARTS] = VERSION;
        memcpy(eventData->metadata->version, version, sizeof(version[0])*VERSIONPARTS);

        eventData->listEvents = (List::Element *)(eventData->metadata+1);
        eventData->waveformEvents = nullptr; //(Waveform::Element *)(eventData->listEvents+listEntries);

        /* fuzzy out-of-bounds check */
        uint32_t bufSize = (char *)(eventData->waveformEvents + eventData->waveformEventsLength) - (char *)data;
        if (bufSize > dataSize) {
            std::stringstream ss;
            ss << "bufSize too large " << bufSize << " , cannot exceed " << dataSize;
            std::cerr << "ERROR: " << ss.str() << std::endl;
            throw std::runtime_error(ss.str());
        }
        return eventData;
    }

    inline PackedEvents packEventData(EventData *eventData)
    {
        PackedEvents packedEvents;
        packedEvents.data = (void *)eventData;
        packedEvents.size = eventData->allocatedSize;
        packedEvents.dataSize = sizeof(EventData);
        packedEvents.dataSize += sizeof(Meta);
        packedEvents.dataSize += eventData->listEventsLength * sizeof(List::Element);
        packedEvents.dataSize += eventData->waveformEventsLength * sizeof(Waveform::Element);
        return packedEvents;
    }
    /** @brief
     * Unpacking is very much like setup but with extraction of actual
     * list and waveform event counts from EventData first. 
     */
    inline EventData *unpackEventData(PackedEvents packedEvents)
    {
        EventData *eventData = (EventData *)packedEvents.data;
        uint32_t listEventsLength = eventData->listEventsLength;
        uint32_t waveformEventsLength = eventData->waveformEventsLength;
        uint32_t checksum = eventData->checksum;
        eventData = setupEventData(packedEvents.data, packedEvents.dataSize,
                                   listEventsLength, waveformEventsLength);
        /* Make sure platform similarity assertion holds true */
        if (checksum != eventData->checksum) {
            std::stringstream ss;
            ss << "received eventData checksum does not match! " << checksum << " vs " << eventData->checksum;
            std::cerr << "ERROR: " << ss.str() << std::endl;
            throw std::runtime_error(ss.str());
        }
        return eventData;
    }
} // namespace Data
#endif //MULTIBLADEDATAHANDLER_DATAFORMAT_HPP
