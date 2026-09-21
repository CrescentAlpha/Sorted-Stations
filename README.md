# Sorted Stations

<p align="center">
  <img src="https://raw.githubusercontent.com/CrescentAlpha/Sorted-Stations/main/Resources/icon.png" alt="Sorted Stations" width="256" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Satisfactory-Update%201.2%2B-orange?style=flat-square" alt="Game Version" />
  <img src="https://img.shields.io/badge/SML-%5E3.12.0-blue?style=flat-square" alt="SML Version" />
  <img src="https://img.shields.io/badge/Multiplayer-Client--Side%20Safe-success?style=flat-square" alt="Multiplayer Safe" />
</p>

**Sorted Stations** automatically organizes and sorts station names, train schedules, drone ports, and custom vehicle markers across the in-game Map and management menus in **Satisfactory**.

Supports both standard **Unicode code-point order** and **Windows Natural Sort Order** (handling numbers logically as `Station 1`, `Station 2`, `Station 10`).

---

## Features

- **Map & Compass Marker Sorting**:
  - Automatically sorts icons and markers displayed in the in-game Map marker list:
    - **Train Stations**
    - **Trains**
    - **Drone Stations**
    - **Drones**
    - **Truck / Tractor Stations**
    - **Custom Named Vehicles** (Tractors, Trucks, Explorers, Cyber Wagons)
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
2. Adjust sorting modes for **Map settings**, **Train time table settings**, and **Drone time table settings** as desired.
3. Load your save game to apply the changes.

| Setting | Values | Default | Description |
| :--- | :--- | :--- | :--- |
| **Map Markers** | None / Unicode / Natural | `Unicode` | Sort markers listed in the Map panel |
| **Train Timetable** | None / Unicode / Natural | `Unicode` | Sort stations & trains in the timetable |
| **Drone Destinations**| None / Unicode / Natural | `Unicode` | Sort destination ports in the drone UI |

---

## Multiplayer & Dedicated Server

- **Client-Side Safe**: All sorting operations only reorder local display lists on the client.
- It does **not** alter game state, network replication, or vehicle automation routes.
- Safe to use on any multiplayer session or Dedicated Server even if the host/server does not have the mod installed.

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
