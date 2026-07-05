# Quaesar-NG User Guide

Welcome to **Quaesar-NG**, a modern, cross-platform Amiga emulator built around a highly accurate UAE core. Quaesar-NG is designed with developers and power users in mind, offering deep integration with the host OS, an integrated debugger, and powerful command-line configuration capabilities.

---

## 1. Basic Usage

Quaesar-NG can be launched entirely from the command line. The two most important switches you will use are `-k` (for the Kickstart ROM) and `-s` (for passing internal UAE configuration settings).

```bash
./quaesar-dbg -k /path/to/kickstart.rom [options]
```

---

## 2. Command Line Switches

### Core Switches
- **`-k <path>`**  
  Path to the Amiga Kickstart ROM file.
- **`-s <key>=<value>`**  
  Injects a UAE configuration parameter directly into the emulator before boot. You can use this switch multiple times to build your machine configuration.

### Common `-s` Configuration Options
- **`quickstart=Model,0`**  
  Sets the base hardware profile. Examples: `A500,0`, `A1200,0`, `A4000,0`.
- **`cpu_model=<model>`**  
  Forces a specific CPU model. Examples: `68000`, `68020`, `68030`, `68040`, `68060`.
- **`fastmem_size=<MB>`**  
  Allocates Fast RAM. Example: `fastmem_size=8` allocates 8MB of Zorro II Fast RAM.

---

## 3. Storage & Filesystems

Mounting hard disk images and host directories is done via the `-s` switch. **Pay close attention to the syntax**, as UAE's parser is strictly positional.

### Mounting a Hardfile (.vhd / .hdf)
To mount a hardfile, use the `hardfile2` parameter. The syntax is complex but powerful, allowing you to specify the controller type.

**Syntax:**  
`-s hardfile2=<access>,<DeviceName>:<Path>,0,0,0,512,0,,<Controller>`

**Example:**  
`-s hardfile2=rw,DH0:/path/to/OS-3.2.3.vhd,0,0,0,512,0,,ide0`
*(Mounts the VHD as `DH0` on the `ide0` controller with Read/Write access).*

### Mounting a Host Directory
To mount a folder from your host OS as an Amiga hard drive, you must use the **`filesystem2`** parameter.

**Syntax:**  
`-s filesystem2=<access>,<DeviceName>:<VolumeName>:<HostPath>`

**Example:**  
`-s filesystem2=rw,DH1:HostDir:/Users/dev/amiga/host_share`

> **WARNING: The `filesystem` vs `filesystem2` Trap**  
> UAE retains a legacy `filesystem=` parameter. This older parameter *only* accepts two arguments (`VolumeName:HostPath`).   
>   
> **Do not attempt to pass an empty volume name to the legacy parameter (e.g., `filesystem=rw,dh0::/path/to/host`).**   
> Doing so causes the parser to split the string at the first colon. It assigns `dh0` as the Volume Name, and passes `:/path/to/host` (with the leading colon attached!) as the Host Path. Because `:/path...` is an invalid path on your host OS, the mount will fail. As an emergency fallback, UAE will quietly mount your emulator's Current Working Directory instead, automatically naming it `RDH0`.  
>   
> **Always use `filesystem2=`** when you need to explicitly define the Device Name, Volume Name, and Host Path.

---

## 4. UI Interaction & Mouse Capture

Quaesar-NG utilizes an intuitive, WinUAE-style mouse capture system alongside modern ImGui overlays for debugging and configuration.

### Mouse Grabbing
- **Capturing the Mouse:** The emulator does not automatically capture your host mouse cursor. To interact with the Amiga, explicitly **click anywhere inside the Amiga display**. Your host cursor will disappear, and mouse inputs will be routed to the emulated machine.
- **Releasing the Mouse:** Press **`ESC`** at any time to release the mouse grab and return control to your host OS.
- **Focus Loss:** If the emulator window loses focus (e.g., you Alt-Tab / Cmd-Tab to another application, or click the separate debugger window), the mouse is automatically released. It will not auto-grab when you return; you must explicitly click the display again.

### Built-in Overlays
- **Main Menu (`F12`):** Opens the Quaesar-NG configuration UI. The mouse is automatically released so you can interact with the menu.
- **Debugger (`Shift+F12`):** Opens the integrated Amiga Debugger window. The mouse is instantly released, allowing you to seamlessly step through assembly code or inspect memory.

---

## 5. Complete Example

Putting it all together, here is a complete command line that boots an A1200 with an 040 CPU, 8MB of Fast RAM, a primary hardfile (`DH0`), and a shared host directory (`DH1`):

```bash
./quaesar-dbg \
  -k /path/to/kicka1200.rom \
  -s quickstart=A1200,0 \
  -s cpu_model=68040 \
  -s fastmem_size=8 \
  -s hardfile2=rw,DH0:/path/to/OS-3.2.3.vhd,0,0,0,512,0,,ide0 \
  -s filesystem2=rw,DH1:HostDir:/path/to/host_share
```
