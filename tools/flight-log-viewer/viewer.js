/* Flight-log viewer for the meas.csv export.
   sys_time in current firmware is time_us/100 (0.1 ms ticks). */

const FLIGHT_STATE = {
  0: "Startup",
  1: "Preflight",
  2: "Boost",
  3: "Stage delay",
  4: "Stage ignition",
  5: "Stage boost",
  6: "Freeflight",
  7: "Freefall",
  8: "Drogue",
  9: "Drogue failure",
  10: "Main",
  11: "Recovery failure",
  12: "Landing",
  13: "Hard error",
  14: "Soft error",
  15: "Shutdown"
};

const CANSAT_STATE = {
  0: "Idle",
  1: "Armed",
  2: "Batch 1",
  3: "Batch 2",
  4: "Batch 3",
  5: "Deployed",
  6: "Manual"
};

const STATE_COLOR = {
  0: "#6d7688",
  1: "#8b95a8",
  2: "#e0a15c",
  3: "#e0a15c",
  4: "#c9844a",
  5: "#e0a15c",
  6: "#7eb6ff",
  7: "#7eb6ff",
  8: "#9ad7b0",
  9: "#ff8b9a",
  10: "#3d8f72",
  11: "#ff8b9a",
  12: "#5c6578",
  13: "#ff8b9a",
  14: "#e0a15c",
  15: "#3d4554"
};

const CHARTS = [
  {
    id: "alt",
    title: "Altitude",
    unit: "m",
    ticks: "10",
    height: 250,
    series: [
      { key: "altitude_press", name: "Pressure", color: "#e0a15c" },
      { key: "altitude_kalman", name: "Kalman", color: "#7eb6ff" },
      { key: "altitude_gnss", name: "GNSS", color: "#9ad7b0" }
    ]
  },
  {
    id: "vz",
    title: "Vertical speed",
    unit: "m/s",
    ticks: "10",
    height: 190,
    zero: true,
    series: [
      { key: "ascent_rate_kalman", name: "Kalman", color: "#e0a15c" },
      { key: "vz_press", name: "From pressure", color: "#8b95a8" }
    ]
  },
  {
    id: "acc",
    title: "Acceleration",
    unit: "g",
    height: 190,
    zero: true,
    series: [
      { key: "accX", name: "X", color: "#e0a15c" },
      { key: "accY", name: "Y", color: "#7eb6ff" },
      { key: "accZ", name: "Z", color: "#9ad7b0" }
    ]
  },
  {
    id: "gyro",
    title: "Rotation",
    unit: "deg/s",
    height: 180,
    zero: true,
    series: [
      { key: "gyroX", name: "X", color: "#e0a15c" },
      { key: "gyroY", name: "Y", color: "#7eb6ff" },
      { key: "gyroZ", name: "Z", color: "#9ad7b0" }
    ]
  },
  {
    id: "vbat",
    title: "Battery",
    unit: "V",
    height: 140,
    series: [
      { key: "vbat_V", name: "Battery", color: "#e0a15c" }
    ]
  }
];

const view = {
  rows: [],
  t: [],
  t0: 0,
  t1: 1,
  view0: 0,
  view1: 1,
  cursor: 0,
  liftoff: 0,
  liftoffFound: false,
  div: 10000,
  note: "",
  rateHz: 0,
  cansat: false,
  segments: [],
  events: [],
  name: ""
};

const shown = {};
CHARTS.forEach(function (chart) {
  chart.series.forEach(function (series) { shown[chart.id + ":" + series.key] = true; });
});

const chartsEl = document.getElementById("charts");
const statusEl = document.getElementById("status");
const statsEl = document.getElementById("stats");
const layoutEl = document.getElementById("layout");
const dropEl = document.getElementById("drop");
const readoutEl = document.getElementById("readout");
const eventsEl = document.getElementById("events");
const trackEl = document.getElementById("track");
const fileNameEl = document.getElementById("file-name");
const resetBtn = document.getElementById("reset");
const saveBtn = document.getElementById("save");

document.getElementById("open").addEventListener("click", function () {
  document.getElementById("file").click();
});
document.getElementById("file").addEventListener("change", function (event) {
  const file = event.target.files && event.target.files[0];
  if (file) readFile(file);
  event.target.value = "";
});
resetBtn.addEventListener("click", function () {
  view.view0 = view.t0;
  view.view1 = view.t1;
  drawAll();
});
saveBtn.addEventListener("click", saveReport);

["dragenter", "dragover"].forEach(function (type) {
  document.body.addEventListener(type, function (event) {
    const types = event.dataTransfer && event.dataTransfer.types;
    if (!types || !Array.prototype.includes.call(types, "Files")) return;
    event.preventDefault();
    document.body.classList.add("drag");
  });
});
document.body.addEventListener("dragleave", function (event) {
  if (event.target === document.body) document.body.classList.remove("drag");
});
document.body.addEventListener("drop", function (event) {
  if (!event.dataTransfer) return;
  event.preventDefault();
  document.body.classList.remove("drag");
  const file = event.dataTransfer.files && event.dataTransfer.files[0];
  if (file) readFile(file);
});

window.addEventListener("resize", function () {
  if (view.rows.length) drawAll();
});

const src = new URLSearchParams(location.search).get("src");
if (src) {
  statusEl.textContent = "Loading " + src;
  fetch(src)
    .then(function (response) {
      if (!response.ok) throw new Error(response.status + " " + response.statusText);
      return response.arrayBuffer();
    })
    .then(function (buffer) { loadBuffer(buffer, src.split("/").pop()); })
    .catch(function (error) { fail(String(error)); });
}

function fail(message) {
  statusEl.textContent = message;
  statusEl.classList.add("bad");
}

function readFile(file) {
  const reader = new FileReader();
  reader.onload = function () { loadBuffer(reader.result, file.name); };
  reader.onerror = function () { fail("Could not read " + file.name); };
  reader.readAsArrayBuffer(file);
}

function loadBuffer(buffer, name) {
  const bytes = buffer instanceof ArrayBuffer ? new Uint8Array(buffer) : new Uint8Array(buffer);
  if (looksLikeSimpleFs(bytes)) {
    loadBinary(bytes, name);
    return;
  }
  loadText(new TextDecoder("utf-8").decode(bytes), name);
}

function loadText(text, name) {
  statusEl.classList.remove("bad");
  let parsed;
  try {
    parsed = parseCsv(text);
  } catch (error) {
    fail(error.message || String(error));
    return;
  }
  if (!parsed.rows.length) {
    fail("The file has a header and no samples.");
    return;
  }
  if (!parsed.header.includes("sys_time")) {
    fail("This CSV has no sys_time column.");
    return;
  }
  adopt(parsed.rows, name, null);
}

function parseCsv(text) {
  const lines = text.replace(/^\uFEFF/, "").split(/\r?\n/).filter(function (line) {
    return line.trim().length > 0;
  });
  if (lines.length < 2) throw new Error("The file is empty.");
  const delimiter = lines[0].includes(";") ? ";" : ",";
  const header = lines[0].split(delimiter).map(function (cell) { return cell.trim(); });
  const rows = [];
  for (let i = 1; i < lines.length; i++) {
    const cells = lines[i].split(delimiter);
    if (cells.length < 2) continue;
    const row = {};
    for (let c = 0; c < header.length; c++) {
      const raw = cells[c] == null ? "" : cells[c].trim();
      const value = Number(raw);
      row[header[c]] = raw === "" || !Number.isFinite(value) ? NaN : value;
    }
    rows.push(row);
  }
  return { header: header, rows: rows };
}

const SFS_PACKET_SIZE = 128;
const SFS_PREAMBLE = 0xAA55;
const SFS_PREAMBLE_SCHEMA = 0xAA56;
const SFS_PAYLOAD_OFFSET = 4;
const LOG_SCHEMA_MAGIC = 0x314C5450;
const LOG_SCHEMA_VERSION = 1;
const LOG_SCHEMA_FLAG_LE = 0x01;
const LOG_SCHEMA_HDR_SIZE = 18;
const LOG_SCHEMA_SLICE = 104;
const LOG_SCHEMA_NAME_MAX = 19;
const LOG_SCHEMA_MAX_PAYLOAD = 122;
const LOG_UNIT_TICK_0_1MS = 4;
const LOG_TYPE_I8 = 1;
const LOG_TYPE_U8 = 2;
const LOG_TYPE_I16 = 3;
const LOG_TYPE_U16 = 4;
const LOG_TYPE_I32 = 5;
const LOG_TYPE_U32 = 6;
const LOG_TYPE_F16 = 7;
const LOG_TYPE_F32 = 8;
const LOG_TYPE_F64 = 9;

const LEGACY_FIELDS = [
  "sys_time",
  "accX", "accY", "accZ",
  "gyroX", "gyroY", "gyroZ",
  "magX", "magY", "magZ",
  "accHX", "accHY", "accHZ",
  "pressure", "temp",
  "latitude", "longitude", "altitude_gnss", "gnss_sats", "gnss_fix",
  "altitude_press", "altitude_kalman", "ascent_rate_kalman", "tilt",
  "q0", "q1", "q2", "q3",
  "flightstate",
  "ign1_cont", "ign2_cont", "ign3_cont", "ign4_cont",
  "ign1_state", "ign2_state", "ign3_state", "ign4_state",
  "vbat_mV",
  "servo_1", "servo_2", "servo_3", "servo_4", "servo_en"
];

function looksLikeSimpleFs(bytes) {
  if (!bytes || bytes.length < SFS_PACKET_SIZE) return false;
  const pre = bytes[0] | (bytes[1] << 8);
  return pre === SFS_PREAMBLE || pre === SFS_PREAMBLE_SCHEMA;
}

function loadBinary(bytes, name) {
  statusEl.classList.remove("bad");
  let decoded;
  try {
    decoded = decodeSimpleFs(bytes);
  } catch (error) {
    fail(error.message || String(error));
    return;
  }
  if (!decoded.rows.length) {
    fail("The log has no valid sample frames.");
    return;
  }
  adopt(decoded.rows, name, decoded.meta);
}

function decodeSimpleFs(bytes) {
  const asm = {
    active: false,
    blob: null,
    filled: 0,
    next: 0,
    chunkCount: 0,
    blobBytes: 0,
    blobCrc: 0,
    fieldCount: 0,
    sampleBytes: 0,
    rateHz: 0,
    fields: null
  };
  const rows = [];
  let skipped = 0;
  for (let offset = 0; offset + SFS_PACKET_SIZE <= bytes.length; offset += SFS_PACKET_SIZE) {
    const packet = bytes.subarray(offset, offset + SFS_PACKET_SIZE);
    const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
    const pre = view.getUint16(0, true);
    if (view.getUint16(126, true) !== crc16Le(packet, 126)) {
      skipped++;
      continue;
    }
    if (pre === SFS_PREAMBLE_SCHEMA) {
      const installed = acceptSchemaFrame(view, asm);
      if (installed) asm.fields = installed;
    } else if (pre === SFS_PREAMBLE) {
      const row = asm.fields
        ? decodeSchemaRow(view, SFS_PAYLOAD_OFFSET, asm.fields)
        : decodeLegacyRow(view, SFS_PAYLOAD_OFFSET);
      if (row) rows.push(row);
      else skipped++;
    }
  }
  const timeField = asm.fields && asm.fields.find(function (field) { return field.name === "sys_time"; });
  return {
    rows: rows,
    meta: {
      schema: !!asm.fields,
      fieldCount: asm.fields ? asm.fields.length : LEGACY_FIELDS.length,
      timeUnit: timeField ? timeField.unit : null,
      rateHz: asm.rateHz || 0,
      skipped: skipped
    }
  };
}

function crc16Le(bytes, len) {
  let crc = 0xFFFF;
  for (let i = 0; i < len; i++) {
    crc ^= bytes[i];
    for (let bit = 0; bit < 8; bit++) {
      crc = (crc & 1) ? ((crc >>> 1) ^ 0xA001) : (crc >>> 1);
    }
  }
  return crc & 0xFFFF;
}

function logTypeSize(type) {
  switch (type) {
  case LOG_TYPE_I8:
  case LOG_TYPE_U8:
    return 1;
  case LOG_TYPE_I16:
  case LOG_TYPE_U16:
  case LOG_TYPE_F16:
    return 2;
  case LOG_TYPE_I32:
  case LOG_TYPE_U32:
  case LOG_TYPE_F32:
    return 4;
  case LOG_TYPE_F64:
    return 8;
  default:
    return 0;
  }
}

function decodeFloat16(bits) {
  const sign = (bits & 0x8000) ? -1 : 1;
  const exp = (bits >> 10) & 0x1f;
  const frac = bits & 0x3ff;
  if (exp === 0) return sign * Math.pow(2, -14) * (frac / 1024);
  if (exp === 31) return frac ? NaN : sign * Infinity;
  return sign * Math.pow(2, exp - 15) * (1 + frac / 1024);
}

function readSchemaName(view, offset, len) {
  let name = "";
  for (let i = 0; i < len; i++) name += String.fromCharCode(view.getUint8(offset + i));
  return name;
}

function readUnsigned(view, offset, size) {
  if (size === 1) return view.getUint8(offset);
  if (size === 2) return view.getUint16(offset, true);
  if (size === 4) return view.getUint32(offset, true);
  const lo = view.getUint32(offset, true);
  const hi = view.getUint32(offset + 4, true);
  return hi * 0x100000000 + lo;
}

function extractBits(word, bitOff, bitLen) {
  const shifted = bitOff === 0 ? word : Math.floor(word / Math.pow(2, bitOff));
  const mask = Math.pow(2, bitLen) - 1;
  return Math.floor(shifted) % (mask + 1);
}

function readMeas(view, offset, field) {
  const size = logTypeSize(field.type);
  if (field.bitLen !== 0) return extractBits(readUnsigned(view, offset, size), field.bitOff, field.bitLen);
  switch (field.type) {
  case LOG_TYPE_I8: return view.getInt8(offset);
  case LOG_TYPE_U8: return view.getUint8(offset);
  case LOG_TYPE_I16: return view.getInt16(offset, true);
  case LOG_TYPE_U16: return view.getUint16(offset, true);
  case LOG_TYPE_I32: return view.getInt32(offset, true);
  case LOG_TYPE_U32: return view.getUint32(offset, true);
  case LOG_TYPE_F16: return decodeFloat16(view.getUint16(offset, true));
  case LOG_TYPE_F32: return view.getFloat32(offset, true);
  case LOG_TYPE_F64: return view.getFloat64(offset, true);
  default: return null;
  }
}

function readSchemaHeader(view) {
  const base = SFS_PAYLOAD_OFFSET;
  return {
    magic: view.getUint32(base, true),
    version: view.getUint8(base + 4),
    chunkIndex: view.getUint8(base + 5),
    chunkCount: view.getUint8(base + 6),
    flags: view.getUint8(base + 7),
    fieldCount: view.getUint16(base + 8, true),
    sampleBytes: view.getUint16(base + 10, true),
    rateHz: view.getUint16(base + 12, true),
    blobBytes: view.getUint16(base + 14, true),
    blobCrc: view.getUint16(base + 16, true)
  };
}

function parseSchemaBlob(asm) {
  const view = new DataView(asm.blob.buffer, asm.blob.byteOffset, asm.blob.byteLength);
  let pos = 0;
  let cursor = 0;
  const fields = [];
  if (asm.sampleBytes === 0 || asm.sampleBytes > LOG_SCHEMA_MAX_PAYLOAD) return null;
  for (let i = 0; i < asm.fieldCount; i++) {
    if (pos >= asm.blobBytes) return null;
    const nameLen = view.getUint8(pos);
    pos += 1;
    if (nameLen < 1 || nameLen > LOG_SCHEMA_NAME_MAX) return null;
    if (pos + nameLen + 5 + 8 > asm.blobBytes) return null;
    const name = readSchemaName(view, pos, nameLen);
    pos += nameLen;
    const type = view.getUint8(pos++);
    const unit = view.getUint8(pos++);
    const bitOff = view.getUint8(pos++);
    const bitLen = view.getUint8(pos++);
    const advance = view.getUint8(pos++);
    const scale = view.getFloat32(pos, true);
    pos += 4;
    const offset = view.getFloat32(pos, true);
    pos += 4;
    const size = logTypeSize(type);
    if (size === 0) return null;
    if (bitLen !== 0 && bitOff + bitLen > size * 8) return null;
    if (cursor + size > asm.sampleBytes) return null;
    cursor += advance;
    if (cursor > asm.sampleBytes) return null;
    fields.push({
      name: name,
      type: type,
      unit: unit,
      bitOff: bitOff,
      bitLen: bitLen,
      advance: advance,
      scale: scale,
      offset: offset
    });
  }
  if (pos !== asm.blobBytes) return null;
  return fields;
}

function acceptSchemaFrame(view, asm) {
  const hdr = readSchemaHeader(view);
  if (hdr.magic !== LOG_SCHEMA_MAGIC) return asm.fields || null;
  if (hdr.version !== LOG_SCHEMA_VERSION || hdr.flags !== LOG_SCHEMA_FLAG_LE) {
    asm.active = false;
    return asm.fields || null;
  }
  if (hdr.chunkCount === 0 || hdr.chunkIndex >= hdr.chunkCount) {
    asm.active = false;
    return asm.fields || null;
  }
  if (hdr.chunkIndex === 0) {
    asm.active = true;
    asm.blob = new Uint8Array(hdr.blobBytes);
    asm.filled = 0;
    asm.next = 0;
    asm.chunkCount = hdr.chunkCount;
    asm.blobBytes = hdr.blobBytes;
    asm.blobCrc = hdr.blobCrc;
    asm.fieldCount = hdr.fieldCount;
    asm.sampleBytes = hdr.sampleBytes;
    asm.rateHz = hdr.rateHz;
  }
  if (!asm.active ||
      hdr.chunkIndex !== asm.next ||
      hdr.chunkCount !== asm.chunkCount ||
      hdr.blobBytes !== asm.blobBytes ||
      hdr.blobCrc !== asm.blobCrc ||
      hdr.fieldCount !== asm.fieldCount ||
      hdr.sampleBytes !== asm.sampleBytes) {
    asm.active = false;
    return asm.fields || null;
  }
  const blobOffset = hdr.chunkIndex * LOG_SCHEMA_SLICE;
  if (blobOffset >= hdr.blobBytes || hdr.blobBytes > asm.blob.length) {
    asm.active = false;
    return asm.fields || null;
  }
  let slice = hdr.blobBytes - blobOffset;
  if (slice > LOG_SCHEMA_SLICE) slice = LOG_SCHEMA_SLICE;
  if (SFS_PAYLOAD_OFFSET + LOG_SCHEMA_HDR_SIZE + slice > view.byteLength) {
    asm.active = false;
    return asm.fields || null;
  }
  if (asm.filled + slice > hdr.blobBytes) {
    asm.active = false;
    return asm.fields || null;
  }
  const src = new Uint8Array(view.buffer, view.byteOffset + SFS_PAYLOAD_OFFSET + LOG_SCHEMA_HDR_SIZE, slice);
  asm.blob.set(src, asm.filled);
  asm.filled += slice;
  asm.next++;
  if (asm.next !== asm.chunkCount) return asm.fields || null;
  if (asm.filled !== asm.blobBytes || crc16Le(asm.blob, asm.blob.length) !== asm.blobCrc) {
    asm.active = false;
    return asm.fields || null;
  }
  const fields = parseSchemaBlob(asm);
  asm.active = false;
  if (!fields) return asm.fields || null;
  asm.fields = fields;
  return fields;
}

function decodeSchemaRow(view, base, fields) {
  const row = {};
  let cursor = 0;
  for (let i = 0; i < fields.length; i++) {
    const field = fields[i];
    const size = logTypeSize(field.type);
    if (base + cursor + size > view.byteLength) return null;
    const raw = readMeas(view, base + cursor, field);
    if (raw === null || !Number.isFinite(raw)) row[field.name] = NaN;
    else row[field.name] = raw * field.scale - field.offset;
    cursor += field.advance;
  }
  return row;
}

function decodeLegacyRow(view, base) {
  if (base + 108 > view.byteLength) return null;
  const f = function (offset) { return view.getFloat32(base + offset, true); };
  const gnssRaw = view.getUint8(base + 69);
  const ign = view.getUint8(base + 100);
  const values = [
    view.getUint32(base, true),
    f(4), f(8), f(12),
    f(16), f(20), f(24),
    f(28), f(32), f(36),
    f(40), f(44), f(48),
    f(52), view.getInt8(base + 56),
    f(57), f(61), f(65), gnssRaw & 0x3F, (gnssRaw >> 6) & 0x03,
    f(70), f(74), f(78), view.getUint8(base + 82),
    f(83), f(87), f(91), f(95),
    view.getUint8(base + 99),
    (ign >> 0) & 1, (ign >> 1) & 1, (ign >> 2) & 1, (ign >> 3) & 1,
    (ign >> 4) & 1, (ign >> 5) & 1, (ign >> 6) & 1, (ign >> 7) & 1,
    view.getUint16(base + 101, true),
    view.getInt8(base + 103), view.getInt8(base + 104), view.getInt8(base + 105), view.getInt8(base + 106),
    view.getUint8(base + 107)
  ];
  const row = {};
  for (let i = 0; i < LEGACY_FIELDS.length; i++) row[LEGACY_FIELDS[i]] = values[i];
  return row;
}

function adopt(rows, name, meta) {
  if (!rows.length || !Number.isFinite(rows[0].sys_time)) {
    fail("The log has no sys_time samples.");
    return;
  }
  const sys = rows.map(function (row) { return row.sys_time; });
  const scale = timeScale(sys, meta && meta.timeUnit);
  const t = sys.map(function (value) { return (value - sys[0]) / scale.div; });
  const dt = medianPositive(diff(t));
  addDerived(rows, t, dt);

  view.rows = rows;
  view.t = t;
  view.t0 = t[0];
  view.t1 = t[t.length - 1];
  view.view0 = view.t0;
  view.view1 = view.t1;
  view.cursor = view.t0;
  view.div = scale.div;
  view.note = scale.note + (meta && meta.schema
    ? " · schema, " + meta.fieldCount + " fields"
    : (meta ? " · legacy layout" : ""));
  if (meta && meta.skipped) view.note += " · " + meta.skipped + " bad frames skipped";
  view.rateHz = dt > 0 ? 1 / dt : 0;
  const liftoff = detectLiftoff(rows, t);
  view.liftoff = liftoff.t;
  view.liftoffFound = liftoff.found;
  view.cansat = looksLikeCansat(rows);
  view.segments = flightSegments(rows, t);
  view.events = buildEvents(rows, t, view.cansat);
  view.name = name || "meas.csv";

  fileNameEl.textContent = view.name;
  statusEl.textContent = [
    rows.length + " samples",
    view.rateHz ? view.rateHz.toFixed(0) + " Hz" : "irregular",
    view.liftoffFound ? "time 0 at the accX step" : "no accX step, time from log start",
    formatSeconds(flightTime(view.t0)) + " to " + formatSeconds(flightTime(view.t1)),
    view.note
  ].join("  ·  ");
  resetBtn.disabled = false;
  saveBtn.disabled = false;
  dropEl.classList.add("hidden");
  statsEl.hidden = false;
  layoutEl.hidden = false;
  renderStats();
  renderCharts();
  renderEvents();
  drawAll();
  requestAnimationFrame(drawAll);
}

function timeScale(sys, unit) {
  if (unit === 4) return { div: 10000, note: "sys_time in 0.1 ms ticks (from the log schema)" };
  const step = medianPositive(diff(sys));
  if (step < 5) return { div: 1, note: "sys_time already in seconds" };
  if (step < 80) return { div: 1000, note: "sys_time in milliseconds" };
  if (step > 2000) return { div: 1e6, note: "sys_time in microseconds" };
  return { div: 10000, note: "sys_time in 0.1 ms ticks (100 ticks = 10 ms)" };
}

function diff(values) {
  const out = [];
  for (let i = 1; i < values.length; i++) out.push(values[i] - values[i - 1]);
  return out;
}

function medianPositive(values) {
  const sorted = values.filter(function (value) { return value > 0; }).sort(function (a, b) { return a - b; });
  if (!sorted.length) return 0;
  return sorted[Math.floor(sorted.length / 2)];
}

function addDerived(rows, t, dt) {
  const alt = rows.map(function (row) { return row.altitude_press; });
  const raw = new Array(rows.length);
  for (let i = 0; i < rows.length; i++) {
    const span = (i === 0) ? (t[1] - t[0]) : (i === rows.length - 1) ? (t[i] - t[i - 1]) : (t[i + 1] - t[i - 1]);
    const delta = (i === 0) ? (alt[1] - alt[0]) : (i === rows.length - 1) ? (alt[i] - alt[i - 1]) : (alt[i + 1] - alt[i - 1]);
    raw[i] = span > 0 ? delta / span : NaN;
  }
  const window = Math.max(1, Math.round(0.2 / (dt || 0.01)));
  for (let i = 0; i < rows.length; i++) {
    let sum = 0;
    let n = 0;
    const a = Math.max(0, i - window);
    const b = Math.min(rows.length - 1, i + window);
    for (let k = a; k <= b; k++) {
      if (Number.isFinite(raw[k])) { sum += raw[k]; n++; }
    }
    rows[i].vz_press = n ? sum / n : NaN;
    rows[i].vbat_V = Number.isFinite(rows[i].vbat_mV) ? rows[i].vbat_mV / 1000 : NaN;
  }
}

function looksLikeCansat(rows) {
  const cols = ["magX", "magY", "magZ"];
  let seen = 0;
  for (let i = 0; i < rows.length; i++) {
    for (let c = 0; c < cols.length; c++) {
      const value = rows[i][cols[c]];
      if (!Number.isFinite(value)) continue;
      seen++;
      if (Math.abs(value) > 20 || Math.abs(value - Math.round(value)) > 1e-6) return false;
    }
  }
  return seen > 0;
}

function flightSegments(rows, t) {
  const segments = [];
  let start = 0;
  for (let i = 1; i <= rows.length; i++) {
    const changed = i === rows.length || rows[i].flightstate !== rows[start].flightstate;
    if (!changed) continue;
    segments.push({
      t0: t[start],
      t1: t[Math.max(start, i - 1)],
      state: rows[start].flightstate
    });
    start = i;
  }
  return segments;
}

function buildEvents(rows, t, cansat) {
  const events = [];
  let state = rows[0].flightstate;
  events.push({ t: t[0], label: stateName(state) });
  for (let i = 1; i < rows.length; i++) {
    if (rows[i].flightstate !== state) {
      state = rows[i].flightstate;
      events.push({ t: t[i], label: stateName(state) });
    }
    for (let n = 1; n <= 4; n++) {
      const key = "ign" + n + "_state";
      if (rows[i][key] === 1 && rows[i - 1][key] !== 1) {
        events.push({ t: t[i], label: igniterName(n) });
      }
    }
    if (cansat && rows[i].magX !== rows[i - 1].magX && Number.isFinite(rows[i].magX)) {
      events.push({ t: t[i], label: "Cansat " + cansatName(rows[i].magX) });
    }
  }
  return events;
}

function stateName(value) {
  if (!Number.isFinite(value)) return "State —";
  const id = Math.round(value);
  return FLIGHT_STATE[id] || ("State " + id);
}

function cansatName(value) {
  const id = Math.round(value);
  return CANSAT_STATE[id] || String(id);
}

function igniterName(n) {
  const role = { 1: "drogue", 2: "main", 3: "stage 2" };
  return role[n] ? "Igniter " + n + " · " + role[n] : "Igniter " + n;
}

function renderStats() {
  const rows = view.rows;
  const t = view.t;
  view.apogees = [
    estimateApogee(t, rows, "altitude_press", "Pressure", "#e0a15c"),
    estimateApogee(t, rows, "altitude_kalman", "Kalman", "#7eb6ff"),
    estimateApogee(t, rows, "altitude_gnss", "GNSS", "#9ad7b0")
  ].filter(function (item) { return item; });
  const climb = extreme(rows, t, "ascent_rate_kalman", 1);
  const acc = extremeAbsAccel(rows, t);
  const items = view.apogees.map(function (item) {
    return {
      value: formatSeconds(flightTime(item.t)),
      label: item.name + " apogee · " + Math.round(item.value) + " m",
      t: item.t
    };
  });
  items.push({
    value: climb ? climb.value.toFixed(0) + " m/s" : "—",
    label: "Max climb rate"
  });
  items.push({
    value: acc ? acc.value.toFixed(1) + " g" : "—",
    label: "Peak acceleration"
  });
  statsEl.innerHTML = "";
  items.forEach(function (item) {
    const el = document.createElement(item.t == null ? "div" : "button");
    el.className = "stat";
    if (item.t != null) el.type = "button";
    el.innerHTML = "<b></b><span></span>";
    el.querySelector("b").textContent = item.value;
    el.querySelector("span").textContent = item.label;
    if (item.t != null) {
      el.addEventListener("click", function () { seekTime(item.t); });
    }
    statsEl.appendChild(el);
  });
}

function estimateApogee(t, rows, key, name, color) {
  const alt = rows.map(function (row) { return row[key]; });
  if (!alt.some(function (value) { return Number.isFinite(value); })) return null;
  const n = alt.length;
  const dt = n > 1 ? medianPositive(diff(t)) : 0.01;
  const raw = new Array(n);
  for (let i = 0; i < n; i++) {
    const span = i === 0 ? t[1] - t[0] : i === n - 1 ? t[i] - t[i - 1] : t[i + 1] - t[i - 1];
    const delta = i === 0 ? alt[1] - alt[0] : i === n - 1 ? alt[i] - alt[i - 1] : alt[i + 1] - alt[i - 1];
    raw[i] = span > 0 && Number.isFinite(delta) ? delta / span : NaN;
  }
  const half = Math.max(1, Math.round(0.15 / (dt || 0.01)));
  const vz = new Array(n);
  for (let i = 0; i < n; i++) {
    let sum = 0;
    let count = 0;
    const a = Math.max(0, i - half);
    const b = Math.min(n - 1, i + half);
    for (let k = a; k <= b; k++) {
      if (Number.isFinite(raw[k])) { sum += raw[k]; count++; }
    }
    vz[i] = count ? sum / count : NaN;
  }
  let maxAlt = -Infinity;
  for (let i = 0; i < n; i++) {
    if (Number.isFinite(alt[i]) && alt[i] > maxAlt) maxAlt = alt[i];
  }
  const minAlt = Math.max(15, 0.25 * Math.max(maxAlt, 0));
  const hold = 0.4;
  for (let i = 1; i < n; i++) {
    if (!(vz[i - 1] > 0 && vz[i] <= 0)) continue;
    if (!Number.isFinite(alt[i]) || alt[i] < minAlt) continue;
    let climbed = false;
    for (let k = i - 1; k >= 0 && t[i] - t[k] <= 3; k--) {
      if (vz[k] > 1) { climbed = true; break; }
    }
    if (!climbed) continue;
    let end = i;
    while (end < n - 1 && t[end] < t[i] + hold) end++;
    if (t[end] - t[i] < hold * 0.8) continue;
    let neg = 0;
    let count = 0;
    for (let k = i; k <= end; k++) {
      if (!Number.isFinite(vz[k])) continue;
      count++;
      if (vz[k] < 0) neg++;
    }
    if (!count || neg / count < 0.7) continue;
    const denom = vz[i - 1] - vz[i];
    const frac = denom !== 0 ? vz[i - 1] / denom : 0;
    const cross = t[i - 1] + frac * (t[i] - t[i - 1]);
    const fitted = parabolaVertex(t, alt, cross, 0.9);
    return {
      name: name,
      color: color,
      t: fitted ? fitted.t : cross,
      value: fitted ? fitted.value : alt[i]
    };
  }
  return null;
}

function parabolaVertex(t, alt, center, halfWidth) {
  const t0 = center - halfWidth;
  const t1 = center + halfWidth;
  let n = 0;
  let s0 = 0;
  let s1 = 0;
  let s2 = 0;
  let s3 = 0;
  let s4 = 0;
  let b0 = 0;
  let b1 = 0;
  let b2 = 0;
  let previous = null;
  for (let i = 0; i < t.length; i++) {
    if (t[i] < t0 || t[i] > t1 || !Number.isFinite(alt[i])) continue;
    if (previous != null && Math.abs(alt[i] - previous) > 25) return null;
    previous = alt[i];
    const x = t[i];
    const x2 = x * x;
    s0 += 1;
    s1 += x;
    s2 += x2;
    s3 += x2 * x;
    s4 += x2 * x2;
    b0 += alt[i];
    b1 += alt[i] * x;
    b2 += alt[i] * x2;
    n++;
  }
  if (n < 8) return null;
  const det = determinant3(s4, s3, s2, s3, s2, s1, s2, s1, s0);
  if (Math.abs(det) < 1e-9) return null;
  const c2 = determinant3(b2, s3, s2, b1, s2, s1, b0, s1, s0) / det;
  const c1 = determinant3(s4, b2, s2, s3, b1, s1, s2, b0, s0) / det;
  const c0 = determinant3(s4, s3, b2, s3, s2, b1, s2, s1, b0) / det;
  if (!(c2 < 0)) return null;
  const vertex = -c1 / (2 * c2);
  if (vertex < t0 || vertex > t1) return null;
  return { t: vertex, value: c2 * vertex * vertex + c1 * vertex + c0 };
}

function determinant3(a, b, c, d, e, f, g, h, i) {
  return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
}

function extreme(rows, t, key, sign) {
  let best = null;
  for (let i = 0; i < rows.length; i++) {
    const value = rows[i][key];
    if (!Number.isFinite(value)) continue;
    if (!best || value * sign > best.value * sign) best = { value: value, t: t[i] };
  }
  return best;
}

function extremeAbsAccel(rows, t) {
  let best = null;
  for (let i = 0; i < rows.length; i++) {
    const x = rows[i].accX;
    const y = rows[i].accY;
    const z = rows[i].accZ;
    if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(z)) continue;
    const value = Math.sqrt(x * x + y * y + z * z);
    if (!best || value > best.value) best = { value: value, t: t[i] };
  }
  return best;
}

function renderCharts() {
  chartsEl.innerHTML = "";
  const strip = document.createElement("section");
  strip.className = "chart";
  strip.innerHTML = "<div class=\"chart-head\"><h2>Flight state</h2></div>";
  const stripCanvas = document.createElement("canvas");
  stripCanvas.className = "plot";
  stripCanvas.dataset.chart = "state";
  stripCanvas.dataset.h = "56";
  strip.appendChild(stripCanvas);
  chartsEl.appendChild(strip);
  bindPlot(stripCanvas, false);

  CHARTS.forEach(function (chart) {
    const present = chart.series.filter(function (series) {
      return view.rows.some(function (row) { return Number.isFinite(row[series.key]); });
    });
    if (!present.length) return;
    chart.live = present;
    const section = document.createElement("section");
    section.className = "chart";
    const head = document.createElement("div");
    head.className = "chart-head";
    head.innerHTML = "<h2>" + chart.title + "</h2>";
    const legend = document.createElement("div");
    legend.className = "legend";
    present.forEach(function (series) {
      const id = chart.id + ":" + series.key;
      const label = document.createElement("label");
      label.innerHTML = "<input type=\"checkbox\"" + (shown[id] ? " checked" : "") + "><i></i><span></span>";
      label.querySelector("i").style.background = series.color;
      label.querySelector("span").textContent = series.name;
      label.querySelector("input").addEventListener("change", function (event) {
        shown[id] = event.target.checked;
        drawAll();
      });
      legend.appendChild(label);
    });
    head.appendChild(legend);
    const canvas = document.createElement("canvas");
    canvas.className = "plot";
    canvas.dataset.chart = chart.id;
    canvas.dataset.h = String(chart.height);
    section.appendChild(head);
    section.appendChild(canvas);
    chartsEl.appendChild(section);
    bindPlot(canvas, true);
  });
}

function bindPlot(canvas, zoomable) {
  canvas.addEventListener("mousemove", function (event) {
    const time = timeAt(canvas, event);
    if (time == null) return;
    view.cursor = clamp(time, view.view0, view.view1);
    drawAll();
  });
  canvas.addEventListener("dblclick", function () {
    view.view0 = view.t0;
    view.view1 = view.t1;
    drawAll();
  });
  if (!zoomable) return;
  canvas.addEventListener("wheel", function (event) {
    event.preventDefault();
    const time = timeAt(canvas, event);
    if (time == null) return;
    const span = view.view1 - view.view0;
    const factor = event.deltaY > 0 ? 1.25 : 0.8;
    let next = span * factor;
    const minSpan = Math.max((view.t1 - view.t0) / view.rows.length * 8, 0.05);
    next = clamp(next, minSpan, view.t1 - view.t0);
    const ratio = (time - view.view0) / span;
    view.view0 = time - ratio * next;
    view.view1 = view.view0 + next;
    if (view.view0 < view.t0) { view.view0 = view.t0; view.view1 = view.t0 + next; }
    if (view.view1 > view.t1) { view.view1 = view.t1; view.view0 = view.t1 - next; }
    drawAll();
  }, { passive: false });

  let drag = null;
  canvas.addEventListener("pointerdown", function (event) {
    drag = { x: event.clientX, view0: view.view0, view1: view.view1 };
    canvas.setPointerCapture(event.pointerId);
  });
  canvas.addEventListener("pointermove", function (event) {
    if (!drag) return;
    const rect = canvas.getBoundingClientRect();
    const plotW = rect.width - 56 - 12;
    const span = drag.view1 - drag.view0;
    const shift = (drag.x - event.clientX) / plotW * span;
    let a = drag.view0 + shift;
    let b = drag.view1 + shift;
    if (a < view.t0) { b += view.t0 - a; a = view.t0; }
    if (b > view.t1) { a -= b - view.t1; b = view.t1; }
    view.view0 = a;
    view.view1 = b;
    drawAll();
  });
  canvas.addEventListener("pointerup", function () { drag = null; });
  canvas.addEventListener("pointercancel", function () { drag = null; });
}

function timeAt(canvas, event) {
  const rect = canvas.getBoundingClientRect();
  const left = 56;
  const right = 12;
  const x = event.clientX - rect.left;
  if (x < left || x > rect.width - right) return null;
  const ratio = (x - left) / (rect.width - left - right);
  return view.view0 + ratio * (view.view1 - view.view0);
}

function renderEvents() {
  eventsEl.innerHTML = "";
  view.events.forEach(function (item, index) {
    const button = document.createElement("button");
    button.type = "button";
    button.innerHTML = "<time>" + formatSeconds(flightTime(item.t)) + "</time><span></span>";
    button.querySelector("span").textContent = item.label;
    button.addEventListener("click", function () { focusEvent(index); });
    eventsEl.appendChild(button);
  });
}

function seekTime(time) {
  view.cursor = time;
  const span = view.view1 - view.view0;
  if (time < view.view0 || time > view.view1) {
    view.view0 = clamp(time - span / 2, view.t0, Math.max(view.t0, view.t1 - span));
    view.view1 = view.view0 + span;
  }
  drawAll();
}

function focusEvent(index) {
  const item = view.events[index];
  view.cursor = item.t;
  const span = view.view1 - view.view0;
  if (item.t < view.view0 || item.t > view.view1) {
    view.view0 = clamp(item.t - span / 2, view.t0, view.t1 - span);
    view.view1 = view.view0 + span;
  }
  drawAll();
}

function drawAll() {
  document.querySelectorAll("canvas.plot").forEach(function (canvas) {
    const id = canvas.dataset.chart;
    if (id === "state") drawState(canvas);
    else drawChart(canvas, CHARTS.find(function (chart) { return chart.id === id; }));
  });
  drawReadout();
  drawTrack();
  highlightEvent();
}

function plotBox(canvas) {
  const dpr = window.devicePixelRatio || 1;
  const width = canvas.clientWidth || 640;
  const height = Number(canvas.dataset.h) || 180;
  canvas.style.height = height + "px";
  canvas.width = Math.round(width * dpr);
  canvas.height = Math.round(height * dpr);
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, width, height);
  return {
    ctx: ctx,
    width: width,
    height: height,
    left: 56,
    right: width - 12,
    top: 10,
    bottom: height - 22
  };
}

function drawState(canvas) {
  const box = plotBox(canvas);
  const ctx = box.ctx;
  view.segments.forEach(function (segment) {
    const x0 = xOf(box, Math.max(segment.t0, view.view0));
    const x1 = xOf(box, Math.min(segment.t1, view.view1));
    if (x1 <= box.left || x0 >= box.right) return;
    ctx.fillStyle = STATE_COLOR[Math.round(segment.state)] || "#3d4554";
    ctx.fillRect(Math.max(box.left, x0), box.top, Math.min(box.right, x1) - Math.max(box.left, x0), box.bottom - box.top);
  });
  ctx.save();
  ctx.beginPath();
  ctx.rect(box.left, box.top, box.right - box.left, box.bottom - box.top);
  ctx.clip();
  ctx.fillStyle = "#121418";
  ctx.font = "12px Segoe UI, sans-serif";
  view.segments.forEach(function (segment) {
    if (segment.t1 < view.view0 || segment.t0 > view.view1) return;
    const x0 = Math.max(box.left, xOf(box, Math.max(segment.t0, view.view0)));
    const x1 = Math.min(box.right, xOf(box, Math.min(segment.t1, view.view1)));
    const label = stateName(segment.state);
    if (x1 - x0 < ctx.measureText(label).width + 12) return;
    ctx.fillText(label, x0 + 6, box.top + 18);
  });
  ctx.restore();
  drawCursor(box);
  drawTimeAxis(box);
}

function seriesExtent(series) {
  let low = Infinity;
  let high = -Infinity;
  for (let s = 0; s < series.length; s++) {
    const key = series[s].key;
    for (let i = 0; i < view.rows.length; i++) {
      const time = view.t[i];
      if (time < view.view0 || time > view.view1) continue;
      const value = view.rows[i][key];
      if (!Number.isFinite(value)) continue;
      if (value < low) low = value;
      if (value > high) high = value;
    }
  }
  if (!Number.isFinite(low) || !Number.isFinite(high)) return [0, 1];
  if (low === high) {
    low -= 1;
    high += 1;
  }
  return [low, high];
}

function drawChart(canvas, chart) {
  const box = plotBox(canvas);
  const ctx = box.ctx;
  const series = chart.live.filter(function (item) { return shown[chart.id + ":" + item.key]; });
  const extent = seriesExtent(series);
  let y0 = extent[0];
  let y1 = extent[1];
  const pad = (y1 - y0) * 0.06;
  y0 -= pad;
  y1 += pad;
  if (chart.zero) {
    y0 = Math.min(y0, 0);
    y1 = Math.max(y1, 0);
  }
  const step = chart.ticks === "10" ? decimalStep(y1 - y0) : power2Step(y1 - y0);
  if (y0 === y1) y1 = y0 + step;

  ctx.lineWidth = 1;
  ctx.font = "11px Consolas, monospace";
  ctx.textAlign = "right";
  ctx.textBaseline = "middle";
  const first = Math.ceil(y0 / step - 1e-9);
  const last = Math.floor(y1 / step + 1e-9);
  for (let i = first; i <= last; i++) {
    const value = i * step;
    const y = yOf(box, value, y0, y1);
    ctx.strokeStyle = Math.abs(value) < step * 1e-6 ? "#465066" : "#2c3340";
    ctx.beginPath();
    ctx.moveTo(box.left, y);
    ctx.lineTo(box.right, y);
    ctx.stroke();
    ctx.fillStyle = "#8b95a8";
    ctx.fillText(formatTick(value, step), box.left - 6, y);
  }

  const gap = view.rateHz > 0 ? 5 / view.rateHz : 1;
  ctx.save();
  ctx.beginPath();
  ctx.rect(box.left, box.top, box.right - box.left, box.bottom - box.top);
  ctx.clip();
  series.forEach(function (item) {
    ctx.beginPath();
    ctx.strokeStyle = item.color;
    ctx.lineWidth = 1.6;
    let drawing = false;
    let previous = null;
    for (let i = 0; i < view.rows.length; i++) {
      const time = view.t[i];
      const value = view.rows[i][item.key];
      if (time < view.view0 || time > view.view1 || !Number.isFinite(value)) {
        drawing = false;
        continue;
      }
      if (previous != null && time - previous > gap) drawing = false;
      const x = xOf(box, time);
      const y = yOf(box, value, y0, y1);
      if (!drawing) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
      drawing = true;
      previous = time;
    }
    ctx.stroke();
  });
  ctx.restore();

  if (chart.id === "alt" && view.apogees) {
    ctx.save();
    ctx.beginPath();
    ctx.rect(box.left, box.top, box.right - box.left, box.bottom - box.top);
    ctx.clip();
    ctx.font = "11px Segoe UI, sans-serif";
    ctx.textBaseline = "top";
    view.apogees.forEach(function (item, index) {
      if (item.t < view.view0 || item.t > view.view1) return;
      const x = xOf(box, item.t);
      ctx.strokeStyle = item.color;
      ctx.lineWidth = 1;
      ctx.setLineDash([3, 3]);
      ctx.beginPath();
      ctx.moveTo(x, box.top);
      ctx.lineTo(x, box.bottom);
      ctx.stroke();
      ctx.setLineDash([]);
      ctx.fillStyle = item.color;
      ctx.fillText(item.name, x + 4, box.top + 2 + index * 13);
    });
    ctx.restore();
  }

  ctx.fillStyle = "#8b95a8";
  ctx.textAlign = "left";
  ctx.textBaseline = "top";
  ctx.fillText(chart.unit, box.left + 4, box.top);
  drawCursor(box);
  drawTimeAxis(box);
}

function drawCursor(box) {
  if (view.cursor < view.view0 || view.cursor > view.view1) return;
  const ctx = box.ctx;
  const x = xOf(box, view.cursor);
  ctx.strokeStyle = "rgba(231, 236, 243, 0.55)";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(x, box.top);
  ctx.lineTo(x, box.bottom);
  ctx.stroke();
}

function drawTimeAxis(box) {
  const ctx = box.ctx;
  ctx.fillStyle = "#8b95a8";
  ctx.font = "11px Consolas, monospace";
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  const span = view.view1 - view.view0;
  const step = niceStep(span / 6);
  const start = flightTime(view.view0);
  const end = flightTime(view.view1);
  let tick = Math.ceil(start / step - 1e-9) * step;
  for (; tick <= end + step * 1e-6; tick += step) {
    const x = xOf(box, tick + view.liftoff);
    if (x < box.left - 1 || x > box.right + 1) continue;
    ctx.fillText(formatSeconds(tick), x, box.bottom + 4);
  }
}

function drawReadout() {
  const index = nearestIndex(view.cursor);
  const pairs = samplePairs(view.rows[index], view.t[index]);
  readoutEl.innerHTML = pairs.map(function (pair) {
    return "<dt>" + pair[0] + "</dt><dd>" + pair[1] + "</dd>";
  }).join("");
}

function samplePairs(row, time) {
  const pairs = [
    ["From liftoff", formatSeconds(flightTime(time))],
    ["State", stateName(row.flightstate)],
    ["Pressure alt", metres(row.altitude_press)],
    ["Kalman alt", metres(row.altitude_kalman)],
    ["GNSS alt", metres(row.altitude_gnss)],
    ["Climb", metresPerSec(row.ascent_rate_kalman)],
    ["Acc X Y Z", triple(row.accX, row.accY, row.accZ, 2)],
    ["Gyro X Y Z", triple(row.gyroX, row.gyroY, row.gyroZ, 0)],
    ["Battery", Number.isFinite(row.vbat_V) ? row.vbat_V.toFixed(2) + " V" : "—"],
    ["Sats", Number.isFinite(row.gnss_sats) ? String(Math.round(row.gnss_sats)) : "—"],
    ["Igniters", igniterLine(row)],
    ["Lat", Number.isFinite(row.latitude) ? row.latitude.toFixed(6) : "—"],
    ["Lon", Number.isFinite(row.longitude) ? row.longitude.toFixed(6) : "—"]
  ];
  if (view.cansat) {
    pairs.push(["Cansat", cansatName(row.magX) + " · batches " + (Number.isFinite(row.magY) ? Math.round(row.magY) : "—")]);
  } else {
    pairs.push(["Tilt", Number.isFinite(row.tilt) ? Math.round(row.tilt) + "° logged" : "—"]);
  }
  return pairs;
}

function drawTrack() {
  const canvas = trackEl;
  const dpr = window.devicePixelRatio || 1;
  const width = canvas.clientWidth || 280;
  const height = 280;
  canvas.width = Math.round(width * dpr);
  canvas.height = Math.round(height * dpr);
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, width, height);

  const origin = firstFix();
  if (!origin) {
    ctx.fillStyle = "#8b95a8";
    ctx.font = "12px Segoe UI, sans-serif";
    ctx.fillText("No GNSS position", 12, 24);
    return;
  }
  const points = [];
  for (let i = 0; i < view.rows.length; i++) {
    const row = view.rows[i];
    if (!Number.isFinite(row.latitude) || !Number.isFinite(row.longitude)) continue;
    if (row.latitude === 0 && row.longitude === 0) continue;
    const ne = northEast(origin.latitude, origin.longitude, row.latitude, row.longitude);
    points.push({ t: view.t[i], n: ne[0], e: ne[1] });
  }
  if (points.length < 2) return;
  let max = 10;
  points.forEach(function (point) {
    max = Math.max(max, Math.abs(point.n), Math.abs(point.e));
  });
  max *= 1.15;
  const left = 28;
  const top = 12;
  const size = Math.min(width - left - 12, height - top - 22);
  function px(e) { return left + (e + max) / (2 * max) * size; }
  function py(n) { return top + (max - n) / (2 * max) * size; }

  ctx.strokeStyle = "#2c3340";
  ctx.strokeRect(left, top, size, size);
  ctx.beginPath();
  ctx.moveTo(px(0), top);
  ctx.lineTo(px(0), top + size);
  ctx.moveTo(left, py(0));
  ctx.lineTo(left + size, py(0));
  ctx.stroke();

  ctx.beginPath();
  ctx.strokeStyle = "#e0a15c";
  ctx.lineWidth = 1.6;
  points.forEach(function (point, index) {
    if (index === 0) ctx.moveTo(px(point.e), py(point.n));
    else ctx.lineTo(px(point.e), py(point.n));
  });
  ctx.stroke();

  const cursor = points.reduce(function (best, point) {
    return Math.abs(point.t - view.cursor) < Math.abs(best.t - view.cursor) ? point : best;
  }, points[0]);
  ctx.fillStyle = "#e7ecf3";
  ctx.beginPath();
  ctx.arc(px(cursor.e), py(cursor.n), 3.5, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = "#8b95a8";
  ctx.font = "11px Consolas, monospace";
  ctx.fillText("N " + cursor.n.toFixed(0) + " m   E " + cursor.e.toFixed(0) + " m", left, top + size + 14);
}

function highlightEvent() {
  const buttons = eventsEl.querySelectorAll("button");
  let best = 0;
  let bestDist = Infinity;
  view.events.forEach(function (item, index) {
    const dist = Math.abs(item.t - view.cursor);
    if (dist < bestDist) { bestDist = dist; best = index; }
  });
  buttons.forEach(function (button, index) {
    button.classList.toggle("on", index === best && bestDist < 0.3);
  });
}

function firstFix() {
  for (let i = 0; i < view.rows.length; i++) {
    const row = view.rows[i];
    if (Number.isFinite(row.latitude) && Number.isFinite(row.longitude) && !(row.latitude === 0 && row.longitude === 0)) {
      return row;
    }
  }
  return null;
}

function northEast(lat0, lon0, lat, lon) {
  const radius = 6371000;
  const north = (lat - lat0) * Math.PI / 180 * radius;
  const east = (lon - lon0) * Math.PI / 180 * radius * Math.cos(lat0 * Math.PI / 180);
  return [north, east];
}

function nearestIndex(time) {
  let lo = 0;
  let hi = view.t.length - 1;
  while (lo < hi) {
    const mid = (lo + hi) >> 1;
    if (view.t[mid] < time) lo = mid + 1;
    else hi = mid;
  }
  if (lo > 0 && Math.abs(view.t[lo - 1] - time) < Math.abs(view.t[lo] - time)) return lo - 1;
  return lo;
}

function xOf(box, time) {
  return box.left + (time - view.view0) / (view.view1 - view.view0) * (box.right - box.left);
}

function yOf(box, value, y0, y1) {
  return box.bottom - (value - y0) / (y1 - y0) * (box.bottom - box.top);
}

function saveReport() {
  if (!view.rows.length) return;
  drawAll();
  const payload = reportPayload();
  const html = reportHtml({
    name: view.name,
    status: statusEl.textContent,
    range: formatSeconds(flightTime(view.view0)) + " to " + formatSeconds(flightTime(view.view1)),
    stats: statItems(),
    charts: payload.charts,
    readout: readoutPairs(),
    events: view.events.filter(function (item) {
      return item.t >= view.view0 && item.t <= view.view1;
    }),
    data: JSON.stringify(payload).replace(/</g, "\\u003c")
  });
  const blob = new Blob([html], { type: "text/html" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = reportFileName(view.name);
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(url);
}

function statItems() {
  const items = [];
  statsEl.querySelectorAll(".stat").forEach(function (el) {
    const value = el.querySelector("b");
    const label = el.querySelector("span");
    items.push({
      value: value ? value.textContent : "",
      label: label ? label.textContent : ""
    });
  });
  return items;
}

function reportPayload() {
  const digits = {
    altitude_press: 2, altitude_kalman: 2, altitude_gnss: 2,
    ascent_rate_kalman: 2, vz_press: 2,
    accX: 3, accY: 3, accZ: 3,
    gyroX: 1, gyroY: 1, gyroZ: 1,
    vbat_V: 3
  };
  const charts = [{
    id: "state",
    title: "Flight state",
    height: 56,
    states: view.segments.filter(function (segment) {
      return segment.t1 >= view.view0 && segment.t0 <= view.view1;
    }).map(function (segment) {
      return {
        t0: round3(flightTime(Math.max(segment.t0, view.view0))),
        t1: round3(flightTime(Math.min(segment.t1, view.view1))),
        name: stateName(segment.state),
        color: STATE_COLOR[Math.round(segment.state)] || "#3d4554"
      };
    })
  }];
  CHARTS.forEach(function (chart) {
    const series = [];
    chart.series.forEach(function (item) {
      if (!shown[chart.id + ":" + item.key]) return;
      const pts = decimateSeries(item.key, digits[item.key] == null ? 3 : digits[item.key]);
      if (!pts.length) return;
      series.push({ name: item.name, color: item.color, pts: pts });
    });
    if (!series.length) return;
    charts.push({
      id: chart.id,
      title: chart.title,
      unit: chart.unit,
      ticks: chart.ticks || "2",
      height: chart.height,
      zero: !!chart.zero,
      series: series
    });
  });
  return {
    t0: round3(flightTime(view.view0)),
    t1: round3(flightTime(view.view1)),
    cursor: round3(flightTime(view.cursor)),
    samples: decimateReadout(),
    charts: charts,
    apogees: (view.apogees || []).filter(function (item) {
      return item.t >= view.view0 && item.t <= view.view1;
    }).map(function (item) {
      return { t: round3(flightTime(item.t)), name: item.name, color: item.color };
    }),
    track: decimateTrack()
  };
}

function decimateSeries(key, digits) {
  const bins = new Map();
  const scale = Math.pow(10, digits);
  for (let i = 0; i < view.rows.length; i++) {
    const time = view.t[i];
    if (time < view.view0 || time > view.view1) continue;
    const value = view.rows[i][key];
    if (!Number.isFinite(value)) continue;
    const bin = Math.floor((time - view.view0) / 0.1);
    const sample = { t: flightTime(time), v: value };
    const slot = bins.get(bin);
    if (!slot) bins.set(bin, { min: sample, max: sample });
    else {
      if (value < slot.min.v) slot.min = sample;
      if (value > slot.max.v) slot.max = sample;
    }
  }
  const pts = [];
  Array.from(bins.keys()).sort(function (a, b) { return a - b; }).forEach(function (bin) {
    const slot = bins.get(bin);
    pts.push(slot.min);
    if (slot.max !== slot.min) pts.push(slot.max);
  });
  pts.sort(function (a, b) { return a.t - b.t || a.v - b.v; });
  return pts.map(function (point) {
    return [Math.round(point.t * 1000) / 1000, Math.round(point.v * scale) / scale];
  });
}

function decimateTrack() {
  const origin = firstFix();
  if (!origin) return [];
  const bins = new Map();
  for (let i = 0; i < view.rows.length; i++) {
    const time = view.t[i];
    if (time < view.view0 || time > view.view1) continue;
    const row = view.rows[i];
    if (!Number.isFinite(row.latitude) || !Number.isFinite(row.longitude)) continue;
    if (row.latitude === 0 && row.longitude === 0) continue;
    const bin = Math.floor((time - view.view0) / 0.1);
    const ne = northEast(origin.latitude, origin.longitude, row.latitude, row.longitude);
    bins.set(bin, [round3(flightTime(time)), Math.round(ne[1] * 10) / 10, Math.round(ne[0] * 10) / 10]);
  }
  return Array.from(bins.keys()).sort(function (a, b) { return a - b; }).map(function (bin) {
    return bins.get(bin);
  });
}

function round3(value) {
  return Math.round(value * 1000) / 1000;
}

function decimateReadout() {
  const bins = new Map();
  for (let i = 0; i < view.rows.length; i++) {
    const time = view.t[i];
    if (time < view.view0 || time > view.view1) continue;
    bins.set(Math.floor((time - view.view0) / 0.1), i);
  }
  return Array.from(bins.keys()).sort(function (a, b) { return a - b; }).map(function (bin) {
    const index = bins.get(bin);
    return { t: round3(flightTime(view.t[index])), pairs: samplePairs(view.rows[index], view.t[index]) };
  });
}

function reportFileName(name) {
  const base = String(name || "flight").replace(/\.csv$/i, "");
  return base + "-report.html";
}

function readoutPairs() {
  const pairs = [];
  const nodes = readoutEl.children;
  for (let i = 0; i < nodes.length - 1; i += 2) {
    pairs.push({ k: nodes[i].textContent, v: nodes[i + 1].textContent });
  }
  return pairs;
}

function reportHtml(report) {
  const stats = report.stats.map(function (item) {
    return "<div class=\"stat\"><b>" + escapeHtml(item.value) + "</b><span>" + escapeHtml(item.label) + "</span></div>";
  }).join("");
  const figures = report.charts.map(function (chart) {
    const legend = (chart.series || []).map(function (entry) {
      return "<span class=\"key\"><i style=\"background:" + escapeHtml(entry.color) + "\"></i>" + escapeHtml(entry.name) + "</span>";
    }).join("");
    return "<section class=\"chart\"><div class=\"chart-head\"><h2>" + escapeHtml(chart.title) + "</h2><div class=\"legend\">" +
      legend + "</div></div><canvas class=\"plot\" data-chart=\"" + escapeHtml(chart.id) + "\" data-h=\"" + chart.height + "\"></canvas></section>";
  }).join("");
  const readout = report.readout.map(function (pair) {
    return "<dt>" + escapeHtml(pair.k) + "</dt><dd>" + escapeHtml(pair.v) + "</dd>";
  }).join("");
  const events = report.events.map(function (item) {
    return "<li><time>" + escapeHtml(formatSeconds(flightTime(item.t))) + "</time><span>" + escapeHtml(item.label) + "</span></li>";
  }).join("");
  return "<!DOCTYPE html>\n<html lang=\"en\"><head><meta charset=\"utf-8\">" +
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" +
    "<title>PTR flight log — " + escapeHtml(report.name) + "</title><style>" +
    reportCss() +
    "</style></head><body>" +
    "<header class=\"top\"><div class=\"brand\"><span class=\"mark\">PTR</span><div><h1>Flight log</h1><p>" +
    escapeHtml(report.name) + "</p></div></div><p class=\"hint\">Saved view " + escapeHtml(report.range) + "</p></header>" +
    "<div id=\"status\">" + escapeHtml(report.status) + "</div>" +
    "<section id=\"stats\">" + stats + "</section>" +
    "<div id=\"layout\"><div id=\"charts\">" + figures + "</div><aside>" +
    "<h2>At cursor</h2><dl id=\"readout\">" + readout + "</dl>" +
    "<h2>Ground track</h2><canvas id=\"track\" data-h=\"280\"></canvas>" +
    "<p class=\"hint\">North up, metres from the first sample. Traces are 10 Hz, keeping the min and max in each 0.1 s.</p>" +
    "<h2>Events</h2><ol id=\"events\">" + (events || "<li><span>None in this window</span></li>") + "</ol>" +
    "</aside></div><script id=\"flight-data\" type=\"application/json\">" + report.data + "</script><script>" +
    reportScript() + "</script></body></html>";
}

function reportCss() {
  return ":root{color-scheme:dark;--bg:#121418;--panel:#191d24;--line:#2c3340;--text:#e7ecf3;--muted:#8b95a8;--amber:#e0a15c;--amber-ink:#2a2116;--mono:Consolas,ui-monospace,monospace;--sans:'Segoe UI',system-ui,sans-serif}" +
    "*{box-sizing:border-box}html,body{margin:0;background:var(--bg);color:var(--text);font-family:var(--sans)}" +
    "body{display:flex;flex-direction:column;min-height:100%}" +
    ".top{display:flex;justify-content:space-between;gap:1rem;align-items:center;padding:0.75rem 1rem;border-bottom:1px solid var(--line);background:#161a20}" +
    ".brand{display:flex;gap:0.75rem;align-items:center;min-width:0}" +
    ".mark{font-family:var(--mono);font-weight:700;letter-spacing:0.08em;background:var(--amber);color:var(--amber-ink);padding:0.35rem 0.45rem;border-radius:4px}" +
    "h1,h2,p{margin:0}h1{font-size:1rem;font-weight:650}" +
    "h2{font-size:0.72rem;font-weight:650;letter-spacing:0.06em;text-transform:uppercase;color:var(--muted);margin:0.9rem 0 0.4rem}" +
    ".brand p,.hint{color:var(--muted)}.brand p{font-size:0.82rem}.hint{font-size:0.78rem;line-height:1.4}" +
    "#status{padding:0.35rem 1rem;font-family:var(--mono);font-size:0.78rem;border-bottom:1px solid var(--line);background:#14181e;color:var(--muted)}" +
    "#stats{display:grid;grid-template-columns:repeat(5,minmax(0,1fr));border-bottom:1px solid var(--line)}" +
    ".stat{padding:0.65rem 1rem;border-right:1px solid var(--line)}.stat:last-child{border-right:0}" +
    ".stat b{display:block;font-family:var(--mono);font-size:1.15rem;font-weight:650}.stat span{color:var(--muted);font-size:0.75rem}" +
    "#layout{display:grid;grid-template-columns:minmax(0,1fr) 320px}" +
    "#charts{padding:0.5rem 0.75rem 1.5rem}.chart{margin-top:0.35rem}" +
    ".chart-head{display:flex;justify-content:space-between;gap:0.75rem;align-items:baseline;margin-bottom:0.2rem}.chart-head h2{margin:0}" +
    ".legend{display:flex;flex-wrap:wrap;gap:0.55rem 0.8rem;justify-content:flex-end}" +
    ".key{display:flex;align-items:center;gap:0.3rem;font-size:0.75rem;color:var(--muted)}" +
    ".key i{width:0.7rem;height:0.7rem;border-radius:2px;display:inline-block}" +
    "#charts canvas,#track{width:100%;display:block;background:var(--panel);border:1px solid var(--line);border-radius:4px}#charts canvas{cursor:crosshair}" +
    "aside{border-left:1px solid var(--line);padding:0.2rem 0.9rem 1.5rem;background:#161a20}" +
    "#readout{display:grid;grid-template-columns:auto 1fr;gap:0.15rem 0.7rem;margin:0;font-family:var(--mono);font-size:0.75rem}" +
    "#readout dt{color:var(--muted)}#readout dd{margin:0;text-align:right}" +
    "#events{list-style:none;margin:0;padding:0}" +
    "#events li{display:grid;grid-template-columns:4.8rem 1fr;gap:0.4rem;padding:0.35rem 0;border-bottom:1px solid var(--line);font-size:0.78rem}" +
    "#events time{font-family:var(--mono);color:var(--muted)}" +
    "@media(max-width:960px){#stats{grid-template-columns:repeat(2,minmax(0,1fr))}.stat{border-bottom:1px solid var(--line)}#layout{grid-template-columns:1fr}aside{border-left:0;border-top:1px solid var(--line)}}";
}

function reportScript() {
  return "var flight=JSON.parse(document.getElementById('flight-data').textContent);" +
    "function power2Step(span){if(!(span>0))return 1;var step=Math.pow(2,Math.ceil(Math.log2(span/8)));return step>0&&isFinite(step)?step:1;}" +
    "function decimalStep(span){if(!(span>0))return 1;var rough=span/8;var power=Math.pow(10,Math.floor(Math.log10(rough)));var fraction=rough/power;var nice=fraction<=1?1:fraction<=2?2:fraction<=5?5:10;var step=nice*power;return step>0&&isFinite(step)?step:1;}" +
    "function formatTick(value,step){var quantum=step>0?step:1;if(quantum>=1)return String(Math.round(value));var decimals=Math.min(4,Math.max(0,Math.ceil(-Math.log10(quantum)-1e-9)));return value.toFixed(decimals);}" +
    "function formatSeconds(value){if(!isFinite(value))return '';var sign=value<0?'-':'';var abs=Math.abs(value);if(abs<60)return sign+abs.toFixed(abs<10?2:1)+' s';var minutes=Math.floor(abs/60);var seconds=abs-minutes*60;return sign+minutes+':'+String(Math.floor(seconds)).padStart(2,'0');}" +
    "function niceStep(rough){if(!(rough>0))return 1;var power=Math.pow(10,Math.floor(Math.log10(rough)));var fraction=rough/power;var nice=fraction<1.5?1:fraction<3.5?2:fraction<7.5?5:10;return nice*power;}" +
    "function boxOf(canvas){var dpr=window.devicePixelRatio||1;var height=Number(canvas.dataset.h)||180;canvas.style.width='100%';canvas.style.height=height+'px';var width=canvas.clientWidth||(canvas.parentElement&&canvas.parentElement.clientWidth)||640;canvas.width=Math.round(width*dpr);canvas.height=Math.round(height*dpr);var ctx=canvas.getContext('2d');ctx.setTransform(dpr,0,0,dpr,0,0);return {ctx:ctx,width:width,height:height,left:56,right:Math.max(width-12,57),top:10,bottom:height-22};}" +
    "function xOf(box,time){return box.left+(time-flight.t0)/(flight.t1-flight.t0)*(box.right-box.left);}" +
    "function yOf(box,value,y0,y1){return box.bottom-(value-y0)/(y1-y0)*(box.bottom-box.top);}" +
    "function timeAxis(box){var ctx=box.ctx;ctx.fillStyle='#8b95a8';ctx.font='11px Consolas,monospace';ctx.textAlign='center';ctx.textBaseline='top';var span=flight.t1-flight.t0;var step=niceStep(span/6);var tick=Math.ceil(flight.t0/step-1e-9)*step;for(;tick<=flight.t1+step*1e-6;tick+=step){var x=xOf(box,tick);if(x<box.left-1||x>box.right+1)continue;ctx.fillText(formatSeconds(tick),x,box.bottom+4);}}" +
    "function cursorLine(box){if(!isFinite(flight.cursor))return;var ctx=box.ctx;var x=xOf(box,Math.min(flight.t1,Math.max(flight.t0,flight.cursor)));ctx.save();ctx.strokeStyle='#e7ecf3';ctx.setLineDash([]);ctx.lineWidth=1.5;ctx.beginPath();ctx.moveTo(x,box.top);ctx.lineTo(x,box.bottom);ctx.stroke();ctx.restore();}" +
    "function drawState(canvas,chart){var box=boxOf(canvas);var ctx=box.ctx;chart.states.forEach(function(segment){var x0=xOf(box,Math.max(segment.t0,flight.t0));var x1=xOf(box,Math.min(segment.t1,flight.t1));if(x1<=box.left||x0>=box.right)return;ctx.fillStyle=segment.color;ctx.fillRect(Math.max(box.left,x0),box.top,Math.min(box.right,x1)-Math.max(box.left,x0),box.bottom-box.top);});ctx.save();ctx.beginPath();ctx.rect(box.left,box.top,box.right-box.left,box.bottom-box.top);ctx.clip();ctx.fillStyle='#121418';ctx.font='12px Segoe UI,sans-serif';ctx.textBaseline='top';chart.states.forEach(function(segment){var x0=Math.max(box.left,xOf(box,Math.max(segment.t0,flight.t0)));var x1=Math.min(box.right,xOf(box,Math.min(segment.t1,flight.t1)));if(x1-x0<ctx.measureText(segment.name).width+12)return;ctx.fillText(segment.name,x0+6,box.top+6);});ctx.restore();timeAxis(box);cursorLine(box);}" +
    "function drawSeries(canvas,chart){var box=boxOf(canvas);var ctx=box.ctx;var y0=Infinity,y1=-Infinity;chart.series.forEach(function(series){series.pts.forEach(function(pt){if(pt[1]<y0)y0=pt[1];if(pt[1]>y1)y1=pt[1];});});if(!isFinite(y0)||!isFinite(y1)){y0=0;y1=1;}if(y0===y1){y0-=1;y1+=1;}var pad=(y1-y0)*0.06;y0-=pad;y1+=pad;if(chart.zero){y0=Math.min(y0,0);y1=Math.max(y1,0);}var step=chart.ticks==='10'?decimalStep(y1-y0):power2Step(y1-y0);ctx.lineWidth=1;ctx.font='11px Consolas,monospace';ctx.textAlign='right';ctx.textBaseline='middle';var first=Math.ceil(y0/step-1e-9),last=Math.floor(y1/step+1e-9);for(var i=first;i<=last;i++){var value=i*step;var y=yOf(box,value,y0,y1);ctx.strokeStyle=Math.abs(value)<step*1e-6?'#465066':'#2c3340';ctx.setLineDash([]);ctx.beginPath();ctx.moveTo(box.left,y);ctx.lineTo(box.right,y);ctx.stroke();ctx.fillStyle='#8b95a8';ctx.fillText(formatTick(value,step),box.left-6,y);}ctx.save();ctx.beginPath();ctx.rect(box.left,box.top,box.right-box.left,box.bottom-box.top);ctx.clip();chart.series.forEach(function(series){ctx.beginPath();ctx.strokeStyle=series.color;ctx.lineWidth=1.6;ctx.setLineDash([]);var drawing=false,previous=null;series.pts.forEach(function(pt){if(previous!=null&&pt[0]-previous>0.25)drawing=false;var x=xOf(box,pt[0]);var y=yOf(box,pt[1],y0,y1);if(!drawing)ctx.moveTo(x,y);else ctx.lineTo(x,y);drawing=true;previous=pt[0];});ctx.stroke();});if(chart.id==='alt'){flight.apogees.forEach(function(item,index){var x=xOf(box,item.t);ctx.strokeStyle=item.color;ctx.lineWidth=1;ctx.setLineDash([3,3]);ctx.beginPath();ctx.moveTo(x,box.top);ctx.lineTo(x,box.bottom);ctx.stroke();ctx.setLineDash([]);ctx.fillStyle=item.color;ctx.font='11px Segoe UI,sans-serif';ctx.textAlign='left';ctx.textBaseline='top';ctx.fillText(item.name,x+4,box.top+2+index*13);});}ctx.restore();ctx.fillStyle='#8b95a8';ctx.font='11px Consolas,monospace';ctx.textAlign='left';ctx.textBaseline='top';ctx.fillText(chart.unit,box.left+4,box.top);timeAxis(box);cursorLine(box);}" +
    "function drawTrack(canvas){var box=boxOf(canvas);var ctx=box.ctx;var pts=flight.track;if(!pts.length){ctx.fillStyle='#8b95a8';ctx.font='12px Segoe UI,sans-serif';ctx.fillText('No GNSS position',12,24);return;}var max=10;pts.forEach(function(pt){max=Math.max(max,Math.abs(pt[1]),Math.abs(pt[2]));});max*=1.15;var left=28,top=12,size=Math.min(box.width-left-12,box.height-top-22);function px(e){return left+(e+max)/(2*max)*size;}function py(n){return top+(max-n)/(2*max)*size;}ctx.strokeStyle='#2c3340';ctx.strokeRect(left,top,size,size);ctx.beginPath();ctx.moveTo(px(0),top);ctx.lineTo(px(0),top+size);ctx.moveTo(left,py(0));ctx.lineTo(left+size,py(0));ctx.stroke();ctx.beginPath();ctx.strokeStyle='#e0a15c';ctx.lineWidth=1.6;pts.forEach(function(pt,index){if(index===0)ctx.moveTo(px(pt[1]),py(pt[2]));else ctx.lineTo(px(pt[1]),py(pt[2]));});ctx.stroke();var here=pts[0],best=Math.abs(pts[0][0]-flight.cursor);for(var i=1;i<pts.length;i++){var d=Math.abs(pts[i][0]-flight.cursor);if(d<best){best=d;here=pts[i];}}ctx.fillStyle='#e7ecf3';ctx.beginPath();ctx.arc(px(here[1]),py(here[2]),3.5,0,Math.PI*2);ctx.fill();ctx.fillStyle='#8b95a8';ctx.font='11px Consolas,monospace';ctx.fillText('N '+here[2].toFixed(0)+' m   E '+here[1].toFixed(0)+' m',left,top+size+14);}" +
    "function updateReadout(){var rows=flight.samples||[];if(!rows.length)return;var best=rows[0],bestD=Math.abs(rows[0].t-flight.cursor);for(var i=1;i<rows.length;i++){var d=Math.abs(rows[i].t-flight.cursor);if(d<bestD){best=rows[i];bestD=d;}}var dl=document.getElementById('readout');dl.innerHTML=best.pairs.map(function(pair){return '<dt>'+esc(pair[0])+'</dt><dd>'+esc(pair[1])+'</dd>';}).join('');}" +
    "function esc(value){return String(value).replace(/&/g,'&amp;').replace(/</g,'&lt;');}" +
    "function timeAt(canvas,event){var rect=canvas.getBoundingClientRect();var x=event.clientX-rect.left;if(x<56||x>rect.width-12)return null;return flight.t0+(x-56)/(rect.width-68)*(flight.t1-flight.t0);}" +
    "function draw(){var charts={};flight.charts.forEach(function(chart){charts[chart.id]=chart;});document.querySelectorAll('canvas.plot').forEach(function(canvas){var chart=charts[canvas.dataset.chart];if(!chart)return;if(chart.id==='state')drawState(canvas,chart);else drawSeries(canvas,chart);});drawTrack(document.getElementById('track'));updateReadout();}" +
    "draw();requestAnimationFrame(draw);window.addEventListener('resize',draw);document.querySelectorAll('canvas.plot').forEach(function(canvas){canvas.addEventListener('mousemove',function(event){var time=timeAt(canvas,event);if(time==null)return;flight.cursor=Math.min(flight.t1,Math.max(flight.t0,time));draw();});});";
}

function escapeHtml(value) {
  return String(value == null ? "" : value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function detectLiftoff(rows, t) {
  for (let i = 1; i < rows.length; i++) {
    const prev = rows[i - 1].accX;
    const cur = rows[i].accX;
    if (!Number.isFinite(prev) || !Number.isFinite(cur)) continue;
    if (prev < 2 && cur >= 2.5 && cur - prev >= 1.5) {
      const frac = (2.5 - prev) / (cur - prev);
      return { t: t[i - 1] + frac * (t[i] - t[i - 1]), found: true };
    }
  }
  return { t: t[0], found: false };
}

function flightTime(logTime) {
  return logTime - (Number.isFinite(view.liftoff) ? view.liftoff : 0);
}

function power2Step(span) {
  if (!(span > 0)) return 1;
  const step = Math.pow(2, Math.ceil(Math.log2(span / 8)));
  return step > 0 && Number.isFinite(step) ? step : 1;
}

function decimalStep(span) {
  if (!(span > 0)) return 1;
  const rough = span / 8;
  const power = Math.pow(10, Math.floor(Math.log10(rough)));
  const fraction = rough / power;
  const nice = fraction <= 1 ? 1 : fraction <= 2 ? 2 : fraction <= 5 ? 5 : 10;
  const step = nice * power;
  return step > 0 && Number.isFinite(step) ? step : 1;
}

function niceStep(rough) {
  if (!(rough > 0)) return 1;
  const power = Math.pow(10, Math.floor(Math.log10(rough)));
  const fraction = rough / power;
  const nice = fraction < 1.5 ? 1 : fraction < 3.5 ? 2 : fraction < 7.5 ? 5 : 10;
  return nice * power;
}

function formatTick(value, step) {
  const quantum = step > 0 ? step : 1;
  if (quantum >= 1) return String(Math.round(value));
  const decimals = Math.min(4, Math.max(0, Math.ceil(-Math.log10(quantum) - 1e-9)));
  return value.toFixed(decimals);
}

function formatSeconds(value) {
  if (!Number.isFinite(value)) return "—";
  const sign = value < 0 ? "-" : "";
  const abs = Math.abs(value);
  if (abs < 60) return sign + abs.toFixed(abs < 10 ? 2 : 1) + " s";
  const minutes = Math.floor(abs / 60);
  const seconds = abs - minutes * 60;
  return sign + minutes + ":" + String(Math.floor(seconds)).padStart(2, "0");
}

function metres(value) {
  return Number.isFinite(value) ? value.toFixed(1) + " m" : "—";
}

function metresPerSec(value) {
  return Number.isFinite(value) ? value.toFixed(1) + " m/s" : "—";
}

function triple(x, y, z, digits) {
  if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(z)) return "—";
  return [x, y, z].map(function (value) { return value.toFixed(digits); }).join("  ");
}

function igniterLine(row) {
  const parts = [];
  for (let n = 1; n <= 4; n++) {
    const state = row["ign" + n + "_state"];
    const cont = row["ign" + n + "_cont"];
    if (state === 1) parts.push(n + " fire");
    else if (cont === 1) parts.push(n + " cont");
  }
  return parts.length ? parts.join("  ") : "open";
}

function clamp(value, lo, hi) {
  return Math.min(hi, Math.max(lo, value));
}
