Beatbox ECE362 Project

## 📝 Git Commit Message Rules

- Use **imperative mood** (e.g., “add feature,” not “added feature”).
- Keep the **summary short**, ideally under **50 characters**.
- Example:

## 🧩 Adafruit Protomatter Library

This project uses the [Adafruit Protomatter](https://learn.adafruit.com/adafruit-protomatter-rgb-matrix-library?view=all) library to drive a HUB75 RGB LED matrix display.  
Protomatter handles the low-level timing and refreshing of the LED panel, enabling smooth graphics and text rendering through the Adafruit GFX interface.

All wiring and configuration are already set up — this library manages the display updates and color output.

## 🧪 How to Test

To test changes safely, create a new branch instead of working directly on `main`.

```bash
# Check which branch you're on
git status

# Switch to main if needed
git checkout main

# Create and switch to a new test branch (NAME IT SOMETHING ELSE)
git checkout -b test-branch

# Initial test push
git add .
git commit -m "test: add experimental changes"

# Push the branch to GitHub
git push -u origin test-branch

## 🔀 Merge Requests

When your test branch is ready, open a merge request (pull request) to merge it into `main`.

This allows others to review your code before changes are integrated.

**Steps:**
1. Push your branch to GitHub.
2. Go to the repository page.
3. Click **Compare & pull request**.
4. Add a clear title and description.
5. Submit for review(Add Isaac as Reviewer).

Only merge after testing and approval to keep `main` stable.

```

## 🖥️ TFT Display Usage

See the [project-lcd README](https://github.com/ece362-purdue/proton-labs/blob/main/project-lcd/README.md) for an example setup.  
It shows how to wire the TFT display, initialize it in code, and draw simple graphics/text with your microcontroller.
