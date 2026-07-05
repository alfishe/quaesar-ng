# Quaesar-NG: The Demoscener's Journey

## Chapter 1: The Call of the Demo

The glow of the CRT monitor flickered in the dimly lit room. It was 2 AM, and Alex, a seasoned demoscener, was staring at a cascade of hex codes and assembly instructions. For weeks, they had been crafting a 64kb intro designed to push the venerable Amiga 500 to its absolute breaking point. The blitter was weeping, the copper was sweating, and the 68000 CPU was gasping for cycles.

But there was a problem. The iterative loop of writing code on a modern machine, building it, launching an emulator, clicking through endless GUI menus to load the floppy image, selecting the Kickstart ROM, and adjusting the chipset options was killing Alex's creative momentum. WinUAE was powerful—undeniably the king of Amiga emulation—but it was designed for everyone: gamers, sysops, and nostalgia seekers. It had windows, dialogs, sliders, and drop-downs. 

Alex didn't want dialogs. Alex wanted execution.

Enter **Quaesar-NG**.

Quaesar-NG is not here to replace WinUAE; it is here to distill it. It is an emulator forged in the fires of the demoscene, specifically designed for developers who live in the terminal. No GUI. No friction. Just your code, your command line, and an instant execution environment that gets out of your way.

This guide chronicles Alex's journey with Quaesar-NG, serving as a comprehensive manual for any developer looking to master this streamlined, terminal-driven Amiga emulator.

---

## Chapter 2: The Philosophy of Quaesar-NG

Before diving into the commands, Alex needed to understand what Quaesar-NG was, and more importantly, what it *wasn't*.

"It's about the workflow," Alex muttered, reading the repository's `readme.md`. 

Quaesar-NG strips away the graphical user interface. If you want to change the amount of fast RAM, you don't click a checkbox; you pass an argument. If you want to mount a hard drive, you don't browse for a folder; you define a string. By embracing the command line, Quaesar-NG allows demosceners to integrate emulator launches directly into their build scripts, Makefiles, or Ninja configurations.

You type `make run`, and your demo is instantly running in a cycle-accurate Amiga 500 environment. When you're done, a single press of `ESC` instantly terminates the emulator, returning you to your shell, ready to tweak the assembly and build again.

### Core Tenets of Quaesar-NG:
1. **Cross-Platform Purity:** Runs identically on macOS, Linux, and Windows.
2. **Focus on the Scene:** Targeted specifically at the A500, A600, A1200, and A4000. Features like Picasso96 or RTG boards are stripped out to keep the codebase lean and focused on classic hardware pushing.
3. **Accuracy by Default:** The default configuration is a pristine, cycle-exact Amiga 500 with 512kb Chip RAM and 512kb Slow RAM.
4. **WinUAE Core:** Underneath its sleek exterior, it leverages the rock-solid, cycle-accurate core of WinUAE, meaning compatibility is uncompromised.

---

## Chapter 3: The Command-Line Arsenal

As Alex opened their terminal, they knew they needed to master the tool's vocabulary. Quaesar-NG uses a hybrid argument system. It has native commands for the most common operations, but it also acts as a transparent proxy to WinUAE's immensely powerful `-s` configuration system.

### Native Quaesar-NG Arguments

| Argument | Shorthand | Description | Example |
| :--- | :--- | :--- | :--- |
| `input` | (Positional) | The primary executable, ADF (floppy), or DMS image to load. Automatically mounted into `DF0:`. | `quaesar intro.adf` |
| `--kickstart` | `-k` | The absolute or relative path to the Kickstart ROM file required to boot the Amiga. | `-k kick13.rom` |
| `--serial_port` | N/A | Maps the Amiga's serial port to a host device or pipe for capturing debug output like `kprintf`. | `--serial_port /dev/ttys0` |
| `--uaeExtArgs` | `-s` | A pass-through for WinUAE configuration keys. This is the ultimate power-user tool. | `-s quickstart=A1200,0` |

### WinUAE Configuration Extensions (`-s` flag)

The true power of Quaesar-NG lies in the `-s` flag. Every time you use `-s`, you are directly manipulating the internal WinUAE configuration parser. Here is the grimoire of options Alex discovered to shape the virtual hardware:

| WinUAE Key | Value Example | Description |
| :--- | :--- | :--- |
| `quickstart` | `A500,0` | Instantly configures the machine to a specific template. Supported models include `A500`, `A600`, `A1200`, `A4000`, `CD32`, etc. The `,0` specifies the default hardware variant for that model. |
| `cpu_model` | `68000`, `68020`, `68040` | Overrides the CPU model. Useful for testing how your code runs on accelerated machines. |
| `fpu_model` | `68881`, `68882`, `0` | Adds or removes a Floating Point Unit. |
| `chipset` | `ocs`, `ecs`, `aga` | Defines the custom chipset generation. Essential for moving between A500 and A1200 development. |
| `chipmem_size` | `1`, `2`, `4`, `8` | Sets Chip RAM size. Note: WinUAE often parses these as multipliers. `1`=512KB, `2`=1MB, `4`=2MB. |
| `fastmem_size` | `0`, `2`, `4`, `8` | Sets Zorro II Fast RAM size in Megabytes. |
| `bogomem_size` | `0`, `1` | Sets "Slow RAM" (trapdoor memory at $C00000). |
| `cpu_speed` | `real`, `max` | `real` enforces cycle-exact emulation. `max` runs the CPU as fast as the host machine allows (great for compiling inside the emulator). |
| `filesystem` | `rw,dh0:/path/to/dir` | Mounts a local directory as an Amiga Hard Drive. The syntax is `[rw|ro],[DeviceName]:[HostPath]`. |
| `floppy0` | `/path/to/disk.adf` | Manually specifies the image for DF0:. You can use `floppy1`, `floppy2`, etc., for additional drives. |
| `floppy_speed` | `100`, `800` | Adjusts the floppy drive read speed. `100` is accurate (slow). `800` is 8x turbo speed. |
| `cpu_cycle_exact`| `true`, `false` | Toggles cycle-exact CPU emulation. |
| `blitter_cycle_exact`| `true`, `false` | Toggles cycle-exact Blitter emulation. Required for highly synchronized visual effects. |

---

## Chapter 4: The A500 512/512 Pure Experience

Alex's first test was simple. They had a standard `.adf` floppy image containing their demo, heavily reliant on cycle-exact copper chasers and delicate blitter interrupts. 

They needed the purest A500 environment. Kickstart 1.3, OCS chipset, 512kb Chip RAM, and 512kb Slow RAM.

Because Quaesar-NG defaults to exactly this setup, the command was elegantly brief:

```bash
quaesar build/my_intro.adf -k roms/kick13.rom
```

The emulator launched. The familiar Kickstart 1.3 hand holding the floppy disk briefly flashed before the screen went black, and the demo began. The timing was flawless. The copper bars lined up perfectly with the raster beam.

But Alex was impatient. Watching the AmigaDOS boot sequence every time they recompiled took 4 seconds. In the demoscene, 4 seconds is an eternity. They wanted to speed up the floppy drive to turbo speeds to bypass the loading screen, then return to normal speed for the demo execution.

Alex cracked open their `Makefile` and updated the run command:

```bash
quaesar build/my_intro.adf -k roms/kick13.rom -s floppy_speed=800
```

Now, the demo loaded in under a second. The iteration loop tightened. Compile, launch, observe, press `ESC` to quit, repeat. Alex was in the zone.

---

## Chapter 5: Upgrading to AGA

The A500 intro was finished and released at the Revision demoparty. It placed 2nd. Alex was happy, but now they had their eyes on a bigger prize: the 64kb PC/Amiga cross-platform competition. This required 256 colors. This required AGA. This required the Amiga 1200.

Alex created a new project directory. They compiled their first chunky-to-planar rendering routine. Now they needed to test it.

If they just ran the default command, Quaesar-NG would boot as an A500, which has no idea what AGA or a 68020 CPU is. The code would instantly Guru Meditation.

Alex needed to leverage the `quickstart` configuration.

```bash
quaesar build/aga_demo.adf -k roms/kick31.rom -s quickstart=A1200,0
```

By passing `-s quickstart=A1200,0`, Quaesar-NG intercepts the command and instructs the underlying WinUAE core to tear down the A500 architecture and instantly reconstruct an A1200. The 68000 is swapped for a 68EC020. The OCS chipset is upgraded to AGA. Chip RAM is bumped to 2MB.

Alex's demo booted. The screen flooded with 256 glorious, chunky-converted colors.

But the 68EC020 at 14MHz wasn't enough to render the 3D voxel terrain smoothly. Alex wanted to test how the demo would perform on an accelerated A1200, perhaps one with a 68030 or 68040 and some Fast RAM.

They stacked the `-s` arguments like building blocks:

```bash
quaesar build/aga_demo.adf \
  -k roms/kick31.rom \
  -s quickstart=A1200,0 \
  -s cpu_model=68040 \
  -s fastmem_size=8
```

The voxel terrain was now rendering at a silky smooth 50 frames per second. The Fast RAM allowed the CPU to fetch instructions without waiting for the custom chipset's memory contention. Alex smiled. The power of the command line was intoxicating.

---

## Chapter 6: The File System Alchemist

Working with `.adf` floppy images is authentic, but it is also tedious during active development. Every time Alex built their executable, they had to run an external tool to inject the `demo.exe` into an `.adf` file just so Quaesar-NG could boot it.

"There must be a better way," Alex thought. 

And there was. Quaesar-NG, through WinUAE's `filesystem` parameter, can mount a folder on the host OS (Windows, macOS, or Linux) directly into the Amiga's filesystem as a hard drive.

Alex stopped generating `.adf` files entirely. Their build script now simply output an AmigaDOS executable named `demo.exe` into a folder called `dist/`.

They ran the following command:

```bash
quaesar -k roms/kick31.rom \
  -s quickstart=A1200,0 \
  -s filesystem=rw,dh0:./dist
```

The emulator booted. Alex was greeted by the AmigaDOS shell (since there was no floppy to boot from, and Kickstart 3.1 falls back to the hard drive). 

They typed:
```amigados
1> dh0:
1> demo.exe
```

The demo ran instantly. But Alex wanted to automate this too. They didn't want to type `demo.exe` every time. They created an AmigaDOS script named `Startup-Sequence` and placed it inside a new folder called `S/` within their `dist/` directory.

The `dist/S/Startup-Sequence` file contained one line:
```amigados
demo.exe
```

Now, when they ran the command, Kickstart mounted `dh0:`, found the `S/Startup-Sequence` script, and executed it automatically. The `.adf` middleman was completely eliminated. The build-to-test time dropped to milliseconds.

---

## Chapter 7: The Serial Port Whisperer

Disaster struck. Alex's memory allocator was leaking. After 30 seconds of running the demo, the Amiga would freeze, and the screen would corrupt into a mess of neon garbage.

Because Quaesar-NG has no GUI, there was no memory viewer readily available. Alex needed to use `kprintf`—the classic Amiga debug function that sends text strings out through the Amiga's hardware serial port.

But where does the serial port output go on a modern MacBook or Linux box? 

Quaesar-NG solves this with the `--serial_port` flag. It bridges the virtual Amiga serial port to a file, a pipe, or a physical serial port on the host machine.

Alex updated their Makefile:

```bash
quaesar -k roms/kick31.rom \
  -s quickstart=A1200,0 \
  -s filesystem=rw,dh0:./dist \
  --serial_port debug_output.txt
```

They ran the demo. It crashed exactly as before. Alex pressed `ESC` to quit Quaesar-NG.

They immediately ran:
```bash
cat debug_output.txt
```

The terminal was flooded with their custom debug messages:
```
[INFO] Initializing Voxel Engine... OK
[INFO] Allocating 1MB for textures... OK
[WARN] Free list corrupted at 0x0024A000!
[FATAL] Out of Chip RAM!
```

Alex found the bug. A missing pointer increment in their memory pool. They fixed the code, typed `make run`, and the demo sailed past the 30-second mark flawlessly.

---

## Chapter 8: The Trackmo Nightmare

Alex's ambition grew. The 64kb intro wasn't enough; they wanted to release a multi-part "Trackmo" spanning three floppy disks, utilizing a custom track loader that completely bypassed AmigaDOS. 

This required precision. Not just cycle-exact CPU emulation, but cycle-exact floppy emulation.

When building a custom track loader, standard AmigaDOS disk formats (`.adf`) are often not enough because developers write raw MFM (Modified Frequency Modulation) data directly to the tracks, circumventing the standard sector layout. Alex was using the `.ipf` format to store raw flux transition data.

They needed to ensure Quaesar-NG was emulating the floppy drive rotation speed and disk DMA with total fidelity.

```bash
quaesar custom_loader.ipf \
  -k roms/kick13.rom \
  -s quickstart=A500,0 \
  -s floppy_speed=100 \
  -s cpu_memory_cycle_exact=true \
  -s blitter_cycle_exact=true
```

With `floppy_speed=100`, the virtual drive rotated at a realistic 300 RPM. The custom track loader, heavily dependent on the Index pulse arriving exactly every 200ms, functioned flawlessly. The second disk was required midway through the demo.

To automate testing the disk swap, Alex configured all three floppy drives simultaneously:

```bash
quaesar disk1.ipf \
  -k roms/kick13.rom \
  -s quickstart=A500,0 \
  -s floppy1=disk2.ipf \
  -s floppy2=disk3.ipf
```

Now, instead of navigating a GUI to swap disks, Alex's code simply requested the user to insert disk 2, and they simulated it by reading from `DF1:` in the emulator. The workflow remained uninterrupted.

---

## Chapter 9: The Power User's Cookbook

Months passed. Alex was no longer just a user of Quaesar-NG; they were an evangelist. They shared their Makefile configurations with other demosceners. 

Here are some of the legendary configurations Alex compiled into their personal cookbook.

### Recipe 1: The A500 "Bad CPU" Tester
Sometimes, you need to ensure your code doesn't rely on 68020+ instructions or fast memory. This profile forces a strict 68000, 512k/512k setup with cycle-exact timings.
```bash
quaesar demo.adf \
  -k roms/kick13.rom \
  -s quickstart=A500,0 \
  -s cpu_cycle_exact=true \
  -s blitter_cycle_exact=true \
  -s cpu_model=68000
```

### Recipe 2: The "Compile Inside the Amiga" Rig
Occasionally, Alex used SAS/C or VBCC inside the emulator itself. For this, they needed maximum CPU speed and lots of Fast RAM, throwing cycle accuracy out the window.
```bash
quaesar -k roms/kick31.rom \
  -s quickstart=A1200,0 \
  -s cpu_model=68040 \
  -s cpu_speed=max \
  -s fastmem_size=16 \
  -s chipmem_size=8 \
  -s filesystem=rw,dh0:./amiga_hdd
```

### Recipe 3: Multi-Drive Floppy Juggler
Testing a multi-disk trackmo? You can mount multiple floppy images simultaneously.
```bash
quaesar disk1.adf \
  -k roms/kick13.rom \
  -s quickstart=A500,0 \
  -s floppy1=disk2.adf \
  -s floppy2=disk3.adf
```

### Recipe 4: The A4000 Heavyweight
Testing high-end 68060 code? You can invoke the A4000 quickstart profile and inject a 68060 CPU with an FPU.
```bash
quaesar heavy_math.exe \
  -k roms/kick31.rom \
  -s quickstart=A4000,0 \
  -s cpu_model=68060 \
  -s fpu_model=68060 \
  -s fastmem_size=32 \
  -s z3fastmem_size=128
```

### Recipe 5: Headless Audio Bouncing
While Quaesar-NG focuses on execution, some users automate it to capture output. By using warp mode (maximum CPU speed), you can run an Amiga executable that generates audio or data and writes it to a mounted host directory, completing hours of Amiga CPU work in seconds.
```bash
quaesar render_audio.exe \
  -k roms/kick31.rom \
  -s cpu_speed=max \
  -s filesystem=rw,dh0:./output_dir
```

---

## Chapter 10: Deep Dive into Audio Output and Latency

Audio is the soul of any Amiga demo. The 4-channel, 8-bit Paula chip defined the sound of an era. For Alex, ensuring the tracker modules played perfectly in sync with the visuals was non-negotiable.

WinUAE has a plethora of sound settings, and Quaesar-NG inherits all of them via the `-s` arguments. 

If Alex noticed the audio was lagging behind the copper effects, they would adjust the sound buffer size.

```bash
quaesar my_demo.adf \
  -k roms/kick13.rom \
  -s sound_max_buff=4096 \
  -s sound_latency=10
```

By tweaking `sound_latency` and reducing `sound_max_buff`, Alex achieved near-hardware audio sync. The demoscene is unforgiving when it comes to visual-audio synchronization, and having access to these deep-level WinUAE flags through the command line meant Alex never had to compromise.

Furthermore, if they needed to test their 8-channel Protracker routines, they could configure the sound filtering. 

```bash
quaesar 8channel.adf \
  -k roms/kick13.rom \
  -s sound_filter=off \
  -s sound_stereo_separation=7
```
`sound_stereo_separation` controls the harsh panning of the original Amiga hardware, giving Alex the exact auditory representation they would hear on a real A500 plugged into a CRT TV.

---

## Chapter 11: Future Integrations and CI/CD

Alex didn't stop at their local machine. With Quaesar-NG's headless-capable arguments and CLI focus, they set up a Continuous Integration (CI) pipeline on GitHub Actions.

Every time they pushed a commit to their `main` branch, a Linux runner would compile their 68k assembly using `vasm`, bundle it into an ADF, and then run Quaesar-NG in a headless mode to execute a suite of unit tests built directly into the Amiga executable.

```bash
# In the CI pipeline
./quaesar test_suite.exe \
  -k kick13.rom \
  -s cpu_speed=max \
  --serial_port output.log \
  -s use_gui=no
```

The Amiga executable would run, perform memory allocation checks, execute math routines, and dump `[PASS]` or `[FAIL]` out to the serial port. The CI script would then parse `output.log` and either pass or fail the GitHub build.

Alex had effectively brought 1980s retro-development into the modern era of DevSecOps.

---

## Epilogue: The Demoparty

The lights in the hall dimmed. The projector hummed to life. Two thousand people fell silent as the compo machine booted up.

Alex's demo, "Cycle Breaker," flashed onto the screen. The music synchronized perfectly with the visuals. The voxel terrain deformed, the copper bars danced, and the blitter objects shattered into a million pieces.

It ran perfectly. Not a single frame was dropped. Not a single artifact appeared.

Alex leaned back in their chair and smiled. WinUAE had run the demo on the big screen, but it was Quaesar-NG that had built it. By stripping away the GUI and embracing the command line, Quaesar-NG hadn't just saved Alex time; it had kept them in the elusive state of flow, turning the daunting task of retro-development into a seamless, modern experience.

For the developer who values execution over dialogs, Quaesar-NG is the ultimate weapon.

Type your command. Press Enter. Let the code run.


---

## Chapter 12: The Comprehensive Option Glossary

As Alex dove deeper into the WinUAE source code, they compiled a massive glossary of all the `-s` arguments that proved useful. Here is that glossary, provided for any demoscener looking to bend Quaesar-NG to their will.

### CPU Configuration
* `-s cpu_model=68000`: The original Motorola 68000 CPU. Perfect for A500.
* `-s cpu_model=68010`: The Motorola 68010 CPU. Rarely used, but good for testing WBR (Vector Base Register) tricks.
* `-s cpu_model=68020`: The Motorola 68020 CPU. The heart of the A1200.
* `-s cpu_model=68030`: Adds a data cache and MMU over the 68020.
* `-s cpu_model=68040`: A major architectural leap. Great for heavy 3D math.
* `-s cpu_model=68060`: The fastest classic 68k processor. Superscalar execution.
* `-s cpu_speed=real`: Locks the CPU emulation to the original hardware timing.
* `-s cpu_speed=max`: Unlocks the CPU emulation. It will run as fast as the host PC can process instructions.
* `-s cpu_cycle_exact=true`: Ensures that CPU operations match the exact clock-cycle timing of the real silicon, down to the bus access.
* `-s cpu_memory_cycle_exact=true`: Extends cycle accuracy to memory accesses, handling DMA contention with the custom chips.

### FPU (Floating Point Unit) Configuration
* `-s fpu_model=68881`: The standard FPU companion to the 68020.
* `-s fpu_model=68882`: An optimized version of the 68881.
* `-s fpu_model=68040`: Uses the 68040's internal FPU.
* `-s fpu_model=68060`: Uses the 68060's internal FPU.
* `-s fpu_strict=true`: Enforces strict IEEE floating-point precision, mirroring the exact rounding errors of the physical FPU.

### Chipset Configuration
* `-s chipset=ocs`: Original Chip Set (A1000, A500, A2000).
* `-s chipset=ecs_denise`: Enhanced Chip Set (A500+, A600). Adds SuperHires and hardware sprites over the borders.
* `-s chipset=aga`: Advanced Graphics Architecture (A1200, A4000). 256 colors, 32-bit memory fetching, 8-bit bitplanes.
* `-s collision_level=0`: Disables hardware sprite/playfield collisions.
* `-s collision_level=1`: Sprite-to-Sprite collisions only.
* `-s collision_level=2`: Sprite-to-Playfield collisions included.
* `-s collision_level=3`: Full pixel-perfect collisions (heavy performance cost).

### Display Configuration
* `-s gfx_width=800`: Sets the window width.
* `-s gfx_height=600`: Sets the window height.
* `-s gfx_fullscreen_amiga=true`: Launches the emulator directly into fullscreen mode.
* `-s gfx_center_horizontal=true`: Centers the Amiga display output horizontally.
* `-s gfx_center_vertical=true`: Centers the Amiga display output vertically.
* `-s gfx_framerate=1`: Renders every frame (default). `2` renders every other frame (useful for slow hosts).
* `-s line_mode=0`: Single scanlines (default).
* `-s line_mode=1`: Double scanlines (useful for making the display look correct on modern monitors).

### Memory Configuration
* `-s chipmem_size=1`: 512KB Chip RAM.
* `-s chipmem_size=2`: 1MB Chip RAM.
* `-s chipmem_size=4`: 2MB Chip RAM (Max for AGA).
* `-s chipmem_size=8`: 4MB Chip RAM (Max for UAE expanded).
* `-s fastmem_size=2`: 2MB Zorro II Fast RAM.
* `-s fastmem_size=4`: 4MB Zorro II Fast RAM.
* `-s fastmem_size=8`: 8MB Zorro II Fast RAM (Max for Zorro II).
* `-s z3fastmem_size=64`: 64MB Zorro III Fast RAM (Requires 32-bit CPU like 68020+).
* `-s bogomem_size=1`: 512KB Slow RAM at $C00000. Often required by older A500 games.

### Floppy Drive Configuration
* `-s floppy0=path`: Path to DF0:.
* `-s floppy1=path`: Path to DF1:.
* `-s floppy2=path`: Path to DF2:.
* `-s floppy3=path`: Path to DF3:.
* `-s floppy_speed=100`: Accurate drive speed.
* `-s floppy_speed=800`: 8x turbo speed.
* `-s floppy0type=0`: 3.5" DD (Double Density, 880KB).
* `-s floppy0type=1`: 3.5" HD (High Density, 1.76MB).

### Audio Configuration
* `-s sound_stereo_separation=7`: Default Amiga harsh panning (100% left/right).
* `-s sound_stereo_separation=3`: Softer panning (useful for headphone users).
* `-s sound_filter=off`: Disables the Amiga hardware audio filter.
* `-s sound_filter=on`: Enables the Amiga hardware audio filter.
* `-s sound_filter=auto`: Emulates the power-LED linked audio filter (A500+ and newer).
* `-s sound_latency=20`: Audio latency in milliseconds. Lower is better for sync, higher prevents crackling on slow hosts.

### File System Configuration
* `-s filesystem=rw,dh0:/path/to/dir`: Mounts a folder with read/write access.
* `-s filesystem=ro,dh0:/path/to/dir`: Mounts a folder with read-only access.
* `-s hardfile=rw,0,1,2,512,/path/to/file.hdf`: Mounts a raw hard disk image file.

---

## Chapter 13: Frequently Asked Questions

As the Quaesar-NG community grew, Alex found themselves answering the same questions over and over. They compiled this FAQ.

**Q: Why doesn't Quaesar-NG have a GUI like WinUAE or FS-UAE?**
A: Quaesar-NG is intentionally stripped down to provide the fastest possible integration into build pipelines. GUIs introduce friction when you are running an emulator 500 times a day to test small code changes. By keeping everything on the command line, Quaesar-NG stays out of your way.

**Q: Can I use Quaesar-NG to play games?**
A: Yes, absolutely. While it's targeted at developers, running `quaesar game.adf -k kickstart.rom` will launch the game just fine. However, without a GUI, changing floppy disks requires launching from the command line with multiple drives pre-configured, or using disk swapper commands if implemented.

**Q: Why does my demo crash immediately when using the default settings, but works fine on WinUAE?**
A: Quaesar-NG defaults to a strict A500 environment with Kickstart 1.3, OCS, and 512k/512k memory. WinUAE's default "Quickstart" often enables extra compatibility hacks or uses a different Kickstart version if not explicitly configured. Ensure you are setting the `quickstart` string or manually defining your memory layout.

**Q: Does Quaesar-NG support Action Replay or other hardware freezers?**
A: Currently, hardware freezers are not the primary focus, as developers usually rely on their own debuggers or the `kprintf` serial output. However, since the core is WinUAE, certain cartridge configurations might still be functional via the `-s cart=` parameter.

**Q: How do I test NTSC vs PAL timing?**
A: The Amiga scene is predominantly PAL (50Hz), which is why Quaesar-NG defaults to it. However, if you need to test NTSC (60Hz) compatibility, you can use `-s ntsc=true`. Keep in mind NTSC gives you less raster time per frame, so effects that run perfectly in PAL might cause frame drops in NTSC.

**Q: Can I use RTG (Retargetable Graphics) like Picasso96?**
A: No. Quaesar-NG explicitly removed RTG support. The focus is entirely on the classic custom chipsets (OCS/ECS/AGA). If you are writing software for RTG, you should use WinUAE or FS-UAE.

**Q: My serial output isn't working on macOS.**
A: Ensure you are piping the serial port to a valid TTY device or a file that your user has write permissions for. `quaesar demo.adf --serial_port ./debug.log` is usually the safest cross-platform method.

---

## Chapter 14: Conclusion and Final Thoughts

The demoscene is a culture of limitation. By restricting the hardware, the software is forced to become art. 

Quaesar-NG embraces this philosophy. By restricting the interface, the workflow is forced to become pure. There are no menus to distract you, no sliders to fiddle with. There is only your code, the command line, and the raw, unadulterated power of the Motorola 68000 and the custom chipset.

As Alex closed their terminal for the night, they realized they hadn't touched their mouse in hours. The keyboard was their only instrument, and the Amiga was their canvas. 

This guide has provided you with the knowledge to harness Quaesar-NG. The rest is up to you. 

Write the code. Break the cycle limit. Push the hardware. 

Welcome to the scene.


---

## Appendix A: The Amiga Chipset Legacy

For developers coming to Quaesar-NG from other platforms, or for younger demosceners discovering the Amiga for the first time, understanding the nuances of the custom chipsets is critical. The `-s chipset=...` flag is the most important switch you can pass when trying to achieve cycle accuracy for a specific competition category.

### The OCS (Original Chip Set)
Found in the Amiga 1000, Amiga 500, and Amiga 2000.
The OCS is the foundation of the demoscene. It features the original Agnus (memory controller), Denise (video), and Paula (audio and floppy). 
* **Limitations:** Maximum of 512KB Chip RAM (or 1MB with a "Fat Agnus" upgrade, often simulated using `chipmem_size=2`). Palettes are strictly 12-bit (4096 possible colors). Hardware sprites are limited to 15 colors plus transparency.

### The ECS (Enhanced Chip Set)
Found in the Amiga 500+, Amiga 600, and Amiga 3000.
The ECS introduced the "Super Fat Agnus," supporting up to 2MB of Chip RAM natively. Denise was upgraded to "Super Denise," allowing for SuperHires resolutions (productivity modes) and the ability to display sprites in the overscan border regions.
* **Why it matters:** Many A500 demos break on ECS because they rely on undocumented hardware quirks of the original Agnus or Denise. Quaesar-NG allows you to toggle this precisely with `-s chipset=ecs_denise`.

### The AGA (Advanced Graphics Architecture)
Found in the Amiga 1200, Amiga 4000, and CD32.
AGA was a monumental leap forward. "Alice" replaced Agnus, "Lisa" replaced Denise (Paula remained largely unchanged). 
* **Features:** Full 32-bit memory fetching from Chip RAM, meaning the CPU and Blitter are much faster. The color palette expanded to 24-bit (16.8 million colors), and the system can display 256 colors simultaneously from a single palette, or up to 262,144 colors in HAM8 mode. Sprites can be 64 pixels wide.
* **Demoscene Impact:** The A1200 became the standard for 3D graphics and complex mathematical demos in the mid-to-late 90s. 

By manipulating these settings via Quaesar-NG's command line, you have a virtual museum of Commodore engineering at your fingertips.

---

## Appendix B: Command Line and Configuration Reference

Quaesar-NG can be launched entirely from the command line. The two most important switches you will use are `-k` (for the Kickstart ROM) and `-s` (for passing internal UAE configuration settings).

```bash
./quaesar-dbg -k /path/to/kickstart.rom [options]
```

### Core Switches
- **`-k <path>`**: Path to the Amiga Kickstart ROM file.
- **`-s <key>=<value>`**: Injects a UAE configuration parameter directly into the emulator before boot. You can use this switch multiple times to build your machine configuration.

### Common `-s` Configuration Options
- **`quickstart=Model,0`**: Sets the base hardware profile. Examples: `A500,0`, `A1200,0`, `A4000,0`.
- **`cpu_model=<model>`**: Forces a specific CPU model. Examples: `68000`, `68020`, `68030`, `68040`, `68060`.
- **`fastmem_size=<MB>`**: Allocates Fast RAM. Example: `fastmem_size=8` allocates 8MB of Zorro II Fast RAM.

### Storage & Filesystems

Mounting hard disk images and host directories is done via the `-s` switch. **Pay close attention to the syntax**, as UAE's parser is strictly positional.

**Mounting a Hardfile (.vhd / .hdf)**
To mount a hardfile, use the `hardfile2` parameter. The syntax allows you to specify the controller type.
**Syntax:** `-s hardfile2=<access>,<DeviceName>:<Path>,0,0,0,512,0,,<Controller>`
**Example:** `-s hardfile2=rw,DH0:/path/to/OS-3.2.3.vhd,0,0,0,512,0,,ide0`

**Mounting a Host Directory**
To mount a folder from your host OS as an Amiga hard drive, you must use the **`filesystem2`** parameter.
**Syntax:** `-s filesystem2=<access>,<DeviceName>:<VolumeName>:<HostPath>,<BootPriority>`
**Example:** `-s filesystem2=rw,DH1:HostDir:/Users/dev/amiga/host_share,0`

> [!WARNING]
> **The `filesystem` vs `filesystem2` Trap & The Missing Comma**
> 1. UAE retains a legacy `filesystem=` parameter. This older parameter *only* accepts two arguments (`VolumeName:HostPath`). Do not attempt to use this parameter for defining device names, as passing `dh0::/path` will result in an invalid host path starting with a colon (`:/path`), causing UAE to silently mount your Current Working Directory instead (naming it `RDH0`).
> 2. When using the modern `filesystem2=` parameter, **you MUST include the boot priority at the very end of the string (e.g., `,0`)**. If you omit this final comma and number, UAE's string parser fails, passes an empty host path to the core, and once again, silently mounts your Current Working Directory as `RDH0`.

### UI Interaction & Mouse Capture

Quaesar-NG utilizes an intuitive, WinUAE-style mouse capture system alongside modern ImGui overlays for debugging and configuration.

- **Capturing the Mouse:** The emulator does not automatically capture your host mouse cursor. To interact with the Amiga, explicitly **click anywhere inside the Amiga display**. Your host cursor will disappear, and mouse inputs will be routed to the emulated machine.
- **Releasing the Mouse:** Press **`ESC`** at any time to release the mouse grab and return control to your host OS.
- **Focus Loss:** If the emulator window loses focus (e.g., you Alt-Tab / Cmd-Tab to another application, or click the separate debugger window), the mouse is automatically released. It will not auto-grab when you return; you must explicitly click the display again.
- **Main Menu (`F12`):** Opens the Quaesar-NG configuration UI. The mouse is automatically released so you can interact with the menu.
- **Debugger (`Shift+F12`):** Opens the integrated Amiga Debugger window. The mouse is instantly released, allowing you to seamlessly step through assembly code or inspect memory.

---
*End of Quaesar-NG Guide.*
