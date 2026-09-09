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
- Latest implementation commit: `88ecd11 Lab02 문자열 명령 b부터 f까지 구현`.
- Assignment sample text was added to `Lab_02/data.txt` in commit `c2bf381 Lab02 과제 입력 데이터 갱신`.
- These commits were pushed to `origin/main` on 2026-09-07.
- Current implemented behavior:
  - `data.txt` exists under `Lab_02`.
  - Program asks for a data file name.
  - Program reads all lines from the file into `vector<string> sentences`.
  - Program prints the loaded lines.
  - `a`: toggles case output for all sentences.
  - Pressing `a` again prints the original sentences.
  - `b`: prints each sentence followed by its word count. A word is counted when a non-space character begins after the start of a line or one or more spaces.
  - `c`: highlights words whose first character is uppercase and prints the total count. Pressing `c` again prints the original sentences.
  - `d`: reverses every whole sentence character-by-character. Pressing `d` again prints the original sentences.
  - `e`: displays every space as `*`. Pressing `e` again prints the original sentences.
  - `f`: keeps word order but reverses the characters inside each word. Both spaces and `*` are treated as word delimiters. Pressing `f` again prints the original sentences.
  - `q`: quits.
- Toggle design:
  - `toggle_c`, `toggle_d`, `toggle_e`, and `toggle_f` live in `main()` so their values survive function calls.
  - Do not declare a toggle as an ordinary local variable inside a command function; it would be reset on every call.
  - Command functions only produce their transformed output; the corresponding branch in `main()` selects transformed or original output.
- Important `f` implementation idea:
  - Accumulate characters in a temporary `string word` until a space or `*` is found.
  - Print the accumulated word backward, clear it, and then print the delimiter.
  - Print the remaining word once more after the loop because the final word usually has no trailing delimiter.
- Build verification:
  - After implementing `b` through `f`, the `Lab_02` Debug x64 build succeeded with 0 errors.
  - Three C4267 warnings remain where `string::length()` (`size_t`) is converted to `int` in reverse loops.
- Remaining assignment commands: `g`, `h`, `i`, and `j`.
- Current transformations print views without modifying `sentences`; review whether `e` and `f` should combine as persistent modes when finishing the assignment.

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
  - `88ecd11 Lab02 문자열 명령 b부터 f까지 구현`
  - `c2bf381 Lab02 과제 입력 데이터 갱신`
  - `cd75750 데스크탑 컴그 시작 자동화 추가`
  - `a223595 AI 노트 최신화`
  - `bf0fa4b Lab02 대소문자 토글 및 출력`
  - `57430d8 실습 1 마무리`
  - `385cb12 Lab 01 기본 명령어와 작업 기록 추가`
  - `0fb2a6d 워밍업 1 세팅`
  - `05d6647 Initial project setup`
- `origin/main` was synchronized through commit `88ecd11` before this note update.
- Desktop repository-local Git identity matches the existing history: `Cho-Jeonghyeon <69703120+Cho-Jeonghyeon@users.noreply.github.com>`.

## 2026-09-08 Work Note
- User asked to commit and push current progress even though known bugs remain.
- Known Lab_01 issue:
  - Problem 1 has a bug around `e` / `f` toggles combined with `+` / `-`.
  - Recheck how base matrices and active view modes interact after increment/decrement commands.
- Current Lab_02 progress:
  - `g`: replaces a requested character with another requested character. Pressing `g` again prints the original sentences.
  - `h`: finds digits and moves the following text to the next line; skips a leading space after the inserted newline.
  - `i`: reads a search word, prints matching words in color, and counts matches case-insensitively.
  - `j`: initially added with a loop-index bug; resolved in the 2026-09-09 update below.

## 2026-09-09 Desktop Handoff
- `Lab_01` was reviewed and built successfully with 0 errors.
- `Lab_01` verification results:
  - `e` alone correctly subtracts each row minimum, and pressing `e` again restores the base matrices.
  - `f` alone correctly adds each column maximum, and pressing `f` again restores the base matrices.
  - `+` and `-` alone correctly use modulo 10, including the `9 -> 0` and `0 -> 9` transitions.
  - Known combination bug remains: while `e` or `f` is active, `+` / `-` changes the base matrices and then recalculates the active view. The visible values therefore do not consistently increase/decrease by one, and `f` combinations can display values above 9.
  - `UpdateCurrentMatrix()` always applies `e` before `f` when both modes are active, regardless of the order in which the commands were entered.
- `Lab_02` update:
  - `Command_j` now performs a cyclic downward shift: the final sentence is printed first, followed by sentences from the beginning through the second-to-last sentence.
  - `Lab_02` Debug x64 build succeeds with the three previously known C4267 warnings.
- `Lab_03` has been started as an incomplete deque-based draft in `Lab_03/main.cpp`.
  - Current draft defines `SIZE`, `Deque`, `init_deque()`, `is_empty()`, and `is_full()`.
  - Additional deque functions were copied/adapted but still refer to undeclared sample names such as `DequeType`, `element`, `MAX_QUEUE_SIZE`, `error`, and `deque_print`.
  - `Lab_03` does not build yet; continue by making the type/constant/function names consistent and then adapt the deque to store the assignment's `(x, y, z)` point structure with up to 10 slots.
  - A C4819 source-encoding warning is also present because Korean comments are not representable in code page 949.

## 2026-09-09 Warming-up Completion
- User said the warming-up assignment period has passed and asked to split the finished work into several commits instead of one large commit.
- `Lab_03` was finished in a deliberately simple student-style C++ implementation:
  - Uses `Point { x, y, z }`, fixed arrays, `bool used[10]`, and simple loops.
  - Keeps `front`, `rear`, and `count` for the deque-style list, but uses `used[]` to decide which visual slots are actually filled.
  - `+`: inserts at the top-side rear position, first moving `rear` forward until an empty slot is found to avoid overwriting existing data after sparse operations.
  - `-`: deletes from the top-side rear position.
  - `e`: inserts at index 0 and shifts existing points upward, matching the page 8 example.
  - `d`: deletes from the bottom by clearing the used flag, so the deleted slot remains visually empty.
  - `a`: prints the stored point count.
  - `b`: moves occupied slots down by one index with wraparound.
  - `c`: clears the list.
  - `f`: prints a distance-from-origin view sorted in ascending distance, displayed from index 0 upward without preserving sparse gaps.
  - `g`: checks all used point pairs with nested loops and prints the nearest and farthest pair.
- `Lab_04` was implemented as a simple console card matching game:
  - Uses global `board[6][6]` and `opened[6][6]` arrays.
  - Accepts board dimensions from 3 to 6.
  - Creates paired lowercase cards, adds `@` joker for odd board sizes, shuffles with `rand()`, and hides cards as `*`.
  - Accepts positions like `a1 c3`, opens matching cards as uppercase, and lets joker match any card.
  - Supports `r` reset, `h` temporary full reveal, and `q` quit.
  - Tracks attempts and score.
  - Uses `system("cls")` in `print_board()` for a game-like redraw instead of cumulative console output.
- Build verification:
  - `Lab_03` Debug x64 builds with 0 warnings and 0 errors.
  - `Lab_04` Debug x64 builds with 0 warnings and 0 errors.
