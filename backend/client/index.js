const APP_WIDTH = 700;
const APP_HEIGHT = 500;

let recvBytes = 0
let N_CELLS = 0;
let GRID_N = 0, GRID_M = 0;
let cells = [];

const EXPECTED_POINT_DATA_LENGTH = 32;
const EXPECTED_INITIAL_MESSAGE_LENGTH = 16;

const socket = new WebSocket("ws://localhost:3333/websocket");
socket.binaryType = "arraybuffer";

socket.addEventListener("open", function (event) {
	//
});

let count = 0;

function handleDataMessage(event) {
    const timeFactor = 0.2;
    const xFactor = 3;
    const yFactor = 4;
	let uintView = new Int32Array(event.data);
	let intView = new Uint32Array(event.data);
	let floatView = new Float32Array(event.data);

    let PDLEN = EXPECTED_POINT_DATA_LENGTH;
    let NI = PDLEN / 4;

	if (intView.length !== N_CELLS * NI) {
		console.log("Incorrect message length (" + intView.length + "); expected " + (N_CELLS * NI));
		return;
	}


    console.log("Received data");

	for (let i = 0; i < uintView.length / NI; i ++) {
		let index = NI*i;
        let cell_index;

		cell_index = uintView[index];
		cells[cell_index].x = xFactor * floatView[index + 1];
		cells[cell_index].y = yFactor * floatView[index + 2];
		cells[cell_index].t1 = timeFactor * intView[index + 3];
		cells[cell_index].t2 = timeFactor * intView[index + 4];
		cells[cell_index].light = intView[index + 5];
		cells[cell_index].cycle = intView[index + 6];
		cells[cell_index].counter = intView[index + 7];
	}

    console.log(`Recv ${floatView[1]} ${floatView[2]}`);

	recvBytes += event.data.byteLength;
}

function handleInitialMessage(event) {
	if (event.data.byteLength != EXPECTED_INITIAL_MESSAGE_LENGTH) {
		console.log("Unknown inital protocol. Stopping.");
		return;
	}
	let data = new Int32Array(event.data);

	GRID_N = data[0];
	GRID_M = data[1];
	N_CELLS = data[2];
	let PD_LEN = data[3];
    
    console.log(PD_LEN);
    if (PD_LEN != EXPECTED_POINT_DATA_LENGTH) {
		console.log("Unknown inital protocol (pdlen). Stopping.");
		return;
    }

	console.log("Number of cells: ", N_CELLS);

	initGraphics();

	socket.addEventListener("message", handleDataMessage);
}


socket.addEventListener("message", handleInitialMessage, {once: true});

let lastTime = null;

function periodicTimer() {
	if (lastTime === null) {
		lastTime = (new Date()).getTime();
		setTimeout(periodicTimer, 2000);
		return;
	}

	let currTime = (new Date()).getTime();
	let delta = (currTime - lastTime) / 1000;

	console.log((recvBytes / delta).toFixed(0) + " bytes per second");

	recvBytes = 0;
	lastTime = currTime;

	setTimeout(periodicTimer, 2000);
}

setTimeout(periodicTimer, 2000)


let app;
// app.stage.interactive = true;

function getNewCell(x = 0, y = 0) {
	const cell = new PIXI.Graphics();
	app.stage.addChild(cell);

	cell.x = x;
	cell.y = y;

	cell.t1 = 30;
	cell.t2 = 30;
	cell.counter = 0;
	cell.light = true;

	return cell;
}

function createCellLight() {
    const radius = 20;
    var canvas = document.createElement('canvas');
    //canvas.width  = 200;
    //canvas.height = 60;
    var ctx = canvas.getContext('2d');
    var gradient = ctx.createRadialGradient(radius, radius, 0, radius, radius, radius);
    gradient.addColorStop(0, "rgba(255, 255, 0, 1)");
    gradient.addColorStop(0.3, "rgba(255, 255, 0, 0.4)");
    gradient.addColorStop(1, "rgba(255, 255, 0, 0)");
    ctx.fillStyle = gradient;
    /*
    var sprite = new PIXI.Sprite(PIXI.Texture.from(canvas));
    sprite.x = 50;
    sprite.y = 50;
    sprite.pivot.x = radius
    sprite.pivot.y = radius
    app.stage.addChild(sprite);
    */
    ctx.arc(radius, radius, radius, 0, 2 * Math.PI);
    ctx.fill();

    return canvas;
}



function initGraphics() {
	app = new PIXI.Application({ 
        width: APP_WIDTH,
        height: APP_HEIGHT,
        view: document.getElementById("main-canvas"),
        antialias: true 
    });
	document.body.appendChild(app.view);

	cells.push(getNewCell(100, 100));
    const cellLightCanvas = createCellLight();

    let lights = []


	for (let i = 0; i < N_CELLS; i++) {
        let cell = getNewCell()
        let light = PIXI.Texture.from(cellLightCanvas);
		cells.push(cell);
        // cell.addChild(light); // Gives error
        //lights.push(light);
	}

    app.ticker.add(() => {
        cells.forEach((cell, index) => {
            cell.clear();

            cell.lineStyle(0);
            if (cell.light) {
                cell.beginFill(0xFFFF0B, 1);
            } else {
                //cell.beginFill(0x00000B, 1);
            }
            cell.drawCircle(0, 0, 3);
            cell.endFill();

            if (!cell.light) {
                if (cell.counter >= cell.t1) {
                    cell.counter = 0;
                    cell.light = true;
                    //lights[index].visible = true;
                }
            } else if (cell.light)  {
                if (cell.counter >= cell.t2) {
                    cell.counter = 0;
                    cell.light = false;
                    //lights[index].visible = false;

                }
                
            }

            cell.counter++;
        });
    });
}

