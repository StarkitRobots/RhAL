#pragma once

#include <stdint.h>

namespace RhAL
{
    class Bus
    {
        public:
            /**
             * Sends data to the bus.
             */
            virtual bool sendData(uint8_t *data, size_t size)=0;

            /**
             * Wait for size bytes to be available on the bus, expiring
             * after timeout (milliseconds).
             *
             * Returns true if data are available, false else.
             */
            virtual bool waitForData(size_t size, unsigned int timeout)=0;

            /**
             * Reads size bytes to the buffer
             */
            virtual void readData(uint8_t *data, size_t size)=0;

            /**
             * Flushes the I/O buffers
             */
            virtual void flush()=0;
    };
}