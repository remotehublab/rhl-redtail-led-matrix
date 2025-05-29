// static/computer-3d.js  (no import lines)
window.addEventListener("DOMContentLoaded", () => {
  const canvas  = document.getElementById("babylonCanvas");
  const engine  = new BABYLON.Engine(canvas, true);
  const scene   = new BABYLON.Scene(engine);

  const cam = new BABYLON.ArcRotateCamera("cam",
      Math.PI/2, Math.PI/3, 10, BABYLON.Vector3.Zero(), scene);
  cam.attachControl(canvas, true);

  new BABYLON.HemisphericLight("light",
      new BABYLON.Vector3(1,1,0), scene);

  BABYLON.SceneLoader.Append("static/models/", "computer-fan.glb",
      scene, () => console.log("✅ Model loaded!"),
      null, (_, msg) => console.error("❌ GLB load failed:", msg));

  engine.runRenderLoop(() => scene.render());
  window.addEventListener("resize", () => engine.resize());
});
