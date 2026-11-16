# SaveMeshcore repeater filtering patch

This fork follows the latest official MeshCore repeater release and adds configurable packet filtering to repeater firmware. It does not follow the upstream development branches. New official `repeater-vX.Y.Z` releases are merged automatically while retaining these filters.

SaveMeshcore is for **repeater firmware only**. The patch changes only `examples/simple_repeater`, and this fork builds and checks only repeater targets. For companion, room server, or sensor firmware, use official MeshCore.

The filter commands are available only in repeater firmware. Changes are saved immediately and remain active after reboot. Run a command without `on` or `off` to show its current state.

| Command | Default | Behavior |
| --- | --- | --- |
| `filter` | | Shows the state of all filters in one line, for example `> 1byte off, data on, public off`. |
| `filter 1byte on` | Off | Stops forwarding 1-byte path-hash flood packets that have already traversed at least one hop. Zero-hop floods, direct packets, and floods using 2-byte or 3-byte path hashes are unaffected. |
| `filter 1byte off` | | Disables the 1-byte flood filter. |
| `filter 1byte` | | Shows whether the 1-byte flood filter is on or off. |
| `filter data on` | On | Stops forwarding payload types `0x06` (`GRP_DATA`), `0x0C`–`0x0E` (reserved unsigned types), and `0x0F` (`RAW_CUSTOM`). |
| `filter data off` | | Disables the data payload filter. |
| `filter data` | | Shows whether the data payload filter is on or off. |
| `filter public on` | Off | Stops forwarding group messages (`GRP_TXT` and `GRP_DATA`) on the default **Public** channel. |
| `filter public off` | | Disables the Public channel filter. |
| `filter public` | | Shows whether the Public channel filter is on or off. |

### CLI examples

Enable all filters:

```text
filter 1byte on
filter data on
filter public on
```

Disable all filters:

```text
filter 1byte off
filter data off
filter public off
```

Check all filters in one request, which is handy over LoRa:

```text
filter
> 1byte off, data on, public off
```

Check one filter at a time:

```text
filter 1byte
filter data
filter public
```

Example responses:

```text
OK - filter 1byte on
OK - filter data off
OK - filter public on
> filter 1byte on
> filter data off
> filter public on
```

### How the Public channel filter works

Group packets name their channel with a single hash byte, and many channels share each value. The Public channel's byte is `0x11`. Blocking that byte alone would also drop other channels that happen to use `0x11`.

Instead, the repeater checks each packet's 2-byte MAC against the published Public channel key. A packet is dropped only when the MAC verifies, which proves it belongs to Public. Other channels sharing `0x11` are still forwarded. The repeater never decrypts the message, so it cannot read Public traffic. The Public key is not a secret: it is published in the MeshCore source and FAQ.

The check runs only for group packets whose hash byte is `0x11`. Every other packet costs a single byte comparison.

> [!IMPORTANT]
> The data filter is enabled by default only when no saved `filter data` setting exists. After `filter data off` is saved, firmware updates from this fork preserve the off state.

### Firmware releases

Ready-to-flash firmware for every repeater board is published on the [Releases page](https://github.com/aigarslv/SaveMeshcore/releases). Each release is tagged `repeater-vX.Y.Z-savemesh`, where `vX.Y.Z` is the official MeshCore repeater version it is built from. For example, `repeater-v1.18.0-savemesh` is official repeater v1.18.0 with the SaveMeshcore filters.

When MeshCore publishes a new official repeater release, the fork merges it, creates the matching tag, and builds all repeater boards automatically.

### Firmware upgrade compatibility

The filter settings are stored in their own file, `/savemesh_prefs`, separate from the official `/prefs.json`. Official MeshCore firmware never reads or rewrites that file. You can install clean official firmware without erasing the device: all radio, identity, and region settings carry over, and the filters simply stop being applied. If you later reinstall SaveMeshcore, your saved filter choices, including `filter data off`, come back unchanged.

Older SaveMeshcore builds stored these settings in `/prefs.json`. On first boot, this build copies them into `/savemesh_prefs` automatically.

The automatic release updater checks each new release for upstream implementations using the same filter settings or commands. If it finds one, synchronization stops instead of combining the upstream and SaveMeshcore filters. You can then review the official implementation and move to clean upstream firmware without erasing the device.

## About MeshCore

MeshCore is a lightweight, portable C++ library that enables multi-hop packet routing for embedded projects using LoRa and other packet radios. It is designed for developers who want to create resilient, decentralized communication networks that work without the internet.

## 🔍 What is MeshCore?

MeshCore now supports a range of LoRa devices, allowing for easy flashing without the need to compile firmware manually. Users can flash a pre-built binary using tools like Adafruit ESPTool and interact with the network through a serial console.
MeshCore provides the ability to create wireless mesh networks, similar to Meshtastic and Reticulum but with a focus on lightweight multi-hop packet routing for embedded projects. Unlike Meshtastic, which is tailored for casual LoRa communication, or Reticulum, which offers advanced networking, MeshCore balances simplicity with scalability, making it ideal for custom embedded solutions, where devices (nodes) can communicate over long distances by relaying messages through intermediate nodes. This is especially useful in off-grid, emergency, or tactical situations where traditional communication infrastructure is unavailable.

## ⚡ Key Features

* Multi-Hop Packet Routing
  * Devices can forward messages across multiple nodes, extending range beyond a single radio's reach.
  * Supports up to a configurable number of hops to balance network efficiency and prevent excessive traffic.
  * Nodes use fixed roles where "Companion" nodes are not repeating messages at all to prevent adverse routing paths from being used.
* Supports LoRa Radios – Works with Heltec, RAK Wireless, and other LoRa-based hardware.
* Decentralized & Resilient – No central server or internet required; the network is self-healing.
* Low Power Consumption – Ideal for battery-powered or solar-powered devices.
* Simple to Deploy – Pre-built example applications make it easy to get started.

## 🎯 What Can You Use MeshCore For?

* Off-Grid Communication: Stay connected even in remote areas.
* Emergency Response & Disaster Recovery: Set up instant networks where infrastructure is down.
* Outdoor Activities: Hiking, camping, and adventure racing communication.
* Tactical & Security Applications: Military, law enforcement, and private security use cases.
* IoT & Sensor Networks: Collect data from remote sensors and relay it back to a central location.

## 🚀 How to Get Started

- Watch the [MeshCore QuickStart Playlist](https://www.youtube.com/watch?v=iaFltojJrAc&list=PLshzThxhw4O4WU_iZo3NmNZOv6KMrUuF9) by The Comms Channel
- Watch the [MeshCore Technical Presentation](https://www.youtube.com/watch?v=OwmkVkZQTf4) by Liam Cottle.
- Read through our [Frequently Asked Questions](./docs/faq.md) and [Documentation](https://docs.meshcore.io).
- Flash the MeshCore firmware on a supported device.
- Connect with a supported client.

For developers:

- Install [PlatformIO](https://docs.platformio.org) in [Visual Studio Code](https://code.visualstudio.com).
- Clone and open the MeshCore repository in Visual Studio Code.
- See the example applications you can modify and run:
  - [Companion Radio](./examples/companion_radio) - For use with an external chat app, over BLE, USB or Wi-Fi.
  - [KISS Modem](./examples/kiss_modem) - Serial KISS protocol bridge for host applications. ([protocol docs](./docs/kiss_modem_protocol.md))
  - [Simple Repeater](./examples/simple_repeater) - Extends network coverage by relaying messages.
  - [Simple Room Server](./examples/simple_room_server) - A simple BBS server for shared Posts.
  - [Simple Secure Chat](./examples/simple_secure_chat) - Secure terminal based text communication between devices.
  - [Simple Sensor](./examples/simple_sensor) - Remote sensor node with telemetry and alerting.

The Simple Secure Chat example can be interacted with through the Serial Monitor in Visual Studio Code, or with a Serial USB Terminal on Android.

## ⚡️ MeshCore Flasher

We have prebuilt firmware ready to flash on supported devices.

- Launch https://meshcore.io/flasher
- Select a supported device
- Flash one of the firmware types:
  - Companion, Repeater or Room Server
- Once flashing is complete, you can connect with one of the MeshCore clients below.

## 📱 MeshCore Clients

**Companion Firmware**

The companion firmware can be connected to via BLE, USB or Wi-Fi depending on the firmware type you flashed.

- Web: https://app.meshcore.nz
- Android: https://play.google.com/store/apps/details?id=com.liamcottle.meshcore.android
- iOS: https://apps.apple.com/us/app/meshcore/id6742354151?platform=iphone
- NodeJS: https://github.com/liamcottle/meshcore.js
- Python: https://github.com/fdlamotte/meshcore-cli

**Repeater and Room Server Firmware**

The repeater and room server firmware can be set up via USB in the web config tool.

- https://config.meshcore.io

They can also be managed via LoRa in the mobile app by using the Remote Management feature.

## 🛠 Hardware Compatibility

MeshCore is designed for devices listed in the [MeshCore Flasher](https://meshcore.io/flasher)

## 📜 License

MeshCore is open-source software released under the MIT License. You are free to use, modify, and distribute it for personal and commercial projects.

## Contributing

Please submit PR's using 'dev' as the base branch!
For minor changes just submit your PR and we'll try to review it, but for anything more 'impactful' please open an Issue first and start a discussion. It is better to sound out what it is you want to achieve first, and try to come to a consensus on what the best approach is, especially when it impacts the structure or architecture of this codebase.

Here are some general principles you should try to adhere to:
* Keep it simple. Please, don't think like a high-level lang programmer. Think embedded, and keep code concise, without any unnecessary layers.
* No dynamic memory allocation, except during setup/begin functions.
* Use the same brace and indenting style that's in the core source modules. (A .clang-format is probably going to be added soon, but please do NOT retroactively re-format existing code. This just creates unnecessary diffs that make finding problems harder)

Help us prioritize! Please react with thumbs-up to issues/PRs you care about most. We look at reaction counts when planning work.

### Running unit tests

To run unit tests, run the following command:

```bash
pio test --environment native --verbose
```

## Road-Map / To-Do

There are a number of fairly major features in the pipeline, with no particular time-frames attached yet. In very rough chronological order:
- [X] Companion radio: UI redesign
- [X] Repeater + Room Server: add ACL's (like Sensor Node has)
- [X] Standardise Bridge mode for repeaters
- [ ] Repeater/Bridge: Standardise the Transport Codes for zoning/filtering
- [X] Core + Repeater: enhanced zero-hop neighbour discovery
- [ ] Core: round-trip manual path support
- [ ] Companion + Apps: support for multiple sub-meshes (and 'off-grid' client repeat mode)
- [ ] Core + Apps: support for LZW message compression
- [ ] Core: dynamic CR (Coding Rate) for weak vs strong hops
- [ ] Core: new framework for hosting multiple virtual nodes on one physical device
- [ ] V2 protocol spec: discussion and consensus around V2 packet protocol, including path hashes, new encryption specs, etc

## 📞 Get Support

- Report bugs and request features on the [GitHub Issues](https://github.com/meshcore-dev/meshcore/issues) page.
- Find additional guides and components on [my site](https://buymeacoffee.com/ripplebiz).
- Join [MeshCore Discord](https://meshcore.gg) to chat with the developers and get help from the community.
