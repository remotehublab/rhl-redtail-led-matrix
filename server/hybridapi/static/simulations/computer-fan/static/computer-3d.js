// static/computer-3d.js
window.addEventListener("DOMContentLoaded", () => {
  const canvas = document.getElementById("babylonCanvas");
  const engine = new BABYLON.Engine(canvas, true);
  const scene  = new BABYLON.Scene(engine);

  const cam = new BABYLON.ArcRotateCamera(
    "cam", Math.PI / 2, Math.PI / 3, 10, BABYLON.Vector3.Zero(), scene
  );
  cam.attachControl(canvas, true);
  new BABYLON.HemisphericLight("light", new BABYLON.Vector3(1, 1, 0), scene);

  /* ▸ build absolute URL to the GLB that sits NEXT TO this JS file */
  const modelUrl = new URL("./computer-fan.glb", import.meta.url).href;
  // modelUrl will be:
  //   http://127.0.0.1:5000/static/simulations/computer-fan/computer-fan.glb

  BABYLON.SceneLoader.Append(
    "",          // empty rootUrl when you pass a full URL as filename
    modelUrl,    // full, absolute URL
    scene,
    ()   => console.log("✅ model loaded"),
    null,
    (_, m) => console.error("❌ GLB load failed:", m)
  );

  engine.runRenderLoop(() => scene.render());
  window.addEventListener("resize", () => engine.resize());
});
