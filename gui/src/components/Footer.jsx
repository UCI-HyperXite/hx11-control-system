import React from "react";
import { StopOctagon } from "./StopOctagon";

export function Footer({downloadCSV, startSending, podState, userOverrideRef}) {
	//blocks user from trying to go wrong state
	const isGUIOK = podState === "GUI_OKSTATE";
	const isInit = podState === "INITSTATE";
	const isLoad = podState === "LOADSTATE";
	const isPrecharge = podState === "PRECHARGESTATE";
	const isStart = podState === "STARTSTATE";
	const isStopped = podState === "STOPSTATE";
	const isFault = podState === "FAULTSTATE";

	const canInit  = isGUIOK || isFault;
	const canLoad  = isInit || isStopped || isFault;
	const canStart = isPrecharge; 			// TODO: also gate on voltage/current stabilized signal
	const canStop  = isLoad || isPrecharge || isStart;


	const [hoveredButton, setHoveredButton] = React.useState(null);

	const darkenColor = (color) => {
		const num = parseInt(color.replace("#", ""), 16);
		const r = Math.max(0, (num >> 16) - 30);
		const g = Math.max(0, ((num >> 8) & 255) - 30);
		const b = Math.max(0, (num & 255) - 30);
		return `rgb(${r}, ${g}, ${b})`;
	};

	const btnStyle = (name, color, enabled = true) => ({
		background: hoveredButton === name ? darkenColor(color) : color,
		border: "none",
		marginLeft: "9vw",
		padding: "0.833vw 2.5vw",
		borderRadius: "1.528vw",
		fontWeight: 700,
		cursor: enabled ? "pointer" : "not-allowed",
		opacity: enabled ? 1 : 0.4,
		height: "3.472vw",
		width: "11vw",
		fontSize: "1vw",
		transition: "background-color 0.2s ease",
	});


	return (
		<div style={{ position: "sticky", bottom: 0, zIndex: 100, flexShrink: 0 }}>
			<div style={{
				display: "flex", 
				padding: "0.347vw 1.389vw",
				gap: "3.4vw", 
				background: "#260e2f", 
				alignItems: "center",
			}}>
				<div style={{ display: "flex", alignItems: "center" }}>
					<button 
						style={btnStyle("init", "#FC95AD", canInit)}
						onMouseEnter={() => canInit && setHoveredButton("init")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => { if (canInit) { userOverrideRef.current = false; startSending("2", "INIT", true); }}}
						disabled={!canInit}>
						Init
					</button>
					<button 
						style={btnStyle("load", "#76BBEF", canLoad)}
						onMouseEnter={() => canLoad && setHoveredButton("load")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => { if (canLoad) { userOverrideRef.current = false; startSending("3", "LOAD", true); }}}
						disabled={!canLoad}>
						Load
					</button>
					<button 
						style={{ ...btnStyle("run", "#1E6A28", canStart), display: "flex", gap: "1.597vw", alignItems: "center" }}
						onMouseEnter={() => canStart && setHoveredButton("run")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => { if (canStart) { userOverrideRef.current = false; startSending("4", "START", true); }}}
						disabled={!canStart}>
						<div style={{
							borderLeft: "0.694vw solid #ffffff",
							borderTop: "0.486vw solid transparent",
							borderBottom: "0.486vw solid transparent",
							flexShrink: 0,
						}} />
						Start
					</button>
				</div>

				<div style={{ marginLeft: "4vw" }}>
					<StopOctagon
						disabled={!canStop}
						onClick={() => { userOverrideRef.current = false; startSending("5", "STOP", true); }}
					/>
				</div>

				<button
					onMouseEnter={() => setHoveredButton("csv")}
					onMouseLeave={() => setHoveredButton(null)}
					onClick={downloadCSV}
					style={{
						background: hoveredButton === "csv" ? "#1a0a20" : "#260e2f",
						color: "gray",                      
						border: "0.1vw solid gray",
						borderRadius: 4,
						cursor: "pointer",
						fontWeight: "bold",
						fontSize: "1vw",
						marginLeft: "4vw",
						height: "3.5vw",
						width: "11vw",
						transition: "background-color 0.2s ease",
					}}>
					Export CSV
				</button>
			</div>
		</div>
	);
}