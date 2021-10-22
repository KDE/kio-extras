/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
*/

#include "transfer.h"

#include <future>

TransferSegment::TransferSegment(const off_t fileSize)
    : buf(segmentSizeForFileSize(fileSize))
{
}

off_t TransferSegment::segmentSizeForFileSize(const off_t fileSize_)
{
    const off_t fileSize = qMax<off_t>(0, fileSize_);

    // read() internally splits our read requests into multiple server
    // requests and then assembles the responses into our buffer.
    // The larger the chunks we request the better the performance.
    // At the same time we'll want a semblance of progress reporting
    // and also not eat too much RAM. It's a balancing act :|
    off_t segmentSize = c_minSegmentSize;
    // The segment size is largely arbitrary and sacrifices better throughput for
    // greater memory use.
    // This only goes up to a maximum because bigger transfer blobs directly
    // translate to more RAM use. Mind that the effective RAM use will
    // be (segmentSize * (segments + 1)). The +1 is because smbc internally will also
    // allocate up to a full segment for one read() call.
    //
    // Unfortunately we have no way of knowing what size smbc will use for the
    // network requests, so we can't use a multiple of that. Which means we'll
    // almost never reach best performance.
    //
    // TODO: perhaps it would actually make sense to read at a multiple of
    // the target drive's block size?
    const off_t idealSegmentSize = qMin<off_t>(fileSize / 50, c_maxSegmentSize);
    segmentSize = qMax<off_t>(segmentSize, idealSegmentSize);
    // If the segment size is larger than the file size it appears we can
    // actually degrade performance, so pick the smaller of the two.
    if (fileSize != 0) {
        segmentSize = qMin<off_t>(segmentSize, fileSize);
    }
    return segmentSize;
}

TransferRingBuffer::TransferRingBuffer(const off_t fileSize)
{
    for (size_t i = 0; i < m_capacity; ++i) {
        m_buffer[i] = std::unique_ptr<TransferSegment>(new TransferSegment(fileSize));
    }
}

TransferSegment *TransferRingBuffer::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while (head == tail) {
        if (!m_done) {
            m_cond.wait(lock);
        } else {
            return nullptr;
        }
    }

    auto segment = m_buffer[tail].get();
    m_cond.notify_all();
    return segment;
}

void TransferRingBuffer::unpop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    tail = (tail + 1) % m_capacity;
    m_cond.notify_all();
}

TransferSegment *TransferRingBuffer::nextFree()
{
    // This does not require synchronization. As soon
    // as we pushed the last item we gained exclusive lock
    // on the new item.
    m_cond.notify_all();
    return m_buffer[head].get();
}

void TransferRingBuffer::push()
{
    const auto newHead = (head + 1) % m_capacity;
    std::unique_lock<std::mutex> lock(m_mutex);
    while (newHead == tail) {
        // do not move to the item the reading thread is on
        m_cond.wait(lock);
    }
    head = newHead;
    m_cond.notify_all();
}

void TransferRingBuffer::done()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_done = true;
    m_cond.notify_all();
}
