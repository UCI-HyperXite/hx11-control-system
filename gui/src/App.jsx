import React from "react";
import "./App.css";
import { Header } from "./components/Header";
import { TopRow } from "./components/TopRow";
import { Footer } from "./components/Footer";
import { EStopModal } from "./components/EStopModal";


const podStateMap = {
	0: "GUI_OKSTATE",
	1: "INITSTATE",
	2: "LOADSTATE",
	3: "PRECHARGESTATE",
	4: "STARTSTATE",
	5: "STOPSTATE",
	6: "FAULTSTATE",
	7: "HALTSTATE",
};

export default function App() {
	// State for telemetry data from API
	const [telemetry, setTelemetry] = React.useState({
	time: "00:00:00",
	distance: "0",
	position: "0.00",
	speed: "0.00",

	gyrox: "0.00",
    gyroy: "0.00",

    limVoltage: "0.00",
    limCurrent: "0.00",
    battVoltage: "0.00",
	lvbattVoltage: "0.00",
    battCurrent: "0.00",
    battSoC: "0.00",
    battTemp: "0.00",
    imdStatus: "Insulated",
	
    current1: "0",
    current2: "0",

    therm1: "0.00",
    therm2: "0.00", 
    therm3: "0.00",
    therm4: "0.00",
    therm5: "0.00",
    therm6: "0.00",
    therm7: "0.00",
    therm8: "0.00",
	});

	// State for POD STATE dot colors from API
	const [podState, setPodState] = React.useState("OFF");
	
	const [isConnected, setIsConnected] = React.useState(false);
	const [consoleLogs, setConsoleLogs] = React.useState([]);
  	const readerRef = React.useRef(null);
  	const portRef = React.useRef(null);
	const csvRowsRef = React.useRef([]); // CSV accumulator

	const heartbeatRef = React.useRef(null);
	const currentCmdRef = React.useRef(null);

	const [showEStop, setShowEStop] = React.useState(false);

	function addLog(message) {
		const timestamp = new Date().toLocaleTimeString();
		setConsoleLogs(prev => [...prev, `[${timestamp}] ${message}`].slice(0, 10000));
	}

	function downloadCSV() {
		const rows = csvRowsRef.current;
		// if (rows.length === 0) return;

		const header = ["time", "therm1", "therm2", "therm3", "therm4"];
		const csvContent = [
			header.join(","),
			...rows.map(r => header.map(k => r[k]).join(","))
		].join("\n");

		const blob = new Blob([csvContent], { type: "text/csv" });
		const url = URL.createObjectURL(blob);
		const a = document.createElement("a");
		a.href = url;
		a.download = `therms_${Date.now()}.csv`;
		a.click();
		URL.revokeObjectURL(url);
	}

	async function sendSerial(message) {
		if (!portRef.current || !isConnected) return;
		try {
		const encoder = new TextEncoderStream();
		encoder.readable.pipeTo(portRef.current.writable);
		const writer = encoder.writable.getWriter();
		await writer.write(message);
		await writer.close();
		addLog(`Sent: ${message.trim()}`);
		} catch (err) {
			addLog(`Send failed: ${err.message}`);
		}
	}

	function startSending(cmd, label) {
		if (currentCmdRef.current === cmd) return; // already sending this, do nothing
		currentCmdRef.current = cmd;
		clearInterval(heartbeatRef.current); // stop previous command
		addLog(`Sending: ${label}`); // log only once

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
			startSending("1", "OK");
			setPodState("GUI_OK");

			console.log("Serial connected ✓");
      		addLog("Serial connected ✓");

			const decoder = new TextDecoderStream();
      		port.readable.pipeTo(decoder.writable);
			const reader = decoder.readable.getReader();
      		readerRef.current = reader;

			let buffer = "";
			while (true) {
				const { value, done } = await reader.read();
				if (done){
					console.log("Serial disconnected");
					addLog("Serial disconnected");
					setIsConnected(false);
					break;
				}

        		buffer += value;
				const lines = buffer.split("\n").map(l => l.trim());
        		buffer = lines.pop();
				
				for (const line of lines) {
         			try {
						const data = JSON.parse(line.trim());
						if (data.RSSI !== undefined) console.log("RSSI:", data.RSSI);

						setTelemetry(prev => ({
							...prev,
							distance: data.lidar ?? prev.distance,
							
							gyrox: data.roll ?? prev.gyrox,
							gyroy: data.pitch ?? prev.gyroy,
							limVoltage: data.lim_volt ?? prev.limVoltage,
							limCurrent: data.lim_curr ?? prev.limCurrent,
							battVoltage: data.hv_batt ?? prev.battVoltage,
							lvbattVoltage: data.lv_batt ?? prev.lvbattVoltage, //ina260
							battCurrent: data.batt_curr ?? prev.battCurrent,
							battSoC: data.batt_soc ?? prev.battSoC,
							battTemp: data.hv_batt_temp ?? prev.battTemp,
							imdStatus: data.imd ?? prev.imdStatus,
							
							current1: data.pt_up ?? prev.current1,
							current2: data.pt_down ?? prev.current2,
							
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
						
						if (data.msg === "ESTOP") { 
    						setShowEStop(true);
						}

						csvRowsRef.current.push({
							time: new Date().toISOString(),
							therm1: data.therms?.[0]?.toFixed(2) ?? "",
							// therm2: data.therms?.[1]?.toFixed(2) ?? "",
							// therm3: data.therms?.[2]?.toFixed(2) ?? "",
							// therm4: data.therms?.[3]?.toFixed(2) ?? "",

						});
						if (data.pod_state !== undefined) {
							const activeState = podStateMap[data.pod_state];
							if (activeState) {
								setPodState(activeState);
							}
						}
					} catch (e) {
						console.warn("Could not parse line:", line);
            			addLog(`Parse error: ${line.trim().slice(0, 40)}`);
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

				
				<Header podState={podState}/>
				<div style={{flex: 1, minHeight: 0, overflowY: "auto",
				padding: "1.25vw", display: "flex", flexDirection: "column",
				gap: "1.25vw", alignItems: "center", width: "100%"}}>
					<TopRow telemetry={telemetry} consoleLogs={consoleLogs} />
				</div>

				{/* <button
					onClick={() => setPodState("STOPSTATE")}
					style={{
						position: "fixed",
						top: 10,
						right: 10,
						zIndex: 999,
						padding: "8px 16px",
						background: "#F24822",
						color: "white",
						border: "none",
						borderRadius: 4,
						cursor: "pointer",
						fontWeight: "bold",
					}}>
					Test Stop State
				</button> */}

				<Footer sendSerial={sendSerial} startSending={startSending} downloadCSV={downloadCSV} podState={podState}/>
				</div>
	);
}