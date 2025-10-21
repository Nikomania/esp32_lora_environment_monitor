const express = require("express");
const bodyParser = require("body-parser");
const sqlite3 = require("sqlite3").verbose();
const path = require("path");

const app = express();
const PORT = process.env.PORT || 3000;

// middlewares
app.use(bodyParser.json({ limit: "1mb" }));
app.use(express.static(path.join(__dirname, "public")));

// DB
const db = new sqlite3.Database(path.join(__dirname, "telemetry.db"));

db.serialize(() => {
  db.run(`CREATE TABLE IF NOT EXISTS telemetry (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts TEXT,
    node_id INTEGER,
    seq INTEGER,
    temp_c REAL,
    hum INTEGER,
    dust INTEGER,
    batt INTEGER,
    rssi REAL,
    snr REAL,
    room TEXT,
    raw_base64 TEXT
  )`);
});

// Helper: decode payload (base64 -> Buffer -> fields)
function decodePayload(buf) {
  if (!Buffer.isBuffer(buf) || buf.length < 8) return null;
  return {
    node_id: buf.readUInt8(0),
    seq: buf.readUInt8(1),
    temp_c: buf.readInt16BE(2) / 100.0,
    hum: buf.readUInt8(4),
    dust: buf.readUInt16BE(5),
    batt: buf.readUInt8(7),
  };
}

// Endpoint to ingest from gateway
app.post("/gateway", (req, res) => {
  try {
    const {
      payload_base64,
      gateway_id,
      rssi, // Received Signal Strength Indicator
      snr, // Signal-to-Noise Ratio
      room,
      timestamp,
    } = req.body;
    if (!payload_base64)
      return res.status(400).json({ error: "payload_base64 required" });

    const buf = Buffer.from(payload_base64, "base64");
    const decoded = decodePayload(buf);
    if (!decoded) return res.status(400).json({ error: "invalid payload" });

    const ts = timestamp || new Date().toISOString();

    const stmt = db.prepare(`INSERT INTO telemetry
      (ts, node_id, seq, temp_c, hum, dust, batt, rssi, snr, room, raw_base64)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`);

    stmt.run(
      ts,
      decoded.node_id,
      decoded.seq,
      decoded.temp_c,
      decoded.hum,
      decoded.dust,
      decoded.batt,
      rssi,
      snr,
      room,
      payload_base64,
      function (err) {
        if (err) {
          console.error(err);
          return res.status(500).json({ error: "db insert failed" });
        }
        res.json({ ok: true, id: this.lastID });
      },
    );
    stmt.finalize();
  } catch (e) {
    console.error(e);
    res.status(500).json({ error: "internal" });
  }
});

// API: latest per node
app.get("/api/latest", (req, res) => {
  const sql = `SELECT * FROM telemetry t1
    WHERE id IN (
      SELECT MAX(id) FROM telemetry GROUP BY node_id
    ) ORDER BY ts DESC`;
  db.all(sql, [], (err, rows) => {
    if (err) return res.status(500).json({ error: "db error" });
    res.json(rows);
  });
});

// API: historical
app.get("/api/telemetry", (req, res) => {
  const room = req.query.room;
  const limit = parseInt(req.query.limit || "200", 10);
  let sql = "SELECT * FROM telemetry";
  const params = [];
  if (room) {
    sql += " WHERE room = ?";
    params.push(room);
  }
  sql += " ORDER BY ts DESC LIMIT ?";
  params.push(limit);

  db.all(sql, params, (err, rows) => {
    if (err) return res.status(500).json({ error: "db error" });
    res.json(rows.reverse()); // envia em ordem cronológica
  });
});

app.listen(PORT, () =>
  console.log(`Server listening on http://localhost:${PORT}`),
);
