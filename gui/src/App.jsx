import React from "react";
import "./App.css";
import { Header } from "./components/Header";
import { TopRow } from "./components/TopRow";
import { MiddleRow } from "./components/MiddleRow";
import { BottomRow } from "./components/BottomRow";
import {BatteryVoltageRow} from "./components/BatteryVoltageRow";

export default function App() {
	// State for telemetry data from API
	const [telemetry, setTelemetry] = React.useState({
    pneumatic: "0.00",
    limVoltage: "0.00",
    limCurrent: "0.00",
    battVoltage: "0.00",
    battCurrent: "0.00",
    battSoC: "0.00",
    battTemp: "0.00",
    imdStatus: "Insulated",
		time: "00:00:00",
		distance: "0",
		position: "0.00",
		accelerationx: "0.00",
    accelerationy: "0.00",
    accelerationz: "0.00",

		speed: "0.00",

    therm1: "0.00",
    therm2: "0.00", 
    therm3: "0.00",
    therm4: "0.00",
    therm5: "0.00",
    therm6: "0.00",
    therm7: "0.00",
    therm8: "0.00",

    vbus1: "0",
    vShunt1: "0",
    current1: "0",
    power1: "0",
    vbus2: "0",
    vShunt2: "0",
    current2: "0",
    power2: "0",


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

	// Fetch data from API
	React.useEffect(() => {
		const fetchData = async () => {
			try {
				const response = await fetch("YOUR_API_ENDPOINT_HERE");
				const data = await response.json();
				// Update state with API data
				setTelemetry({
					pneumatic: data.pneumatic || telemetry.pneumatic,
					limVoltage: data.limVoltage || telemetry.limVoltage,
          limCurrent: data.limCurrent || telemetry.limCurrent,
					battVoltage: data.battVoltage || telemetry.battVoltage,
					battCurrent: data.battCurrent || telemetry.battCurrent,
					battSoC: data.battSoC || telemetry.battSoC,
          battTemp: data.battTemp || telemetry.battTemp,
					imdStatus: data.imdStatus || telemetry.imdStatus,
					time: data.time || telemetry.time,
					distance: data.distance || telemetry.distance,
					position: data.position || telemetry.position,
					acceleration: data.acceleration || telemetry.acceleration,
					speed: data.speed || telemetry.speed,

          therm1: data.therm1 || telemetry.therm1,
          therm2: data.therm2 || telemetry.therm2,
          therm3: data.therm3 || telemetry.therm3,
          therm4: data.therm4 || telemetry.therm4,
          therm5: data.therm5 || telemetry.therm5,
          therm6: data.therm6 || telemetry.therm6,
          therm7: data.therm7 || telemetry.therm7,
          therm8: data.therm8 || telemetry.therm8,


          

				});

				// Update POD STATE colors
				if (data.podStates) {
					setPodStates({
						INITSTATE: data.podStates.INITSTATE || podStates.INITSTATE,
						LOADSTATE: data.podStates.LOADSTATE || podStates.LOADSTATE,
						PRECHARGESTATE: data.podStates.PRECHARGESTATE || podStates.PRECHARGESTATE,
						STARTSTATE: data.podStates.STARTSTATE || podStates.STARTSTATE,
						STOPSTATE: data.podStates.STOPSTATE || podStates.STOPSTATE,
						FAULTSTATE: data.podStates.FAULTSTATE || podStates.FAULTSTATE,
						HALTSTATE: data.podStates.HALTSTATE || podStates.HALTSTATE
					});
				}
			} catch (error) {
				console.error("Error fetching data:", error);
			}
		};
		// Fetch data immediately
		fetchData();

		// Optional: Set up interval to fetch data periodically (e.g., every second)
		const interval = setInterval(fetchData, 100);

		// Cleanup interval on component unmount
		return () => clearInterval(interval);
	}, []);

	const containerStyle = {
		minHeight: "100vh",
		background: "#5d3b73",
		padding: 0,
		fontFamily: "'Courier New', Courier, monospace",
		color: "#1f1f1f",
		display: "flex",
		flexDirection: "column",
	};

	const contentStyle = {
		padding: 18,
		display: "flex",
		flexDirection: "column",
	};

	return (
		<div style={containerStyle}>
			<Header podStates={podStates} />
			<div style={contentStyle}>
				<TopRow telemetry={telemetry} />
				<MiddleRow telemetry={telemetry} />
				<BottomRow />
        <BatteryVoltageRow />
			</div>
		</div>
	);
  
}

