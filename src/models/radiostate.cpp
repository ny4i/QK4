#include "radiostate.h"
#include <QDebug>

RadioState::RadioState(QObject *parent)
    : QObject(parent)
{
}

void RadioState::parseCATCommand(const QString &command)
{
    QString cmd = command.trimmed();
    if (cmd.isEmpty()) return;

    // Remove trailing semicolon for parsing
    if (cmd.endsWith(';')) {
        cmd.chop(1);
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
    // Mic Gain (MG 0-60)
    else if (cmd.startsWith("MG") && cmd.length() > 2) {
        bool ok;
        int gain = cmd.mid(2).toInt(&ok);
        if (ok) {
            m_micGain = (gain * 100) / 60;
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
    // QSK/VOX Delay (SD) - SD0Mxxx where M=mode (C/V/D), xxx=delay in 10ms
    // RDY dump provides: SD0C005;SD0V025;SD0D005; (all three modes)
    // Query response: SDxyzzz where x=QSK flag, y=mode code, zzz=delay
    else if (cmd.startsWith("SD") && cmd.length() >= 7) {
        QChar modeChar = cmd.at(3);
        bool ok;
        int delay = cmd.mid(4, 3).toInt(&ok);
        if (ok) {
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
    }
    else if (cmd == "RX") {
        if (m_isTransmitting) {
            m_isTransmitting = false;
            emit transmitStateChanged(false);
        }
    }
    // Noise Blanker Sub (NB$)
    else if (cmd.startsWith("NB$") && cmd.length() >= 5) {
        QString nbStr = cmd.mid(3);
        if (nbStr.length() >= 4) {
            bool ok1, ok2;
            int level = nbStr.left(2).toInt(&ok1);
            int enabled = nbStr.mid(2, 1).toInt(&ok2);
            if (ok1 && ok2) {
                m_noiseBlankerLevelB = qMin(level, 25);
                m_noiseBlankerEnabledB = (enabled == 1);
                emit processingChangedB();
            }
        }
    }
    // Noise Blanker Main (NB)
    else if (cmd.startsWith("NB") && cmd.length() >= 4) {
        QString nbStr = cmd.mid(2);
        if (nbStr.length() >= 4) {
            bool ok1, ok2, ok3;
            int level = nbStr.left(2).toInt(&ok1);
            int enabled = nbStr.mid(2, 1).toInt(&ok2);
            int filter = nbStr.mid(3, 1).toInt(&ok3);
            if (ok1 && ok2 && ok3) {
                m_noiseBlankerLevel = qMin(level, 25);
                m_noiseBlankerEnabled = (enabled == 1);
                m_noiseBlankerFilterWidth = qMin(filter, 2);
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
    // Auto Notch (NA) - NAn; where n=0/1
    else if (cmd.startsWith("NA") && !cmd.startsWith("NA$") && cmd.length() >= 3) {
        bool enabled = (cmd.at(2) == '1');
        if (m_autoNotchEnabled != enabled) {
            m_autoNotchEnabled = enabled;
            emit notchChanged();
        }
    }
    // Manual Notch (NM) - NMnnnnm; or NMm;
    else if (cmd.startsWith("NM") && !cmd.startsWith("NM$") && cmd.length() >= 3) {
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
            if (gt == 0) m_agcSpeedB = AGC_Off;
            else if (gt == 1) m_agcSpeedB = AGC_Slow;
            else if (gt == 2) m_agcSpeedB = AGC_Fast;
            emit processingChangedB();
        }
    }
    // AGC Speed Main (GT) - GTn where n=0(off)/1(slow)/2(fast)
    else if (cmd.startsWith("GT") && cmd.length() > 2) {
        bool ok;
        int gt = cmd.mid(2).toInt(&ok);
        if (ok) {
            if (gt == 0) m_agcSpeed = AGC_Off;
            else if (gt == 1) m_agcSpeed = AGC_Slow;
            else if (gt == 2) m_agcSpeed = AGC_Fast;
            emit processingChanged();
        }
    }
    // Filter Position (FP)
    else if (cmd.startsWith("FP") && cmd.length() > 2) {
        bool ok;
        int fp = cmd.mid(2).toInt(&ok);
        if (ok && fp >= 1 && fp <= 3) {
            m_filterPosition = fp;
        }
    }
    // Tuning Step (VT)
    else if (cmd.startsWith("VT") && cmd.length() > 2) {
        QString vtStr = cmd.mid(2);
        if (!vtStr.isEmpty()) {
            bool ok;
            int step = vtStr.left(1).toInt(&ok);
            if (ok) {
                m_tuningStep = qBound(0, step, 5);
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
            int pitchHz = pitchCode * 10;  // Convert code to Hz
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
    else if (cmd.startsWith("SB") && cmd.length() > 2) {
        m_subReceiverEnabled = (cmd.mid(2) == "1");
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
    else if (cmd.startsWith("AR") && !cmd.startsWith("AR$") && cmd.length() > 2) {
        bool ok;
        int ar = cmd.mid(2).toInt(&ok);
        if (ok && ar >= 1 && ar <= 6 && ar != m_receiveAntenna) {
            m_receiveAntenna = ar;
            emit antennaChanged(m_selectedAntenna, m_receiveAntenna, m_receiveAntennaSub);
        }
    }
    // RX Antenna Sub (AR$)
    else if (cmd.startsWith("AR$") && cmd.length() > 3) {
        bool ok;
        int ar = cmd.mid(3).toInt(&ok);
        if (ok && ar >= 1 && ar <= 6 && ar != m_receiveAntennaSub) {
            m_receiveAntennaSub = ar;
            emit antennaChanged(m_selectedAntenna, m_receiveAntenna, m_receiveAntennaSub);
        }
    }
    // ATU Mode (AT)
    else if (cmd.startsWith("AT") && cmd.length() >= 3) {
        bool ok;
        int at = cmd.mid(2).toInt(&ok);
        if (ok && at >= 0 && at <= 2) {
            m_atuMode = at;
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
            QString vsStr = (commaIndex > vsIndex) ? data.mid(vsIndex + 3, commaIndex - vsIndex - 3) : data.mid(vsIndex + 3);
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
            QString isStr = (commaIndex > isIndex) ? data.mid(isIndex + 3, commaIndex - isIndex - 3) : data.mid(isIndex + 3);
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
    // RIT (RT) - RT0/RT1
    else if (cmd.startsWith("RT") && cmd.length() >= 3) {
        bool newRit = (cmd.mid(2, 1) == "1");
        if (newRit != m_ritEnabled) {
            m_ritEnabled = newRit;
            emit ritXitChanged(m_ritEnabled, m_xitEnabled, m_ritXitOffset);
        }
    }
    // XIT (XT) - XT0/XT1
    else if (cmd.startsWith("XT") && cmd.length() >= 3) {
        bool newXit = (cmd.mid(2, 1) == "1");
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
        QString refStr = cmd.mid(4);  // Get "-110" from "#REF-110"
        bool ok;
        int level = refStr.toInt(&ok);
        if (ok && level != m_refLevel) {
            m_refLevel = level;
            emit refLevelChanged(m_refLevel);
        }
    }
    // Panadapter Span (#SPN) - #SPN10000 (Hz)
    else if (cmd.startsWith("#SPN") && cmd.length() > 4) {
        QString spanStr = cmd.mid(4);  // Get "10000" from "#SPN10000"
        qDebug() << "Parsing #SPN response:" << cmd << "spanStr:" << spanStr;
        bool ok;
        int span = spanStr.toInt(&ok);
        if (ok && span > 0 && span != m_spanHz) {
            qDebug() << "Span changed from" << m_spanHz << "to" << span;
            m_spanHz = span;
            emit spanChanged(m_spanHz);
        }
    }

    emit stateUpdated();
}

RadioState::Mode RadioState::modeFromCode(int code)
{
    switch (code) {
    case 1: return LSB;
    case 2: return USB;
    case 3: return CW;
    case 4: return FM;
    case 5: return AM;
    case 6: return DATA;
    case 7: return CW_R;
    case 9: return DATA_R;
    default: return USB;
    }
}

QString RadioState::modeToString(Mode mode)
{
    switch (mode) {
    case LSB: return "LSB";
    case USB: return "USB";
    case CW: return "CW";
    case FM: return "FM";
    case AM: return "AM";
    case DATA: return "DATA";
    case CW_R: return "CW-R";
    case DATA_R: return "DATA-R";
    default: return "USB";
    }
}

QString RadioState::modeString() const
{
    return modeToString(m_mode);
}

QString RadioState::sMeterString() const
{
    if (m_sMeter <= 9.0) {
        return QString("S%1").arg(static_cast<int>(m_sMeter));
    } else {
        int dbOver = static_cast<int>((m_sMeter - 9.0) * 10);
        return QString("S9+%1").arg(dbOver);
    }
}

QString RadioState::sMeterStringB() const
{
    if (m_sMeterB <= 9.0) {
        return QString("S%1").arg(static_cast<int>(m_sMeterB));
    } else {
        int dbOver = static_cast<int>((m_sMeterB - 9.0) * 10);
        return QString("S9+%1").arg(dbOver);
    }
}
