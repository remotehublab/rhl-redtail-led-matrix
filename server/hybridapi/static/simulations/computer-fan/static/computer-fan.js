import kaboom from "https://cdn.jsdelivr.net/npm/kaboom@3000.1.15/dist/kaboom.mjs";

// Initialize kaboom
kaboom({
    width: 360,
    height: 160,
    scale: 2,
    background: [0, 0, 0], // Black background
});

// Default values
let tempValue = 0;
let rpmValue = 0;

// Display labels and values
const tempLabel = add([
    text("CPU Temp: ", { size: 16 }),
    pos(20, 40),
    color(255, 255, 255),
]);

const tempDisplay = add([
    text(`${tempValue} °C`, { size: 16 }),
    pos(160, 40),
    color(0, 255, 0),
    "temp"
]);

const rpmLabel = add([
    text("Fan Speed: ", { size: 16 }),
    pos(20, 80),
    color(255, 255, 255),
]);

const rpmDisplay = add([
    text(`${rpmValue} RPM`, { size: 16 }),
    pos(160, 80),
    color(0, 255, 0),
    "rpm"
]);

// Function to update text
function updateDisplays(temp, rpm) {
    destroyAll("temp");
    destroyAll("rpm");

    add([
        text(`${temp} °C`, { size: 16 }),
        pos(160, 40),
        color(0, 255, 0),
        "temp"
    ]);

    add([
        text(`${rpm} RPM`, { size: 16 }),
        pos(160, 80),
        color(0, 255, 0),
        "rpm"
    ]);
}

// Listen for messages from parent
window.addEventListener("message", (event) => {
    if (event.source !== parent) return;

    if (event.data.messageType === "sim2web") {
        const { temp, rpm } = event.data.value;

        if (typeof temp === "number" && typeof rpm === "number") {
            tempValue = temp;
            rpmValue = rpm;
            updateDisplays(temp, rpm);
        }
    }
}, false);
