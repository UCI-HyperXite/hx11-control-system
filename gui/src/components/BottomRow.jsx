import React from "react";
import { StopOctagon } from "./StopOctagon";

export function BottomRow() {
	const [hoveredButton, setHoveredButton] = React.useState(null);

	const bottomRowStyle = {
		display: "flex",
		gap: 18,
		marginBottom: 18,
	};

	const getButtonStyle = (buttonName, baseColor) => ({
		background: hoveredButton === buttonName ? darkenColor(baseColor) : baseColor,
		border: "none",
		padding: "12px 36px",
		borderRadius: 22,
		fontWeight: 700,
		cursor: "pointer",
		transition: "background-color 0.2s ease",
	});

	const darkenColor = (color) => {
		// Convert hex to RGB, darken it
		const num = parseInt(color.replace("#", ""), 16);
		const r = Math.max(0, (num >> 16) - 30);
		const g = Math.max(0, ((num >> 8) & 255) - 30);
		const b = Math.max(0, (num & 255) - 30);
		return `rgb(${r}, ${g}, ${b})`;
	};
    

	return (
		<div style={bottomRowStyle}>
			{/* bottom-left empty spacer */}
			<div style={{ marginTop: 100, height: 10,width: 200, flexShrink: 0 }}></div>

			{/* bottom-center controls */}
			<div style={{
					flex: 1,
					display: "flex",
					gap: 20,
					alignItems: "center",
					justifyContent: "center",
				}}>
                <button
					style={getButtonStyle("init", "#FC95AD")}
					onMouseEnter={() => setHoveredButton("init")}
					onMouseLeave={() => setHoveredButton(null)}
				>
					Init
				</button>

				<button
					style={getButtonStyle("load", "#76BBEF")}
					onMouseEnter={() => setHoveredButton("load")}
					onMouseLeave={() => setHoveredButton(null)}
				>
					Load
				</button>

				<button
					style={{
						...getButtonStyle("run", "#1E6A28"),
						display: "flex",
						gap: 10,
						alignItems: "center",
						cursor: "pointer",
					}}
					onMouseEnter={() => setHoveredButton("run")}
					onMouseLeave={() => setHoveredButton(null)}
				>
                
					<div
						style={{
							width: 0,
							height: 0,
							borderLeft: "10px solid #ffffff",
							borderTop: "7px solid transparent",
							borderBottom: "7px solid transparent",
						}}
					/>
					Run
				</button>
			</div>

			{/* bottom-right STOP */}
			<div
				style={{
					width: 360,
					flexShrink: 0,
					display: "flex",
					alignItems: "center",
					justifyContent: "center",
				}}
			>
				<StopOctagon onClick={() => alert("STOP pressed")} />
			</div>
		</div>
	);
}
