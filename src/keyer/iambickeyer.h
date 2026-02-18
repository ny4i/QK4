#ifndef IAMBICKEYER_H
#define IAMBICKEYER_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

enum class IambicMode { IambicA, IambicB };

/**
 * Software iambic keyer engine.
 *
 * Converts paddle press/release events into timed key-down/key-up signals.
 * Supports Iambic Mode A and Mode B.
 *
 * State machine:
 *   Idle → TonePlaying → InterElementSpace → (next element or Idle)
 *
 * Timing:
 *   dit = 1200 / WPM ms
 *   dah = 3 × dit ms
 *   inter-element space = 1 dit
 *
 * Iambic alternation: opposite paddle pressed during tone queues alternate element.
 * Iambic repetition: same paddle held continues same element.
 * Mode B squeeze: both paddles held then released sends one more alternate element.
 *
 * Emits keyDown(isDit)/keyUp() signals for MainWindow to send KZ CAT commands.
 *
 * Ported from TR4QT's IambicKeyer (based on NetKeyer's implementation).
 */
class IambicKeyer : public QObject {
    Q_OBJECT

public:
    explicit IambicKeyer(QObject *parent = nullptr);
    ~IambicKeyer() override = default;

    void setWpm(int wpm);
    int wpm() const { return m_wpm; }

    void setMode(IambicMode mode) { m_modeB = (mode == IambicMode::IambicB); }
    IambicMode mode() const { return m_modeB ? IambicMode::IambicB : IambicMode::IambicA; }

public slots:
    void updatePaddleState(bool dit, bool dah);
    void stop();

signals:
    void keyDown(bool isDit);
    void keyUp();

private slots:
    void onElementTimerExpired();
    void onSpaceTimerExpired();

private:
    enum class KeyerState {
        Idle,
        TonePlaying,
        InterElementSpace
    };

    void startNextElement();
    int determineNextToneDuration() const;
    void startTone(int durationMs);

    KeyerState m_state = KeyerState::Idle;

    QTimer m_elementTimer;
    QTimer m_spaceTimer;

    int m_wpm = 25;
    int m_ditLengthMs = 48;

    bool m_modeB = true;

    bool m_currentDit = false;
    bool m_currentDah = false;

    bool m_ditLatched = false;
    bool m_dahLatched = false;

    bool m_ditAtToneStart = false;
    bool m_dahAtToneStart = false;
    bool m_ditAtSpaceStart = false;
    bool m_dahAtSpaceStart = false;

    bool m_lastWasDit = true;

    QElapsedTimer m_stateTimer;
    static constexpr int SAFETY_TIMEOUT_MS = 1000;

    // Diagnostic elapsed timer (monotonic, started in constructor)
    QElapsedTimer m_diagTimer;
};

#endif // IAMBICKEYER_H
