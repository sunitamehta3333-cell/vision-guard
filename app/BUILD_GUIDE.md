# Vision Guard app — MIT App Inventor build guide

This guide reproduces the Vision Guard companion app exactly, block for block,
in **MIT App Inventor** (`ai2.appinventor.mit.edu`). Follow it top to bottom and
you will have a working app in about half an hour, no prior App Inventor
experience required.

> **Why a build guide and not a ready `.aia`?**
> An `.aia` is a packed binary of App Inventor's internal component and block
> files. Hand-authoring one that imports cleanly across App Inventor versions is
> unreliable — a single version mismatch makes the whole import fail silently.
> A guide always works, teaches you the app, and lets you adapt it. Build it
> once here and export your own `.aia` from **Projects → Export**.

---

## What the app does

The app is **for the family, not the blind user** — a relative uses it to
confirm the cane is working and to get help fast. Four things on one screen:

| Panel | What it shows |
|---|---|
| **Connection status** | Whether the phone is linked to the cane |
| **Alert status** | A full-width **CLEAR** (green) or **OBSTACLE** (red) panel |
| **Live distance** | The cane sensor's reading in cm, updating ~5×/second |
| **GPS + SOS** | Current location, and a big SOS button that texts it to family |

Plus a **Demo mode** switch so you can see the whole app work with no hardware.

---

## The data contract

The cane firmware (`cane/cane.ino`) sends one text line per reading over the
HC-05, terminated by a newline:

```
87,ALERT\n     <- distance in cm, comma, status, newline
250,OK\n
```

The app's only job on the sensor side is: receive one line, split it on the
comma, show the number, and colour the panel red on `ALERT` / green on `OK`.
Everything below implements exactly that.

---

## Bluetooth: HC-05 (Bluetooth Classic)

The cane uses an **HC-05**, a **Bluetooth Classic** (SPP / serial) module. App
Inventor talks to it with the **built-in `BluetoothClient` component** — no
extension needed.

Two important consequences of Classic Bluetooth:

1. **Pairing happens in Android's Bluetooth settings, not in the app.** Pair the
   phone with the HC-05 once (passcode `1234`), then the app connects to the
   already-paired device.
2. **You poll for data, you don't get pushed events.** The app reads incoming
   lines on a timer (a `Clock`), checking whether bytes are waiting. This same
   Clock also drives Demo mode.

> This app is **Android-only.** iPhones do not expose Bluetooth Classic serial
> to apps, and App Inventor builds Android apps only — both point the same way.

---

## Step 0 — new project

1. Sign in at **`ai2.appinventor.mit.edu`**.
2. **Projects → Start new project**, name it `VisionGuard`.

No extensions are required — `BluetoothClient` is built in (Palette →
**Connectivity**).

---

## Step 1 — Designer: components and properties

Drag these onto **Screen1** in order. Set `Scrollable = checked` and
`AlignHorizontal = Center : 3` on Screen1 first. Rename each component (the
Rename button under the component tree) to the **Name** column below — the
blocks refer to these names.

### Screen1

| Property | Value |
|---|---|
| Title | `Vision Guard` |
| AppName | `Vision Guard` |
| AlignHorizontal | Center : 3 |
| Icon | (optional) a logo image |

### Visible components (top to bottom)

**1. Title**
| Component | Name | Key properties |
|---|---|---|
| Label | `lblTitle` | Text `Vision Guard`, FontSize `28`, FontBold ✓ |

**2. Connection row** — a `HorizontalArrangement` named `rowConn`
(Width = Fill parent, AlignVertical = Center) containing:
| Component | Name | Key properties |
|---|---|---|
| Label | `lblConnCaption` | Text `Cane:` , FontBold ✓ |
| Label | `lblConn` | Text `Disconnected`, FontBold ✓, TextColor **Red** |

**3. Connect / disconnect buttons** — a `HorizontalArrangement` named
`rowConnBtns` (Width = Fill parent) containing:
| Component | Name | Key properties |
|---|---|---|
| ListPicker | `pickCane` | Text `Connect`, Width = Fill parent |
| Button | `btnDisconnect` | Text `Disconnect`, Enabled ✗ |

> A **ListPicker** (not a plain Button) is used to connect because it pops up
> the list of paired Bluetooth devices for the user to choose from.

**4. Alert status panel** — a `VerticalArrangement` named `pnlStatus`
| Property | Value |
|---|---|
| Width | Fill parent |
| Height | 180 pixels |
| AlignHorizontal | Center : 3 |
| AlignVertical | Center : 2 |
| BackgroundColor | Gray (changed at runtime) |

Inside it:
| Component | Name | Key properties |
|---|---|---|
| Label | `lblStatus` | Text `— — —`, FontSize `60`, FontBold ✓, TextColor **White** |

**5. Distance panel** — a `VerticalArrangement` named `pnlDist`
(Width = Fill parent, AlignHorizontal = Center) containing:
| Component | Name | Key properties |
|---|---|---|
| Label | `lblDistCaption` | Text `Live distance` |
| Label | `lblDistance` | Text `-- cm`, FontSize `44`, FontBold ✓ |

**6. Location panel** — a `VerticalArrangement` named `pnlLoc`
(Width = Fill parent, AlignHorizontal = Center) containing:
| Component | Name | Key properties |
|---|---|---|
| Label | `lblLocCaption` | Text `Location`, FontBold ✓ |
| Label | `lblLocation` | Text `Locating…` |

**7. SOS button**
| Component | Name | Key properties |
|---|---|---|
| Button | `btnSOS` | Text `SOS — send my location`, FontSize `26`, FontBold ✓, BackgroundColor **Red**, TextColor **White**, Height `90`, Width Fill parent |

**8. Settings** — a `VerticalArrangement` named `pnlSettings`
(Width = Fill parent) containing:
| Component | Name | Key properties |
|---|---|---|
| Label | (none) | Text `Family contact number` |
| TextBox | `txtContact` | Hint `e.g. +919812345678`, Width Fill parent |
| Button | `btnSaveContact` | Text `Save contact` |
| HorizontalArrangement | `rowDemo` | AlignVertical = Center, containing: Label `Demo mode (no hardware)` + a `Switch` named `swDemo` |

### Non-visible components

Drag these anywhere; they drop into the tray under the phone preview.

| Palette group | Component | Name | Notes |
|---|---|---|---|
| Connectivity | **BluetoothClient** | `BluetoothClient1` | the HC-05 link. Set `DelimiterByte = 10` (newline) |
| Sensors | LocationSensor | `LocationSensor1` | GPS. Set `TimeInterval = 5000` |
| Social | Texting | `Texting1` | sends the SOS SMS |
| Sensors | Clock | `Clock1` | polls Bluetooth + drives Demo mode. `TimerInterval = 200`, `TimerEnabled ✓` |
| Storage | TinyDB | `TinyDB1` | remembers the contact number |
| Media | TextToSpeech | `TextToSpeech1` | speaks alerts and confirmations |
| User Interface | Notifier | `Notifier1` | short messages / prompts |

> **`DelimiterByte = 10` matters.** The firmware ends each reading with
> `println` (a newline, ASCII 10). Setting the delimiter to 10 lets
> `ReceiveText` return exactly one whole line per read — no split-line glitches.

That is the whole Designer. Now switch to **Blocks** (top-right).

---

## Step 2 — Blocks

Each heading below is one event or procedure; drag the named blocks from the
left tray and snap them together as written. Built-in operators like `join`,
`split`, `contains`, and `random integer` live in the **Text**, **Lists**, and
**Math** drawers.

### Global variables

```
initialize global CONTACT_KEY to "contact"
initialize global wasAlert    to false      // for edge-triggered speech
```

### Screen1.Initialize

```
when Screen1.Initialize:
    // restore saved contact number
    set txtContact.Text to  TinyDB1.GetValue(tag = get global CONTACT_KEY,
                                             valueIfTagNotThere = "")
    call setDisconnectedUI
    set lblLocation.Text to "Locating…"
```

### Procedure setDisconnectedUI

```
to setDisconnectedUI:
    set lblConn.Text        to "Disconnected"
    set lblConn.TextColor   to  Red
    set btnDisconnect.Enabled to  false
    set lblDistance.Text    to "-- cm"
    set pnlStatus.BackgroundColor to  Gray
    set lblStatus.Text      to "— — —"
```

### Connecting — pick a paired device and connect

The ListPicker shows every device already paired in Android settings; the user
taps the HC-05.

```
when pickCane.BeforePicking:
    set pickCane.Elements to  BluetoothClient1.AddressesAndNames

when pickCane.AfterPicking:
    if  BluetoothClient1.Connect(address = pickCane.Selection)  then
        set lblConn.Text      to "Connected"
        set lblConn.TextColor to  Green
        set btnDisconnect.Enabled to  true
        set swDemo.On to  false          // real data wins over demo
        call TextToSpeech1.Speak  message = "Cane connected"
    else
        call Notifier1.ShowAlert  notice = "Could not connect. Is the cane on and paired?"
```

> `BluetoothClient1.Connect` returns **true/false** — that's why it sits inside
> an `if`. `AddressesAndNames` lists only *paired* devices, so pair the HC-05 in
> Android settings (passcode `1234`) before this will show it.

Disconnect:

```
when btnDisconnect.Click:
    call BluetoothClient1.Disconnect
    call setDisconnectedUI
    call TextToSpeech1.Speak  message = "Cane disconnected"
```

### Receiving data — poll on the Clock

Because Classic Bluetooth is polled, the `Clock1.Timer` (every 200 ms = 5×/sec)
checks for a waiting line and reads it. The same timer runs Demo mode when no
cane is connected.

```
when Clock1.Timer:
    // 1) real cane data, if connected
    if  BluetoothClient1.IsConnected  then
        if  BluetoothClient1.BytesAvailableToReceive > 0  then
            initialize local line to
                BluetoothClient1.ReceiveText(numberOfBytes = -1)
            call handleLine  text = get line

    // 2) otherwise, demo data if the switch is on
    else if  swDemo.On  then
        initialize local d to  random integer from 20 to 400
        initialize local s to  if d < 100 then "ALERT" else "OK"
        call handleLine  text = join(d, ",", s)
```

> `ReceiveText(-1)` with `DelimiterByte = 10` returns everything up to the next
> newline — exactly one reading. `BytesAvailableToReceive > 0` avoids blocking
> when nothing has arrived yet.

### Procedure handleLine — the heart of the app

Parses `"87,ALERT"` and updates the three sensor panels. Shared by the real
feed **and** Demo mode, so both look identical on screen.

```
to handleLine(text):
    // ignore blank or partial lines
    if  is empty(trim(text))  or  not contains(text, ",")  then
        return

    initialize local parts    to  split(text = trim(text), at = ",")
    initialize local distance to  select list item(parts, 1)
    initialize local status   to  upcase( trim( select list item(parts, 2) ) )

    set lblDistance.Text to  join(distance, " cm")

    if  status = "ALERT"  then
        set pnlStatus.BackgroundColor to  Red
        set lblStatus.Text to "OBSTACLE"
        // speak once, only on the CLEAR -> OBSTACLE edge, so it isn't a stutter
        if  not get global wasAlert  then
            call TextToSpeech1.Speak  message = "Obstacle"
        set global wasAlert to  true
    else
        set pnlStatus.BackgroundColor to  Green
        set lblStatus.Text to "CLEAR"
        set global wasAlert to  false
```

### Demo mode switch

The Clock is already running; the switch just tells `Clock1.Timer` (above) to
generate readings, and resets the UI when turned off.

```
when swDemo.Changed:
    if  swDemo.On  then
        set lblConn.Text to "Demo"
        set lblConn.TextColor to  Blue
    else
        call setDisconnectedUI
```

`TimerInterval = 200` ms gives 5 updates/second — the same rate the real cane
streams at, satisfying "updates several times a second."

### GPS

```
when LocationSensor1.LocationChanged:
    latitude, longitude, altitude, speed
    set lblLocation.Text to  join(
        "Lat ", round(latitude × 1000000) ÷ 1000000,
        "  Lng ", round(longitude × 1000000) ÷ 1000000 )
```

(The round-and-divide trims to ~6 decimal places for a tidy display.)

### SOS

```
when btnSOS.Click:
    if  is empty(txtContact.Text)  then
        call Notifier1.ShowAlert  notice = "Set a family contact number first."
        return

    if  not LocationSensor1.HasAccuracy  then
        call Notifier1.ShowAlert  notice = "No GPS fix yet — sending without a precise location."

    initialize local maps to  join(
        "https://maps.google.com/?q=",
        LocationSensor1.Latitude, ",", LocationSensor1.Longitude )

    set Texting1.PhoneNumber to  txtContact.Text
    set Texting1.Message to  join("SOS. I need help. My location: ", get maps)
    call Texting1.SendMessageDirect

    call TextToSpeech1.Speak  message = "Sending S O S with your location"
    call Notifier1.ShowAlert  notice = join("SOS sent to ", txtContact.Text)
```

> **`SendMessage` vs `SendMessageDirect`:** `SendMessageDirect` sends silently
> in the background (needs the `SEND_SMS` permission, granted at first use).
> `SendMessage` instead opens the phone's messaging app pre-filled and the user
> taps send — no special permission, but a manual step. For a true emergency
> button, `SendMessageDirect` is better; if you'd rather not request SMS
> permission, use `SendMessage`.

### Save the contact

```
when btnSaveContact.Click:
    call TinyDB1.StoreValue  tag = get global CONTACT_KEY,
                             valueToStore = txtContact.Text
    call Notifier1.ShowAlert  notice = "Contact saved."
```

That completes the blocks.

---

## Step 3 — permissions

App Inventor requests these at runtime the first time each is used. On the test
phone, **allow** them:

- **Location** (fine) — for GPS and the SOS map link.
- **Nearby devices / Bluetooth** — Android 12+ needs Bluetooth connect
  permission for `BluetoothClient`.
- **SMS** — only if you used `SendMessageDirect`.

---

## Step 4 — test it

**Without hardware (do this first):**
1. Install **MIT AI2 Companion** on an Android phone, open your project,
   **Connect → AI Companion**, scan the QR.
2. Flip **Demo mode** on. The status panel should flip between green **CLEAR**
   and red **OBSTACLE**, the distance number should change ~5×/sec, and you
   should hear "Obstacle" on each new obstacle.
3. Type a phone number you own, **Save contact**, then tap **SOS**. Confirm the
   text arrives with a working Google Maps link.

**With the cane:**
1. Flash `cane/cane.ino` to the Arduino (HC-05 wired per the README).
2. **Pair once:** on the phone, Android **Settings → Bluetooth**, pair the
   HC-05 (name often `HC-05`), passcode **`1234`**.
3. Power the cane. In the app, tap **Connect**, choose the HC-05 from the list.
4. `lblConn` turns green **Connected**. Wave your hand in front of the sensor —
   the panel flips to **OBSTACLE** and the distance tracks your hand.

---

## Step 5 — export your `.aia`

**Projects → Export selected project (.aia) to my computer.** Commit that file
to the repo at `app/VisionGuard.aia` so others can import it directly
(**Projects → Import project (.aia)**) instead of rebuilding from this guide.

---

## Accessibility notes

Although the app is aimed at a sighted family member, it's built to be usable
by the blind user too:

- **TextToSpeech** narrates connection state and obstacle alerts, so the app is
  useful without looking at it.
- **Large, high-contrast** status text (60 pt) and a full-width colour panel
  read at a glance and for low-vision users.
- The **SOS button** is oversized and bottom-placed for thumb reach without
  aiming.

If you extend the app, keep colour **and** words/speech together — never signal
CLEAR/OBSTACLE by colour alone.
