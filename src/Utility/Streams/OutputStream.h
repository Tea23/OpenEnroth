#pragma once

#include <cstddef>

/**
 * Abstract base class for all data output streams.
 *
 * Compared to `std::ostream` it is:
 * - Non-buffered.
 * - Uses exceptions for error handling.
 * - Has an extremely simple and intuitive interface consisting of exactly three methods.
 */
class OutputStream {
 public:
    virtual ~OutputStream() {}

    /**
     * Writes provided data into the output stream.
     *
     * @param data                      Pointer to the data to write.
     * @param size                      Data size.
     * @throws std::runtime_error       On error.
     */
    virtual void Write(const void *data, size_t size) = 0;

    /**
     * Flushes this output stream if it provides any kind of buffering.
     *
     * @throws std::runtime_error       On error.
     */
    virtual void Flush() = 0;

    /**
     * Closes this output stream. Writing into or flushing a closed stream will result in undefined behavior.
     *
     * Does nothing if the stream is already closed.
     *
     * @throws std::runtime_error       On error.
     */
    virtual void Close() = 0;
};
