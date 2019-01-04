# DOOM From Scratch (DOOMFS)!

This is my personal take on implementing the original DOOM game from scratch but leveraging modern programming constructs. This means taking advantage of any feature that might make programming less error-prone, more efficient, easier to compile, and faster while keeping the same approach the original developers took to implement the whole game engine).

For the moment, this is just a personal and side project that barely gets any attention throughout the week except from those lonely programming sessions when you can escape from reality.

At this stage, I am currently devising a well-documented and complete way to interpret DOOM WAD files. The current goal is being able to load, read, and interpret the whole file. That implies turning every asset into a format that can be easily understood, e.g., we must be able to extract music files as MIDI or palettes as PPM files for visualization.

The whole process is being documented at this project's [GitHub pages](https://blitzman.github.io/doomfs/).

DISCLAIMER: There have already been a lot of DOOM ports, reboots, remakes, and all those fancy names. Most of them are open-source, some of them are not. Few of them are well-document, most of them are not. Lots of great programmers have already posted very useful texts about DOOM's inner mechanisms. This project does not intend to override any of them. It is just a personal journey I decided to take by collecting all the knowledge available and creating my own little sandbox. The idea is to generate as much documentation as possible to fill the gaps. Mistakes will be made.

Feel free to drop a mail to albertgg93@gmail.com for any suggestion, issue, or just to say hi!

## Requirements

* CMake >= 3.10.2
  * pkgconfig
* GLFW >= 3.2.1
* GLM >= 0.9.9.3
* Vulkan
  * LunarG Vulkan SDK >= 1.1.92.1