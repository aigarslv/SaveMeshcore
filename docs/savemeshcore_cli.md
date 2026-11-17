# SaveMeshcore repeater CLI reference

This page documents the commands and limits SaveMeshcore adds to MeshCore repeater
firmware. Everything else is in the upstream [CLI Commands](./cli_commands.md)
reference, which this fork leaves unchanged so official releases merge cleanly.
Where the two disagree, this page applies to SaveMeshcore repeaters.

## Enforced repeater settings

At every boot, out-of-policy values are repaired and saved before the radio and
advert timers start. CLI values outside these limits are rejected without
changing anything.

| Setting | Allowed | Default | Boot repair |
| --- | --- | --- | --- |
| `path.hash.mode` | `1` (2-byte) or `2` (3-byte) | `1` | `0` (1-byte) becomes `1`; `2` is kept |
| `multi.acks` | `1` | `1` | any other value becomes `1` |
| `loop.detect` | `minimal`, `moderate`, `strict` | `minimal` | `off` becomes `minimal` |
| `advert.interval` | `0` | `0` | any non-zero value becomes `0` |
| `flood.advert.interval` | `0`, or `47`-`168` hours | `47` | `1`-`46` becomes `47`; `0` is kept |

These settings keep their upstream meaning:

- `path.hash.mode` sets the path hash size the repeater uses for its own adverts.
  It does not change which hash sizes it forwards.
- `multi.acks 1` sends one extra multi-ACK in addition to the normal ACK.
- `loop.detect` thresholds are described in the upstream reference.

## Packet filters

#### View all repeater filters
**Usage:**
- `filter`

**Example reply:** `> 1byte off, data on, public off`

**Note:** Shows the state of every SaveMeshcore filter in one reply, so all of them can be checked with a single remote request.

---

#### Filter flood packets with 1-byte path hashes
**Usage:**
- `filter 1byte`
- `filter 1byte on`
- `filter 1byte off`

**Default:** `off`

**Note:** When `on`, the repeater does not forward flood packets that use 1-byte path hashes and have already traversed at least one hop. Flood packets with 2-byte or 3-byte path hashes, zero-hop packets (no path yet), and direct-routed packets are still forwarded.

---

#### Filter data payload types
**Usage:**
- `filter data`
- `filter data on`
- `filter data off`

**Default:** `on`

**Note:** When `on`, the repeater does not forward packets with payload type 0x06 (GRP_DATA), 0x0C to 0x0E (unsigned), or 0x0F (RAW_CUSTOM).

---

#### Filter the Public channel
**Usage:**
- `filter public`
- `filter public on`
- `filter public off`

**Default:** `off`

**Note:** When `on`, the repeater does not forward group messages (GRP_TXT and GRP_DATA) on the default Public channel. The repeater checks the packet's MAC with the published Public channel key without decrypting the message. MeshCore uses a 16-bit channel MAC, so a non-Public channel that also uses hash byte `0x11` has a 1-in-65,536 false-match probability per candidate packet.

On/off arguments are case-insensitive and ignore trailing spaces, so
`filter data ON` and `filter data on ` both work.
