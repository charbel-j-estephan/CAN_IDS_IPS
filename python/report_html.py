"""Build the self-contained results page (report.html) for a replay run.

No internet needed: the data is embedded and the charts are drawn on canvas.
The table holds up to MAX_TABLE_ROWS frames; frames.csv next to the page always
holds every frame.
"""
import json

import numpy as np

MAX_TABLE_ROWS = 150000
BINS = 100

PAGE = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>CAN replay results</title>
<style>
:root{--bg:#f7f7f5;--panel:#fff;--ink:#1c1c1a;--muted:#5f5f59;--line:#d9d8d2;--accent:#2f6fdc;--ok:#1f8a52;--bad:#c23b2b;--warn:#b7791f;--row:#f1f1ed}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--bg:#151514;--panel:#1f1f1d;--ink:#ecebe6;--muted:#a3a39b;--line:#3a3a36;--accent:#7aa2f0;--ok:#5cc48a;--bad:#ff7a66;--warn:#e8a64a;--row:#262624}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
main{max-width:1100px;margin:0 auto;padding:24px 16px 48px}
h1{font-size:22px;margin:0 0 2px}h2{font-size:16px;margin:0 0 8px}
.sub{color:var(--muted);margin:0 0 18px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:12px;padding:14px}
.grid{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));margin-bottom:14px}
.kpi b{display:block;font-size:26px;line-height:1.2}.kpi span{color:var(--muted);font-size:14px}
.ok{color:var(--ok)}.bad{color:var(--bad)}.warn{color:var(--warn)}
.two{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(420px,1fr));margin-bottom:14px}
canvas{width:100%;height:240px;display:block}
table.small{border-collapse:collapse;width:100%}
table.small td,table.small th{padding:5px 8px;text-align:left;border-bottom:1px solid var(--line);font-size:14px}
table.small th{color:var(--muted);font-weight:600}
.tools{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:10px;align-items:center}
select,input{font:inherit;padding:6px 8px;border-radius:8px;border:1px solid var(--line);background:var(--panel);color:var(--ink)}
.vt{height:460px;overflow:auto;border:1px solid var(--line);border-radius:8px;position:relative;background:var(--panel)}
.vt .sp{position:relative}
.vt .r{position:absolute;left:0;right:0;height:28px;display:grid;grid-template-columns:70px 100px 60px 44px 1fr 100px 80px 110px;align-items:center;padding:0 8px;font:13px ui-monospace,Consolas,monospace;border-bottom:1px solid var(--line)}
.vt .r:nth-child(odd){background:var(--row)}
.vt .hd{position:sticky;top:0;z-index:2;background:var(--panel);font-weight:600;color:var(--muted);height:30px}
.pill{padding:1px 8px;border-radius:99px;font-size:12px}
.pill.a{background:color-mix(in srgb,var(--ok) 18%,transparent);color:var(--ok)}
.pill.l{background:color-mix(in srgb,var(--bad) 18%,transparent);color:var(--bad)}
.note{color:var(--muted);font-size:14px;margin-top:8px}
@media (max-width:560px){.two{grid-template-columns:1fr}.vt .r{grid-template-columns:56px 80px 50px 36px 1fr 90px}.vt .r span:nth-child(n+7){display:none}}
</style></head><body><main>
<h1 id="title"></h1><p class="sub" id="sub"></p>
<div class="grid" id="kpis"></div>
<div class="two">
  <div class="card"><h2>Frames over the capture</h2><canvas id="c1"></canvas>
    <div class="note" id="n1"></div></div>
  <div class="card"><h2 id="h2b">Timing against the original capture</h2><canvas id="c2"></canvas>
    <div class="note" id="n2"></div></div>
</div>
<div class="card" style="margin-bottom:14px"><h2>By class</h2><table class="small" id="cls"></table></div>
<div class="card" style="margin-bottom:14px" id="nodes_card"><h2>Boards on the bus</h2><table class="small" id="nodes"></table>
  <div class="note">late: frames handed to the CAN controller more than 100 µs after their time. tec / rec: transmit / receive error counters (error passive above 127, bus off above 255). lost arbitration: another node with a lower ID won the bus.</div></div>
<div class="card"><h2>Every frame</h2>
  <div class="tools">
    <select id="fr"><option value="all">All results</option><option value="a">Arrived</option><option value="l">Lost</option></select>
    <select id="fc"></select>
    <input id="fi" placeholder="CAN ID, e.g. 316" size="14">
    <span id="cnt" class="note"></span>
  </div>
  <div class="vt" id="vt"><div class="sp" id="sp"></div></div>
  <div class="note" id="tn"></div>
</div>
</main>
<script>
const D = __DATA__;
const $ = id => document.getElementById(id);
const css = n => getComputedStyle(document.documentElement).getPropertyValue(n).trim();
const S = D.summary;
$("title").textContent = "CAN replay: " + S.file;
$("sub").textContent = S.started + " · " + S.mode + " · capture span " + S.capture_duration_s + " s";

const kp = [
  [S.frames_sent.toLocaleString(), "frames sent"],
  [S.frames_arrived.toLocaleString(), "frames received", "ok"],
  [S.frames_lost.toLocaleString(), "lost on the way", S.frames_lost ? "bad" : "ok"],
  [S.delivery_percent.toFixed(3) + "%", "delivered", S.frames_lost ? "warn" : "ok"]
];
if (S.timing_jitter_us) {
  kp.push([S.timing_jitter_us.p95 + " µs", "timing, 95% of frames within"]);
  kp.push([S.timing_jitter_us.max + " µs", "worst timing"]);
  kp.push([S.clock_drift_ppm + " ppm", "clock drift between boards (removed)"]);
}
$("kpis").innerHTML = kp.map(k => `<div class="card kpi"><b class="${k[2]||""}">${k[0]}</b><span>${k[1]}</span></div>`).join("");

let h = "<tr><th>Class</th><th>Sent</th><th>Arrived</th><th>Lost</th></tr>";
for (const [name, v] of Object.entries(S.by_class))
  h += `<tr><td>${name}</td><td>${v.sent.toLocaleString()}</td><td>${v.arrived.toLocaleString()}</td><td class="${v.lost?"bad":""}">${v.lost.toLocaleString()}</td></tr>`;
h += `<tr><td colspan="4" class="note">Sender: ${S.sender_report || "n/a"}<br>Receiver: ${S.receiver_report}</td></tr>`;
$("cls").innerHTML = h;

if (S.nodes) {
  const cols = [["sent", "sent"], ["rx", "heard"], ["late", "late"], ["maxlate", "worst late (µs)"],
                ["tec", "tec"], ["rec", "rec"], ["state", "state"], ["arblost", "lost arbitration"],
                ["txfail", "failed sends"], ["buserr", "bus errors"], ["drop", "dropped"]];
  const used = cols.filter(c => Object.values(S.nodes).some(n => n[c[0]] !== undefined));
  let t = "<tr><th>Board</th>" + used.map(c => `<th>${c[1]}</th>`).join("") + "</tr>";
  for (const [name, n] of Object.entries(S.nodes)) {
    t += `<tr><td>${name}</td>` + used.map(c => {
      const v = n[c[0]];
      const bad = (c[0] === "state" && v && v !== "running") || (["late", "txfail", "drop"].includes(c[0]) && v > 0);
      return `<td class="${bad ? "bad" : ""}">${v === undefined ? "" : (typeof v === "number" ? v.toLocaleString() : v)}</td>`;
    }).join("") + "</tr>";
  }
  if (S.attacker_sync_offset_us !== undefined)
    t += `<tr><td colspan="${used.length + 1}" class="note">Attacker sends ${S.attacker_sync_offset_us} µs from the sender's clock (frames with a clear bus before them, + is late), its clock drifts ${S.attacker_clock_drift_ppm} ppm.</td></tr>`;
  $("nodes").innerHTML = t;
} else {
  $("nodes_card").style.display = "none";
}

function setup(c) {
  const r = c.getBoundingClientRect(), d = window.devicePixelRatio || 1;
  c.width = r.width * d; c.height = r.height * d;
  const g = c.getContext("2d"); g.scale(d, d);
  return [g, r.width, r.height];
}
function axes(g, W, H, ml, mb, ymax, xlab, ylab) {
  g.font = "12px system-ui"; g.fillStyle = css("--muted"); g.strokeStyle = css("--line"); g.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const y = H - mb - (H - mb - 10) * i / 4;
    g.beginPath(); g.moveTo(ml, y); g.lineTo(W - 8, y); g.stroke();
    g.textAlign = "right"; g.fillText(Math.round(ymax * i / 4).toLocaleString(), ml - 6, y + 4);
  }
  g.textAlign = "center"; g.fillText(xlab, (ml + W) / 2, H - 4);
  g.save(); g.translate(12, H / 2); g.rotate(-Math.PI / 2); g.fillText(ylab, 0, 0); g.restore();
}
function drawTimeline() {
  const [g, W, H] = setup($("c1")), B = D.bins, ml = 52, mb = 34, n = B.sent.length;
  const ymax = Math.max(1, ...B.sent, ...B.recv);
  axes(g, W, H, ml, mb, ymax, "seconds into the capture (0 to " + B.span.toFixed(2) + ")", "frames per bin");
  const bw = (W - ml - 8) / n;
  const bar = (arr, col, off, wid) => { g.fillStyle = col; arr.forEach((v, i) => {
    const h2 = (H - mb - 10) * v / ymax; g.fillRect(ml + i * bw + off, H - mb - h2, wid, h2); }); };
  bar(B.sent, css("--line"), 0, Math.max(1, bw));
  bar(B.recv, css("--accent"), 0, Math.max(1, bw * 0.6));
  bar(B.lost, css("--bad"), bw * 0.6, Math.max(1, bw * 0.4));
  $("n1").innerHTML = "Grey: frames sent. Blue: frames received. Red: lost. Each bar is " + (B.span / n).toFixed(3) + " s.";
}
function drawHist() {
  if (!D.hist) { $("h2b").textContent = "Timing"; const [g, W, H] = setup($("c2")); g.fillStyle = css("--muted");
    g.font = "14px system-ui"; g.textAlign = "center"; g.fillText("Fast mode: timing was not replayed", W / 2, H / 2);
    $("n2").textContent = ""; return; }
  const [g, W, H] = setup($("c2")), hs = D.hist, ml = 52, mb = 34, n = hs.counts.length;
  const ymax = Math.max(1, ...hs.counts);
  axes(g, W, H, ml, mb, ymax, "offset from the original timing (µs), drift removed", "frames");
  const bw = (W - ml - 8) / n;
  hs.counts.forEach((v, i) => { const h2 = (H - mb - 10) * v / ymax;
    g.fillStyle = Math.abs((hs.lo + (i + 0.5) * hs.step)) > 1000 ? css("--warn") : css("--accent");
    g.fillRect(ml + i * bw, H - mb - h2, Math.max(1, bw - 1), h2); });
  g.fillStyle = css("--muted"); g.textAlign = "left"; g.fillText(Math.round(hs.lo), ml, H - mb + 14);
  g.textAlign = "right"; g.fillText(Math.round(hs.lo + n * hs.step), W - 8, H - mb + 14);
  $("n2").textContent = "Each frame's time on the bus compared with its time in the original capture. The steady drift between the two boards' clocks is removed first. Orange bars are more than 1 ms off.";
}

// ---- table of every frame ----
const R = D.rows;           // [i, capture_s, id, dlc, data, class, arrived, err_us|null]
const cls = D.classes;
const fc = $("fc"); fc.innerHTML = '<option value="-1">All classes</option>' + cls.map((c, i) => `<option value="${i}">${c}</option>`).join("");
let idx = [];
function filter() {
  const fr = $("fr").value, c = +fc.value, q = $("fi").value.trim().toUpperCase();
  const qi = q ? parseInt(q, 16) : null;
  idx = [];
  for (let i = 0; i < R.length; i++) {
    const r = R[i];
    if (fr === "a" && !r[6]) continue; if (fr === "l" && r[6]) continue;
    if (c >= 0 && r[5] !== c) continue;
    if (qi !== null && !isNaN(qi) && r[2] !== qi) continue;
    idx.push(i);
  }
  $("cnt").textContent = idx.length.toLocaleString() + " frames shown";
  $("sp").style.height = (idx.length * 28 + 30) + "px";
  $("vt").scrollTop = 0; render();
}
function render() {
  const vt = $("vt"), top = vt.scrollTop, first = Math.max(0, Math.floor((top - 30) / 28) - 3), last = Math.min(idx.length, first + 25);
  let h = '<div class="r hd" style="top:0"><span>#</span><span>time (s)</span><span>ID</span><span>DLC</span><span>data</span><span>class</span><span>result</span><span>error (µs)</span></div>';
  for (let k = first; k < last; k++) {
    const r = R[idx[k]];
    h += `<div class="r" style="top:${30 + k * 28}px"><span>${r[0]}</span><span>${r[1].toFixed(6).slice(-10)}</span><span>${r[2].toString(16).toUpperCase().padStart(3, "0")}</span><span>${r[3]}</span><span>${r[4]}</span><span>${cls[r[5]]}</span><span><i class="pill ${r[6] ? "a" : "l"}">${r[6] ? "arrived" : "lost"}</i></span><span>${r[7] === null ? "" : r[7].toFixed(0)}</span></div>`;
  }
  $("sp").innerHTML = h;
}
$("vt").addEventListener("scroll", render);
["fr", "fc"].forEach(i => $(i).addEventListener("change", filter));
$("fi").addEventListener("input", filter);
$("tn").textContent = D.truncated ? "The table shows the first " + R.length.toLocaleString() + " of " + S.frames_sent.toLocaleString() + " frames. frames.csv next to this page has all of them." : "This table holds all " + R.length.toLocaleString() + " frames. frames.csv next to this page has the same data.";
function all() { drawTimeline(); drawHist(); }
window.addEventListener("resize", all);
all(); filter();
</script></body></html>
"""


def write(path, summary, rows, res, class_names):
    n = len(rows)
    t0 = rows[0][0]
    span = max(rows[-1][0] - t0, 1e-9)
    cap = np.array([r[0] - t0 for r in rows])
    arrived = np.array(res["arrived"], dtype=bool)

    sent_b, _ = np.histogram(cap, bins=BINS, range=(0, span))
    lost_b, _ = np.histogram(cap[~arrived], bins=BINS, range=(0, span))
    # Received frames are placed by when they were SENT in the capture, which is
    # what makes a lost stretch visible as a gap in blue and a spike in red.
    recv_b, _ = np.histogram(cap[arrived], bins=BINS, range=(0, span))
    bins = {"sent": sent_b.tolist(), "recv": recv_b.tolist(), "lost": lost_b.tolist(), "span": span}

    hist = None
    err = np.array([e for e in res.get("jitter", res["err"]) if e is not None])
    if res["realtime"] and len(err):
        lim = max(float(np.percentile(np.abs(err), 99.5)), 50.0)
        counts, edges = np.histogram(np.clip(err, -lim, lim), bins=40, range=(-lim, lim))
        hist = {"counts": counts.tolist(), "lo": float(edges[0]), "step": float(edges[1] - edges[0])}

    keep = min(n, MAX_TABLE_ROWS)
    table = []
    for i in range(keep):
        r = rows[i]
        e = res["err"][i]
        table.append([i, round(r[0] - t0, 6), r[1], r[2],
                      " ".join("%02X" % b for b in r[3:3 + r[2]]), r[11],
                      1 if res["arrived"][i] else 0, None if e is None else round(e, 1)])

    data = {"summary": summary, "rows": table, "bins": bins, "hist": hist,
            "classes": class_names, "truncated": keep < n}
    blob = json.dumps(data, separators=(",", ":")).replace("</", "<\\/")
    with open(path, "w", encoding="utf-8") as f:
        f.write(PAGE.replace("__DATA__", blob))
