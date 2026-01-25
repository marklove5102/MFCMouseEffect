# README Language Switch Rendering

## Goal
- Keep the language toggle as plain Markdown text instead of HTML blocks.
- Match the presentation style used in the Desk Tidy README.

## Change
- Removed the `<p align="center">` wrapper around the language toggle in `README.md` and `README.en.md`.
- Use a single Markdown line so GitHub renders it as normal text (not HTML).

## Notes
- This avoids mixed HTML/Markdown rendering quirks.
- The toggle stays visually consistent with other repos.
