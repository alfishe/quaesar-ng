# 18 — Supported, Disabled & Removed Features

← [Testing & CI](17-testing-and-ci.md) · [Index](index.md) · → [Release & Packaging](19-release-packaging.md)

Quaesar-NG is built on WinUAE/FS-UAE cores, but it is deliberately scoped to the
**demoscene A500/A1200** experience. This is the definitive table of what works,
what's compiled-out, and what was removed — so users don't waste time enabling
features that don't exist.

> Authoritative sources: `src/uae_lib_imp/sysconfig.h` (compile switches),
> [`doc/user_guide.md`](../doc/user_guide.md) (narrative & FAQ).

## At a glance

```mermaid
graph TB
    ALL["WinUAE / FS-UAE featurespace"]
    All --> ON["Supported / compiled-in"]
    All --> OFF["Disabled (commented out in sysconfig.h)"]
    All --> REMOVED["Removed by Quaesar-NG"]

    ON --> DEMO["demoscene core:<br/>OCS/ECS/AGA · M68k · AHI · Enforcer<br/>Debugger · DriveSound · AVIOutput"]
    OFF --> NET["BSDSocket networking"]
    OFF --> SCSI["SCSI (NCR / NCR9X / SCSIEMU)"]
    OFF --> LCD["LogitechLCD"]
    OFF --> RP["RetroPlatform (Cloanto)"]
    OFF --> FLT["GFXFILTER (old filter)"]
    REMOVED --> RTG["RTG / Picasso96 / P96 display"]
    REMOVED --> GUI["Host config GUI"]

    style REMOVED fill:#5a2323,stroke:#a33,color:#fff
    style OFF fill:#5a4a23,stroke:#a83,color:#fff
    style ON fill:#1a3a2a,stroke:#4a8,color:#fff
```

## Feature matrix

### Compiled-in (`#define` in `sysconfig.h`)

| Feature | `sysconfig.h` switch | Status | Notes |
|---------|----------------------|--------|-------|
| Integrated debugger | `DEBUGGER` | ✅ on | WinUAE's built-in `debug()`/console is the escape hatch (see [UAE Backend](05-backend-uae.md)). |
| AHI audio emulation | `AHI` | ✅ on | Higher-quality sound. |
| UAE Enforcer | `ENFORCER` | ✅ on | Halts on illegal memory access with a stack dump — essential for catching bad pointers in demos. |
| AVIOutput | `AVIOUTPUT` | ✅ on | Capture video/audio to AVI. |
| Drive sound | `DRIVESOUND` | ✅ on | Mechanical floppy seek sounds. |

### Disabled (commented `// #define` in `sysconfig.h`)

These exist in the source but are compiled out. Re-enabling requires editing
`sysconfig.h` and rebuilding.

| Feature | Switch | Default | Notes |
|---------|--------|---------|-------|
| BSDSocket networking | `BSDSOCKET` | ❌ off | `bsdsocket.library` host-network bridge. Off by default; see [`doc/bsdsocket_integration_plan.md`](../doc/bsdsocket_integration_plan.md). Use `-s bsdsocket_emu=true` only after enabling it at build time. |
| SCSI device emulation | `SCSIEMU` | ❌ off | uaescsi.device. |
| NCR 53C710/53C770 SCSI | `NCR` | ❌ off | A4000T / A4091. |
| NCR 53C9X SCSI | `NCR9X` | ❌ off | 53C9X-family SCSI. |
| Logitech G15 LCD | `LOGITECHLCD` | ❌ off | Status display for the old G15 keyboard. |
| Cloanto RetroPlayer | `RETROPLATFORM` | ❌ off | Cloanto-specific hooks. |
| GFX filter | `GFXFILTER` | ❌ off | Legacy scaling-filter path. |

### Removed / out of scope (Quaesar-NG design choices)

| Area | Removed? | Why |
|------|----------|-----|
| RTG / Picasso96 / P96 graphics | Removed | Out of scope — demoscene is native chipset. (Documented in [`doc/user_guide.md`](../doc/user_guide.md) FAQ.) |
| Host configuration GUI | Removed | Quaesar-NG is CLI-first; configuration is via `-s` (see [CLI Reference](13-cli-reference.md)). |
| JIT (dynarec) on non-x86 | Not enabled on ARM64 macOS | Cycle-exact M68k is the priority for demoscene accuracy. |

## Machine / chipset coverage

| Target | Default config? | Quality |
|--------|-----------------|---------|
| **A500** (OCS, 68000, 512K chip + 512K slow) | ✅ the zero-config default | First-class |
| A500+ / A600 (ECS) | via `-s chipset=ecs` or `quickstart=A600,0` | Supported |
| **A1200** (AGA, 68020) | via `-s quickstart=A1200,0` | First-class |
| A4000 (68040, Zorro III) | via `-s quickstart=A4000,0` | Best-effort |
| CD32 | via `-s quickstart=CD32,0` | Best-effort |

See [CLI Reference § Recipes](13-cli-reference.md#recipes) for exact invocations.

## Why the scope exists

```mermaid
graph LR
    GOAL["Quaesar-NG goal:<br/>accurate, CLI-first demoscene rig"]
    GOAL --> KEEP["keep: cycle-exact OCS/ECS/AGA,<br/>AHI, Enforcer, debugger"]
    GOAL --> CUT["cut: RTG, host GUI, networking,<br/>enterprise SCSI/CD"]
    KEEP --> FOCUS["focus & correctness"]
    CUT --> FOCUS
    style CUT fill:#5a4a23,stroke:#a83,color:#fff
```

The cuts are deliberate: every removed feature is one less source of timing
jitter and one less surface to maintain. If you need Picasso96 RTG or full SCSI,
upstream WinUAE/FS-UAE is the right tool.

---

← [Testing & CI](17-testing-and-ci.md) · [Index](index.md) · → [Release & Packaging](19-release-packaging.md)
