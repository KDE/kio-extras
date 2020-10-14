/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
*/

#ifndef TRANSFER_H
#define TRANSFER_H

#include <QtGlobal>
#include <QVarLengthArray>

#include <array>
#include <condition_variable>
#include <mutex>

constexpr off_t c_minSegmentSize = 64 * 1024; // minimal size on stack
constexpr off_t c_maxSegmentSize = 4L * 1024 * 1024; // 4MiB is the largest request we make

struct TransferSegment
{
    explicit TransferSegment(const off_t fileSize);

    ssize_t size = 0; // current size (i.e. the size that was put into buf)
    QVarLengthArray<char, c_minSegmentSize> buf; // data buffer, only filled up to size!

private:
    static off_t segmentSizeForFileSize(const off_t fileSize_);
};

// Naive ring buffer.
// Segment instances are held in the buffer, i.e. only alloc'd once at
// beginning of the operation. Kind of a mix between ring and pool.
//
// The popping thread cannot pop while the pushing thread is still on
// an element. As such we need at least 3 elements to prevent dead locks.
class TransferRingBuffer
{
public:
    // fileSize is the stat'd file size of the source file.
    explicit TransferRingBuffer(const off_t fileSize_);
    ~TransferRingBuffer() = default;

    // Pops an item into the pull thread. This blocks
    // when the push thread is also currently on that index.
    // This can return nullptr if the push thread set the done state.
    // @note once done unpop() needs calling
    TransferSegment *pop();

    // Frees the item used by the pull thread. So it may be used by the
    // push thread.
    void unpop();

    // Simply returns a ptr to the item the current push thread marker is
    // at. i.e. the item "locked" for reading.
    // @note once done push() needs calling
    TransferSegment *nextFree();

    // Pushes ahead from the item obtained by nextFree.
    // This effectively allows the pull thread to pop() this item again.
    void push();

    // Only called by push thread to mark the buffer done and wake waiting
    // threads.
    void done();

private:
    bool m_done = false;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    static const size_t m_capacity = 4;
    std::array<std::unique_ptr<TransferSegment>, m_capacity> m_buffer;
    size_t head = 0; // index of push thread (prevents pop() from pull thread)
    size_t tail = 0; // index of pull thread (prevents push() from push thread)
};

#endif // TRANSFER_H
