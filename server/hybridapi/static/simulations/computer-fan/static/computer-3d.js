/* -------- static/computer-3d.js --------------- */
/* NO import lines – we use the global BABYLON that the <script> tag provides */

window.addEventListener("DOMContentLoaded", () => {
  const canvas  = document.getElementById("babylonCanvas");
  const engine  = new BABYLON.Engine(canvas, true);
  const scene   = new BABYLON.Scene(engine);
  scene.debugLayer.show({ embedMode: true });

  /* camera & light */
  const cam = new BABYLON.ArcRotateCamera(
    "cam",
    Math.PI / 2,           // α – side-on view
    Math.PI / 5,           // β – slight downward tilt
    0.5,                  // *much* shorter radius ⇒ zoom-in
    BABYLON.Vector3.Zero(),// look-at point (temp: origin)
    scene
  );
  cam.attachControl(canvas, true);

  /* let the user zoom even closer (or back out a bit) */
  cam.lowerRadiusLimit = 0.05;   // how close they can scroll-wheel
  cam.upperRadiusLimit = 2.0;    // how far out they can scroll-wheel

  /* avoid near-plane clipping now that we’re so close */
  cam.minZ = 0.01;

  new BABYLON.HemisphericLight("light", new BABYLON.Vector3(1, 1, 0), scene);

  /* -------- load the model -------- */
  const modelUrl = new URL("./liquid cooler.obj", import.meta.url).href;

  /* fan IDs we care about */
  const FAN_NODE_NAMES = [
    "FAN.001_Cylinder.033",
    "FAN.002_Cylinder.034",
    "FAN_Cylinder.012",
  ];

  /* after load we’ll store { node, axis } objects here */
  const fans = [];

  /* helper: pick the local axis whose bounding-box extent is smallest
             (that’s almost always the axis sticking through the hub) */
  function localThinAxis(node) {
    const bb   = node.getBoundingInfo().boundingBox;
    const size = bb.maximum.subtract(bb.minimum);     // local extents
    const xyz  = [size.x, size.y, size.z];
    const minI = xyz.indexOf(Math.min(...xyz));
    return [BABYLON.Axis.X, BABYLON.Axis.Y, BABYLON.Axis.Z][minI];
  }

  BABYLON.SceneLoader.Append(
    "", modelUrl, scene,
    () => {
      console.log("✅ Model loaded");

      FAN_NODE_NAMES.forEach(name => {
        const node = scene.getNodeByName(name);
        if (!node) {
          console.warn(`⚠️  ${name} not found`);
          return;
        }

        /* pivot → hub centre (optional but nicer) */
        const hub = node.getBoundingInfo().boundingBox.center;
        node.setPivotPoint(hub);

        /* world-space spin axis derived from geometry */
        const axisLocal = localThinAxis(node);           // BABYLON.Axis.*
        const axisWorld = node.getDirection(axisLocal);  // → world unit vector

        fans.push({ node, axisWorld });

        if (fans.length) {
  cam.setTarget(fans[0].node
                  .getBoundingInfo()
                  .boundingSphere
                  .centerWorld);
}
      });

      if (fans.length === 0) console.warn("No fan meshes registered.");

    },
    null,
    (_, msg) => console.error("❌ OBJ load failed:", msg)
  );

  /* -------- per–frame rotation -------- */
  let rpm      = 600;
  let lastTime = performance.now();

  scene.onBeforeRenderObservable.add(() => {
    if (fans.length === 0) return;
    const now = performance.now();
    const dt  = (now - lastTime) / 1000;      // seconds
    lastTime  = now;

    const delta = rpm * Math.PI * 2 / 60 * dt / 10; // radians this frame

    fans.forEach(({ node, axisWorld }) => {
      node.rotate(axisWorld, delta, BABYLON.Space.WORLD);
    });
  });

  /* -------- listen for sim2web RPM events -------- */
  window.addEventListener("message", ev => {
    if (ev.source !== parent || ev.data.messageType !== "sim2web") return;
    const [, rpmStr] = ev.data.value.split(":");   // "temp:rpm"
    const newRpm     = parseInt(rpmStr, 10);
    if (!isNaN(newRpm)) rpm = newRpm;
  });

  /* -------- render loop -------- */
  engine.runRenderLoop(() => scene.render());
  window.addEventListener("resize", () => engine.resize());
});
