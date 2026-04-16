import React from "react";
import { StopOctagon } from "./StopOctagon";

export function Footer({sendSerial}) {
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
		width: "11.111vw",
		fontSize: "1.111vw",
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
					<button style={btnStyle("init", "#FC95AD")}
						onMouseEnter={() => setHoveredButton("init")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => sendSerial("1\n")}>
						Init
					</button>
					<button style={btnStyle("load", "#76BBEF")}
						onMouseEnter={() => setHoveredButton("load")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => sendSerial("2\n")}>
						Load
					</button>
					<button style={{ ...btnStyle("run", "#1E6A28"), display: "flex", gap: "1.597vw", alignItems: "center" }}
						onMouseEnter={() => setHoveredButton("run")}
						onMouseLeave={() => setHoveredButton(null)}
						onClick={() => sendSerial("3\n")}>
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
					<StopOctagon onClick={() => sendSerial("4\n")} />
				</div>
			</div>
		</div>
	);
}