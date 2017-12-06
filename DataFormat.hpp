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

/* TODO: switch to a hdf5 struct */
#define EVENTFIELDS (3)
#define EVENTINIT {{0, 0, 0}}

#define DEFAULT_UDP_ADDRESS "127.0.0.1"
#define DEFAULT_UDP_PORT "12345"

/** Every Data set contains meta data with the digitizer and format
 * followed by the actual List and/or waveform data points. */
namespace Data {
    /* Shared meta data for the entire data package */
    struct Meta { // 24 bytes
        uint64_t globalTime;
        uint16_t digitizerID;
        char digitizerModel[MAXMODELSIZE];
        uint16_t version[VERSIONPARTS];
    };
    namespace List {
        struct Element // 72 bit
        {
            uint32_t localTime;
            uint16_t extendTime;
            uint32_t adcValue;
            uint16_t channel;
        };
    }; // namespace List
    namespace Waveform {
        struct Element // variable size
        {
            uint32_t localTime;
            uint16_t channel;
            uint16_t waveformLength;
            uint16_t waveform[];
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
    std::string makeVersion(uint16_t version[]) {
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
    EventData *setupEventData(void *data, uint32_t dataSize, uint32_t listEntries, uint32_t waveformEntries) {
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
        eventData->waveformEvents = (Waveform::Element *)(eventData->listEvents+listEntries);

        /* fuzzy out-of-bounds check */
        uint32_t bufSize = (char *)(eventData->waveformEvents + eventData->waveformEventsLength * (sizeof(Waveform::Element) + sizeof(uint16_t *))) - (char *)data;
        if (bufSize > dataSize) {
            std::stringstream ss;
            ss << "bufSize too large " << bufSize << " , cannot exceed " << dataSize;
            std::cerr << "ERROR: " << ss.str() << std::endl;
            throw std::runtime_error(ss.str());
        }
        return eventData;
    }

    PackedEvents packEventData(EventData *eventData, uint32_t listEntries, uint32_t waveformEntries) 
    {
        PackedEvents packedEvents;
        packedEvents.data = (void *)eventData;
        packedEvents.size = eventData->allocatedSize;
        packedEvents.dataSize = sizeof(EventData);
        packedEvents.dataSize += sizeof(Meta);
        packedEvents.dataSize += eventData->listEventsLength * sizeof(List::Element);
        Waveform::Element *wave = eventData->waveformEvents;
        size_t waveSize = 0;
        for (int i = 0; i < eventData->waveformEventsLength; i++) {
            waveSize = eventData->waveformEvents[i].waveformLength * sizeof(uint16_t);
            packedEvents.dataSize += waveSize;
        }
        return packedEvents;
    }
    /** @brief
     * Unpacking is very much like setup but with extraction of actual
     * list and waveform event counts from EventData first. 
     */
    EventData *unpackEventData(PackedEvents packedEvents) 
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
