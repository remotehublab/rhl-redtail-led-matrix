/* ------------ static/computer-fan.js ---------------- */
import kaboom from
  "https://cdn.jsdelivr.net/npm/kaboom@3000.1.15/dist/kaboom.mjs";

/* Mount inside the div so the canvas never hides your buttons */
kaboom({
  width: 640,
  height: 40,
  background: [15, 15, 25],
  root: document.getElementById("kaboomCanvasContainer"),

  stretch: true,      // let the canvas grow with its container
  letterbox: true,    // letterbox the canvas to fit the container
});

/* ---------- colours ---------- */
const GREEN  = rgb(0, 252, 0);
const YELLOW = rgb(255, 215, 0);
const RED    = rgb(255, 60, 60);
const LABEL  = rgb(190, 190, 190);

/* ---------- live values ---------- */
let temp = 20;
let rpm  = 600;

const colourFor = (t) => (t < 70 ? GREEN : t < 90 ? YELLOW : RED);

/* ---------- layout ---------- */
const Y     = 20;   // vertical centre
const GAP   = 18;   // space between sections

const cpuLabel = add([ text("CPU", { size: 20 }), pos(20, Y), color(LABEL), anchor("left") ]);
const cpuVal   = add([ text("",    { size: 20 }), pos(0,  Y), color(GREEN), anchor("left") ]);

const divider  = add([ text("",   { size: 20 }), pos(0,  Y), color(LABEL), anchor("left") ]);

const fanLabel = add([ text("FAN", { size: 20 }), pos(0,  Y), color(LABEL), anchor("left") ]);
const fanVal   = add([ text("",    { size: 20 }), pos(0,  Y), color(GREEN), anchor("left") ]);

/* position-refresh helper */
function layout() {
  cpuVal.pos.x   = cpuLabel.pos.x + cpuLabel.width + 8;
  divider.pos.x  = cpuVal.pos.x   + cpuVal.width   + GAP;
  fanLabel.pos.x = divider.pos.x  + divider.width  + GAP;
  fanVal.pos.x   = fanLabel.pos.x + fanLabel.width + 8;
}

/* redraw numbers & colours */
function redraw() {
  cpuVal.text = `${temp} °C`;
  fanVal.text = `${rpm} RPM`;

  const c = colourFor(temp);
  cpuVal.color = c;
  fanVal.color = LABEL;

  layout();
}

/* expose setters for outer page */
export const setTemp = (n) => { temp = n; redraw(); };
export const setRPM  = (n) => { rpm  = n; redraw(); };

/* initial draw */
redraw();

/* ---------- receive updates from simulator ---------- */
window.addEventListener("message", (ev) => {
  if (ev.source !== parent || ev.data.messageType !== "sim2web") return;
  console.log("Received sim2web message:", ev.data);
  const [tStr, rStr] = ev.data.value.split(":");
  const t = parseInt(tStr, 10);
  const r = parseInt(rStr, 10);
  if (!isNaN(t) && !isNaN(r)) {
    temp = t; rpm = r; redraw();
  }
}, false);

/* ---------- send user input back ---------- */
document.addEventListener("change", (e) => {
  if (e.target.name === "state") {
    parent.postMessage({ messageType:"web2sim", version:"1.0", value:e.target.value }, "*");
  } else if (e.target.id === "reset") {
    parent.postMessage({ messageType:"web2sim", version:"1.0", value:e.target.checked?"1":"0" }, "*");
  }
});
