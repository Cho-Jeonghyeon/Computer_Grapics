# AI Notes

## User Preferences
- Korean conversation preferred.
- User likes pragmatic explanations that connect code structure to intuition.
- The user is learning C++ and computer graphics assignments in Visual Studio.
- For conceptual questions, explain the underlying design intuition, not only syntax.
- For direct implementation requests, apply the change and verify with build/run when possible.

## Project Context
- Workspace: `C:\Users\USER\source\repos\computer_grapics`
- GitHub repository: `https://github.com/Cho-Jeonghyeon/Computer_Grapics`
- Main branch: `main`
- Solution file: `computer_grapics.slnx`
- Visual Studio path used by automation: `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.exe`
- Chrome path used by automation: `C:\Program Files\Google\Chrome\Application\chrome.exe`

## Local Automation
- User command phrase: `컴그 시작하자`
- The notebook and desktop have different layouts; do not overwrite one with the other.
- Desktop setup (updated 2026-09-06):
  - Repository: `C:\Users\user\source\repos\Computer_Grapics\Computer_Grapics`.
  - Entry point: `C:\Users\user\start-comg.ps1`.
  - Git-tracked implementation: `scripts\start-comg-desktop.ps1` (solution resolved relative to the script).
  - When the user says `컴그 시작하자` on the desktop, run this script to open the apps and arrange them.
  - Use the primary monitor's working area, excluding the taskbar, split into three columns.
  - Left: ChatGPT; YouTube Music in the same column behind ChatGPT.
  - Center: the user's existing PowerShell / Windows Terminal window, or a new PowerShell if none exists.
  - Right: Visual Studio with `computer_grapics.slnx` open.
  - Launch closed apps. If the user closes apps before starting, actually reopen them; arranging existing windows alone is not sufficient.
  - Existing matching windows are retained to avoid duplicate apps or losing work.
  - Prefer the installed YouTube Music Chrome app; fall back to Chrome app mode.
  - Reapply placement after startup to counter restored window positions.
  - If several terminal windows exist, use `-TerminalWindowHandle` to identify the intended one.
  - `-Inspect` reports current windows without launching or moving them.
  - The home-directory entry point is local; commit/push the tracked script and this note to transfer them to another machine.
- Previous notebook setup: local `start-comg.ps1` places ChatGPT on the left half, Visual Studio on the right half, and maximizes YouTube Music behind them. Preserve this notebook layout unless the user asks to change it.

## Warm-up 1 Assignment Mapping
- Assignment PDF: `docs\컴그 워밍업 1.pdf`
- `Lab_01`: problem 1, 4x4 matrix program.
- `Lab_02`: problem 2, file/string processing.
- `Lab_03`: problem 3, point list using structs.
- `Lab_04`: problem 4, card matching game.

## Lab_01 Status
- `Lab_01` problem 1 is complete and committed.
- Commit: `57430d8 실습 1 마무리`
- Implemented commands:
  - `m`: matrix multiplication.
  - `a`: matrix addition.
  - `d`: matrix subtraction.
  - `r`: determinant for both matrices.
  - `t`: transposed matrix and determinant of the transposed matrix for both matrices.
  - `e`: subtract each row's minimum value from that row; toggles back to original view when pressed again.
  - `f`: add each column's maximum value to that column; toggles back to original view when pressed again.
  - `+`: add 1 to every value with modulo 10 range `0..9`.
  - `-`: subtract 1 from every value with modulo 10 range `0..9`.
  - `s`: reset both matrices with new random values.
  - `q`: quit.
- Important implementation idea:
  - `baseMatrix1` and `baseMatrix2` preserve the source-of-truth matrices.
  - `matrix1` and `matrix2` are the current display/operation matrices.
  - `rowMinMode` and `colMaxMode` store toggle state.
  - `UpdateCurrentMatrix()` rebuilds current matrices from base matrices plus active modes, preventing state bugs when `e` and `f` are toggled in different orders.

## Lab_02 Status
- `Lab_02` problem 2 has started.
- Commit: `bf0fa4b Lab02 대소문자 토글 및 출력`
- Pushed to `origin/main`.
- Current implemented behavior:
  - `data.txt` exists under `Lab_02`.
  - Program asks for a data file name.
  - Program reads all lines from the file into `vector<string> sentences`.
  - Program prints the loaded lines.
  - `a`: toggles case output for all sentences.
  - Pressing `a` again prints the original sentences.
  - `q`: quits.
- Build verification:
  - `Lab_02` Debug x64 build succeeded with 0 warnings and 0 errors after removing Korean comments that caused C4819.
  - Run test with `data.txt`, `a`, `a`, `q` succeeded.

## Lab_02 Concepts Discussed
- `vector<string>` stores multiple lines; each element is one sentence/line.
- `sentences.size()` is the number of stored lines, expected to be 10 for the assignment data.
- `sentences[i].length()` is the number of characters in the `i`th line.
- `const vector<string>& sentences` means pass the vector by reference without copying and prevent the function from modifying it.
- `islower`, `isupper`, `toupper`, and `tolower` come from `<cctype>`.
- ASCII uppercase/lowercase English letters differ by 32, but using `<cctype>` is clearer and more standard.
- For word counting in problem 2:
  - `vector` only stores lines; it does not automatically satisfy the spacing/special-character rule.
  - The algorithm should count words by whitespace transitions.
  - Multiple spaces should count as one separator.
  - Special characters like `/`, `-`, `*`, and punctuation should not split words unless separated by spaces.

## Git State To Remember
- Recent commits:
  - `bf0fa4b Lab02 대소문자 토글 및 출력`
  - `57430d8 실습 1 마무리`
  - `385cb12 Lab 01 기본 명령어와 작업 기록 추가`
  - `0fb2a6d 워밍업 1 세팅`
  - `05d6647 Initial project setup`
- After `bf0fa4b`, `origin/main` was pushed successfully.
- Before this note update, the working tree was clean.
