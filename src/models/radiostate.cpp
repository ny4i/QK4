#include "radiostate.h"
#include <QDateTime>
#include <QDebug>

RadioState::RadioState(QObject *parent) : QObject(parent) {}

void RadioState::parseCATCommand(const QString &command) {
    QString cmd = command.trimmed();
    if (cmd.isEmpty())
        return;

    // Remove trailing semicolon for parsing
    if (cmd.endsWith(';')) {
        cmd.chop(1);
    }

    // Error/Notification messages (ERxx:message) - e.g., ER44:KPA1500 Status: operate.
    if (cmd.startsWith("ER") && cmd.length() > 2) {
        // Find the colon that separates error code from message
        int colonPos = cmd.indexOf(':');
        if (colonPos > 2) {
            bool ok;
            int errorCode = cmd.mid(2, colonPos - 2).toInt(&ok);
            if (ok) {
                QString message = cmd.mid(colonPos + 1);
                emit errorNotificationReceived(errorCode, message);
            }
        }
        // Don't return - let other parsers potentially handle it too
    }

    // VFO A Frequency (FA)
    if (cmd.startsWith("FA") && cmd.length() > 2) {
        bool ok;
        quint64 freq = cmd.mid(2).toULongLong(&ok);
        if (ok) {
            m_vfoA = freq;
            m_frequency = freq;
            emit frequencyChanged(freq);
        }
    }
    // VFO B Frequency (FB)
    else if (cmd.startsWith("FB") && cmd.length() > 2) {
        bool ok;
        quint64 freq = cmd.mid(2).toULongLong(&ok);
        if (ok && m_vfoB != freq) {
            m_vfoB = freq;
            emit frequencyBChanged(freq);
        }
    }
    // Mode VFO B (MD$)
    else if (cmd.startsWith("MD$") && cmd.length() > 3) {
        bool ok;
        int modeCode = cmd.mid(3).toInt(&ok);
        if (ok) {
            Mode newMode = modeFromCode(modeCode);
            if (m_modeB != newMode) {
                m_modeB = newMode;
                emit modeBChanged(m_modeB);
            }
        }
    }
    // Mode (MD)
    else if (cmd.startsWith("MD") && cmd.length() > 2) {
        bool ok;
        int modeCode = cmd.mid(2).toInt(&ok);
        if (ok) {
            Mode newMode = modeFromCode(modeCode);
            if (m_mode != newMode) {
                m_mode = newMode;
                emit modeChanged(m_mode);
                // Also emit delay change since delay is mode-specific
                int currentDelay = delayForCurrentMode();
                if (currentDelay >= 0) {
                    emit qskDelayChanged(currentDelay);
                }
            }
        }
    }
    // Filter Bandwidth VFO B (BW$) - K4 returns value/10
    else if (cmd.startsWith("BW$") && cmd.length() > 3) {
        bool ok;
        int bw = cmd.mid(3).toInt(&ok);
        if (ok) {
            int newBw = bw * 10;
            if (m_filterBandwidthB != newBw) {
                m_filterBandwidthB = newBw;
                emit filterBandwidthBChanged(m_filterBandwidthB);
            }
        }
    }
    // Filter Bandwidth (BW) - K4 returns value/10
    else if (cmd.startsWith("BW") && cmd.length() > 2) {
        bool ok;
        int bw = cmd.mid(2).toInt(&ok);
        if (ok) {
            int newBw = bw * 10;
            if (m_filterBandwidth != newBw) {
                m_filterBandwidth = newBw;
                emit filterBandwidthChanged(m_filterBandwidth);
            }
        }
    }
    // Mic Gain (MG 0-80)
    else if (cmd.startsWith("MG") && cmd.length() > 2) {
        bool ok;
        int gain = cmd.mid(2).toInt(&ok);
        if (ok && gain != m_micGain) {
            m_micGain = gain;
            emit micGainChanged(m_micGain);
        }
    }
    // Monitor Level (ML mode 0-2, level 0-100)
    // Format: MLmnnn where m=mode (0=CW, 1=Data, 2=Voice), nnn=000-100
    else if (cmd.startsWith("ML") && cmd.length() >= 5) {
        bool ok;
        int mode = cmd.mid(2, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 2) {
            int level = cmd.mid(3).toInt(&ok);
            if (ok && level >= 0 && level <= 100) {
                int *target = nullptr;
                switch (mode) {
                case 0:
                    target = &m_monitorLevelCW;
                    break;
                case 1:
                    target = &m_monitorLevelData;
                    break;
                case 2:
                    target = &m_monitorLevelVoice;
                    break;
                }
                if (target && *target != level) {
                    *target = level;
                    emit monitorLevelChanged(mode, level);
                }
            }
        }
    }
    // Speech Compression (CP 0-30) - SSB modes only
    else if (cmd.startsWith("CP") && cmd.length() > 2) {
        bool ok;
        int comp = cmd.mid(2).toInt(&ok);
        if (ok && comp != m_compression) {
            m_compression = comp;
            emit compressionChanged(m_compression);
        }
    }
    // RF Gain Sub RX (RG$)
    else if (cmd.startsWith("RG$") && cmd.length() > 3) {
        bool ok;
        int rg = cmd.mid(3).toInt(&ok);
        if (ok && m_rfGainB != rg) {
            m_rfGainB = rg;
            emit rfGainBChanged(m_rfGainB);
        }
    }
    // RF Gain Main RX (RG)
    else if (cmd.startsWith("RG") && cmd.length() > 2) {
        bool ok;
        int rg = cmd.mid(2).toInt(&ok);
        if (ok && m_rfGain != rg) {
            m_rfGain = rg;
            emit rfGainChanged(m_rfGain);
        }
    }
    // Squelch Sub RX (SQ$)
    else if (cmd.startsWith("SQ$") && cmd.length() > 3) {
        bool ok;
        int sq = cmd.mid(3).toInt(&ok);
        if (ok && m_squelchLevelB != sq) {
            m_squelchLevelB = sq;
            emit squelchBChanged(m_squelchLevelB);
        }
    }
    // Squelch Main RX (SQ)
    else if (cmd.startsWith("SQ") && cmd.length() > 2) {
        bool ok;
        int sq = cmd.mid(2).toInt(&ok);
        if (ok && m_squelchLevel != sq) {
            m_squelchLevel = sq;
            emit squelchChanged(m_squelchLevel);
        }
    }
    // Keyer Speed (KS) - WPM
    else if (cmd.startsWith("KS") && cmd.length() > 2) {
        bool ok;
        int wpm = cmd.mid(2).toInt(&ok);
        if (ok && m_keyerSpeed != wpm) {
            m_keyerSpeed = wpm;
            emit keyerSpeedChanged(m_keyerSpeed);
        }
    }
    // QSK/VOX Delay (SD) - SDxMzzz where x=QSK flag (0/1), M=mode (C/V/D), zzz=delay in 10ms
    // RDY dump provides: SD0C005;SD0V025;SD0D005; (all three modes)
    // Query response: SDxyzzz where x=QSK flag, y=mode code, zzz=delay
    // x=1 means full QSK (delay=0), x=0 means use specified delay
    else if (cmd.startsWith("SD") && cmd.length() >= 7) {
        // Extract QSK flag from position 2 (only for CW mode - QSK is CW-specific)
        QChar qskFlag = cmd.at(2);
        QChar modeChar = cmd.at(3);
        bool ok;
        int delay = cmd.mid(4, 3).toInt(&ok);
        if (ok) {
            // Update QSK enabled state (only meaningful for CW mode)
            if (modeChar == 'C') {
                bool qskOn = (qskFlag == '1');
                if (qskOn != m_qskEnabled) {
                    m_qskEnabled = qskOn;
                    emit qskEnabledChanged(m_qskEnabled);
                }
            }

            bool isCurrentMode = false;
            if (modeChar == 'C') {
                if (m_qskDelayCW != delay) {
                    m_qskDelayCW = delay;
                    isCurrentMode = (m_mode == CW || m_mode == CW_R);
                }
            } else if (modeChar == 'V') {
                if (m_qskDelayVoice != delay) {
                    m_qskDelayVoice = delay;
                    isCurrentMode = (m_mode == LSB || m_mode == USB || m_mode == AM || m_mode == FM);
                }
            } else if (modeChar == 'D') {
                if (m_qskDelayData != delay) {
                    m_qskDelayData = delay;
                    isCurrentMode = (m_mode == DATA || m_mode == DATA_R);
                }
            }
            // Only emit if the changed delay is for the current operating mode
            if (isCurrentMode) {
                emit qskDelayChanged(delay);
            }
        }
    }
    // S-Meter VFO B / Sub RX (SM$)
    else if (cmd.startsWith("SM$") && cmd.length() > 3) {
        bool ok;
        int bars = cmd.mid(3).toInt(&ok);
        if (ok) {
            if (bars <= 18) {
                m_sMeterB = bars / 2.0;
            } else {
                int dbAboveS9 = (bars - 18) * 3;
                m_sMeterB = 9.0 + dbAboveS9 / 10.0;
            }
            emit sMeterBChanged(m_sMeterB);
        }
    }
    // S-Meter (SM)
    else if (cmd.startsWith("SM") && cmd.length() > 2) {
        bool ok;
        int bars = cmd.mid(2).toInt(&ok);
        if (ok) {
            if (bars <= 18) {
                m_sMeter = bars / 2.0;
            } else {
                int dbAboveS9 = (bars - 18) * 3;
                m_sMeter = 9.0 + dbAboveS9 / 10.0;
            }
            emit sMeterChanged(m_sMeter);
        }
    }
    // Power Output (PO)
    else if (cmd.startsWith("PO") && cmd.length() > 2) {
        bool ok;
        int po = cmd.mid(2).toInt(&ok);
        if (ok) {
            m_powerMeter = po;
            emit powerMeterChanged(m_powerMeter);
        }
    }
    // TX/RX state
    else if (cmd == "TX" || cmd.startsWith("TX1")) {
        if (!m_isTransmitting) {
            m_isTransmitting = true;
            emit transmitStateChanged(true);
        }
    } else if (cmd == "RX") {
        if (m_isTransmitting) {
            m_isTransmitting = false;
            emit transmitStateChanged(false);
        }
    }
    // Noise Blanker Sub (NB$) - Format: NB$nnm; or NB$nnmf; where nn=level, m=on/off, f=filter(0/1/2)
    else if (cmd.startsWith("NB$") && cmd.length() >= 5) {
        QString nbStr = cmd.mid(3);
        if (nbStr.length() >= 3) {
            bool ok1, ok2;
            int level = nbStr.left(2).toInt(&ok1);
            int enabled = nbStr.mid(2, 1).toInt(&ok2);
            if (ok1 && ok2) {
                m_noiseBlankerLevelB = qMin(level, 15);
                m_noiseBlankerEnabledB = (enabled == 1);
                // Filter field is optional (4th char)
                if (nbStr.length() >= 4) {
                    bool ok3;
                    int filter = nbStr.mid(3, 1).toInt(&ok3);
                    if (ok3) {
                        m_noiseBlankerFilterWidthB = qMin(filter, 2);
                    }
                }
                emit processingChangedB();
            }
        }
    }
    // Noise Blanker Main (NB) - Format: NBnnm; or NBnnmf; where nn=level(0-15), m=on/off, f=filter(0/1/2)
    else if (cmd.startsWith("NB") && cmd.length() >= 4) {
        QString nbStr = cmd.mid(2);
        if (nbStr.length() >= 3) {
            bool ok1, ok2;
            int level = nbStr.left(2).toInt(&ok1);
            int enabled = nbStr.mid(2, 1).toInt(&ok2);
            if (ok1 && ok2) {
                m_noiseBlankerLevel = qMin(level, 15);
                m_noiseBlankerEnabled = (enabled == 1);
                // Filter field is optional (4th char)
                if (nbStr.length() >= 4) {
                    bool ok3;
                    int filter = nbStr.mid(3, 1).toInt(&ok3);
                    if (ok3) {
                        m_noiseBlankerFilterWidth = qMin(filter, 2);
                    }
                }
                emit processingChanged();
            }
        }
    }
    // Noise Reduction Sub (NR$)
    else if (cmd.startsWith("NR$") && cmd.length() >= 4) {
        QString nrStr = cmd.mid(3);
        if (nrStr.length() >= 3) {
            bool ok1, ok2;
            int level = nrStr.left(2).toInt(&ok1);
            int enabled = nrStr.right(1).toInt(&ok2);
            if (ok1 && ok2) {
                m_noiseReductionLevelB = level;
                m_noiseReductionEnabledB = (enabled == 1);
                emit processingChangedB();
            }
        }
    }
    // Noise Reduction Main (NR)
    else if (cmd.startsWith("NR") && cmd.length() >= 3) {
        QString nrStr = cmd.mid(2);
        if (nrStr.length() >= 3) {
            bool ok1, ok2;
            int level = nrStr.left(2).toInt(&ok1);
            int enabled = nrStr.right(1).toInt(&ok2);
            if (ok1 && ok2) {
                m_noiseReductionLevel = level;
                m_noiseReductionEnabled = (enabled == 1);
                emit processingChanged();
            }
        }
    }
    // Auto Notch (NT) - legacy
    else if (cmd.startsWith("NT") && !cmd.startsWith("NT$") && cmd.length() > 2) {
        m_autoNotchFilter = (cmd.mid(2) == "1");
    }
    // Auto Notch Sub (NA$) - NA$n; where n=0/1
    else if (cmd.startsWith("NA$") && cmd.length() >= 4) {
        bool enabled = (cmd.at(3) == '1');
        if (m_autoNotchEnabledB != enabled) {
            m_autoNotchEnabledB = enabled;
            emit notchBChanged();
        }
    }
    // Auto Notch (NA) - NAn; where n=0/1
    else if (cmd.startsWith("NA") && cmd.length() >= 3) {
        bool enabled = (cmd.at(2) == '1');
        if (m_autoNotchEnabled != enabled) {
            m_autoNotchEnabled = enabled;
            emit notchChanged();
        }
    }
    // Manual Notch Sub (NM$) - NM$nnnnm; or NM$m;
    else if (cmd.startsWith("NM$") && cmd.length() >= 4) {
        QString data = cmd.mid(3);
        if (data.length() >= 5) {
            // Full format: NM$nnnnm (4-digit pitch + on/off)
            bool ok;
            int pitch = data.left(4).toInt(&ok);
            bool enabled = (data.at(4) == '1');
            bool changed = false;
            if (ok && pitch >= 150 && pitch <= 5000 && m_manualNotchPitchB != pitch) {
                m_manualNotchPitchB = pitch;
                changed = true;
            }
            if (m_manualNotchEnabledB != enabled) {
                m_manualNotchEnabledB = enabled;
                changed = true;
            }
            if (changed) {
                emit notchBChanged();
            }
        } else if (data.length() >= 1) {
            // Short format: NM$m (on/off only)
            bool enabled = (data.at(0) == '1');
            if (m_manualNotchEnabledB != enabled) {
                m_manualNotchEnabledB = enabled;
                emit notchBChanged();
            }
        }
    }
    // Manual Notch Main (NM) - NMnnnnm; or NMm;
    else if (cmd.startsWith("NM") && cmd.length() >= 3) {
        QString data = cmd.mid(2);
        if (data.length() >= 5) {
            // Full format: NMnnnnm (4-digit pitch + on/off)
            bool ok;
            int pitch = data.left(4).toInt(&ok);
            bool enabled = (data.at(4) == '1');
            bool changed = false;
            if (ok && pitch >= 150 && pitch <= 5000 && m_manualNotchPitch != pitch) {
                m_manualNotchPitch = pitch;
                changed = true;
            }
            if (m_manualNotchEnabled != enabled) {
                m_manualNotchEnabled = enabled;
                changed = true;
            }
            if (changed) {
                emit notchChanged();
            }
        } else if (data.length() >= 1) {
            // Short format: NMm (on/off only)
            bool enabled = (data.at(0) == '1');
            if (m_manualNotchEnabled != enabled) {
                m_manualNotchEnabled = enabled;
                emit notchChanged();
            }
        }
    }
    // Preamp Sub (PA$) - PA$nm where n=level(0-3), m=on/off(0/1)
    else if (cmd.startsWith("PA$") && cmd.length() >= 5) {
        QString paStr = cmd.mid(3);
        if (paStr.length() >= 2) {
            bool ok1, ok2;
            int level = paStr.left(1).toInt(&ok1);
            int enabled = paStr.mid(1, 1).toInt(&ok2);
            if (ok1 && ok2) {
                m_preampB = level;
                m_preampEnabledB = (enabled == 1);
                emit processingChangedB();
            }
        }
    }
    // Preamp Main (PA) - PAnm where n=level(0-3), m=on/off(0/1)
    else if (cmd.startsWith("PA") && cmd.length() >= 4) {
        QString paStr = cmd.mid(2);
        if (paStr.length() >= 2) {
            bool ok1, ok2;
            int level = paStr.left(1).toInt(&ok1);
            int enabled = paStr.mid(1, 1).toInt(&ok2);
            if (ok1 && ok2) {
                m_preamp = level;
                m_preampEnabled = (enabled == 1);
                emit processingChanged();
            }
        }
    }
    // Attenuator Sub (RA$) - RA$nnm where nn=level(0-21), m=on/off
    else if (cmd.startsWith("RA$") && cmd.length() >= 6) {
        QString raStr = cmd.mid(3);
        if (raStr.length() >= 3) {
            bool ok1, ok2;
            int level = raStr.left(2).toInt(&ok1);
            int enabled = raStr.mid(2, 1).toInt(&ok2);
            if (ok1 && ok2) {
                m_attenuatorLevelB = level;
                m_attenuatorEnabledB = (enabled == 1);
                emit processingChangedB();
            }
        }
    }
    // Attenuator Main (RA) - RAnnotm where nn=level(0-21), m=on/off
    else if (cmd.startsWith("RA") && cmd.length() >= 5) {
        QString raStr = cmd.mid(2);
        if (raStr.length() >= 3) {
            bool ok1, ok2;
            int level = raStr.left(2).toInt(&ok1);
            int enabled = raStr.mid(2, 1).toInt(&ok2);
            if (ok1 && ok2) {
                m_attenuatorLevel = level;
                m_attenuatorEnabled = (enabled == 1);
                emit processingChanged();
            }
        }
    }
    // AGC Speed Sub (GT$) - GT$n where n=0(off)/1(slow)/2(fast)
    else if (cmd.startsWith("GT$") && cmd.length() >= 4) {
        bool ok;
        int gt = cmd.mid(3).toInt(&ok);
        if (ok) {
            if (gt == 0)
                m_agcSpeedB = AGC_Off;
            else if (gt == 1)
                m_agcSpeedB = AGC_Slow;
            else if (gt == 2)
                m_agcSpeedB = AGC_Fast;
            emit processingChangedB();
        }
    }
    // AGC Speed Main (GT) - GTn where n=0(off)/1(slow)/2(fast)
    else if (cmd.startsWith("GT") && cmd.length() > 2) {
        bool ok;
        int gt = cmd.mid(2).toInt(&ok);
        if (ok) {
            if (gt == 0)
                m_agcSpeed = AGC_Off;
            else if (gt == 1)
                m_agcSpeed = AGC_Slow;
            else if (gt == 2)
                m_agcSpeed = AGC_Fast;
            emit processingChanged();
        }
    }
    // Audio Effects (FX) - FXn where n=0(off)/1(delay)/2(pitch-map)
    else if (cmd.startsWith("FX") && cmd.length() >= 3) {
        bool ok;
        int fx = cmd.mid(2).toInt(&ok);
        if (ok && fx >= 0 && fx <= 2 && fx != m_afxMode) {
            m_afxMode = fx;
            emit afxModeChanged(m_afxMode);
        }
    }
    // Audio Peak Filter - Sub RX (AP$) - AP$mb where m=enabled(0/1), b=bandwidth(0/1/2)
    // Must check AP$ BEFORE AP to avoid AP$ matching AP prefix
    else if (cmd.startsWith("AP$") && cmd.length() >= 5) {
        bool ok;
        int m = cmd.mid(3, 1).toInt(&ok);
        if (ok) {
            int b = cmd.mid(4, 1).toInt(&ok);
            if (ok) {
                bool enabled = (m == 1);
                int bandwidth = qBound(0, b, 2);
                if (enabled != m_apfEnabledB || bandwidth != m_apfBandwidthB) {
                    m_apfEnabledB = enabled;
                    m_apfBandwidthB = bandwidth;
                    emit apfBChanged(m_apfEnabledB, m_apfBandwidthB);
                }
            }
        }
    }
    // Audio Peak Filter - Main RX (AP) - APmb where m=enabled(0/1), b=bandwidth(0/1/2)
    else if (cmd.startsWith("AP") && cmd.length() >= 4) {
        bool ok;
        int m = cmd.mid(2, 1).toInt(&ok);
        if (ok) {
            int b = cmd.mid(3, 1).toInt(&ok);
            if (ok) {
                bool enabled = (m == 1);
                int bandwidth = qBound(0, b, 2);
                if (enabled != m_apfEnabled || bandwidth != m_apfBandwidth) {
                    m_apfEnabled = enabled;
                    m_apfBandwidth = bandwidth;
                    emit apfChanged(m_apfEnabled, m_apfBandwidth);
                }
            }
        }
    }
    // VFO Link (LN) - LNn where n=0(not linked)/1(linked)
    else if (cmd.startsWith("LN") && cmd.length() >= 3) {
        bool ok;
        int ln = cmd.mid(2).toInt(&ok);
        if (ok) {
            bool linked = (ln == 1);
            if (linked != m_vfoLink) {
                m_vfoLink = linked;
                emit vfoLinkChanged(m_vfoLink);
            }
        }
    }
    // VFO B Lock (LK$) - LK$n where n=0(unlocked)/1(locked) - must check before LK
    else if (cmd.startsWith("LK$") && cmd.length() >= 4) {
        bool ok;
        int lk = cmd.mid(3).toInt(&ok);
        if (ok) {
            bool locked = (lk == 1);
            if (locked != m_lockB) {
                m_lockB = locked;
                emit lockBChanged(m_lockB);
            }
        }
    }
    // VFO A Lock (LK) - LKn where n=0(unlocked)/1(locked)
    else if (cmd.startsWith("LK") && cmd.length() >= 3) {
        bool ok;
        int lk = cmd.mid(2).toInt(&ok);
        if (ok) {
            bool locked = (lk == 1);
            if (locked != m_lockA) {
                m_lockA = locked;
                emit lockAChanged(m_lockA);
            }
        }
    }
    // LO - Line Out levels
    // Format: LOlllrrrm where lll=left(000-040), rrr=right(000-040), m=mode(0/1)
    else if (cmd.startsWith("LO") && cmd.length() >= 9) {
        bool okL, okR;
        int left = cmd.mid(2, 3).toInt(&okL);
        int right = cmd.mid(5, 3).toInt(&okR);
        int mode = cmd.mid(8, 1).toInt();

        if (okL && okR && left >= 0 && left <= 40 && right >= 0 && right <= 40) {
            bool changed = false;
            if (left != m_lineOutLeft) {
                m_lineOutLeft = left;
                changed = true;
            }
            if (right != m_lineOutRight) {
                m_lineOutRight = right;
                changed = true;
            }
            if ((mode == 1) != m_lineOutRightEqualsLeft) {
                m_lineOutRightEqualsLeft = (mode == 1);
                changed = true;
            }
            if (changed)
                emit lineOutChanged();
        }
    }
    // LI - Line In levels and source
    // Format: LIuuullls where uuu=soundcard(000-250), lll=linein(000-250), s=source(0/1)
    else if (cmd.startsWith("LI") && cmd.length() >= 9) {
        bool okS, okL;
        int soundCard = cmd.mid(2, 3).toInt(&okS);
        int lineIn = cmd.mid(5, 3).toInt(&okL);
        int source = cmd.mid(8, 1).toInt();

        if (okS && okL && soundCard >= 0 && soundCard <= 250 && lineIn >= 0 && lineIn <= 250 &&
            (source == 0 || source == 1)) {
            bool changed = false;
            if (soundCard != m_lineInSoundCard) {
                m_lineInSoundCard = soundCard;
                changed = true;
            }
            if (lineIn != m_lineInJack) {
                m_lineInJack = lineIn;
                changed = true;
            }
            if (source != m_lineInSource) {
                m_lineInSource = source;
                changed = true;
            }
            if (changed)
                emit lineInChanged();
        }
    }
    // MI - Mic Input Select
    // Format: MIn where n=0-4 (0=front, 1=rear, 2=line in, 3=front+line, 4=rear+line)
    else if (cmd.startsWith("MI") && cmd.length() >= 3) {
        int input = cmd.mid(2, 1).toInt();
        if (input >= 0 && input <= 4 && input != m_micInput) {
            m_micInput = input;
            emit micInputChanged(m_micInput);
        }
    }
    // MS - Mic Setup
    // Format: MSabcde where a=frontPreamp(0-2), b=frontBias(0-1), c=frontButtons(0-1),
    //         d=rearPreamp(0-1), e=rearBias(0-1)
    else if (cmd.startsWith("MS") && cmd.length() >= 7) {
        int frontPreamp = cmd.mid(2, 1).toInt();
        int frontBias = cmd.mid(3, 1).toInt();
        int frontButtons = cmd.mid(4, 1).toInt();
        int rearPreamp = cmd.mid(5, 1).toInt();
        int rearBias = cmd.mid(6, 1).toInt();

        bool changed = false;
        if (frontPreamp >= 0 && frontPreamp <= 2 && frontPreamp != m_micFrontPreamp) {
            m_micFrontPreamp = frontPreamp;
            changed = true;
        }
        if ((frontBias == 0 || frontBias == 1) && frontBias != m_micFrontBias) {
            m_micFrontBias = frontBias;
            changed = true;
        }
        if ((frontButtons == 0 || frontButtons == 1) && frontButtons != m_micFrontButtons) {
            m_micFrontButtons = frontButtons;
            changed = true;
        }
        if ((rearPreamp == 0 || rearPreamp == 1) && rearPreamp != m_micRearPreamp) {
            m_micRearPreamp = rearPreamp;
            changed = true;
        }
        if ((rearBias == 0 || rearBias == 1) && rearBias != m_micRearBias) {
            m_micRearBias = rearBias;
            changed = true;
        }
        if (changed)
            emit micSetupChanged();
    }
    // Text Decode Sub RX (TD$) - must check before TD
    else if (cmd.startsWith("TD$") && cmd.length() >= 6) {
        int mode = cmd.mid(3, 1).toInt();
        int threshold = cmd.mid(4, 1).toInt();
        int lines = cmd.mid(5, 1).toInt();

        bool changed = false;
        if (mode != m_textDecodeModeB) {
            m_textDecodeModeB = mode;
            changed = true;
        }
        if (threshold != m_textDecodeThresholdB) {
            m_textDecodeThresholdB = threshold;
            changed = true;
        }
        if (lines != m_textDecodeLinesB && lines >= 1 && lines <= 9) {
            m_textDecodeLinesB = lines;
            changed = true;
        }
        if (changed)
            emit textDecodeBChanged();
    }
    // Text Decode Main RX (TD)
    else if (cmd.startsWith("TD") && cmd.length() >= 5) {
        int mode = cmd.mid(2, 1).toInt();
        int threshold = cmd.mid(3, 1).toInt();
        int lines = cmd.mid(4, 1).toInt();
        qDebug() << "TD received: mode=" << mode << "threshold=" << threshold << "lines=" << lines;

        bool changed = false;
        if (mode != m_textDecodeMode) {
            m_textDecodeMode = mode;
            changed = true;
        }
        if (threshold != m_textDecodeThreshold) {
            m_textDecodeThreshold = threshold;
            changed = true;
        }
        if (lines != m_textDecodeLines && lines >= 1 && lines <= 9) {
            m_textDecodeLines = lines;
            changed = true;
        }
        if (changed)
            emit textDecodeChanged();
    }
    // Text Buffer Sub RX (TB$) - must check before TB
    // Format: TB$trrC; where t=tx queue (1 digit), rr=rx count (2 digits), C=character(s)
    else if (cmd.startsWith("TB$") && cmd.length() >= 6) {
        // Extract character(s) after header (TB$ + t + rr = 6 chars)
        QString text = cmd.mid(6);
        if (text.endsWith(';'))
            text.chop(1);
        if (!text.isEmpty()) {
            qDebug() << "TB$ received (Sub), char:" << text;
            emit textBufferReceived(text, true); // Sub RX
        }
    }
    // Text Buffer Main RX (TB) - decoded text from K4
    // Format: TBtrrC; where t=tx queue (1 digit), rr=rx count (2 digits), C=character(s)
    // Example: TB002E ; means t=0, rr=02, char="E "
    else if (cmd.startsWith("TB") && cmd.length() >= 5) {
        // Extract character(s) after header (TB + t + rr = 5 chars)
        QString text = cmd.mid(5);
        // Remove trailing semicolon if present
        if (text.endsWith(';'))
            text.chop(1);
        if (!text.isEmpty()) {
            qDebug() << "TB received (Main), char:" << text;
            emit textBufferReceived(text, false); // Main RX
        }
    }
    // Filter Position Sub RX (FP$) - must check before FP
    else if (cmd.startsWith("FP$") && cmd.length() > 3) {
        bool ok;
        int fp = cmd.mid(3).toInt(&ok);
        if (ok && fp >= 1 && fp <= 3 && fp != m_filterPositionB) {
            m_filterPositionB = fp;
            emit filterPositionBChanged(m_filterPositionB);
        }
    }
    // Filter Position Main RX (FP)
    else if (cmd.startsWith("FP") && cmd.length() > 2) {
        bool ok;
        int fp = cmd.mid(2).toInt(&ok);
        if (ok && fp >= 1 && fp <= 3 && fp != m_filterPosition) {
            m_filterPosition = fp;
            emit filterPositionChanged(m_filterPosition);
        }
    }
    // Tuning Step SUB (VT$) - check before VT
    else if (cmd.startsWith("VT$") && cmd.length() > 3) {
        QString vtStr = cmd.mid(3); // Skip "VT$"
        if (!vtStr.isEmpty()) {
            bool ok;
            int step = vtStr.left(1).toInt(&ok);
            if (ok) {
                int newStep = qBound(0, step, 5);
                if (newStep != m_tuningStepB) {
                    m_tuningStepB = newStep;
                    emit tuningStepBChanged(m_tuningStepB);
                }
            }
        }
    }
    // Tuning Step MAIN (VT)
    else if (cmd.startsWith("VT") && cmd.length() > 2) {
        QString vtStr = cmd.mid(2); // Skip "VT"
        if (!vtStr.isEmpty()) {
            bool ok;
            int step = vtStr.left(1).toInt(&ok);
            if (ok) {
                int newStep = qBound(0, step, 5);
                if (newStep != m_tuningStep) {
                    m_tuningStep = newStep;
                    emit tuningStepChanged(m_tuningStep);
                }
            }
        }
    }
    // VOX (VX) - VXmn where m=mode (C=CW, V=Voice, D=Data), n=0/1
    else if (cmd.startsWith("VX") && cmd.length() >= 4) {
        QChar mode = cmd.at(2);
        bool enabled = (cmd.at(3) == '1');
        bool changed = false;
        if (mode == 'C' && m_voxCW != enabled) {
            m_voxCW = enabled;
            changed = true;
        } else if (mode == 'V' && m_voxVoice != enabled) {
            m_voxVoice = enabled;
            changed = true;
        } else if (mode == 'D' && m_voxData != enabled) {
            m_voxData = enabled;
            changed = true;
        }
        if (changed) {
            emit voxChanged(voxEnabled());
        }
    }
    // VOX Gain (VG) - VGmnnn where m=V(voice)/D(data), nnn=000-060
    else if (cmd.startsWith("VG") && cmd.length() >= 5) {
        QChar modeChar = cmd.at(2);
        bool ok;
        int gain = cmd.mid(3, 3).toInt(&ok);
        if (ok && gain >= 0 && gain <= 60) {
            if (modeChar == 'V' && gain != m_voxGainVoice) {
                m_voxGainVoice = gain;
                emit voxGainChanged(0, gain);
            } else if (modeChar == 'D' && gain != m_voxGainData) {
                m_voxGainData = gain;
                emit voxGainChanged(1, gain);
            }
        }
    }
    // Anti-VOX (VI) - VInnn where nnn=000-060
    else if (cmd.startsWith("VI") && cmd.length() >= 5) {
        bool ok;
        int level = cmd.mid(2, 3).toInt(&ok);
        if (ok && level >= 0 && level <= 60 && level != m_antiVox) {
            m_antiVox = level;
            emit antiVoxChanged(level);
        }
    }
    // ESSB/SSB TX Bandwidth (ES) - ESnbb where n=0/1, bb=bandwidth
    // SSB mode (n=0): bb range is 24-28 (2.4-2.8 kHz)
    // ESSB mode (n=1): bb range is 30-45 (3.0-4.5 kHz)
    else if (cmd.startsWith("ES") && cmd.length() >= 4) {
        bool ok;
        int modeVal = cmd.mid(2, 1).toInt(&ok);
        if (ok && (modeVal == 0 || modeVal == 1)) {
            bool newEssb = (modeVal == 1);
            int newBw = -1;
            // Check if bandwidth is included (full format ESnbb)
            if (cmd.length() >= 5) {
                newBw = cmd.mid(3, 2).toInt(&ok);
                // Accept valid range for either mode (24-45 covers both)
                if (!ok || newBw < 24 || newBw > 45) {
                    newBw = m_ssbTxBw; // Keep existing if invalid
                }
            }
            bool changed = false;
            if (newEssb != m_essbEnabled) {
                m_essbEnabled = newEssb;
                changed = true;
            }
            if (newBw >= 24 && newBw <= 45 && newBw != m_ssbTxBw) {
                m_ssbTxBw = newBw;
                changed = true;
            }
            if (changed) {
                emit essbChanged(m_essbEnabled, m_ssbTxBw);
            }
        }
    }
    // IF Shift Sub RX (IS$) - check BEFORE IS to avoid prefix collision
    else if (cmd.startsWith("IS$") && cmd.length() > 3) {
        bool ok;
        int is = cmd.mid(3).toInt(&ok);
        if (ok && is != m_ifShiftB) {
            m_ifShiftB = is;
            emit ifShiftBChanged(m_ifShiftB);
        }
    }
    // IF Shift (IS) - IS0099 format (0-99, 50=centered)
    else if (cmd.startsWith("IS") && cmd.length() > 2) {
        bool ok;
        int is = cmd.mid(2).toInt(&ok);
        if (ok && is != m_ifShift) {
            m_ifShift = is;
            emit ifShiftChanged(m_ifShift);
        }
    }
    // CW Sidetone Pitch (CW) - CWnn where nn=pitch/10 (25-95, so 50=500Hz)
    else if (cmd.startsWith("CW") && cmd.length() >= 4 && !cmd.startsWith("CW-")) {
        bool ok;
        int pitchCode = cmd.mid(2).toInt(&ok);
        if (ok && pitchCode >= 25 && pitchCode <= 95) {
            int pitchHz = pitchCode * 10; // Convert code to Hz
            if (pitchHz != m_cwPitch) {
                m_cwPitch = pitchHz;
                emit cwPitchChanged(m_cwPitch);
            }
        }
    }
    // Split TX/RX (FT)
    else if (cmd.startsWith("FT") && cmd.length() > 2) {
        bool newSplit = (cmd.mid(2) == "1");
        if (newSplit != m_splitEnabled) {
            m_splitEnabled = newSplit;
            emit splitChanged(m_splitEnabled);
        }
    }
    // Sub Receiver (SB)
    // SB0 = off, SB1 = on (standalone), SB3 = on (for diversity)
    // Always emit to ensure UI syncs on initial connect (can't use -1 trick for bools)
    else if (cmd.startsWith("SB") && cmd.length() > 2) {
        m_subReceiverEnabled = (cmd.mid(2) != "0"); // Any non-zero value means SUB is enabled
        emit subRxEnabledChanged(m_subReceiverEnabled);
    }
    // Diversity (DV)
    else if (cmd.startsWith("DV") && cmd.length() > 2) {
        bool newState = (cmd.mid(2) == "1");
        if (newState != m_diversityEnabled) {
            m_diversityEnabled = newState;
            emit diversityChanged(m_diversityEnabled);
        }
    }
    // Antenna (AN) - TX antenna
    else if (cmd.startsWith("AN") && cmd.length() > 2) {
        bool ok;
        int an = cmd.mid(2).toInt(&ok);
        if (ok && an >= 1 && an <= 6 && an != m_selectedAntenna) {
            m_selectedAntenna = an;
            emit antennaChanged(m_selectedAntenna, m_receiveAntenna, m_receiveAntennaSub);
        }
    }
    // RX Antenna Main (AR)
    // AR values: 0=OFF, 1=EXT XVTR, 2=INT XVTR, 3=RX1, 4=RX2, 5=ANT1, 6=ANT2, 7=ANT3
    else if (cmd.startsWith("AR") && !cmd.startsWith("AR$") && cmd.length() > 2) {
        bool ok;
        int ar = cmd.mid(2).toInt(&ok);
        if (ok && ar >= 0 && ar <= 7 && ar != m_receiveAntenna) {
            m_receiveAntenna = ar;
            emit antennaChanged(m_selectedAntenna, m_receiveAntenna, m_receiveAntennaSub);
        }
    }
    // RX Antenna Sub (AR$)
    // AR$ values: 0=RX2, 1=RX1, 2=ANT1, 3=ANT2, 4=ANT3, 5==TX, 6==OPP TX
    else if (cmd.startsWith("AR$") && cmd.length() > 3) {
        bool ok;
        int ar = cmd.mid(3).toInt(&ok);
        if (ok && ar >= 0 && ar <= 7 && ar != m_receiveAntennaSub) {
            m_receiveAntennaSub = ar;
            emit antennaChanged(m_selectedAntenna, m_receiveAntenna, m_receiveAntennaSub);
        }
    }
    // ATU Mode (AT) - AT0=not installed, AT1=bypass, AT2=auto
    else if (cmd.startsWith("AT") && cmd.length() >= 3) {
        bool ok;
        int at = cmd.mid(2).toInt(&ok);
        if (ok && at >= 0 && at <= 2 && at != m_atuMode) {
            m_atuMode = at;
            emit atuModeChanged(m_atuMode);
        }
    }
    // TX Test Mode (TS) - TS0=off, TS1=on
    else if (cmd.startsWith("TS") && cmd.length() >= 3) {
        bool enabled = (cmd.mid(2, 1) == "1");
        if (enabled != m_testMode) {
            m_testMode = enabled;
            emit testModeChanged(m_testMode);
        }
    }
    // B SET (BS) - BS0=off, BS1=on (controls feature menu VFO targeting)
    else if (cmd.startsWith("BS") && cmd.length() >= 3) {
        bool enabled = (cmd.mid(2, 1) == "1");
        if (enabled != m_bSetEnabled) {
            m_bSetEnabled = enabled;
            emit bSetChanged(m_bSetEnabled);
        }
    }
    // Radio ID (ID)
    else if (cmd.startsWith("ID") && cmd.length() > 2) {
        m_radioID = cmd.mid(2);
    }
    // Option Modules (OM)
    else if (cmd.startsWith("OM") && cmd.length() > 2) {
        m_optionModules = cmd.mid(2).trimmed();
        // Parse radio model
        if (m_optionModules.length() > 8) {
            bool hasS = m_optionModules.length() > 3 && m_optionModules[3] == 'S';
            bool hasH = m_optionModules.length() > 4 && m_optionModules[4] == 'H';
            bool has4 = m_optionModules.length() > 8 && m_optionModules[8] == '4';

            if (hasH && hasS && has4) {
                m_radioModel = "K4HD";
            } else if (hasS && has4) {
                m_radioModel = "K4D";
            } else if (has4) {
                m_radioModel = "K4";
            }
        }
    }
    // Firmware Version (RV.)
    else if (cmd.startsWith("RV.") && cmd.length() > 3) {
        QString versionData = cmd.mid(3);
        int dashIndex = versionData.indexOf('-');
        if (dashIndex > 0) {
            QString component = versionData.left(dashIndex);
            QString version = versionData.mid(dashIndex + 1);
            m_firmwareVersions[component] = version;
        }
    }
    // Power Setting (PC) - PCxxxL (QRP 0-10W) or PCxxxH (QRO 11-110W)
    else if (cmd.startsWith("PC") && cmd.length() >= 5) {
        QString powerStr = cmd.mid(2);
        QChar modeChar = powerStr.right(1).at(0);
        QString valueStr = powerStr.left(powerStr.length() - 1);
        bool ok;
        int value = valueStr.toInt(&ok);
        if (ok) {
            if (modeChar == 'L') {
                // QRP mode: value is in tenths of watts (0-100 = 0.0-10.0W)
                m_rfPower = value / 10.0;
                m_isQrpMode = true;
            } else if (modeChar == 'H') {
                // QRO mode: value is in watts (11-110W)
                m_rfPower = value;
                m_isQrpMode = false;
            }
            emit rfPowerChanged(m_rfPower, m_isQrpMode);
        }
    }
    // TX Meter Data (TM) - TMaaabbbcccddd; (ALC, CMP, FWD, SWR)
    else if (cmd.startsWith("TM") && cmd.length() >= 14) {
        QString data = cmd.mid(2);
        if (data.length() >= 12) {
            bool ok1, ok2, ok3, ok4;
            int alc = data.mid(0, 3).toInt(&ok1);
            int cmp = data.mid(3, 3).toInt(&ok2);
            int fwd = data.mid(6, 3).toInt(&ok3);
            int swrRaw = data.mid(9, 3).toInt(&ok4);
            if (ok1 && ok2 && ok3 && ok4) {
                m_alcMeter = alc;
                m_compressionDb = cmp;
                // FWD is watts in QRO, tenths in QRP
                m_forwardPower = m_isQrpMode ? fwd / 10.0 : fwd;
                m_swrMeter = swrRaw / 10.0; // SWR in 1/10th units
                emit txMeterChanged(m_alcMeter, m_compressionDb, m_forwardPower, m_swrMeter);
                emit swrChanged(m_swrMeter);
            }
        }
    }
    // Power Supply Info (SIFP) - SIFPVS:xx.xx,IS:x.xx,...
    else if (cmd.startsWith("SIFP")) {
        QString data = cmd.mid(4);
        // Parse VS (voltage)
        int vsIndex = data.indexOf("VS:");
        if (vsIndex >= 0) {
            int commaIndex = data.indexOf(',', vsIndex);
            QString vsStr =
                (commaIndex > vsIndex) ? data.mid(vsIndex + 3, commaIndex - vsIndex - 3) : data.mid(vsIndex + 3);
            bool ok;
            double voltage = vsStr.toDouble(&ok);
            if (ok && voltage != m_supplyVoltage) {
                m_supplyVoltage = voltage;
                emit supplyVoltageChanged(m_supplyVoltage);
            }
        }
        // Parse IS (current)
        int isIndex = data.indexOf("IS:");
        if (isIndex >= 0) {
            int commaIndex = data.indexOf(',', isIndex);
            QString isStr =
                (commaIndex > isIndex) ? data.mid(isIndex + 3, commaIndex - isIndex - 3) : data.mid(isIndex + 3);
            bool ok;
            double current = isStr.toDouble(&ok);
            if (ok && current != m_supplyCurrent) {
                m_supplyCurrent = current;
                emit supplyCurrentChanged(m_supplyCurrent);
            }
        }
    }
    // Antenna Name (ACN) - ACNnssssss; where n is 1-5
    else if (cmd.startsWith("ACN") && cmd.length() >= 4) {
        bool ok;
        int antNum = cmd.mid(3, 1).toInt(&ok);
        if (ok && antNum >= 1 && antNum <= 5) {
            QString name = cmd.mid(4);
            if (name != m_antennaNames.value(antNum)) {
                m_antennaNames[antNum] = name;
                emit antennaNameChanged(antNum, name);
            }
        }
    }
    // Main RX Antenna Config (ACM) - ACMzabcdefg where z=displayAll, a-g=antenna enables
    else if (cmd.startsWith("ACM") && cmd.length() >= 11) {
        QString data = cmd.mid(3); // Skip "ACM"
        bool changed = false;
        bool newDisplayAll = (data[0] == '1');
        if (newDisplayAll != m_mainRxDisplayAll) {
            m_mainRxDisplayAll = newDisplayAll;
            changed = true;
        }
        for (int i = 0; i < 7 && i + 1 < data.length(); i++) {
            bool enabled = (data[i + 1] == '1');
            if (enabled != m_mainRxAntMask[i]) {
                m_mainRxAntMask[i] = enabled;
                changed = true;
            }
        }
        if (changed) {
            emit mainRxAntCfgChanged();
        }
    }
    // Sub RX Antenna Config (ACS) - ACSzabcdefg where z=displayAll, a-g=antenna enables
    else if (cmd.startsWith("ACS") && cmd.length() >= 11) {
        QString data = cmd.mid(3); // Skip "ACS"
        bool changed = false;
        bool newDisplayAll = (data[0] == '1');
        if (newDisplayAll != m_subRxDisplayAll) {
            m_subRxDisplayAll = newDisplayAll;
            changed = true;
        }
        for (int i = 0; i < 7 && i + 1 < data.length(); i++) {
            bool enabled = (data[i + 1] == '1');
            if (enabled != m_subRxAntMask[i]) {
                m_subRxAntMask[i] = enabled;
                changed = true;
            }
        }
        if (changed) {
            emit subRxAntCfgChanged();
        }
    }
    // TX Antenna Config (ACT) - ACTzabc where z=displayAll, a-c=antenna enables
    else if (cmd.startsWith("ACT") && cmd.length() >= 7) {
        QString data = cmd.mid(3); // Skip "ACT"
        bool changed = false;
        bool newDisplayAll = (data[0] == '1');
        if (newDisplayAll != m_txDisplayAll) {
            m_txDisplayAll = newDisplayAll;
            changed = true;
        }
        for (int i = 0; i < 3 && i + 1 < data.length(); i++) {
            bool enabled = (data[i + 1] == '1');
            if (enabled != m_txAntMask[i]) {
                m_txAntMask[i] = enabled;
                changed = true;
            }
        }
        if (changed) {
            emit txAntCfgChanged();
        }
    }
    // RIT (RT) - RT0/RT1 (not RT$ which is a different command)
    else if (cmd.startsWith("RT") && cmd.length() >= 3 && (cmd[2] == '0' || cmd[2] == '1')) {
        bool newRit = (cmd[2] == '1');
        if (newRit != m_ritEnabled) {
            m_ritEnabled = newRit;
            emit ritXitChanged(m_ritEnabled, m_xitEnabled, m_ritXitOffset);
        }
    }
    // XIT (XT) - XT0/XT1 (not XT$ which is a different command)
    else if (cmd.startsWith("XT") && cmd.length() >= 3 && (cmd[2] == '0' || cmd[2] == '1')) {
        bool newXit = (cmd[2] == '1');
        if (newXit != m_xitEnabled) {
            m_xitEnabled = newXit;
            emit ritXitChanged(m_ritEnabled, m_xitEnabled, m_ritXitOffset);
        }
    }
    // RIT/XIT Offset (RO) - RO+/-nnnnn
    else if (cmd.startsWith("RO") && cmd.length() >= 3) {
        QString offsetStr = cmd.mid(2);
        bool ok;
        int offset = offsetStr.toInt(&ok);
        if (ok && offset != m_ritXitOffset) {
            m_ritXitOffset = offset;
            emit ritXitChanged(m_ritEnabled, m_xitEnabled, m_ritXitOffset);
        }
    }
    // Message Bank (MN) - MN1 or MN2
    else if (cmd.startsWith("MN") && cmd.length() >= 3) {
        bool ok;
        int bank = cmd.mid(2, 1).toInt(&ok);
        if (ok && (bank == 1 || bank == 2) && bank != m_messageBank) {
            m_messageBank = bank;
            emit messageBankChanged(m_messageBank);
        }
    }
    // Panadapter Reference Level (#REF) - #REF-110 (not #REF$ which is Sub RX)
    else if (cmd.startsWith("#REF") && !cmd.startsWith("#REF$") && cmd.length() > 4) {
        QString refStr = cmd.mid(4); // Get "-110" from "#REF-110"
        bool ok;
        int level = refStr.toInt(&ok);
        if (ok && level != m_refLevel) {
            m_refLevel = level;
            emit refLevelChanged(m_refLevel);
        }
    }
    // Sub RX Panadapter Reference Level (#REF$) - #REF$-110
    else if (cmd.startsWith("#REF$") && cmd.length() > 5) {
        QString refStr = cmd.mid(5); // Get "-110" from "#REF$-110"
        bool ok;
        int level = refStr.toInt(&ok);
        if (ok && level != m_refLevelB) {
            m_refLevelB = level;
            emit refLevelBChanged(m_refLevelB);
        }
    }
    // Panadapter Scale (#SCL) - #SCL75 (10-150) - GLOBAL setting, applies to both panadapters
    // Higher values = more compressed display (signals appear weaker)
    // Lower values = more expanded display (signals appear stronger)
    // Note: There is NO #SCL$ variant - scale is a global setting
    else if (cmd.startsWith("#SCL") && cmd.length() > 4) {
        QString scaleStr = cmd.mid(4); // Get "75" from "#SCL75"
        bool ok;
        int scale = scaleStr.toInt(&ok);
        if (ok && scale >= 10 && scale <= 150 && scale != m_scale) {
            m_scale = scale;
            emit scaleChanged(m_scale);
        }
    }
    // Panadapter Span (#SPN) - #SPN10000 (Hz) - not #SPN$ which is Sub RX
    else if (cmd.startsWith("#SPN") && !cmd.startsWith("#SPN$") && cmd.length() > 4) {
        QString spanStr = cmd.mid(4); // Get "10000" from "#SPN10000"
        bool ok;
        int span = spanStr.toInt(&ok);
        if (ok && span > 0 && span != m_spanHz) {
            m_spanHz = span;
            emit spanChanged(m_spanHz);
        }
    }
    // Sub RX Panadapter Span (#SPN$) - #SPN$10000 (Hz)
    else if (cmd.startsWith("#SPN$") && cmd.length() > 5) {
        QString spanStr = cmd.mid(5); // Get "10000" from "#SPN$10000"
        bool ok;
        int span = spanStr.toInt(&ok);
        if (ok && span > 0 && span != m_spanHzB) {
            m_spanHzB = span;
            emit spanBChanged(m_spanHzB);
        }
    }
    // Mini-Pan Sub RX (#MP$) - #MP$0 or #MP$1
    else if (cmd.startsWith("#MP$") && cmd.length() > 4) {
        bool enabled = (cmd.mid(4, 1) == "1");
        if (m_miniPanBEnabled != enabled) {
            m_miniPanBEnabled = enabled;
            emit miniPanBEnabledChanged(enabled);
        }
    }
    // Mini-Pan Main RX (#MP) - #MP0 or #MP1
    else if (cmd.startsWith("#MP") && cmd.length() > 3) {
        bool enabled = (cmd.mid(3, 1) == "1");
        if (m_miniPanAEnabled != enabled) {
            m_miniPanAEnabled = enabled;
            emit miniPanAEnabledChanged(enabled);
        }
    }
    // Dual Panadapter Mode - EXT (#HDPM) - must check before #DPM
    else if (cmd.startsWith("#HDPM") && cmd.length() > 5) {
        bool ok;
        int mode = cmd.mid(5, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 2 && mode != m_dualPanModeExt) {
            m_dualPanModeExt = mode;
            emit dualPanModeExtChanged(mode);
        }
    }
    // Dual Panadapter Mode - LCD (#DPM)
    else if (cmd.startsWith("#DPM") && cmd.length() > 4) {
        bool ok;
        int mode = cmd.mid(4, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 2 && mode != m_dualPanModeLcd) {
            m_dualPanModeLcd = mode;
            emit dualPanModeLcdChanged(mode);
        }
    }
    // Display Mode - EXT (#HDSM) - must check before #DSM
    else if (cmd.startsWith("#HDSM") && cmd.length() > 5) {
        bool ok;
        int mode = cmd.mid(5, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 1 && mode != m_displayModeExt) {
            m_displayModeExt = mode;
            emit displayModeExtChanged(mode);
        }
    }
    // Display Mode - LCD (#DSM)
    else if (cmd.startsWith("#DSM") && cmd.length() > 4) {
        bool ok;
        int mode = cmd.mid(4, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 1 && mode != m_displayModeLcd) {
            m_displayModeLcd = mode;
            emit displayModeLcdChanged(mode);
        }
    }
    // Display FPS (#FPS) - #FPS12 to #FPS30
    else if (cmd.startsWith("#FPS") && cmd.length() > 4) {
        bool ok;
        // Parse value after "#FPS" (could be 1 or 2 digits)
        QString fpsStr = cmd.mid(4);
        if (fpsStr.endsWith(';')) {
            fpsStr.chop(1);
        }
        int fps = fpsStr.toInt(&ok);
        if (ok && fps >= 12 && fps <= 30 && fps != m_displayFps) {
            m_displayFps = fps;
            emit displayFpsChanged(fps);
        }
    }
    // Waterfall Color (#WFC) - #WFC0-4 or #WFC$0-4 for VFO B
    else if (cmd.startsWith("#WFC") && cmd.length() > 4) {
        int offset = cmd.startsWith("#WFC$") ? 5 : 4;
        if (cmd.length() > offset) {
            bool ok;
            int color = cmd.mid(offset, 1).toInt(&ok);
            if (ok && color >= 0 && color <= 4 && color != m_waterfallColor) {
                m_waterfallColor = color;
                emit waterfallColorChanged(color);
            }
        }
    }
    // Averaging (#AVG) - #AVGnn (1-20)
    else if (cmd.startsWith("#AVG") && cmd.length() > 4) {
        bool ok;
        int avg = cmd.mid(4).toInt(&ok);
        if (ok && avg >= 1 && avg <= 20 && avg != m_averaging) {
            m_averaging = avg;
            emit averagingChanged(avg);
        }
    }
    // Peak Mode (#PKM) - #PKM0/1
    else if (cmd.startsWith("#PKM") && cmd.length() > 4) {
        bool enabled = (cmd.mid(4, 1) == "1");
        if (enabled != m_peakMode) {
            m_peakMode = enabled;
            emit peakModeChanged(enabled);
        }
    }
    // Fixed Tune (#FXT) - #FXT0/1 (0=track, 1=fixed)
    else if (cmd.startsWith("#FXT") && cmd.length() > 4) {
        bool ok;
        int fxt = cmd.mid(4, 1).toInt(&ok);
        if (ok && fxt >= 0 && fxt <= 1 && fxt != m_fixedTune) {
            m_fixedTune = fxt;
            emit fixedTuneChanged(m_fixedTune, m_fixedTuneMode);
        }
    }
    // Fixed Tune Mode (#FXA) - #FXA0-4
    else if (cmd.startsWith("#FXA") && cmd.length() > 4) {
        bool ok;
        int fxa = cmd.mid(4, 1).toInt(&ok);
        if (ok && fxa >= 0 && fxa <= 4 && fxa != m_fixedTuneMode) {
            m_fixedTuneMode = fxa;
            emit fixedTuneChanged(m_fixedTune, m_fixedTuneMode);
        }
    }
    // Freeze (#FRZ) - #FRZ0/1
    else if (cmd.startsWith("#FRZ") && cmd.length() > 4) {
        bool enabled = (cmd.mid(4, 1) == "1");
        if (enabled != m_freeze) {
            m_freeze = enabled;
            emit freezeChanged(enabled);
        }
    }
    // VFO A Cursor Mode (#VFA) - #VFA0-3
    else if (cmd.startsWith("#VFA") && cmd.length() > 4) {
        bool ok;
        int mode = cmd.mid(4, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 3 && mode != m_vfoACursor) {
            m_vfoACursor = mode;
            emit vfoACursorChanged(mode);
        }
    }
    // VFO B Cursor Mode (#VFB) - #VFB0-3
    else if (cmd.startsWith("#VFB") && cmd.length() > 4) {
        bool ok;
        int mode = cmd.mid(4, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 3 && mode != m_vfoBCursor) {
            m_vfoBCursor = mode;
            emit vfoBCursorChanged(mode);
        }
    }
    // Auto-Ref Level (#AR) - Format: #ARaadd+oom;
    // aa=averaging(01-20), dd=debounce(04-09), +oo=offset(-08 to +08), m=mode(1=auto,0=manual)
    // Example: #AR1203+041; means averaging=12, debounce=03, offset=+04, mode=1 (AUTO)
    else if (cmd.startsWith("#AR") && cmd.length() >= 12) {
        QChar modeChar = cmd.at(cmd.length() - 2); // Last char before ';'
        bool enabled = (modeChar == '1');          // 1=auto, 0=manual
        int newVal = enabled ? 1 : 0;
        if (newVal != m_autoRefLevel) {
            m_autoRefLevel = newVal;
            emit autoRefLevelChanged(enabled);
        }
    }
    // DDC Noise Blanker Mode (#NB$) - #NB$0/1/2
    else if (cmd.startsWith("#NB$") && cmd.length() > 4) {
        bool ok;
        int mode = cmd.mid(4, 1).toInt(&ok);
        if (ok && mode >= 0 && mode <= 2 && mode != m_ddcNbMode) {
            m_ddcNbMode = mode;
            emit ddcNbModeChanged(mode);
        }
    }
    // DDC Noise Blanker Level (#NBL$) - #NBL$0-14
    else if (cmd.startsWith("#NBL$") && cmd.length() > 5) {
        bool ok;
        int level = cmd.mid(5).chopped(cmd.endsWith(';') ? 1 : 0).toInt(&ok);
        if (ok && level >= 0 && level <= 14 && level != m_ddcNbLevel) {
            m_ddcNbLevel = level;
            emit ddcNbLevelChanged(level);
        }
    }
    // Waterfall Height - EXT (#HWFH) - must check BEFORE #WFH (longer prefix)
    else if (cmd.startsWith("#HWFH") && cmd.length() > 5) {
        bool ok;
        int percent = cmd.mid(5).toInt(&ok);
        if (ok && percent >= 0 && percent <= 100 && percent != m_waterfallHeightExt) {
            m_waterfallHeightExt = percent;
            emit waterfallHeightExtChanged(percent);
        }
    }
    // Waterfall Height - LCD (#WFH)
    else if (cmd.startsWith("#WFH") && cmd.length() > 4) {
        bool ok;
        int percent = cmd.mid(4).toInt(&ok);
        if (ok && percent >= 0 && percent <= 100 && percent != m_waterfallHeight) {
            m_waterfallHeight = percent;
            emit waterfallHeightChanged(percent);
        }
    }
    // Data Sub-Mode Sub RX (DT$) - must check before DT
    else if (cmd.startsWith("DT$") && cmd.length() >= 4) {
        bool ok;
        int subMode = cmd.mid(3, 1).toInt(&ok);
        if (ok && subMode >= 0 && subMode <= 3) {
            // Ignore echoes within 500ms of optimistic update (K4 echoes stale values)
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            bool inCooldown = (now - m_dataSubModeBOptimisticTime) < 500;
            if (!inCooldown && subMode != m_dataSubModeB) {
                m_dataSubModeB = subMode;
                emit dataSubModeBChanged(subMode);
            }
        }
    }
    // Data Sub-Mode Main RX (DT) - 0=DATA-A, 1=AFSK-A, 2=FSK-D, 3=PSK-D
    else if (cmd.startsWith("DT") && cmd.length() >= 3) {
        bool ok;
        int subMode = cmd.mid(2, 1).toInt(&ok);
        if (ok && subMode >= 0 && subMode <= 3) {
            // Ignore echoes within 500ms of optimistic update (K4 echoes stale values)
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            bool inCooldown = (now - m_dataSubModeOptimisticTime) < 500;
            if (!inCooldown && subMode != m_dataSubMode) {
                m_dataSubMode = subMode;
                emit dataSubModeChanged(subMode);
            }
        }
    }
    // Remote Client Stats (SIRC) - SIRCR:73.88,T:0.03,P:2,C:14,A:74
    // R=RX kB/s, T=TX kB/s, P=Ping ms, C=Connected time, A=Audio buffer ms
    // TODO: Wire these stats to UI status bar
    else if (cmd.startsWith("SIRC") && cmd.length() > 4) {
        // Currently parsed but not displayed - stats available for future UI integration
    }
    // RX Graphic Equalizer (RE) - RE+00+00+00+00+00+00+00+00
    // 8 bands, each is +XX or -XX (signed, -16 to +16 dB)
    // Bands: 100, 200, 400, 800, 1200, 1600, 2400, 3200 Hz
    // Note: Main RX and Sub RX share the same EQ settings
    else if (cmd.startsWith("RE") && cmd.length() >= 26) {
        // Format: RE+00+00+00+00+00+00+00+00 (2 + 8*3 = 26 chars minimum)
        QString data = cmd.mid(2); // Skip "RE"
        bool changed = false;
        int newBands[8];
        bool parseOk = true;

        // Parse each 3-character band value (+XX or -XX)
        for (int i = 0; i < 8 && parseOk; i++) {
            if (data.length() >= (i + 1) * 3) {
                QString bandStr = data.mid(i * 3, 3); // +00, -05, +16, etc.
                bool ok;
                int value = bandStr.toInt(&ok);
                if (ok && value >= -16 && value <= 16) {
                    newBands[i] = value;
                } else {
                    parseOk = false;
                }
            } else {
                parseOk = false;
            }
        }

        if (parseOk) {
            for (int i = 0; i < 8; i++) {
                if (m_rxEqBands[i] != newBands[i]) {
                    m_rxEqBands[i] = newBands[i];
                    changed = true;
                }
            }
            if (changed) {
                emit rxEqChanged();
            }
        }
    }
    // TX Graphic Equalizer (TE) - TE+00+00+00+00+00+00+00+00
    // 8 bands, each is +XX or -XX (signed, -16 to +16 dB)
    // Bands: 100, 200, 400, 800, 1200, 1600, 2400, 3200 Hz
    else if (cmd.startsWith("TE") && cmd.length() >= 26) {
        // Format: TE+00+00+00+00+00+00+00+00 (2 + 8*3 = 26 chars minimum)
        QString data = cmd.mid(2); // Skip "TE"
        bool changed = false;
        int newBands[8];
        bool parseOk = true;

        // Parse each 3-character band value (+XX or -XX)
        for (int i = 0; i < 8 && parseOk; i++) {
            if (data.length() >= (i + 1) * 3) {
                QString bandStr = data.mid(i * 3, 3); // +00, -05, +16, etc.
                bool ok;
                int value = bandStr.toInt(&ok);
                if (ok && value >= -16 && value <= 16) {
                    newBands[i] = value;
                } else {
                    parseOk = false;
                }
            } else {
                parseOk = false;
            }
        }

        if (parseOk) {
            for (int i = 0; i < 8; i++) {
                if (m_txEqBands[i] != newBands[i]) {
                    m_txEqBands[i] = newBands[i];
                    changed = true;
                }
            }
            if (changed) {
                emit txEqChanged();
            }
        }
    }

    emit stateUpdated();
}

RadioState::Mode RadioState::modeFromCode(int code) {
    switch (code) {
    case 1:
        return LSB;
    case 2:
        return USB;
    case 3:
        return CW;
    case 4:
        return FM;
    case 5:
        return AM;
    case 6:
        return DATA;
    case 7:
        return CW_R;
    case 9:
        return DATA_R;
    default:
        return USB;
    }
}

QString RadioState::modeToString(Mode mode) {
    switch (mode) {
    case LSB:
        return "LSB";
    case USB:
        return "USB";
    case CW:
        return "CW";
    case FM:
        return "FM";
    case AM:
        return "AM";
    case DATA:
        return "DATA";
    case CW_R:
        return "CW-R";
    case DATA_R:
        return "DATA-R";
    default:
        return "USB";
    }
}

QString RadioState::modeString() const {
    return modeToString(m_mode);
}

QString RadioState::dataSubModeToString(int subMode) {
    switch (subMode) {
    case 0:
        return "DATA"; // DATA-A
    case 1:
        return "AFSK"; // AFSK-A
    case 2:
        return "FSK"; // FSK-D
    case 3:
        return "PSK"; // PSK-D
    default:
        return "DATA";
    }
}

QString RadioState::modeStringFull() const {
    // For DATA/DATA-R modes, show the sub-mode instead
    if (m_mode == DATA || m_mode == DATA_R) {
        return dataSubModeToString(m_dataSubMode);
    }
    return modeToString(m_mode);
}

QString RadioState::modeStringFullB() const {
    // For DATA/DATA-R modes, show the sub-mode instead
    if (m_modeB == DATA || m_modeB == DATA_R) {
        return dataSubModeToString(m_dataSubModeB);
    }
    return modeToString(m_modeB);
}

QString RadioState::sMeterString() const {
    if (m_sMeter <= 9.0) {
        return QString("S%1").arg(static_cast<int>(m_sMeter));
    } else {
        int dbOver = static_cast<int>((m_sMeter - 9.0) * 10);
        return QString("S9+%1").arg(dbOver);
    }
}

QString RadioState::sMeterStringB() const {
    if (m_sMeterB <= 9.0) {
        return QString("S%1").arg(static_cast<int>(m_sMeterB));
    } else {
        int dbOver = static_cast<int>((m_sMeterB - 9.0) * 10);
        return QString("S9+%1").arg(dbOver);
    }
}

// Optimistic setters for scroll wheel updates (radio doesn't echo these commands)
void RadioState::setKeyerSpeed(int wpm) {
    if (m_keyerSpeed != wpm) {
        m_keyerSpeed = wpm;
        emit keyerSpeedChanged(m_keyerSpeed);
    }
}

void RadioState::setCwPitch(int pitchHz) {
    if (m_cwPitch != pitchHz) {
        m_cwPitch = pitchHz;
        emit cwPitchChanged(m_cwPitch);
    }
}

void RadioState::setRfPower(double watts) {
    if (m_rfPower != watts) {
        m_rfPower = watts;
        emit rfPowerChanged(m_rfPower, m_isQrpMode);
    }
}

void RadioState::setFilterBandwidth(int bwHz) {
    if (m_filterBandwidth != bwHz) {
        m_filterBandwidth = bwHz;
        emit filterBandwidthChanged(m_filterBandwidth);
    }
}

void RadioState::setIfShift(int shift) {
    if (m_ifShift != shift) {
        m_ifShift = shift;
        emit ifShiftChanged(m_ifShift);
    }
}

void RadioState::setFilterBandwidthB(int bwHz) {
    if (m_filterBandwidthB != bwHz) {
        m_filterBandwidthB = bwHz;
        emit filterBandwidthBChanged(m_filterBandwidthB);
    }
}

void RadioState::setIfShiftB(int shift) {
    if (m_ifShiftB != shift) {
        m_ifShiftB = shift;
        emit ifShiftBChanged(m_ifShiftB);
    }
}

void RadioState::setRfGain(int gain) {
    if (m_rfGain != gain) {
        m_rfGain = gain;
        emit rfGainChanged(m_rfGain);
    }
}

void RadioState::setSquelchLevel(int level) {
    if (m_squelchLevel != level) {
        m_squelchLevel = level;
        emit squelchChanged(m_squelchLevel);
    }
}

void RadioState::setRfGainB(int gain) {
    if (m_rfGainB != gain) {
        m_rfGainB = gain;
        emit rfGainBChanged(m_rfGainB);
    }
}

void RadioState::setSquelchLevelB(int level) {
    if (m_squelchLevelB != level) {
        m_squelchLevelB = level;
        emit squelchBChanged(m_squelchLevelB);
    }
}

void RadioState::setMicGain(int gain) {
    if (m_micGain != gain) {
        m_micGain = gain;
        emit micGainChanged(m_micGain);
    }
}

void RadioState::setCompression(int level) {
    if (m_compression != level) {
        m_compression = level;
        emit compressionChanged(m_compression);
    }
}

void RadioState::setMonitorLevel(int mode, int level) {
    level = qBound(0, level, 100);
    int *target = nullptr;
    switch (mode) {
    case 0:
        target = &m_monitorLevelCW;
        break;
    case 1:
        target = &m_monitorLevelData;
        break;
    case 2:
        target = &m_monitorLevelVoice;
        break;
    default:
        return;
    }
    if (*target != level) {
        *target = level;
        emit monitorLevelChanged(mode, level);
    }
}

void RadioState::setNoiseBlankerLevel(int level) {
    if (m_noiseBlankerLevel != level) {
        m_noiseBlankerLevel = qMin(level, 15);
        emit processingChanged();
    }
}

void RadioState::setNoiseBlankerLevelB(int level) {
    if (m_noiseBlankerLevelB != level) {
        m_noiseBlankerLevelB = qMin(level, 15);
        emit processingChangedB();
    }
}

void RadioState::setNoiseBlankerFilter(int filter) {
    filter = qBound(0, filter, 2);
    if (m_noiseBlankerFilterWidth != filter) {
        m_noiseBlankerFilterWidth = filter;
        emit processingChanged();
    }
}

void RadioState::setNoiseBlankerFilterB(int filter) {
    filter = qBound(0, filter, 2);
    if (m_noiseBlankerFilterWidthB != filter) {
        m_noiseBlankerFilterWidthB = filter;
        emit processingChangedB();
    }
}

void RadioState::setNoiseReductionLevel(int level) {
    if (m_noiseReductionLevel != level) {
        m_noiseReductionLevel = qMin(level, 10);
        emit processingChanged();
    }
}

void RadioState::setNoiseReductionLevelB(int level) {
    if (m_noiseReductionLevelB != level) {
        m_noiseReductionLevelB = qMin(level, 10);
        emit processingChangedB();
    }
}

void RadioState::setManualNotchPitch(int pitch) {
    pitch = qBound(150, pitch, 5000);
    if (m_manualNotchPitch != pitch) {
        m_manualNotchPitch = pitch;
        emit notchChanged();
    }
}

void RadioState::setManualNotchPitchB(int pitch) {
    pitch = qBound(150, pitch, 5000);
    if (m_manualNotchPitchB != pitch) {
        m_manualNotchPitchB = pitch;
        emit notchBChanged();
    }
}

void RadioState::setDataSubMode(int subMode) {
    subMode = qBound(0, subMode, 3);
    if (m_dataSubMode != subMode) {
        m_dataSubMode = subMode;
        m_dataSubModeOptimisticTime = QDateTime::currentMSecsSinceEpoch();
        emit dataSubModeChanged(subMode);
    }
}

void RadioState::setDataSubModeB(int subMode) {
    subMode = qBound(0, subMode, 3);
    if (m_dataSubModeB != subMode) {
        m_dataSubModeB = subMode;
        m_dataSubModeBOptimisticTime = QDateTime::currentMSecsSinceEpoch();
        emit dataSubModeBChanged(subMode);
    }
}

void RadioState::setMiniPanAEnabled(bool enabled) {
    if (m_miniPanAEnabled != enabled) {
        m_miniPanAEnabled = enabled;
        emit miniPanAEnabledChanged(enabled);
    }
}

void RadioState::setMiniPanBEnabled(bool enabled) {
    if (m_miniPanBEnabled != enabled) {
        m_miniPanBEnabled = enabled;
        emit miniPanBEnabledChanged(enabled);
    }
}

void RadioState::setWaterfallHeight(int percent) {
    percent = qBound(10, percent, 90);
    if (m_waterfallHeight != percent) {
        m_waterfallHeight = percent;
        emit waterfallHeightChanged(percent);
    }
}

void RadioState::setWaterfallHeightExt(int percent) {
    percent = qBound(10, percent, 90);
    if (m_waterfallHeightExt != percent) {
        m_waterfallHeightExt = percent;
        emit waterfallHeightExtChanged(percent);
    }
}

void RadioState::setAveraging(int value) {
    value = qBound(1, value, 20);
    if (m_averaging != value) {
        m_averaging = value;
        emit averagingChanged(value);
    }
}

void RadioState::setRxEqBand(int index, int dB) {
    if (index < 0 || index >= 8)
        return;
    dB = qBound(-16, dB, 16);
    if (m_rxEqBands[index] != dB) {
        m_rxEqBands[index] = dB;
        emit rxEqBandChanged(index, dB);
        emit rxEqChanged();
    }
}

void RadioState::setRxEqBands(const QVector<int> &bands) {
    bool changed = false;
    for (int i = 0; i < qMin(bands.size(), 8); i++) {
        int dB = qBound(-16, bands[i], 16);
        if (m_rxEqBands[i] != dB) {
            m_rxEqBands[i] = dB;
            changed = true;
        }
    }
    if (changed) {
        emit rxEqChanged();
    }
}

void RadioState::setTxEqBand(int index, int dB) {
    if (index < 0 || index >= 8)
        return;
    dB = qBound(-16, dB, 16);
    if (m_txEqBands[index] != dB) {
        m_txEqBands[index] = dB;
        emit txEqBandChanged(index, dB);
        emit txEqChanged();
    }
}

void RadioState::setTxEqBands(const QVector<int> &bands) {
    bool changed = false;
    for (int i = 0; i < qMin(bands.size(), 8); i++) {
        int dB = qBound(-16, bands[i], 16);
        if (m_txEqBands[i] != dB) {
            m_txEqBands[i] = dB;
            changed = true;
        }
    }
    if (changed) {
        emit txEqChanged();
    }
}

void RadioState::setMainRxAntConfig(bool displayAll, const QVector<bool> &mask) {
    bool changed = false;
    if (displayAll != m_mainRxDisplayAll) {
        m_mainRxDisplayAll = displayAll;
        changed = true;
    }
    for (int i = 0; i < qMin(mask.size(), 7); i++) {
        if (mask[i] != m_mainRxAntMask[i]) {
            m_mainRxAntMask[i] = mask[i];
            changed = true;
        }
    }
    if (changed) {
        emit mainRxAntCfgChanged();
    }
}

void RadioState::setSubRxAntConfig(bool displayAll, const QVector<bool> &mask) {
    bool changed = false;
    if (displayAll != m_subRxDisplayAll) {
        m_subRxDisplayAll = displayAll;
        changed = true;
    }
    for (int i = 0; i < qMin(mask.size(), 7); i++) {
        if (mask[i] != m_subRxAntMask[i]) {
            m_subRxAntMask[i] = mask[i];
            changed = true;
        }
    }
    if (changed) {
        emit subRxAntCfgChanged();
    }
}

void RadioState::setTxAntConfig(bool displayAll, const QVector<bool> &mask) {
    bool changed = false;
    if (displayAll != m_txDisplayAll) {
        m_txDisplayAll = displayAll;
        changed = true;
    }
    for (int i = 0; i < qMin(mask.size(), 3); i++) {
        if (mask[i] != m_txAntMask[i]) {
            m_txAntMask[i] = mask[i];
            changed = true;
        }
    }
    if (changed) {
        emit txAntCfgChanged();
    }
}

void RadioState::setLineOutLeft(int level) {
    level = qBound(0, level, 40);
    if (level != m_lineOutLeft) {
        m_lineOutLeft = level;
        emit lineOutChanged();
    }
}

void RadioState::setLineOutRight(int level) {
    level = qBound(0, level, 40);
    if (level != m_lineOutRight) {
        m_lineOutRight = level;
        emit lineOutChanged();
    }
}

void RadioState::setLineOutRightEqualsLeft(bool enabled) {
    if (enabled != m_lineOutRightEqualsLeft) {
        m_lineOutRightEqualsLeft = enabled;
        emit lineOutChanged();
    }
}

// Line In optimistic setters
void RadioState::setLineInSoundCard(int level) {
    level = qBound(0, level, 250);
    if (level != m_lineInSoundCard) {
        m_lineInSoundCard = level;
        emit lineInChanged();
    }
}

void RadioState::setLineInJack(int level) {
    level = qBound(0, level, 250);
    if (level != m_lineInJack) {
        m_lineInJack = level;
        emit lineInChanged();
    }
}

void RadioState::setLineInSource(int source) {
    if ((source == 0 || source == 1) && source != m_lineInSource) {
        m_lineInSource = source;
        emit lineInChanged();
    }
}

// Mic Input/Setup optimistic setters
void RadioState::setMicInput(int input) {
    if (input >= 0 && input <= 4 && input != m_micInput) {
        m_micInput = input;
        emit micInputChanged(m_micInput);
    }
}

void RadioState::setMicFrontPreamp(int preamp) {
    if (preamp >= 0 && preamp <= 2 && preamp != m_micFrontPreamp) {
        m_micFrontPreamp = preamp;
        emit micSetupChanged();
    }
}

void RadioState::setMicFrontBias(int bias) {
    if ((bias == 0 || bias == 1) && bias != m_micFrontBias) {
        m_micFrontBias = bias;
        emit micSetupChanged();
    }
}

void RadioState::setMicFrontButtons(int buttons) {
    if ((buttons == 0 || buttons == 1) && buttons != m_micFrontButtons) {
        m_micFrontButtons = buttons;
        emit micSetupChanged();
    }
}

void RadioState::setMicRearPreamp(int preamp) {
    if ((preamp == 0 || preamp == 1) && preamp != m_micRearPreamp) {
        m_micRearPreamp = preamp;
        emit micSetupChanged();
    }
}

void RadioState::setMicRearBias(int bias) {
    if ((bias == 0 || bias == 1) && bias != m_micRearBias) {
        m_micRearBias = bias;
        emit micSetupChanged();
    }
}

// Text Decode optimistic setters - Main RX
void RadioState::setTextDecodeMode(int mode) {
    mode = qBound(0, mode, 4);
    if (mode != m_textDecodeMode) {
        m_textDecodeMode = mode;
        emit textDecodeChanged();
    }
}

void RadioState::setTextDecodeThreshold(int threshold) {
    threshold = qBound(0, threshold, 9);
    if (threshold != m_textDecodeThreshold) {
        m_textDecodeThreshold = threshold;
        emit textDecodeChanged();
    }
}

void RadioState::setTextDecodeLines(int lines) {
    lines = qBound(1, lines, 10);
    if (lines != m_textDecodeLines) {
        m_textDecodeLines = lines;
        emit textDecodeChanged();
    }
}

// Text Decode optimistic setters - Sub RX
void RadioState::setTextDecodeModeB(int mode) {
    mode = qBound(0, mode, 4);
    if (mode != m_textDecodeModeB) {
        m_textDecodeModeB = mode;
        emit textDecodeBChanged();
    }
}

void RadioState::setTextDecodeThresholdB(int threshold) {
    threshold = qBound(0, threshold, 9);
    if (threshold != m_textDecodeThresholdB) {
        m_textDecodeThresholdB = threshold;
        emit textDecodeBChanged();
    }
}

void RadioState::setTextDecodeLinesB(int lines) {
    lines = qBound(1, lines, 10);
    if (lines != m_textDecodeLinesB) {
        m_textDecodeLinesB = lines;
        emit textDecodeBChanged();
    }
}

// VOX Gain optimistic setters
void RadioState::setVoxGainVoice(int gain) {
    gain = qBound(0, gain, 60);
    if (gain != m_voxGainVoice) {
        m_voxGainVoice = gain;
        emit voxGainChanged(0, gain);
    }
}

void RadioState::setVoxGainData(int gain) {
    gain = qBound(0, gain, 60);
    if (gain != m_voxGainData) {
        m_voxGainData = gain;
        emit voxGainChanged(1, gain);
    }
}

void RadioState::setAntiVox(int level) {
    level = qBound(0, level, 60);
    if (level != m_antiVox) {
        m_antiVox = level;
        emit antiVoxChanged(level);
    }
}

void RadioState::setEssbEnabled(bool enabled) {
    if (enabled != m_essbEnabled) {
        m_essbEnabled = enabled;
        emit essbChanged(m_essbEnabled, m_ssbTxBw);
    }
}

void RadioState::setSsbTxBw(int bw) {
    bw = qBound(30, bw, 45);
    if (bw != m_ssbTxBw) {
        m_ssbTxBw = bw;
        emit essbChanged(m_essbEnabled, m_ssbTxBw);
    }
}
