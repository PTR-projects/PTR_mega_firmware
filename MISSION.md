# Mission configurator

The flight-state list is compiled into the firmware. A mission file chooses which of the existing transitions are enabled, in what order they are tested, which comparisons must be true, and which effectors fire when a transition is taken.

The page is served by the rocket. It does not need an internet connection.

## Hardware

### Boot

`Preferences_mission_init()` runs after the `www` SPIFFS partition is mounted and preferences are loaded. It reads `/www/mission.cfg`.

The file is a little-endian CRC-16 of the JSON, then the JSON. A missing file, a bad CRC, a file that is too small or larger than the reader allows, or JSON the firmware rejects is discarded. The firmware then uses the compiled default.

The default copies two values from the settings file when that file loads:

- main-parachute altitude (`main_alt_m`, or 200 m)
- staging delay (`staging_delay_ms`, or 0)

After a mission file has been saved, later edits on the Settings tab do not change the mission. The mission file is the copy used in flight.

Effector outputs are registered once, when the effector task starts. A mission with at least one binding registers those bindings. A mission with an empty binding list uses the built-in igniter list: drogue on igniter channel 0, main on channel 1, second-stage igniter on channel 2. Each pulse is 100 ms. Builds with `CFG_CANSAT_ROCKET` also include the cansat SBUS bindings in that built-in list and in the compiled default.

### What stays in firmware

These actions are not comparisons and are not edited on the page:

- While disarmed, the detector holds `STARTUP` and does not test transitions. It keeps updating reference pressure, gyro calibration, and the maximum-altitude mark.
- `STARTUP` and `PREFLIGHT` update reference pressure, calibrate the gyro, and reset the maximum-altitude mark.
- Entering `BOOST` calls `AHRS_setInFlight()`.
- The transition into `BOOST` records the ignition time. Until that happens, time since launch is 0.

Effectors run when a transition is taken, before the state changes. Two ways into the same state can fire different effectors.

### Signals

The main loop samples these once per tick, then the state detector evaluates the current state.

| Name | Meaning | Unit |
| --- | --- | --- |
| `ascent_rate` | Vertical velocity from AHRS | m/s |
| `velocity_p` | Vertical velocity from pressure | m/s |
| `altitude` | Altitude | m |
| `altitude_p` | Pressure altitude | m |
| `alt_drop` | `max_altitude - altitude_p` | m |
| `acc_x` | Longitudinal acceleration, low-pass filtered | m/s² |
| `acc_y`, `acc_z` | Lateral acceleration in the rocket frame | m/s² |
| `gyro_x`, `gyro_y`, `gyro_z` | Primary LSM6DSO32 rotation rate | deg/s |
| `time_launch_ms` | Time since the transition into `BOOST` | ms |
| `time_state_ms` | Time since the current state was entered | ms |
| `tilt` | Tilt from vertical, from pitch and yaw | deg |
| `vbat` | Battery voltage | V |
| `pressure` | Barometer pressure | Pa |

Comparisons are `>`, `>=`, `<`, and `<=`.

### How a transition fires

Each state has a fixed set of outgoing transitions. On every tick the firmware walks the enabled transitions that leave the current state.

- Comparisons in one group must all be true (AND).
- A transition is true when any of its groups is true (OR).
- An enabled transition with no comparisons does not fire.
- If several transitions are true, the one with the lowest order wins. Equal order is broken by the fixed transition id, lowest first.

The transition then activates each selected effector id. Every physical binding registered under that id is commanded. The state changes, and the time-in-state counter starts again on the next pass through the new state.

Limits shared by the page and the firmware:

| Limit | Value |
| --- | --- |
| Groups per transition | 4 |
| Comparisons per group | 8 |
| Effector ids fired by one transition | 4 |
| Physical bindings per effector id | 4 |
| Binding rows in one mission | 32 |

### Effector bindings

A binding connects one logical id to one output.

| Field | Meaning |
| --- | --- |
| id | `DROGUE`, `MAIN`, `CANSAT_1`, `CANSAT_2`, `CANSAT_3`, `STAGE1_IGN`, `STAGE2_IGN`, `MOV`, `MFV` |
| type | `pwm` servo, `sbus` servo, or `igniter` |
| channel | PWM servo number, SBUS channel, or igniter channel |
| active | Value commanded when the transition fires. Page range is −100 to 100 |
| inactive | Value commanded when the timed pulse ends. Same range |
| ignore arm | Output follows the mission even when servos or igniters are disarmed |
| time | `0` holds the active value. Any other time is 10–10000 ms in steps of 10. The effector task then commands the inactive value |

### Fixed transitions

The page can enable, disable, and reorder these. It cannot add states or change where a transition goes.

| From | To | Default test | Effectors |
| --- | --- | --- | --- |
| `STARTUP` | `PREFLIGHT` | time in state ≥ 500 ms | |
| `PREFLIGHT` | `BOOST` | time in state ≥ 100 ms and `acc_x` ≥ 2.6 g | |
| `BOOST` | `SECOND_STAGE_DELAY` | time in state ≥ 500 ms and `acc_x` < 0 | |
| `SECOND_STAGE_DELAY` | `SECOND_STAGE_IGNITION` | time in state ≥ staging delay | `STAGE2_IGN` |
| `SECOND_STAGE_DELAY` | `FREEFALL` | time in state ≥ 200 ms and `alt_drop` > 10 m | |
| `SECOND_STAGE_IGNITION` | `SECOND_STAGE_BOOST` | time in state ≥ 100 ms and `acc_x` ≥ 2.6 g | |
| `SECOND_STAGE_IGNITION` | `FREEFLIGHT` | time in state ≥ 3000 ms | |
| `SECOND_STAGE_IGNITION` | `FREEFALL` | time in state ≥ 1000 ms and `alt_drop` > 10 m | |
| `SECOND_STAGE_BOOST` | `FREEFLIGHT` | time in state ≥ 200 ms and `acc_x` < 0 | |
| `FREEFLIGHT` | `FREEFALL` | time in state ≥ 200 ms and `alt_drop` > 10 m | |
| `FREEFALL` | `DRAGCHUTE_FALL` | time in state ≥ 0 | `DROGUE` |
| `DRAGCHUTE_FALL` | `MAINSHUTE_FALL` | time in state ≥ 100 ms and pressure altitude ≤ main altitude | `MAIN` |
| `DRAGCHUTE_FALL` | `DRAGCHUTE_FAILURE` | time in state ≥ 2000 ms and ascent rate < −60 m/s | |
| `DRAGCHUTE_FAILURE` | `MAINSHUTE_FALL` | time in state ≥ 100 ms | `DROGUE`, `MAIN` |
| `MAINSHUTE_FALL` | `RECOVERY_FAILURE` | time in state ≥ 5000 ms and ascent rate < −60 m/s | |
| `MAINSHUTE_FALL` | `LANDING` | time in state ≥ 30 s, pressure altitude < 200 m, and pressure velocity > −2 m/s | |
| `RECOVERY_FAILURE` | `LANDING` | same landing test | |
| `LANDING` | `SHUTDOWN` | time in state ≥ 30 s | |

Within each state, the default order is the order of this table. `HARD_ERROR` and `SOFT_ERROR` have no outgoing transitions.

On a state with more than one transition, only the first match is taken. For example, once the staging delay has elapsed, `SECOND_STAGE_DELAY` takes the ignition transition even if the freefall test is also true. Move the freefall transition above ignition on the page if apogee during the delay should win.

### Web API

`GET /mission` returns the current mission plus the name lists and the limits above.

`POST /mission` expects `crc,json`. The CRC is CRC-16-CCITT (polynomial `0x1021`, initial value `0xFFFF`), the same checksum the Settings tab uses. The JSON may contain `effectors` and `edges`. Edges are matched by their fixed `id`. The firmware checks the CRC, checks the limits, writes `/www/mission.cfg`, replies `ok`, and restarts. A rejected body gets HTTP 400 and the previous mission stays in memory and on disk.

## Frontend

Files live in `components/Web_driver/www_uncompressed/` and are copied into `www` by `helpers/minify_www.py` when the project is configured.

- `configurator.html` is the page.
- `configurator.js` loads the mission, edits it in the browser, and posts it back.
- `styles.css` contains the `.cfg` rules.
- `index.html` has a **Mission** link in the top menu.

The page has no libraries and does not load anything from the network. `GET /mission` fills the form. Edits stay in the browser until **Save and restart**.

The effectors section is one row per binding. The state section is the fixed state list. Choosing a state shows only the transitions that leave that state. Each transition has an enable checkbox, Up/Down buttons and a drag handle for order, checkboxes for the effector ids it fires, and comparison rows inside OR groups.

## User manual

1. Join the rocket Wi-Fi and open the web interface.
2. Choose **Mission**.
3. Edit bindings and transitions.
4. Press **Save and restart** and confirm.
5. Wait for the rocket to reboot, join its Wi-Fi again, and open **Mission** to confirm the saved values.

**Rejected.** means the device refused the file. The previous mission is unchanged. **No response. Reconnect if the device restarted.** means the link dropped during reboot; reconnect and check the page.

### Edit an output

Each row is one physical output.

- **id** is the logical name a transition can fire. Up to four outputs may share one id. Firing that id commands all of them.
- **type** is `pwm`, `sbus`, or `igniter`.
- **ch** is the hardware channel for that type.
- **active** and **inactive** are the commanded values, from −100 to 100.
- **ms** is how long the active value is held. `0` holds it until something else commands the output. Other values snap to a 10 ms step between 10 ms and 10 s.
- **ignore arm**, when checked, lets that output move while the rocket is disarmed.

**Add binding** appends a row on the first id that still has a free slot. **Remove** deletes that row. Removing every row and saving makes the next boot use the built-in igniter registration described above.

### Edit a transition

Select a state. Transitions are shown from first tested to last tested.

- The checkbox enables that transition.
- **Up** and **Down**, or dragging the `::` handle, change which match wins when several are true.
- **Fire** selects up to four logical ids. Those ids must also have bindings, or the command has no output.
- A row inside a group is one comparison: signal, comparator, threshold. Every row in the group must be true.
- **Add AND** adds a comparison to that group.
- **Add OR group** adds another group. The transition fires when any group is fully true.
- **Remove** deletes one comparison. Removing the last comparison in a group deletes the group.

A checked transition with no comparisons does not fire. Uncheck a transition to take it out of the flight without deleting its conditions.

States with no outgoing transitions, including `HARD_ERROR` and `SOFT_ERROR`, show “No outgoing transitions.”

### Example

To open the main parachute at 150 m of pressure altitude, select `DRAGCHUTE_FALL`, find the transition to `MAINSHUTE_FALL`, and set the `altitude_p <=` threshold to 150. Leave its order above the drogue-failure transition if a normal main deployment should win when both tests are true. Save and restart.
