/*
 * Copyright (C) 2003-2018 Andreas Kling <awesomekling@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREAS KLING ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANDREAS KLING OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Common.h"
#include "debug.h"
#include "pic.h"
#include "pit.h"
#include <math.h>
#include <QElapsedTimer>
#include <QThread>

//#define PIT_DEBUG

static const double baseFrequency = 1193.1816666; // 1.193182 MHz

enum DecrementMode { DecrementBinary = 0, DecrementBCD = 1 };
enum CounterAccessState { ReadLatchedLSB, ReadLatchedMSB, AccessMSBOnly, AccessLSBOnly, AccessLSBThenMSB, AccessMSBThenLSB };

struct CounterInfo {
    WORD startValue { 0xffff };
    WORD reload { 0xffff };
    WORD value();
    BYTE mode { 0 };
    DecrementMode decrementMode { DecrementBinary };
    WORD latchedValue { 0xffff };
    CounterAccessState accessState { ReadLatchedLSB };
    BYTE format { 0 };
    QElapsedTimer qtimer;

    void check(PIT&);
    void rollOver(PIT&);
};

struct TimerInfo {
    CounterInfo counter[3];
};

struct PIT::Private
{
    TimerInfo timer[2];
    int frequency { 0 };
    int timerId { -1 };
};

PIT::PIT(Machine& machine)
    : QObject(nullptr)
    , IODevice("PIT", machine, 0)
    , d(make<Private>())
{
    listen(0x40, IODevice::ReadWrite);
    listen(0x41, IODevice::ReadWrite);
    listen(0x42, IODevice::ReadWrite);
    listen(0x43, IODevice::ReadWrite);

    reset();
}

PIT::~PIT()
{
    killTimer(d->timerId);
}

void PIT::reset()
{
    d->frequency = 0;

    d->timer[0] = TimerInfo();
    d->timer[1] = TimerInfo();
}

WORD CounterInfo::value()
{
    double nsec = qtimer.nsecsElapsed() / 1000;
    int ticks = floor(nsec * baseFrequency);

    int currentValue = startValue - ticks;
    if (currentValue >= reload) {
        vlog(LogTimer, "Current value{%d} >= reload{%d}", currentValue, reload);
        currentValue %= reload;
    } else if (currentValue < 0) {
        currentValue = currentValue % reload + reload;
    }

    vlog(LogTimer, "nsec elapsed: %g, ticks: %g, value: %u", nsec, ticks, currentValue);
    return currentValue;
}

void CounterInfo::check(PIT& pit)
{
    double nsec = qtimer.nsecsElapsed() / 1000;
    int ticks = floor(nsec * baseFrequency);
    int currentValue = startValue - ticks;
    if (currentValue < 0) {
        startValue = reload;
        if (mode == 0)
            pit.raiseIRQ();
    }
}

void PIT::reconfigureTimer(BYTE index)
{
    auto& counter = d->timer[index].counter[index];
    counter.qtimer.start();
}

void PIT::boot()
{
    d->timerId = startTimer(10);

    // FIXME: This should be done by the BIOS instead.
    reconfigureTimer(0);
    reconfigureTimer(1);
    reconfigureTimer(2);
}

void PIT::timerEvent(QTimerEvent*)
{
#ifndef CT_DETERMINISTIC
    d->timer[0].counter[0].check(*this);
    d->timer[0].counter[1].check(*this);
    d->timer[0].counter[2].check(*this);
#endif
}

BYTE PIT::readCounter(BYTE index)
{
    auto& counter = d->timer[0].counter[index];
    BYTE data = 0;
    switch (counter.accessState) {
    case ReadLatchedLSB:
        data = getLSB(counter.latchedValue);
        counter.accessState = ReadLatchedMSB;
        break;
    case ReadLatchedMSB:
        data = getMSB(counter.latchedValue);
        counter.accessState = ReadLatchedLSB;
        break;
    case AccessLSBOnly:
        data = getLSB(counter.latchedValue);
        break;
    case AccessMSBOnly:
        data = getMSB(counter.latchedValue);
        break;
    case AccessLSBThenMSB:
        data = getLSB(counter.value());
        counter.accessState = AccessMSBThenLSB;
        break;
    case AccessMSBThenLSB:
        data = getMSB(counter.value());
        counter.accessState = AccessLSBThenMSB;
        break;
    }
    return data;
}

void PIT::writeCounter(BYTE index, BYTE data)
{
    auto& counter = d->timer[0].counter[index];
    switch (counter.accessState) {
    case ReadLatchedLSB:
    case ReadLatchedMSB:
        break;
    case AccessLSBOnly:
        counter.reload = makeWORD(getMSB(counter.reload), data);
        reconfigureTimer(index);
        break;
    case AccessMSBOnly:
        counter.reload = makeWORD(data, getLSB(counter.reload));
        reconfigureTimer(index);
        break;
    case AccessLSBThenMSB:
        counter.reload = makeWORD(getMSB(counter.reload), data);
        counter.accessState = AccessMSBThenLSB;
        break;
    case AccessMSBThenLSB:
        counter.reload = makeWORD(data, getLSB(counter.reload));
        counter.accessState = AccessLSBThenMSB;
        reconfigureTimer(index);
        break;
    }
}

BYTE PIT::in8(WORD port)
{
    BYTE data = 0;
    switch (port) {
    case 0x40:
    case 0x41:
    case 0x42:
        data = readCounter(port - 0x40);
        break;
    case 0x43:
        ASSERT_NOT_REACHED();
        break;
    }

    //vlog(LogTimer, "Unhandled read from port 0x%02X", port);
#ifdef PIT_DEBUG
    vlog(LogTimer, " in8 %03x = %02x", port, data);
#endif
    return data;
}

void PIT::out8(WORD port, BYTE data)
{
#ifdef PIT_DEBUG
    vlog(LogTimer, "out8 %03x, %02x", port, data);
#endif
    switch (port) {
    case 0x40:
    case 0x41:
    case 0x42:
        writeCounter(port - 0x40, data);
        break;
    case 0x43:
        modeControl(0, data);
        break;
    }
}

void PIT::modeControl(int timerIndex, BYTE data)
{
    ASSERT(timerIndex == 0 || timerIndex == 1);
    TimerInfo& timer = d->timer[timerIndex];

    BYTE counterIndex = (data >> 6);

    if (counterIndex > 2) {
        vlog(LogTimer, "Invalid counter index %d specified.", counterIndex);
        return;
    }

    ASSERT(counterIndex <= 2);
    CounterInfo& counter = timer.counter[counterIndex];

    counter.decrementMode = static_cast<DecrementMode>(data & 1);
    counter.mode = (data >> 1) & 7;

    counter.format = (data >> 4) & 3;
    switch (counter.format) {
    case 0:
        counter.accessState = ReadLatchedLSB;
        counter.latchedValue = counter.value();
        break;
    case 1:
        counter.accessState = AccessMSBOnly;
        break;
    case 2:
        counter.accessState = AccessLSBOnly;
        break;
    case 3:
        counter.accessState = AccessLSBThenMSB;
        break;
    }

#ifdef PIT_DEBUG
    vlog(LogTimer, "Setting mode for counter %u { dec: %s, mode: %u, fmt: %02x }",
        counterIndex,
        (counter.decrementMode == DecrementBCD) ? "BCD" : "binary",
        counter.mode,
        counter.format);
#endif

    reconfigureTimer(counterIndex);
}
