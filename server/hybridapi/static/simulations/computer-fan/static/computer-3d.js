/* -------- static/computer-3d.js --------------- */
/* NO import lines – we use the global BABYLON that the <script> tag provides */

window.addEventListener("DOMContentLoaded", () => {
  const canvas = document.getElementById("babylonCanvas");
  const engine = new BABYLON.Engine(canvas, true);
  const scene  = new BABYLON.Scene(engine);

  /* camera & light */
  const cam = new BABYLON.ArcRotateCamera(
    "cam", Math.PI / 2, Math.PI / 3, 10, BABYLON.Vector3.Zero(), scene);
  cam.attachControl(canvas, true);

  new BABYLON.HemisphericLight("light", new BABYLON.Vector3(1, 1, 0), scene);

  /* -------- load the model that sits next to this JS file -------- */
  const modelUrl = new URL("./cooler_master_cpu_cooler.glb", import.meta.url).href;

  let fanNode = null;                 // node we rotate each frame
  const FAN_NODE_NAME = "Blades";     // adjust if needed

  BABYLON.SceneLoader.Append(
    "",                                // empty rootUrl because modelUrl is absolute
    modelUrl,
    scene,
    () => {
      console.log("✅ Model loaded");
      fanNode = scene.getNodeByName(FAN_NODE_NAME) || scene.meshes[0];
    },
    null,
    (_, msg) => console.error("❌ GLB load failed:", msg)
  );

  /* -------- per–frame rotation driven by rpm -------- */
  let rpm = 600;                      // default until first event arrives
  let lastTime = performance.now();

  scene.onBeforeRenderObservable.add(() => {
    if (!fanNode) return;             // wait until the GLB is ready
    const now = performance.now();
    const dt  = (now - lastTime) / 1000;
    lastTime  = now;

    const radPerSec = rpm * Math.PI * 2 / 60;
    fanNode.rotation.y += radPerSec * dt;  // spin around Y axis
  });

  /* -------- listen for sim2web events to update rpm -------- */
  window.addEventListener("message", (ev) => {
    if (ev.source !== parent || ev.data.messageType !== "sim2web") return;
    const [, rpmStr] = ev.data.value.split(":");   // value = "temp:rpm"
    const r = parseInt(rpmStr, 10);
    if (!isNaN(r) && r != rpm) {
      console.log("Received RPM:", r);
      rpm = r;
    }
    
  }, false);

  /* -------- start render loop -------- */
  engine.runRenderLoop(() => scene.render());
  window.addEventListener("resize", () => engine.resize());
});
