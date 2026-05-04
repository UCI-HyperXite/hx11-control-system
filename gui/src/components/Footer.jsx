import React from "react";
import { StopOctagon } from "./StopOctagon";

export function Footer({downloadCSV, startSending, podState}) {
	const isStopped = podState === "STOPSTATE";

	const [hoveredButton, setHoveredButton] = React.useState(null);

	const darkenColor = (color) => {
		const num = parseInt(color.replace("#", ""), 16);
		const r = Math.max(0, (num >> 16) - 30);
		const g = Math.max(0, ((num >> 8) & 255) - 30);
		const b = Math.max(0, (num & 255) - 30);
		return `rgb(${r}, ${g}, ${b})`;
	};

	const btnStyle = (name, color) => ({
		background: hoveredButton === name ? darkenColor(color) : color,
		border: "none",
		marginLeft: "11.167vw",
		padding: "0.833vw 2.5vw",
		borderRadius: "1.528vw",
		fontWeight: 700,
		cursor: "pointer",
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
				gap: "3.472vw", 
				background: "#260e2f", 
				alignItems: "center",
			}}>
				<div style={{ display: "flex", alignItems: "center" }}>
					<button 
						style={{
							...btnStyle("init", "#FC95AD"),
							opacity: isStopped ? 0.7 : 1,
							cursor: isStopped ? "not-allowed" : "pointer",
						}}
						onMouseEnter={() => !isStopped && setHoveredButton("init")} 
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => !isStopped && startSending("2", "INIT")} 
						disabled={isStopped}>
						Init
					</button>
					<button style={btnStyle("load", "#76BBEF")}
						onMouseEnter={() => setHoveredButton("load")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => startSending("3", "LOAD")}>
						Load
					</button>
					<button style={{ ...btnStyle("run", "#1E6A28"), display: "flex", gap: "1.597vw", alignItems: "center" }}
						onMouseEnter={() => setHoveredButton("run")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => startSending("4", "START")}>
						<div style={{
							borderLeft: "0.694vw solid #ffffff",
							borderTop: "0.486vw solid transparent",
							borderBottom: "0.486vw solid transparent",
							flexShrink: 0,
						}} />
						Start
					</button>
				</div>

				<div style={{ marginLeft: "5.556vw", display: "flex", alignItems: "center" }}>
					<StopOctagon onClick={() => startSending("5", "STOP")}/>
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
						marginLeft: "1vw",
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