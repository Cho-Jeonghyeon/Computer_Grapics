# AI Notes

## User Preferences
- Korean conversation preferred.
- For code changes, show the proposed code in chat first unless the user clearly asks to apply it directly.
- The user is learning/working through computer graphics assignments in C++ with Visual Studio.

## Project Context
- Workspace: `C:\Users\USER\source\repos\computer_grapics`
- GitHub repository: `https://github.com/Cho-Jeonghyeon/Computer_Grapics`
- Solution file: `computer_grapics.slnx`
- Intended structure: one Visual Studio solution with separate projects `Lab_01`, `Lab_02`, `Lab_03`, `Lab_04`, eventually continuing like `Lab_05` ... up to around `Lab_30`.

## Current Warm-up 1 Setup
- Assignment PDF: `docs\컴그 워밍업 1.pdf`
- `Lab_01`: problem 1, 4x4 matrix program.
- `Lab_02`: problem 2, file/string processing.
- `Lab_03`: problem 3, point list using structs.
- `Lab_04`: problem 4, card matching game.

## Git State To Remember
- Initial GitHub push was completed to `origin/main`.
- Commit `0fb2a6d` was created locally with message `워밍업 1 세팅`.
- As of this note, local `main` is ahead of `origin/main` by 1 commit unless it has been pushed later.
- `Lab_01/main.cpp` was modified after that commit with early `s` and `q` command handling.

## Lab_01 Current Direction
- Need a 4x4 random matrix program with two matrices.
- Values initially random from 1 to 9.
- Commands from assignment include `m`, `a`, `d`, `r`, `t`, `e`, `f`, `+`, `-`, `s`, `q`.
- `s`: reset both matrices with new random values.
- `q`: quit program.
- Important: `-` means decrement all matrix values, while `d` means matrix subtraction.
