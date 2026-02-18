#include "iambickeyer.h"
#include <QDebug>

static const char *stateStr(int s) {
    switch (s) {
    case 0: return "Idle";
    case 1: return "Tone";
    case 2: return "Space";
    default: return "???";
    }
}

IambicKeyer::IambicKeyer(QObject *parent)
    : QObject(parent)
{
    m_elementTimer.setSingleShot(true);
    m_elementTimer.setTimerType(Qt::PreciseTimer);
    m_spaceTimer.setSingleShot(true);
    m_spaceTimer.setTimerType(Qt::PreciseTimer);

    connect(&m_elementTimer, &QTimer::timeout, this, &IambicKeyer::onElementTimerExpired);
    connect(&m_spaceTimer, &QTimer::timeout, this, &IambicKeyer::onSpaceTimerExpired);

    m_diagTimer.start();
}

void IambicKeyer::setWpm(int wpm) {
    if (wpm <= 0) wpm = 25;
    m_wpm = wpm;
    m_ditLengthMs = 1200 / wpm;
}

void IambicKeyer::updatePaddleState(bool dit, bool dah) {
    const qint64 ms = m_diagTimer.elapsed();
    qDebug().nospace() << "[KEYER " << ms << "ms] paddle dit=" << dit << " dah=" << dah
                       << " state=" << stateStr(static_cast<int>(m_state))
                       << " lastWasDit=" << m_lastWasDit
                       << " ditLatch=" << m_ditLatched << " dahLatch=" << m_dahLatched;

    // Safety check: force reset if state machine stuck
    if (m_state != KeyerState::Idle && m_stateTimer.elapsed() > SAFETY_TIMEOUT_MS) {
        qDebug().nospace() << "[KEYER " << ms << "ms] SAFETY TIMEOUT ("
                           << m_stateTimer.elapsed() << "ms in state "
                           << stateStr(static_cast<int>(m_state)) << ") — forcing reset";
        stop();
    }

    m_currentDit = dit;
    m_currentDah = dah;

    if (m_state == KeyerState::Idle && (dit || dah)) {
        qDebug().nospace() << "[KEYER " << ms << "ms] Idle→startNextElement";
        startNextElement();
    } else if (m_state == KeyerState::TonePlaying) {
        if (m_lastWasDit && dah && !m_dahAtToneStart && !m_dahLatched) {
            m_dahLatched = true;
            qDebug().nospace() << "[KEYER " << ms << "ms] DAH latch set (opposite during dit)";
        }
        if (!m_lastWasDit && dit && !m_ditAtToneStart && !m_ditLatched) {
            m_ditLatched = true;
            qDebug().nospace() << "[KEYER " << ms << "ms] DIT latch set (opposite during dah)";
        }
    } else if (m_state == KeyerState::InterElementSpace) {
        if (dit && !m_ditAtSpaceStart && !m_ditLatched) {
            m_ditLatched = true;
            qDebug().nospace() << "[KEYER " << ms << "ms] DIT latch set (during space)";
        }
        if (dah && !m_dahAtSpaceStart && !m_dahLatched) {
            m_dahLatched = true;
            qDebug().nospace() << "[KEYER " << ms << "ms] DAH latch set (during space)";
        }
    }
}

void IambicKeyer::stop() {
    const qint64 ms = m_diagTimer.elapsed();
    qDebug().nospace() << "[KEYER " << ms << "ms] stop() called, was state="
                       << stateStr(static_cast<int>(m_state));

    m_elementTimer.stop();
    m_spaceTimer.stop();

    if (m_state == KeyerState::TonePlaying) {
        emit keyUp();
    }

    m_state = KeyerState::Idle;
    m_ditLatched = false;
    m_dahLatched = false;
    m_ditAtToneStart = false;
    m_dahAtToneStart = false;
    m_ditAtSpaceStart = false;
    m_dahAtSpaceStart = false;
}

void IambicKeyer::onElementTimerExpired() {
    const qint64 ms = m_diagTimer.elapsed();
    qDebug().nospace() << "[KEYER " << ms << "ms] elementExpired → keyUp, entering Space ("
                       << m_ditLengthMs << "ms)  curDit=" << m_currentDit
                       << " curDah=" << m_currentDah
                       << " ditLatch=" << m_ditLatched << " dahLatch=" << m_dahLatched;

    emit keyUp();

    m_state = KeyerState::InterElementSpace;
    m_stateTimer.restart();

    m_ditAtSpaceStart = m_currentDit;
    m_dahAtSpaceStart = m_currentDah;

    m_spaceTimer.start(m_ditLengthMs);
}

void IambicKeyer::onSpaceTimerExpired() {
    const qint64 ms = m_diagTimer.elapsed();
    int nextDuration = determineNextToneDuration();

    if (nextDuration > 0) {
        qDebug().nospace() << "[KEYER " << ms << "ms] spaceExpired → next "
                           << (nextDuration == m_ditLengthMs ? "dit" : "dah")
                           << " (" << nextDuration << "ms)";
        startTone(nextDuration);
    } else {
        qDebug().nospace() << "[KEYER " << ms << "ms] spaceExpired → IDLE (no next element)"
                           << " curDit=" << m_currentDit << " curDah=" << m_currentDah
                           << " ditLatch=" << m_ditLatched << " dahLatch=" << m_dahLatched
                           << " ditAtTone=" << m_ditAtToneStart << " dahAtTone=" << m_dahAtToneStart
                           << " ditAtSpace=" << m_ditAtSpaceStart << " dahAtSpace=" << m_dahAtSpaceStart
                           << " modeB=" << m_modeB;
        m_state = KeyerState::Idle;
        m_ditLatched = false;
        m_dahLatched = false;
        m_ditAtToneStart = false;
        m_dahAtToneStart = false;
        m_ditAtSpaceStart = false;
        m_dahAtSpaceStart = false;
    }
}

void IambicKeyer::startNextElement() {
    int duration = determineNextToneDuration();

    if (duration > 0) {
        startTone(duration);
    }
}

int IambicKeyer::determineNextToneDuration() const {
    bool sendDit = false;
    bool sendDah = false;

    if (m_lastWasDit || m_state == KeyerState::Idle) {
        // After dit (or starting from idle):
        // Priority 1: Alternation — opposite paddle latched or pressed
        if (m_dahLatched || m_currentDah) {
            sendDah = true;
        }
        // Priority 2: Repetition — same paddle latched or pressed
        else if (m_ditLatched || m_currentDit) {
            sendDit = true;
        }
        // Priority 3 (Mode B): Squeeze — both held at tone start, both now released
        else if (m_modeB && m_dahAtToneStart && !m_currentDah && !m_currentDit) {
            sendDah = true;
        }
    } else {
        // After dah:
        // Priority 1: Alternation
        if (m_ditLatched || m_currentDit) {
            sendDit = true;
        }
        // Priority 2: Repetition
        else if (m_dahLatched || m_currentDah) {
            sendDah = true;
        }
        // Priority 3 (Mode B): Squeeze
        else if (m_modeB && m_ditAtToneStart && !m_currentDit && !m_currentDah) {
            sendDit = true;
        }
    }

    // Special case: both paddles from idle → dit first
    if (m_state == KeyerState::Idle && m_currentDit && m_currentDah) {
        sendDit = true;
        sendDah = false;
    }

    if (sendDit) return m_ditLengthMs;
    if (sendDah) return m_ditLengthMs * 3;
    return 0;
}

void IambicKeyer::startTone(int durationMs) {
    const qint64 ms = m_diagTimer.elapsed();
    bool isDit = (durationMs == m_ditLengthMs);

    qDebug().nospace() << "[KEYER " << ms << "ms] startTone " << (isDit ? "DIT" : "DAH")
                       << " (" << durationMs << "ms)"
                       << " curDit=" << m_currentDit << " curDah=" << m_currentDah;

    m_ditAtToneStart = m_currentDit;
    m_dahAtToneStart = m_currentDah;

    m_ditLatched = false;
    m_dahLatched = false;

    m_lastWasDit = isDit;

    m_state = KeyerState::TonePlaying;
    m_stateTimer.restart();
    emit keyDown(isDit);

    m_elementTimer.start(durationMs);
}
