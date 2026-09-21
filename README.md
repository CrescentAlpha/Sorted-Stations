# Sorted Stations

<p align="center">
  <img src="https://raw.githubusercontent.com/CrescentAlpha/Sorted-Stations/main/Resources/icon.png" alt="Sorted Stations" width="256" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Satisfactory-Update%201.2%2B-orange?style=flat-square" alt="Game Version" />
  <img src="https://img.shields.io/badge/SML-%5E3.12.0-blue?style=flat-square" alt="SML Version" />
  <img src="https://img.shields.io/badge/Multiplayer-Client--Side%20Safe-success?style=flat-square" alt="Multiplayer Safe" />
</p>

**Sorted Stations** automatically organizes and sorts station names, train schedules, drone ports, and custom vehicle names in the management menus in **Satisfactory**.

Supports both standard **Unicode code-point order** and **Windows Natural Sort Order** (handling numbers logically as `Station 1`, `Station 2`, `Station 10`).

---

## Features

- **Train Schedule & Timetable Sorting**:
  - Train stations and locomotive lists in the train menu and timetable are sorted alphabetically/naturally.
- **Drone Port Search Sorting**:
  - Available drone station destinations are organized and sorted for quick destination selection.
- **Multi-Mode Sorting System**:
  - Each feature can be individually configured via the **Mod Configurations** menu.

---

## Sorting Modes Explained

| Mode | Logic | Example Sequence | Description |
| :--- | :--- | :--- | :--- |
| `0: None` | Vanilla | `Station B`, `Station A` (or distance) | Disables sorting for this category |
| `1: Unicode` | Lexicographical | `Station 1`, `Station 10`, `Station 2` | Traditional code-point sort (Default) |
| `2: Natural` | Windows Natural | `Station 1`, `Station 2`, `Station 10` | Intelligent numeric sort (`StrCmpLogicalW`) |

---

## Configuration

> [!NOTE]
> **Default Setting & How to Change**:  
> By default, all categories are automatically configured to **Unicode** sort upon initial installation. If you wish to choose another pattern (such as Natural sort or None), please change the settings from the **Main Menu before loading your save game**.

To customize sorting options:

1. From the Satisfactory **Main Menu** (before loading a save), go to **Mods** > **Sorted Stations**.
2. Adjust sorting modes for **Train timetable settings** and **Drone timetable settings** as desired.
3. Load your save game to apply the changes.

| Setting | Values | Default | Description |
| :--- | :--- | :--- | :--- |
| **Train Timetable** | None / Unicode / Natural | `Unicode` | Sort stations & trains in the timetable |
| **Drone Destinations**| None / Unicode / Natural | `Unicode` | Sort destination ports in the drone UI |

---

## Multiplayer & Dedicated Server

- **Sorting Features (v1.0.0)**:
  - **Client-Side Safe**: All sorting operations only reorder local display lists on the client.
  - Does **not** alter game state, network replication, or vehicle automation routes.
  - Safe to use on any multiplayer session or Dedicated Server, even if the host/server does not have the mod installed.
- **Multiplayer Editing Limitation**:
  - Edits made by the host-side player are reflected normally.
  - Edits made by other players are not reflected for the host or other players.
---

## Planned Features (Roadmap / Next: v1.0.1+)

The following features are planned for future updates. Specific features and implementation scope will be determined incrementally starting from v1.0.1:

- **Remote Renaming from Map Menu**:
  - Adds an edit (pencil) icon to each item in the in-game Map left-hand list.
  - Clicking the button opens a lightweight, dedicated rename dialog to rename stations and vehicles remotely and safely.
  - **Supported Objects**:
    - Train Stations & Trains / Locomotives
    - Drone Stations & Drones
    - Truck / Tractor Stations (Docking Stations)
    - All named vehicles (Tractors, Trucks, Explorers, Cyber Wagons)
- **Instant Auto Re-Sorting**:
  - Immediately re-sorts the Map list in real-time according to the active sort mode (Unicode / Natural) as soon as an object is renamed.
- **Custom Prefix / Tag Insertion**:
  - User-configurable tag buttons (e.g., `[IN]`, `[OUT]`, `[IRON]`, `[HUB]`) in the rename dialog.
  - Clicking a tag button inserts the text directly at the current cursor position in the name input field.
- **Marker Color Customization**:
  - Allows selecting and changing marker colors directly from the rename dialog for all customizable station and vehicle icons.
- **Single-Player / Host Focused**:
  - Designed safely for local and single-player environments without complex server RPC overhead.

---

## Requirements

- **Satisfactory**: Update 1.2+ (CL#491125 or newer)
- **Satisfactory Mod Loader (SML)**: `^3.12.0`

---

## Bug Reports & Support

- Found a bug or have a suggestion? Open an issue on [GitHub Issues](https://github.com/CrescentAlpha/Sorted-Stations/issues).

---

## Author

- **Created by**: CrescentAlpha
