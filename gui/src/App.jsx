import React from "react";
import "./App.css";
import { Header } from "./components/Header";
import { TopRow } from "./components/TopRow";
import { Footer } from "./components/Footer";
import { EStopModal } from "./components/EStopModal";

// TODO: 
// disallow START until a msg that says voltage/current stabalized

// TEST: should gui_ok if connected to another device than esp32??
	// if connected to pod, does GUI_OK? 
// TODO: if ESTOP, state should be FAULT


const podStateMap = {
	1: "GUI_OKSTATE",
	2: "INITSTATE",
	3: "LOADSTATE",
	4: "PRECHARGESTATE",
	5: "STARTSTATE",
	6: "STOPSTATE",
	7: "FAULTSTATE",
};

export default function App() {
	// State for telemetry data from API
	const [telemetry, setTelemetry] = React.useState({
	time: "00:00:00",
	distance: "0",
	position: "0.00",
	speed: "0.00",

	roll: "0.00",
    pitch: "0.00",

	//VFD
	drivingDirection: "N/A",
	encoderSpeed: "0.00", // m/s
	errorCode: "N/A",	  // tbd
	batteryVoltage: "0.00", //V
	motorCurrent: "0.00",   //A
	motorTemp: "0.00",		// C or F
	controllerTemp: "0.00", // C or F

	//BMS
	lowestCellVoltage: "0.00",	// V
	highestCellVoltage: "0.00", // V
	packSOC: "0.00",			
	highestTemp: "0.00",		// signed C?
	bmsTestCounter: "0.00",
	relayStatus: "0.00",
	packVoltage: "0.00",
	lowestTemp: "0.00",
	dischargeEnableStatus: "0.00",

	//IMD
	insulationResistance: "0.00",
	iso_status: "0.00",
	imd_counter: "0.00",
	imd_warnings: "0.00",

	
    limCurrent: "0.00",
    battVoltage: "0.00",
	lvbattVoltage: "0.00",
    battCurrent: "0.00",
    battSoC: "0.00",
    battTemp: "0.00",
    imdStatus: "Insulated",
	
	//Pressure
    pressure1: "0",
    pressure2: "0",

    therm1: "00.00",
    therm2: "0.00", 
    therm3: "0.00",
    therm4: "0.00",
    therm5: "00.00",
    therm6: "0.00",
    therm7: "0.00",
    therm8: "0.00",
	});

	// State for POD STATE dot colors from API
	const [podState, setPodState] = React.useState("IDLE");
	
	const [isConnected, setIsConnected] = React.useState(false);
	const [consoleLogs, setConsoleLogs] = React.useState([]);
  	const readerRef = React.useRef(null);
  	const portRef = React.useRef(null);
	const csvRowsRef = React.useRef([]); // CSV accumulator

	const heartbeatRef = React.useRef(null);
	const currentCmdRef = React.useRef(null);
	const connectedAtRef = React.useRef(null);
	const userOverrideRef = React.useRef(false);

	const timerIntervalRef = React.useRef(null);

	const [showEStop, setShowEStop] = React.useState(false);

	function addLog(message) {
		const timestamp = new Date().toLocaleTimeString();
		setConsoleLogs(prev => [...prev, `[${timestamp}] ${message}`].slice(0, 10000));
	}

	function downloadCSV() {
		const rows = csvRowsRef.current;
		// if (rows.length === 0) return;

		const header = ["time", "therm1", "therm2", "therm3", "therm4", "therm5", "therm6", "therm7", "therm8", "roll", "pitch"];
		const csvContent = [
			header.join(","),
			...rows.map(r => header.map(k => r[k]).join(","))
		].join("\n");

		const blob = new Blob([csvContent], { type: "text/csv" });
		const url = URL.createObjectURL(blob);
		const a = document.createElement("a");
		a.href = url;
		a.download = `PodRun_${new Date().toLocaleString("en-US", { timeZone: "America/Los_Angeles", year: "numeric", month: "2-digit", day: "2-digit", hour: "2-digit", minute: "2-digit", second: "2-digit", hour12: false }).replace(/[/,: ]/g, "")}.csv`;
		a.click();
		URL.revokeObjectURL(url);
	}

	function startSending(cmd, label, fromUser = false) {
		if (currentCmdRef.current === cmd) return;
		if (fromUser) userOverrideRef.current = true;
		currentCmdRef.current = cmd;
		clearInterval(heartbeatRef.current);
		addLog(`Sending: ${cmd} ${label}`); 

		const sendCmd = async () => {
			if (!portRef.current) return;
			try {
				const encoder = new TextEncoderStream();
				encoder.readable.pipeTo(portRef.current.writable);
				const writer = encoder.writable.getWriter();
				await writer.write(`${cmd}\n`);
				await writer.close();
			} catch (e) {
				clearInterval(heartbeatRef.current);
				addLog(`Send failed: ${err.message}`);
			}
		};
		sendCmd();
		heartbeatRef.current = setInterval(sendCmd, 500);
	}

	async function connectSerial(){
		try{
			const port = await navigator.serial.requestPort();
			await port.open({ baudRate: 115200 });
			portRef.current = port;
			setIsConnected(true);
			connectedAtRef.current = Date.now();

			timerIntervalRef.current = setInterval(() => {
				const elapsed = Date.now() - connectedAtRef.current;
				const h = Math.floor(elapsed / 3600000);
				const m = Math.floor((elapsed % 3600000) / 60000);
				const s = Math.floor((elapsed % 60000) / 1000);
				setTelemetry(prev => ({
					...prev,
					time: `${String(h).padStart(2,"0")}:${String(m).padStart(2,"0")}:${String(s).padStart(2,"0")}`
				}));
			}, 1000);

			startSending("1", "OK");
			setPodState("GUI_OKSTATE");

      		addLog("Serial connected ✓");

			const decoder = new TextDecoderStream();
      		port.readable.pipeTo(decoder.writable);
			const reader = decoder.readable.getReader();
      		readerRef.current = reader;

			let buffer = "";
			while (true) {
				const { value, done } = await reader.read();
				if (done){
					addLog("Serial disconnected");
					setIsConnected(false);
					clearInterval(heartbeatRef.current);
					clearInterval(timerIntervalRef.current);
					connectedAtRef.current = null;
					currentCmdRef.current = null;
					portRef.current = null;
					setPodState("IDLE");
					break;
				}

        		buffer += value;
				const lines = buffer.split("\n").map(l => l.trim());
        		buffer = lines.pop();
				
				for (const line of lines) {
					addLog(`RX: ${line.trim()}`);
         			try {
						const data = JSON.parse(line.trim());
						if (data.RSSI !== undefined) console.log("RSSI:", data.RSSI);

						setTelemetry(prev => ({
							...prev,
							distance: data.lidar ?? prev.distance,
							
							roll: data.roll ?? prev.roll,
							pitch: data.pitch ?? prev.pitch,

							lowestCellVoltage: data.low_cell ?? prev.lowestCellVoltage,
							highestCellVoltage: data.high_cell ?? prev.highestCellVoltage,

							limCurrent: data.lim_curr ?? prev.limCurrent,
							battVoltage: data.hv_batt ?? prev.battVoltage,
							lvbattVoltage: data.lv_batt ?? prev.lvbattVoltage, //ina260
							battCurrent: data.batt_curr ?? prev.battCurrent,
							battSoC: data.batt_soc ?? prev.battSoC,
							battTemp: data.hv_batt_temp ?? prev.battTemp,
							imdStatus: data.imd ?? prev.imdStatus,
							
							pressure1: data.pt_up ?? prev.pressure1,
							pressure2: data.pt_down ?? prev.pressure2,
							
							// {"lidar":0,"pod_state":1,"roll":0.00,"pitch":0.00,"therms":[20.43,33.09,41.80,59.81,65.01,70.80,83.82,80.72],"pt_up":0.00,"pt_down":0.00,"lv_batt":0.00,"hv_batt_temp":0.00,"hv_batt":0.00,"msg":"Whatever message"}
							
							therm1: data.therms?.[0]?.toFixed(2) ?? prev.therm1,
							therm2: data.therms?.[1]?.toFixed(2) ?? prev.therm2,
							therm3: data.therms?.[2]?.toFixed(2) ?? prev.therm3,
							therm4: data.therms?.[3]?.toFixed(2) ?? prev.therm4,
							therm5: data.therms?.[4]?.toFixed(2) ?? prev.therm5,
							therm6: data.therms?.[5]?.toFixed(2) ?? prev.therm6,
							therm7: data.therms?.[6]?.toFixed(2) ?? prev.therm7,
							therm8: data.therms?.[7]?.toFixed(2) ?? prev.therm8,
						}));
						
						if (data.msg === "ESTOP1" || data.msg === "ESTOP2") { 
    						setShowEStop(true);
						}

						csvRowsRef.current.push({
							time: new Date().toLocaleString("en-US", { timeZone: "America/Los_Angeles", year: "numeric", month: "2-digit", day: "2-digit", hour: "2-digit", minute: "2-digit", second: "2-digit", hour12: false }),
							therm1: data.therms?.[0]?.toFixed(2) ?? "",
							therm2: data.therms?.[1]?.toFixed(2) ?? "",
							therm3: data.therms?.[2]?.toFixed(2) ?? "",
							therm4: data.therms?.[3]?.toFixed(2) ?? "",
							therm5: data.therms?.[4]?.toFixed(2) ?? "",
							therm6: data.therms?.[5]?.toFixed(2) ?? "",
							therm7: data.therms?.[6]?.toFixed(2) ?? "",
							therm8: data.therms?.[7]?.toFixed(2) ?? "",
							roll: data.roll,
							pitch: data.pitch,
						});

						if (data.pod_state !== undefined) {
							const activeState = podStateMap[data.pod_state];
							if (activeState) {
								setPodState(activeState);
								if (activeState === "FAULTSTATE" && !userOverrideRef.current) {
									startSending("6", "FAULT");
								}
							}
						}
					} catch (e) {
						//console.warn("Could not parse line:", line);
            			//addLog(`Parse error: ${line.trim()}`);
					}
				}
			}
		} catch (err) {
			console.error("Serial connection failed ✗", err);
			addLog(`Connection failed: ${err.message}`);
			setIsConnected(false);
		}
	}

	return (
		<div style={{ 
			display: "flex",
			flexDirection: "column",
			height: "100vh",
			width: "100vw",
			fontFamily: "Arial, sans-serif",
			color: "#1f1f1f",
			background: "#5d3b73",
			overflow: "hidden" }}>
				{!isConnected && (
					<div style={{ padding: "10px 18px", background: "#5d3b73", textAlign: "center" }}>
					{!isConnected && (
						<button
							onClick={connectSerial}
							style={{
								padding: "8px 24px",
								background: "#359D43",
								color: "white",
								border: "none",
								borderRadius: 4,
								cursor: "pointer",
								fontWeight: "bold",
								fontSize: 14
							}}
						>
							Connect Serial
						</button>
					)}
				</div>
				)}

				{showEStop && <EStopModal onClose={() => setShowEStop(false)} />} {}

				{/* <div style={{ position: "fixed", top: 500, right: 10, zIndex: 999, display: "flex", flexDirection: "column", gap: 4 }}>
					{Object.entries(podStateMap).map(([num, name]) => (
						<button
							key={num}
							onClick={() => {
								setPodState(name);
								startSending(String(num), name.replace("STATE", ""));
							}}
							style={{
								padding: "4px 12px",
								background: "#333",
								color: "white",
								border: "none",
								borderRadius: 4,
								cursor: "pointer",
								fontSize: 12,
							}}>
							Simulate {num}: {name}
						</button>
					))}
				</div> */}

				<Header podState={podState}/>
				<div style={{flex: 1, minHeight: 0, overflowY: "auto",
				padding: "1.25vw", display: "flex", flexDirection: "column",
				gap: "1.25vw", alignItems: "center", width: "100%"}}>
					<TopRow telemetry={telemetry} consoleLogs={consoleLogs} />
				</div>
				<Footer startSending={startSending} downloadCSV={downloadCSV} podState={podState} userOverrideRef={userOverrideRef}/>
				</div>
	);
}