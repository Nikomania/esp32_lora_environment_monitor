async function fetchLatest() {
  const res = await fetch("/api/latest");
  return res.json();
}

function renderCards(rows) {
  const container = document.getElementById("cards");
  container.innerHTML = "";
  rows.forEach((r) => {
    const div = document.createElement("div");
    div.className = "card";
    div.innerHTML = `
      <strong>Nó ${r.node_id}</strong><br>
      Sala: ${r.room || "-"}<br>
      Temp: ${r.temp_c.toFixed(2)} °C<br>
      Hum: ${r.hum} %<br>
      Poeira: ${r.dust} µg/m³<br>
      Batt: ${r.batt} %<br>
      RSSI: ${r.rssi ?? "-"} dBm
    `;
    container.appendChild(div);
  });
}

async function loadLatest() {
  const rows = await fetchLatest();
  renderCards(rows);
}

// Chart
let tempChart = null;
async function loadHistory(room) {
  const res = await fetch(
    `/api/telemetry?room=${encodeURIComponent(room)}&limit=200`,
  );
  const rows = await res.json();
  const labels = rows.map((r) => new Date(r.ts).toLocaleString());
  const temps = rows.map((r) => r.temp_c);
  const hums = rows.map((r) => r.hum);

  const ctx = document.getElementById("tempChart").getContext("2d");
  if (tempChart) tempChart.destroy();
  tempChart = new Chart(ctx, {
    type: "line",
    data: {
      labels: labels,
      datasets: [
        {
          label: "Temperatura (°C)",
          data: temps,
          tension: 0.2,
          fill: false,
        },
      ],
    },
    options: {
      responsive: true,
      scales: { x: { display: true }, y: { display: true } },
    },
  });
}

document.getElementById("btn-load").addEventListener("click", () => {
  const room = document.getElementById("room-input").value.trim() || "Sala A";
  loadHistory(room);
});

// periodic refresh
loadLatest();
setInterval(loadLatest, 15_000);
