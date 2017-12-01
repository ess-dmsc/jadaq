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

/* TODO: switch to a static buffer using MAXBUFSIZE */
#define EVENTFIELDS (3)
#define EVENTINIT {{0, 0, 0}}

#define MAXBUFSIZE (8192)

#define VERSIONPARTS (3)
#define VERSION {1, 0, 0}
#define MAXMODELSIZE (8)

/* Every Data set contains meta data with the digitizer and format
 * followed by the actual List and/or waveform data points. */
namespace Data {
    /* Shared meta data for the entire data package */
    struct Meta { // 28 bytes
        uint16_t version[VERSIONPARTS] = VERSION;
        char digitizerModel[MAXMODELSIZE];
        uint16_t digitizerID;
        uint64_t globalTime;
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
            uint32_t adcValue;
            uint16_t channel;
            uint16_t WaveformLength;
            uint16_t waveform[];
        };
    }; // namespace Waveform
    /* Wrapped up data with multiple events */
    /* Actual metadata, listevent and waveformevent contents are
     * appended right after EventData during setup. */
    struct EventData // 24 bytes
    {
        Meta *metadata;
        /* Raw buffer size */
        uint32_t allocatedSize;
        uint32_t checksum;
        uint16_t listEventsLength;
        List::Element *listEvents;
        uint16_t waveformEventsLength;
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
        Takes a pre-allocated data buffer and prepares it for EventData
        use with given number of list events and waveform events.
        User can then manually fill metadata, listEvents and
        waveformEvents with actual contents afterwards.
        The waveformEvents array has variable sized entries so it is
        saved last to avoid interference and it is left to the user to
        do the slicing.
    */
    EventData *setupEventData(void *data, uint32_t dataSize, uint32_t listEntries, uint32_t waveformEntries) {
        std::cout << "DEBUG: in setupEventData " << data << " " << dataSize << " " << listEntries << " " << waveformEntries << std::endl;
        EventData *eventData = (EventData *)data;
        std::cout << "DEBUG: setting allocatedSize in setupEventData: " << eventData->allocatedSize << std::endl;
        eventData->allocatedSize = dataSize;
        std::cout << "DEBUG: set allocatedSize to " << eventData->allocatedSize << " in setupEventData" << std::endl;

        /* NOTE: We must make sure padding and layout on receiver
         * matches the sender. We could use boost::serialize but it comes
         * with significant overhead so we just assume platform
         * similarity for now. */
        /* TODO: implement better checksum for validation after transfer */
        eventData->checksum = sizeof(EventData) + sizeof(Meta) + sizeof(List::Element) * listEntries + sizeof(Waveform::Element) * waveformEntries;
        std::cout << "DEBUG: set checksum to " << eventData->checksum << " in setupEventData" << std::endl;
        eventData->listEventsLength = listEntries;
        std::cout << "DEBUG: set listEventsLength to " << eventData->listEventsLength << " in setupEventData" << std::endl;
        eventData->metadata = (Meta *)(eventData+1);
        std::cout << "DEBUG: set metadata to " << eventData->metadata << " i.e. +" << ((char*)(eventData->metadata) - (char *)data) << " in setupEventData" << std::endl;
        eventData->listEvents = (List::Element *)(eventData->metadata+1);
        std::cout << "DEBUG: set listEvents to " << eventData->listEvents << " in setupEventData" << std::endl;
        eventData->waveformEventsLength = waveformEntries;
        std::cout << "DEBUG: set waveformEventsLength to " << eventData->waveformEventsLength << " in setupEventData" << std::endl;
        eventData->waveformEvents = (Waveform::Element *)(eventData->listEvents+listEntries);
        std::cout << "DEBUG: set waveformEvents to " << eventData->waveformEvents << " in setupEventData" << std::endl;
        /* fuzzy out-of-bounds check */
        uint32_t bufSize = (char *)(eventData->waveformEvents + eventData->waveformEventsLength * (sizeof(Waveform::Element) + sizeof(uint16_t *))) - (char *)data;
        if (bufSize > dataSize) {
            std::stringstream ss;
            ss << "bufSize too large " << bufSize << " , cannot exceed " << dataSize;
            std::cerr << "ERROR: " << ss.str() << std::endl;
            throw std::runtime_error(ss.str());
        }
        std::cout << "DEBUG: return eventData " << eventData << " in setupEventData" << std::endl;
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
            waveSize = eventData->waveformEvents[i].WaveformLength * sizeof(uint16_t);
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
        return setupEventData(packedEvents.data, packedEvents.dataSize,
                              listEventsLength, waveformEventsLength);
    }
} // namespace Data
#endif //MULTIBLADEDATAHANDLER_DATAFORMAT_HPP
