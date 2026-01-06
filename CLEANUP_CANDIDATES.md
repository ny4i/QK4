# Cleanup Candidates

Items identified during documentation audit that warrant review.

---

## ~~Severity: HIGH (Orphan Files)~~ RESOLVED

### ~~Legacy OpenGL Files~~ DELETED 2026-01-06

The following orphan files were deleted:
- `src/dsp/panadapter.cpp/.h`
- `src/dsp/minipanwidget.cpp/.h`

These were the original QOpenGLWidget implementations, replaced by QRhiWidget versions.

---

## Severity: MEDIUM (Debug Artifacts)

### qDebug() Statement Count

| File | Count | Notes |
|------|-------|-------|
| `src/network/tcpclient.cpp` | 24 | Connection/auth debugging |
| `src/mainwindow.cpp` | 22 | Signal routing debugging |
| `src/dsp/panadapter_rhi.cpp` | 15 | Shader/rendering debugging |
| `src/main.cpp` | 12 | Startup debugging |
| `src/hardware/kpoddevice.cpp` | 10 | HID communication |
| `src/models/radiostate.cpp` | 9 | CAT parsing |
| `src/network/kpa1500client.cpp` | 2 | Amplifier debugging |
| `src/models/menumodel.cpp` | 2 | Menu parsing |
| `src/network/protocol.cpp` | 1 | Packet parsing |
| `src/ui/displaypopupwidget.cpp` | 1 | UI debugging |

**Total:** 98 qDebug() statements

**Recommendation:** Review before release builds. Consider:
- Wrap in `#ifdef QT_DEBUG` for development-only output
- Use `qCDebug()` with categories for filterable output
- Remove or comment out noisy high-frequency statements

---

## Severity: LOW (TODO Comments)

### Active TODOs in Code

| Location | Comment |
|----------|---------|
| `src/mainwindow.cpp:257` | `// TODO: Enable when TX meter placement is finalized` |
| `src/mainwindow.cpp:1007` | `// TODO: Show help dialog` |

**Note:** Line 2023 comment `// Insert dots: XX.XXX.XXX` is documentation, not a TODO.

**Recommendation:** Address or create issues for tracking.

---

## Severity: INFO (Style/Naming)

### Temporary UI Elements

| Location | Item | Notes |
|----------|------|-------|
| `src/ui/bottommenubar.h:71` | `m_styleBtn` | Marked "TEMP: Spectrum style cycling for testing" |
| `src/ui/bottommenubar.h:54` | `styleClicked()` signal | For temporary testing feature |

**Recommendation:** Remove when spectrum style is finalized or move to debug menu.

---

## No Issues Found

The following categories showed no problems:

- **#if 0 blocks**: None found (clean!)
- **Naming convention violations**: All member variables use `m_` prefix
- **Missing m_ prefixes**: None found
- **Hardcoded test values**: None found in production code
- **FIXME/XXX/HACK comments**: None found

---

## Summary Statistics

| Category | Count | Action Required |
|----------|-------|-----------------|
| ~~Orphan files~~ | ~~4~~ | ✓ DELETED |
| qDebug statements | 98 | Review for release |
| TODO comments | 2 | Resolve or track |
| Temporary UI | 2 | Remove when stable |

---

## Suggested Next Steps

1. ~~**Immediate**: Delete orphan OpenGL files to clean repository~~ ✓ DONE
2. **Before release**: Audit qDebug statements and wrap/remove
3. **Backlog**: Create issues for TODO items
4. **Minor**: Remove temporary style button when spectrum UI finalized
