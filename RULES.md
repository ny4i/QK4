# Rules

Development principles and review checklist for K4Controller.

## Development Rules

- Preserve existing functionality when adding features
- Use Qt signal/slot for inter-component communication
- Use `QByteArray` for binary protocol data
- Follow naming conventions in CONVENTIONS.md
- Test with actual K4 hardware when possible
- Keep custom widgets self-contained with clear signal interfaces

## Clean Coding Practices

This project prioritizes **maintainability over speed of implementation**.

### Guiding Principles

1. **Do it right the first time** - A proper implementation now prevents technical debt later.

2. **Avoid patchwork** - If a feature requires changes to multiple components, make cohesive changes rather than scattered workarounds.

3. **Consistent patterns** - Follow established patterns:
   - RadioState for all radio state with signals for UI updates
   - Signal/slot connections in MainWindow for orchestration
   - Protocol for CAT parsing, emitting typed signals
   - Widgets self-contained with clear public interfaces

4. **Document ALL changes** - Mandatory:
   - `docs/DEVLOG.md` - Technical log of what changed and why
   - `CHANGELOG.md` - User-facing release notes
   - Update IMMEDIATELY after completing any feature, fix, or change

5. **Parse order matters** - Check specific patterns FIRST (e.g., `RG$` before `RG`)

6. **Initialize state to trigger signals** - Use invalid initial values (`-1`, `-999`) for state that needs to emit signals on first update

## Pre-Commit Checklist

**REQUIRED before every commit:**

```bash
# 1. Run lint check (MUST pass before commit)
find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror

# 2. Auto-fix if lint fails
find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

If lint fails in CI, it means this step was skipped locally.

## Code Review Checklist

Before considering a feature complete:
- [ ] Does it follow existing patterns?
- [ ] Are signals/slots properly connected?
- [ ] Is state initialized correctly?
- [ ] Lint check passes? (REQUIRED - run pre-commit checklist above)
- [ ] Is DEVLOG.md updated? (REQUIRED)
- [ ] Is CHANGELOG.md updated? (REQUIRED for user-facing changes)
- [ ] Would another developer understand this code?
