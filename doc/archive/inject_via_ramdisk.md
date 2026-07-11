# Direct RAM Disk Injection (Strategy 2)

We will implement Strategy 2 by leveraging `amidisk_cpp` (from the AmigaFSTool project) to construct an FFS disk image entirely on the host side, and then hot-plug it directly into AmigaOS's native `ramdrive.device` (RAD:). This bypasses the need for the guest OS to perform any file-writing operations via traps.

## Background & Discovery
Reviewing `ramdrive.asm` from AmigaOS 3.2 (`os-source/v40_src/kickstart/ramdrive/ramdrive.asm`) revealed a critical behavior:
When `ramdrive.device/OpenDevice` is called, it iterates its internal `rd_UnitList`. If the requested unit number (e.g. Unit 99) is found, it **immediately returns success** without allocating memory or formatting the disk. It simply assumes the unit is already valid. 

Furthermore, `ramdrive.device`'s `CMD_READ` and `CMD_WRITE` operations simply add the sector offset to the `ru_Data` pointer, treating it as a flat byte array of FFS sectors.

## User Review Required
> [!IMPORTANT]
> **Dependency on `amidisk_cpp`:** This plan requires integrating `amidisk_cpp` (or at least its `FFSVolume` and `BlockDevice` headers/sources) into `quaesar-ng`'s build system so the File Stager can use it. Are you comfortable with adding `amidisk_cpp` as a submodule or static dependency to `quaesar-ng`?

## Proposed Changes

### 1. Dynamic Size Analysis & Host-Side FFS Construction (File Stager)
Instead of relying on fixed-size disks, the File Stager will dynamically analyze and construct the exact payload size:
1. **Analyze Requirements:** The host calculates the base byte size required for the dropped payload (e.g., a `.tar` archive, its uncompressed contents, or a complex folder structure) plus FFS metadata overhead. It then adds a +10% padding (or a minimum overhead margin) to allow the guest OS to generate `.info` icons or unpack archives.
2. **Verify Guest Memory:** The host executes an `AvailMem(MEMF_PUBLIC)` trap check to ensure the guest has sufficient Fast RAM. If not, the injection gracefully aborts with a warning.
3. **Format Exact Disk:** Allocate an in-memory block device (`std::vector<uint8_t>`) of this exact size.
4. **Populate Files:** Format it as an Amiga FastFileSystem (FFS) volume using `amidisk`'s `FFSVolume::format()` logic, and copy the target files into it.

### 2. Guest Memory Allocation & Copy (Cross-Core Compatible)
To ensure this mechanism works universally across UAE, vAmiga, and other cores, we will rely strictly on standard AmigaOS Exec traps rather than emulator-specific internals. 
1. The host dispatches a request (e.g. via a `native2amiga` style queue) to allocate guest memory.
2. The guest executes `AllocMem` via a trap (`CallLib(SysBase, -198)`) to allocate a block sized to match the FFS volume.
3. Once the address is returned to the host, the host pauses/halts the CPU and copies the entire FFS byte array directly into this guest memory block.

### 3. Hot-Plugging into `ramdrive.device`
1. Locate `ramdrive.device` in `ExecBase->DeviceList`.
2. Allocate a `RamdriveUnit` struct in guest memory (`uae_AllocMem`).
3. Populate the struct:
   - `ru_UnitNum = 99` (to avoid colliding with standard RAD: unit 0).
   - `ru_Data = <guest_address_of_FFS_block>`
   - `ru_MaxBytes = <size_of_FFS_block>`
4. Insert the `RamdriveUnit` at the head of `ramdrive.device`'s `rd_UnitList` (offset 84).

### 4. DOS Mounting
To make the OS aware of `QHUNK:`, we must inject a DOS `DeviceNode`:
1. Allocate memory in the guest for a `DeviceNode`, `FileSysStartupMsg`, and `DosEnvec`.
2. Populate them to describe a device named `QHUNK` that uses `ramdrive.device` unit `99`.
3. Link the `DeviceNode` into `DosBase->dl_Root->rn_Info (DosInfo) -> di_DevInfo`.

### 5. Execution & Unpause
1. Unpause the emulator.
2. Dispatch `uae_ShellExecute("Execute QHUNK:launch.bat")`.
3. When the AmigaOS Shell accesses `QHUNK:`, `dos.library` will open `ramdrive.device` unit 99. The device will find our manually inserted unit and instantly serve the pre-populated FFS volume.

## Idempotence & Clean Eject Mechanisms (Multiple Injections)
To support injecting multiple executables sequentially and to ensure 100% clean resource recycling both inside and outside the Amiga, we will implement a proper "Eject" sequence that formally kills the DOS handler. We abandon the "fixed-size cache flush" in favor of exact-sized disks that are completely destroyed and rebuilt per injection.

**The "Eject & Recycle" Sequence:**
When a new payload is dropped, or the user clicks "Eject" in the UI, the host commands the guest to properly dismount the existing `QHUNK:` disk:

1. **Inside Amiga (Clean Dismount via ACTION_DIE):**
   - The host pushes a `CMD_EJECT_RAMDISK` command into the `native2amiga` queue.
   - The `native2amiga` listener (running as a guest task) executes `FindPort("QHUNK")` to locate the active DOS handler.
   - It allocates and sends a standard DOS `ACTION_DIE` packet (packet type 8) via `PutMsg()` and waits for the reply.
   - **The Handler Exits:** The FastFileSystem handler gracefully flushes any pending writes, unmounts the volume, replies to the packet, and formally exits (`RemTask`). 
   - **Reclaim Structures:** With the handler completely dead, `native2amiga` safely unlinks the `DeviceNode` from `DosInfo->DevInfo` and unlinks the `RamdriveUnit` from `rd_UnitList`.
   - **Reclaim RAM:** `native2amiga` executes `FreeMem()` to release the exact-sized `ru_Data` block, the `DeviceNode`, and the `RamdriveUnit`. 

2. **Outside Amiga:**
   - The host drops any host-side tracking of the previous unit.
   - It proceeds to **Step 1 (Dynamic Size Analysis)** to build the new, perfectly sized disk for the next payload.

This guarantees perfect idempotence, zero guest memory leaks, and respects the AmigaDOS lifecycle architecture.

## Verification Plan

### Manual Verification
- Drop a multi-file folder (e.g., a demo with data files) into the emulator.
- Verify that `QHUNK:` mounts instantly and the program runs without any host-directory mapping.
- Open a CLI in the guest and type `info`. Verify that `QHUNK:` appears as a valid DOS disk with the expected size.
