# CartFoundry documentation by version

Choose the edition matching the version shown in **About CartFoundry**.
Commands and controls in a newer edition may not exist in an older application.

| Application version | Documentation |
| --- | --- |
| 0.9.1 | [Handbook, MIDI exchange and reference chapters](versions/0.9.1/README.md) |
| 0.9.0 | [Handbook, examples and reference chapters](versions/0.9.0/README.md) |

The desktop **Help → Documentation book…** (F1) reads the edition bundled with
that executable. It never silently switches to the latest online book. Each
release keeps its own versioned snapshot here so older clients retain matching
instructions. Ctrl/Cmd+F focuses the desktop book's whole-book search.

For contributors, editable chapter sources live alongside this index. CMake
builds a versioned edition under `build/documentation/<app-version>/`.
Release packages bundle that edition under `resources/docs/versions/<app-version>`
(`Contents/Resources` on macOS). Publish the same snapshot to
`docs/versions/<app-version>` on GitHub when updating that edition. Bumping the
application version creates a separate book; retain previous version directories.
