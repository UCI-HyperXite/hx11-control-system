import React from "react";
import "./App.css";
import { Header } from "./components/Header";
import { TopRow } from "./components/TopRow";
import { BottomRow } from "./components/BottomRow";
import { Footer } from "./components/Footer";

const podStateColors = {
	INITSTATE:      "#FC95AD",
	LOADSTATE:      "#3DADFF",
	PRECHARGESTATE: "#FFCD29",
	STARTSTATE:     "#359D43",
	STOPSTATE:      "#F24822",
	FAULTSTATE:     "#1E1E1E",
	HALTSTATE:      "#FFA629",
};

const podStateMap = {
	0: "INITSTATE",
	1: "LOADSTATE",
	2: "PRECHARGESTATE",
	3: "STARTSTATE",
	4: "STOPSTATE",
	5: "FAULTSTATE",
	6: "HALTSTATE",
};


export default function App() {
	// State for telemetry data from API
	const [telemetry, setTelemetry] = React.useState({
	time: "00:00:00",
	distance: "0",
	position: "0.00",
	speed: "0.00",

	accelerationx: "0.00",
    accelerationy: "0.00",
    accelerationz: "0.00",

	gyrox: "0.00",
    gyroy: "0.00",
    gyroz: "0.00",

    limVoltage: "0.00",
    limCurrent: "0.00",
    battVoltage: "0.00",
	lvbattVoltage: "0.00",
    battCurrent: "0.00",
    battSoC: "0.00",
    battTemp: "0.00",
    imdStatus: "Insulated",
	
	vbus1: "0",
    vShunt1: "0",
    current1: "0",
    power1: "0",
    vbus2: "0",
    vShunt2: "0",
    current2: "0",
    power2: "0",

    therm1: "0.00",
    therm2: "0.00", 
    therm3: "0.00",
    therm4: "0.00",
    therm5: "0.00",
    therm6: "0.00",
    therm7: "0.00",
    therm8: "0.00",


  //   hvbatt: [
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7:
  // "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //     { cell0: "0.00", cell1: "0.00", cell2: "0.00", cell3: "0.00", cell4: "0.00", cell5: "0.00", cell6: "0.00", cell7: "0.00", cell8: "0.00", cell9: "0.00", cell10: "0.00", cell11: "0.00", cell12: "0.00", sum: "0.00" },
  //   ]
	});

	// State for POD STATE dot colors from API
	const [podStates, setPodStates] = React.useState({
		INITSTATE: "#FC95AD",
		LOADSTATE: "#3DADFF",
		PRECHARGESTATE: "#FFCD29",
		STARTSTATE: "#359D43",
		STOPSTATE: "#F24822",
		FAULTSTATE: "#1E1E1E",
		HALTSTATE: "#FFA629"
	});

	const [isConnected, setIsConnected] = React.useState(false);
	const [consoleLogs, setConsoleLogs] = React.useState([]);
  	const readerRef = React.useRef(null);
  	const portRef = React.useRef(null);
	const csvRowsRef = React.useRef([]); // CSV accumulator

	function addLog(message) {
		const timestamp = new Date().toLocaleTimeString();
		setConsoleLogs(prev => [`[${timestamp}] ${message}`, ...prev].slice(0, 50));
  	}

	// function downloadCSV() {
	// 	const rows = csvRowsRef.current;
	// 	if (rows.length === 0) {
	// 		addLog("No data to download yet.");
	// 		return;
	// 	}
	// 	const headers = Object.keys(rows[0]).join(",");
	// 	const body = rows.map(r => Object.values(r).join(",")).join("\n");
	// 	const csv = `${headers}\n${body}`;

	// 	const blob = new Blob([csv], { type: "text/csv" });
	// 	const url = URL.createObjectURL(blob);
	// 	const a = document.createElement("a");
	// 	a.href = url;
	// 	a.download = `telemetry_${new Date().toISOString().replace(/[:.]/g, "-")}.csv`;
	// 	a.click();
	// 	URL.revokeObjectURL(url);
	// 	addLog(`Downloaded ${rows.length} rows as CSV ✓`);
	// }

	async function connectSerial(){
		try{
			const port = await navigator.serial.requestPort();
			await port.open({ baudRate: 115200 });
			portRef.current = port;
			setIsConnected(true);
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
							// time: data.time ?? prev.time,
							distance: data.lidar_distance ?? prev.distance,
							// position: data.position ?? prev.position,
							// speed: data.speed ?? prev.speed,
							// accelerationx: data.accelerationx ?? prev.accelerationx,
							// accelerationy: data.accelerationy ?? prev.accelerationy,
							// accelerationz: data.accelerationz ?? prev.accelerationz,
							// gyrox: data.gyrox ?? prev.gyrox,
							// gyroy: data.gyroy ?? prev.gyroy,
							// gyroz: data.gyroz ?? prev.gyroz,
							// limVoltage: data.limVoltage ?? prev.limVoltage,
							// limCurrent: data.limCurrent ?? prev.limCurrent,
							// battVoltage: data.battVoltage ?? prev.battVoltage,
							// lvbattVoltage: data.lvbattVoltage ?? prev.lvbattVoltage,
							// battCurrent: data.battCurrent ?? prev.battCurrent,
							// battSoC: data.battSoC ?? prev.battSoC,
							// battTemp: data.battTemp ?? prev.battTemp,
							// imdStatus: data.imdStatus ?? prev.imdStatus,
							// vbus1: data.vbus1 ?? prev.vbus1,
							// vShunt1: data.vShunt1 ?? prev.vShunt1,
							// current1: data.current1 ?? prev.current1,
							// power1: data.power1 ?? prev.power1,
							// vbus2: data.vbus2 ?? prev.vbus2,
							// vShunt2: data.vShunt2 ?? prev.vShunt2,
							// current2: data.current2 ?? prev.current2,
							// power2: data.power2 ?? prev.power2,



							// {"RSSI":0,"lidar_distance":0,"pod_state":1,"therms":[72.01,72.01,72.01,72.01,72.01,72.01,72.01,72.01]}
							

							therm1: data.therms?.[0]?.toFixed(2) ?? prev.therm1,
							therm2: data.therms?.[1]?.toFixed(2) ?? prev.therm2,
							therm3: data.therms?.[2]?.toFixed(2) ?? prev.therm3,
							therm4: data.therms?.[3]?.toFixed(2) ?? prev.therm4,
							// therm5: data.therms?.[4]?.toFixed(2) ?? prev.therm5,
							// therm6: data.therms?.[5]?.toFixed(2) ?? prev.therm6,
							// therm7: data.therms?.[6]?.toFixed(2) ?? prev.therm7,
							// therm8: data.therms?.[7]?.toFixed(2) ?? prev.therm8,
						}));


						if (data.pod_state !== undefined) {
							const activeState = podStateMap[data.pod_state];
							if (activeState) {
								setPodStates({
									INITSTATE:      "#1E1E1E",
									LOADSTATE:      "#1E1E1E",
									PRECHARGESTATE: "#1E1E1E",
									STARTSTATE:     "#1E1E1E",
									STOPSTATE:      "#1E1E1E",
									FAULTSTATE:     "#1E1E1E",
									HALTSTATE:      "#1E1E1E",
									[activeState]:  podStateColors[activeState],
								});
							}
						}
					
					} catch (e) {
						// Not valid JSON — could be a partial line or non-JSON message
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
					</div>
				)}
				<Header podStates={podStates} />
				<div style={{flex: 1, minHeight: 0, overflowY: "auto",
				padding: "1.25vw", display: "flex", flexDirection: "column",
				gap: "1.25vw", alignItems: "center",}}>
					<TopRow telemetry={telemetry} />
					<BottomRow consoleLogs={consoleLogs}/>
				</div>		
				<Footer/>
		</div>
	);
}