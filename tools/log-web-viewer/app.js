/** ESP-IDF line parsing and tag catalog for the PTR serial log viewer. */

const RAW_ID = "__raw__";

const GROUPS = [
  {
    id: "analog",
    label: "Analog",
    tags: [
      { id: "Analog", color: "#f5b942" },
      { id: "temperature_sensor", color: "#e08a3c" },
    ],
  },
  {
    id: "storage",
    label: "Storage",
    tags: [
      { id: "SPIFLASH", color: "#2ec4b6" },
      { id: "SFS", color: "#5eead4" },
      { id: "Data ag.", color: "#14b8a6", aliases: ["Data_mgr"] },
      { id: "SD_driver", color: "#7dd3c7" },
    ],
  },
  {
    id: "gnss",
    label: "GNSS",
    tags: [
      { id: "GNSS", color: "#60a5fa" },
      { id: "GNSS-UBX", color: "#38bdf8" },
    ],
  },
  {
    id: "lora",
    label: "Lora",
    tags: [
      { id: "LORA driver", color: "#c084fc", aliases: ["LORA_driver"] },
      { id: "PTR_DataPacket", color: "#e879f9" },
      { id: "TMTC", color: "#a78bfa" },
      { id: "sx126x driver", color: "#818cf8" },
    ],
  },
  {
    id: "sensors",
    label: "Sensors",
    tags: [
      { id: "Sensors", color: "#4ade80" },
      { id: "MS5607", color: "#86efac" },
      { id: "MMC5983MA", color: "#3ddc84" },
      { id: "LIS331", color: "#a3e635" },
      { id: "LSM6DSO32", color: "#bef264", aliases: ["LSM6DO32"] },
      { id: "BMP581", color: "#6ee7b7" },
    ],
  },
  {
    id: "effectors",
    label: "Effectors",
    tags: [
      { id: "Effector", color: "#fb7185", aliases: ["Effectro"] },
      { id: "IGN_driver", color: "#ff6b81" },
      { id: "Servo_driver", color: "#fb923c" },
      { id: "SBUS", color: "#fdba74" },
    ],
  },
  {
    id: "wifi",
    label: "WiFi",
    tags: [
      { id: "WIFI", color: "#22d3ee" },
      { id: "wifi", color: "#93c5fd" },
      { id: "wifi_init", color: "#bae6fd" },
      { id: "esp_netif_lwip", color: "#c7d2fe" },
      { id: "Web_driver", color: "#67e8f9" },
      { id: "Web_driver_json", color: "#a5f3fc" },
      { id: "Web_driver_cmd", color: "#7dd3fc" },
    ],
  },
];

const FILTER_OF = new Map();
const COLOR_OF = new Map();

for (const group of GROUPS) {
  for (const tag of group.tags) {
    FILTER_OF.set(tag.id, tag.id);
    COLOR_OF.set(tag.id, tag.color);
    for (const alias of tag.aliases || []) FILTER_OF.set(alias, tag.id);
  }
}

COLOR_OF.set(RAW_ID, "#a8b0bd");

const LINE_RE = /^([EWIDV]) \((\d+)\) ([^:]+): ?(.*)$/;
const ANSI_RE = /\x1b\[[0-9;?]*[ -/]*[@-~]/g;
const YAT_ESC_RE = /<ESC>\[[0-9;]*m/gi;

function cleanLine(raw) {
  return raw
    .replace(YAT_ESC_RE, "")
    .replace(ANSI_RE, "")
    .replace(/<CR>$/i, "")
    .replace(/\r$/, "");
}

/** Map a wire tag onto a checkbox id. Unknown tags stay themselves. */
function filterIdFor(tag) {
  if (!tag) return RAW_ID;
  return FILTER_OF.get(tag) || tag;
}

function isCatalogId(id) {
  return COLOR_OF.has(id);
}

function colorFor(id) {
  return COLOR_OF.get(id) || hashColor(id);
}

function aliasTitle(tag) {
  if (!tag.aliases?.length) return "";
  return `Also matches: ${tag.aliases.join(", ")}`;
}

/**
 * Parse one captured line.
 * Returns null for a blank line.
 * Non ESP-IDF text is kept as an untagged record.
 */
function parseLine(raw) {
  const text = cleanLine(raw);
  if (text.length === 0) return null;
  const match = LINE_RE.exec(text);
  if (!match) {
    return {
      level: "",
      ts: "",
      tag: "",
      text,
      filterId: RAW_ID,
      raw: text,
    };
  }
  const tag = match[3].trim();
  return {
    level: match[1],
    ts: match[2],
    tag,
    text: match[4],
    filterId: filterIdFor(tag),
    raw: text,
  };
}

/** A line is shown when its tag checkbox is on. Callers OR the checkboxes. */
function isShown(entry, enabled) {
  return enabled(entry.filterId);
}

function hashColor(id) {
  let hash = 0;
  for (let i = 0; i < id.length; i++) hash = (hash * 31 + id.charCodeAt(i)) >>> 0;
  const hue = hash % 360;
  return `hsl(${hue} 62% 66%)`;
}

const MAX_RENDER = 4000;
const STORE_KEY = "ptr-log-filters";

const baudSel = document.getElementById("baud");
const connectBtn = document.getElementById("connect");
const openBtn = document.getElementById("open");
const fileInput = document.getElementById("file");
const followBtn = document.getElementById("follow");
const saveBtn = document.getElementById("save");
const clearBtn = document.getElementById("clear");
const statusEl = document.getElementById("status");
const sidebar = document.getElementById("sidebar");
const banner = document.getElementById("banner");
const logEl = document.getElementById("log");
const emptyEl = document.getElementById("empty");
const jumpBtn = document.getElementById("jump");

const serialOk = "serial" in navigator && window.isSecureContext;
const openedAsFile = location.protocol === "file:";

const captured = [];
const counts = new Map();
const boxes = new Map();
const countEls = new Map();
const disabled = new Set();
const groupState = new Map();

let shown = 0;
let follow = true;
let suppressNew = false;
let clearArm = 0;
let pendingNew = 0;
let epoch = 0;
let scheduled = false;
let batch = [];
let pinning = false;
let lastTop = 0;
let statusTimer = 0;

let port = null;
let reader = null;
let closing = false;
let pendingSerial = "";

const saved = loadSaved();
if (saved) {
  for (const id of saved.disabled || []) disabled.add(id);
  suppressNew = !!saved.suppressNew;
  if (saved.baud && [...baudSel.options].some((o) => o.value === String(saved.baud))) {
    baudSel.value = String(saved.baud);
  }
  if (typeof saved.follow === "boolean") follow = saved.follow;
}

buildSidebar();
syncFollowButton();
setStatus(serialOk
  ? "Not connected."
  : openedAsFile
    ? "Opened as a file, so Connect is blocked. Double-click Open viewer.bat in this folder. Open log still works here."
    : "WebSerial needs Chrome or Edge on http://localhost. Opening a saved log still works.");
updateEmpty();
updateStats();

connectBtn.disabled = !serialOk;
connectBtn.addEventListener("click", () => {
  if (port) disconnect();
  else connect();
});
openBtn.addEventListener("click", () => fileInput.click());
fileInput.addEventListener("change", () => {
  const file = fileInput.files && fileInput.files[0];
  fileInput.value = "";
  if (file) readFile(file);
});
followBtn.addEventListener("click", () => {
  follow = !follow;
  syncFollowButton();
  persist();
  if (follow) rebuild();
});
saveBtn.addEventListener("click", saveAll);
clearBtn.addEventListener("click", onClear);
jumpBtn.addEventListener("click", () => {
  follow = true;
  syncFollowButton();
  persist();
  rebuild();
});

logEl.addEventListener("scroll", () => {
  const top = logEl.scrollTop;
  const movedUp = top < lastTop - 2;
  lastTop = top;
  if (pinning) return;
  if (movedUp && follow) {
    follow = false;
    syncFollowButton();
  }
}, { passive: true });

window.addEventListener("dragover", (event) => event.preventDefault());
window.addEventListener("drop", (event) => {
  event.preventDefault();
  const file = event.dataTransfer && event.dataTransfer.files && event.dataTransfer.files[0];
  if (file) readFile(file);
});

function buildSidebar() {
  const head = document.createElement("div");
  head.className = "side-head";
  const title = document.createElement("strong");
  title.textContent = "Tags";
  const meta = document.createElement("span");
  meta.id = "tag-meta";
  head.append(title, meta);

  const actions = document.createElement("div");
  actions.className = "side-actions";
  const allBtn = document.createElement("button");
  allBtn.type = "button";
  allBtn.textContent = "All";
  const noneBtn = document.createElement("button");
  noneBtn.type = "button";
  noneBtn.textContent = "None";
  allBtn.addEventListener("click", selectAll);
  noneBtn.addEventListener("click", selectNone);
  actions.append(allBtn, noneBtn);

  const hint = document.createElement("p");
  hint.className = "hint";
  hint.textContent = "A line is shown when its tag is checked. Checks are OR. Unchecked lines stay in the capture.";

  sidebar.append(head, actions, hint);

  for (const group of GROUPS) addGroup(group, group.tags);

  const other = addGroup({ id: "other", label: "Other" }, []);
  addTagRow(other, {
    id: RAW_ID,
    label: "Untagged",
    color: colorFor(RAW_ID),
    title: "Bootloader and other lines without an ESP-IDF tag",
  });
  syncGroup(other);
}

function addGroup(group, tags) {
  const section = document.createElement("section");
  section.className = "group";

  const head = document.createElement("div");
  head.className = "group-head";
  const box = document.createElement("input");
  box.type = "checkbox";
  box.id = `group-${group.id}`;
  box.checked = true;
  box.setAttribute("aria-label", group.label);
  const name = document.createElement("label");
  name.htmlFor = box.id;
  name.textContent = group.label;
  const twist = document.createElement("button");
  twist.type = "button";
  twist.className = "twist";
  twist.textContent = "▾";
  twist.setAttribute("aria-label", `Collapse ${group.label}`);
  twist.setAttribute("aria-expanded", "true");
  twist.addEventListener("click", (event) => {
    event.preventDefault();
    event.stopPropagation();
    const closed = list.classList.toggle("collapsed");
    twist.textContent = closed ? "▸" : "▾";
    twist.setAttribute("aria-expanded", closed ? "false" : "true");
  });
  head.append(box, name, twist);

  const list = document.createElement("div");
  list.className = "tags";
  section.append(head, list);
  sidebar.append(section);

  const state = { id: group.id, box, list, ids: [] };
  groupState.set(group.id, state);

  box.addEventListener("change", () => {
    const on = box.checked;
    box.indeterminate = false;
    for (const id of state.ids) setEnabled(id, on, false);
    persist();
    recountShown();
    rebuild();
  });

  for (const tag of tags) {
    addTagRow(state, {
      id: tag.id,
      label: tag.id,
      color: tag.color,
      title: aliasTitle(tag),
    });
  }
  syncGroup(state);
  return state;
}

function addTagRow(state, spec) {
  if (boxes.has(spec.id)) return;
  const label = document.createElement("label");
  label.className = "tag";
  label.dataset.tag = spec.id;
  if (spec.title) label.title = spec.title;

  const box = document.createElement("input");
  box.type = "checkbox";
  const on = !disabled.has(spec.id);
  box.checked = on;
  if (!on) disabled.add(spec.id);

  const swatch = document.createElement("i");
  swatch.className = "swatch";
  swatch.style.background = spec.color;

  const name = document.createElement("span");
  name.className = "name";
  name.textContent = spec.label;

  const count = document.createElement("span");
  count.className = "count";
  count.textContent = String(counts.get(spec.id) || 0);

  label.append(box, swatch, name, count);
  state.list.append(label);
  state.ids.push(spec.id);
  boxes.set(spec.id, box);
  countEls.set(spec.id, count);

  box.addEventListener("change", () => {
    setEnabled(spec.id, box.checked, false);
    syncGroup(state);
    persist();
    recountShown();
    rebuild();
  });
}

function ensureDynamic(tag) {
  if (!tag || isCatalogId(tag) || boxes.has(tag)) return;
  if (suppressNew) disabled.add(tag);
  const other = groupState.get("other");
  addTagRow(other, { id: tag, label: tag, color: colorFor(tag), title: "" });
  syncGroup(other);
}

function setEnabled(id, on, sync) {
  if (on) disabled.delete(id);
  else disabled.add(id);
  const box = boxes.get(id);
  if (box) box.checked = on;
  if (sync) {
    for (const state of groupState.values()) {
      if (state.ids.includes(id)) syncGroup(state);
    }
  }
}

function syncGroup(state) {
  if (state.ids.length === 0) {
    state.box.checked = false;
    state.box.indeterminate = false;
    return;
  }
  let on = 0;
  for (const id of state.ids) if (!disabled.has(id)) on++;
  state.box.checked = on === state.ids.length;
  state.box.indeterminate = on > 0 && on < state.ids.length;
}

function selectAll() {
  suppressNew = false;
  disabled.clear();
  for (const box of boxes.values()) box.checked = true;
  for (const state of groupState.values()) syncGroup(state);
  persist();
  recountShown();
  rebuild();
}

function selectNone() {
  suppressNew = true;
  for (const id of boxes.keys()) disabled.add(id);
  for (const box of boxes.values()) box.checked = false;
  for (const state of groupState.values()) syncGroup(state);
  persist();
  recountShown();
  rebuild();
}

function enabled(id) {
  return !disabled.has(id);
}

function ingest(line, bulk) {
  const entry = parseLine(line);
  if (!entry) return;
  if (entry.filterId !== RAW_ID && !isCatalogId(entry.filterId)) ensureDynamic(entry.filterId);
  captured.push(entry);
  bump(entry.filterId);
  if (enabled(entry.filterId)) shown++;
  if (bulk) return;
  batch.push(entry);
  schedule();
}

function ingestText(text) {
  if (text.charCodeAt(0) === 0xfeff) text = text.slice(1);
  const lines = text.split(/\r?\n/);
  for (const line of lines) ingest(line, true);
  rebuild();
}

function bump(id) {
  const next = (counts.get(id) || 0) + 1;
  counts.set(id, next);
  const el = countEls.get(id);
  if (el) el.textContent = String(next);
}

function schedule() {
  if (scheduled) return;
  scheduled = true;
  const my = epoch;
  requestAnimationFrame(() => {
    scheduled = false;
    if (my !== epoch) {
      batch = [];
      return;
    }
    flush();
  });
}

function flush() {
  const items = batch;
  batch = [];
  if (!follow) {
    let fresh = 0;
    for (const entry of items) if (enabled(entry.filterId)) fresh++;
    if (fresh) {
      pendingNew += fresh;
      jumpBtn.hidden = false;
      jumpBtn.textContent = `${pendingNew} new`;
    }
    updateBanner();
    updateEmpty();
    updateStats();
    return;
  }
  const frag = document.createDocumentFragment();
  let added = 0;
  for (const entry of items) {
    if (!enabled(entry.filterId)) continue;
    frag.append(renderRow(entry));
    added++;
  }
  if (added) {
    logEl.append(frag);
    while (logEl.childElementCount > MAX_RENDER) logEl.removeChild(logEl.firstChild);
    pin();
  }
  updateBanner();
  updateEmpty();
  updateStats();
}

function rebuild() {
  epoch++;
  batch = [];
  const rows = [];
  for (let i = captured.length - 1; i >= 0 && rows.length < MAX_RENDER; i--) {
    if (enabled(captured[i].filterId)) rows.push(captured[i]);
  }
  rows.reverse();
  const frag = document.createDocumentFragment();
  for (const entry of rows) frag.append(renderRow(entry));
  logEl.replaceChildren(frag);
  pendingNew = 0;
  jumpBtn.hidden = true;
  if (follow) pin();
  updateBanner();
  updateEmpty();
  updateStats();
}

function renderRow(entry) {
  const row = document.createElement("div");
  row.className = "row" + (entry.level ? ` lvl-${entry.level}` : "");
  row.style.setProperty("--c", colorFor(entry.filterId));
  row.dataset.tag = entry.tag || "";
  row.dataset.filter = entry.filterId;

  const level = document.createElement("span");
  level.className = "lv";
  level.textContent = entry.level || "·";

  const time = document.createElement("span");
  time.className = "ts";
  time.textContent = entry.ts;

  const tag = document.createElement("span");
  tag.className = "tagname";
  tag.textContent = entry.tag || "untagged";

  const msg = document.createElement("span");
  msg.className = "msg";
  msg.textContent = entry.tag ? entry.text : entry.raw;

  row.append(level, time, tag, msg);
  return row;
}

function pin() {
  pinning = true;
  logEl.scrollTop = logEl.scrollHeight;
  lastTop = logEl.scrollTop;
  pinning = false;
}

function recountShown() {
  shown = 0;
  for (const entry of captured) if (enabled(entry.filterId)) shown++;
}

function updateBanner() {
  const rendered = logEl.childElementCount;
  if (shown > rendered) {
    banner.hidden = false;
    banner.textContent = `Showing the latest ${rendered} of ${shown} matching lines. The capture still holds all ${captured.length} lines.`;
  } else {
    banner.hidden = true;
  }
}

function updateEmpty() {
  if (captured.length === 0) {
    emptyEl.hidden = false;
    emptyEl.textContent = serialOk
      ? "Connect the board, or drop a log file here. Every line is stored. Only checked tags are drawn, and a line is shown if any checked tag matches."
      : openedAsFile
        ? "Connect is blocked on a file:// page. Double-click Open viewer.bat, then use Connect. You can still drop a log file here."
        : "This browser cannot open a serial port. Drop a log file here, or reopen this page from http://localhost in Chrome or Edge.";
  } else if (logEl.childElementCount === 0) {
    emptyEl.hidden = false;
    emptyEl.textContent = "No captured lines match the checked tags.";
  } else {
    emptyEl.hidden = true;
  }
}

function updateStats() {
  const on = [...boxes.keys()].filter((id) => enabled(id)).length;
  const meta = document.getElementById("tag-meta");
  if (meta) meta.textContent = `${on} on`;
  saveBtn.disabled = captured.length === 0;
  const link = connectionLabel();
  const countsText = `Captured ${captured.length} · shown ${shown}`;
  if (!statusEl.dataset.sticky) statusEl.textContent = `${link} · ${countsText}`;
}

function connectionLabel() {
  if (!port) return "Not connected";
  return `Connected ${portLabel(port)} @ ${baudSel.value}`;
}

function setStatus(text, bad) {
  statusEl.dataset.sticky = "1";
  statusEl.textContent = text;
  statusEl.classList.toggle("bad", !!bad);
  clearTimeout(statusTimer);
  statusTimer = setTimeout(() => {
    delete statusEl.dataset.sticky;
    statusEl.classList.remove("bad");
    updateStats();
  }, 4000);
}

function syncFollowButton() {
  followBtn.classList.toggle("on", follow);
  followBtn.setAttribute("aria-pressed", follow ? "true" : "false");
  followBtn.textContent = follow ? "Following" : "Follow";
}

function onClear() {
  if (captured.length === 0) return;
  const now = Date.now();
  if (now - clearArm > 2500) {
    clearArm = now;
    setStatus("Click Clear again to drop the capture.");
    return;
  }
  clearArm = 0;
  captured.length = 0;
  counts.clear();
  shown = 0;
  pendingNew = 0;
  for (const el of countEls.values()) el.textContent = "0";
  rebuild();
  setStatus("Capture cleared.");
}

function saveAll() {
  if (captured.length === 0) return;
  const text = captured.map((entry) => entry.raw).join("\n") + "\n";
  const blob = new Blob([text], { type: "text/plain;charset=utf-8" });
  const link = document.createElement("a");
  const stamp = stampNow();
  link.href = URL.createObjectURL(blob);
  link.download = `ptr-log-${stamp}.txt`;
  link.click();
  URL.revokeObjectURL(link.href);
}

async function readFile(file) {
  try {
    const text = await file.text();
    ingestText(text);
    setStatus(`Opened ${file.name}`);
  } catch (err) {
    setStatus(err.message || "Could not read that file.", true);
  }
}

async function connect() {
  if (!serialOk) return;
  let selected;
  try {
    selected = await navigator.serial.requestPort();
  } catch (err) {
    if (err && err.name === "NotFoundError") return;
    setStatus(err.message || "Port selection failed.", true);
    return;
  }
  try {
    await selected.open({
      baudRate: Number(baudSel.value),
      dataBits: 8,
      stopBits: 1,
      parity: "none",
      flowControl: "none",
      bufferSize: 65536,
    });
  } catch (err) {
    setStatus(err.message || "Could not open the port.", true);
    return;
  }
  port = selected;
  closing = false;
  pendingSerial = "";
  connectBtn.textContent = "Disconnect";
  connectBtn.classList.add("live");
  baudSel.disabled = true;
  setStatus(`Connected ${portLabel(port)} @ ${baudSel.value}`);
  void startRead();
}

async function startRead() {
  const decoder = new TextDecoder();
  try {
    reader = port.readable.getReader();
    while (true) {
      const { value, done } = await reader.read();
      if (done) break;
      if (value) pushSerial(decoder.decode(value, { stream: true }));
    }
    pushSerial(decoder.decode());
  } catch (err) {
    if (!closing) setStatus(err.message || "Serial read failed.", true);
  } finally {
    try { reader && reader.releaseLock(); } catch { /* already released */ }
    reader = null;
    if (!closing) {
      try { await port.close(); } catch { /* unplugged */ }
      port = null;
      baudSel.disabled = false;
      connectBtn.textContent = "Connect";
      connectBtn.classList.remove("live");
      flushPartial();
      setStatus("Disconnected.");
    }
  }
}

function pushSerial(chunk) {
  pendingSerial += chunk;
  const parts = pendingSerial.split(/\r\n|\n|\r/);
  pendingSerial = parts.pop();
  for (const line of parts) ingest(line, false);
}

function flushPartial() {
  if (!pendingSerial) return;
  const rest = pendingSerial;
  pendingSerial = "";
  ingest(rest, false);
}

async function disconnect() {
  closing = true;
  try { await reader?.cancel(); } catch { /* closing */ }
  try { await port?.close(); } catch { /* closing */ }
  reader = null;
  port = null;
  flushPartial();
  baudSel.disabled = false;
  connectBtn.textContent = "Connect";
  connectBtn.classList.remove("live");
  setStatus("Disconnected.");
}

function portLabel(current) {
  const info = current.getInfo ? current.getInfo() : {};
  if (info.usbVendorId == null) return "serial";
  const hex = (value) => value.toString(16).padStart(4, "0");
  return `USB ${hex(info.usbVendorId)}:${hex(info.usbProductId)}`;
}

function persist() {
  const data = {
    disabled: [...disabled],
    suppressNew,
    baud: Number(baudSel.value),
    follow,
  };
  localStorage.setItem(STORE_KEY, JSON.stringify(data));
}

function loadSaved() {
  try {
    return JSON.parse(localStorage.getItem(STORE_KEY) || "null");
  } catch {
    return null;
  }
}

function stampNow() {
  const now = new Date();
  const p = (n) => String(n).padStart(2, "0");
  return `${now.getFullYear()}${p(now.getMonth() + 1)}${p(now.getDate())}-${p(now.getHours())}${p(now.getMinutes())}${p(now.getSeconds())}`;
}

window.ptrLog = { ingestText };

baudSel.addEventListener("change", persist);
