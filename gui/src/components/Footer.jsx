import React from "react";
import { Card } from "./Card";
import { StopOctagon } from "./StopOctagon";

export function Footer({ telemetry }) {
    const [hoveredButton, setHoveredButton] = React.useState(null);
    const FooterStyle = {
        display: "flex",
		padding: "5px 20px",
        gap: 50,
        marginTop: 20,
		marginBottom: -18,
		marginRight: -18,
		marginLeft: -18,
    	background: "#260e2f",
    };

    const getButtonStyle = (buttonName, baseColor) => ({
		background: hoveredButton === buttonName ? darkenColor(baseColor) : baseColor,
		border: "none",
		marginLeft: 60,
		padding: "12px 36px",
		borderRadius: 22,
		fontWeight: 700,
		cursor: "pointer",
		height: 50,
        width: 160,
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
		<div className="footer">
			<div style={FooterStyle}>
				<div style={{
						display: "flex",
						alignItems: "center",
						justifyContent: "center",
						fontSize: 19,
					}}>
						{/*Init Button */}
					<button
						style={getButtonStyle("init", "#FC95AD")}
						onMouseEnter={() => setHoveredButton("init")}
						onMouseLeave={() => setHoveredButton(null)}
					>
						Init
					</button>
						{/*Load Button */}
					<button
						style={getButtonStyle("load", "#76BBEF")}
						onMouseEnter={() => setHoveredButton("load")}
						onMouseLeave={() => setHoveredButton(null)}
					>
						Load
					</button>
						{/*Run Button */}
					<button
						style={{
							...getButtonStyle("run", "#1E6A28"),
							display: "flex",
							gap: 23,
							alignItems: "center",
							cursor: "pointer",
						}}
						onMouseEnter={() => setHoveredButton("run")}
						onMouseLeave={() => setHoveredButton(null)}
					>
						<div
							style={{
								borderLeft: "10px solid #ffffff",
								borderTop: "7px solid transparent",
								borderBottom: "7px solid transparent",
							}}
						/>
						Run
					</button>
				</div>

				<div
					style={{
						marginLeft: 80,
						display: "flex",
						alignItems: "center",
						justifyContent: "left",
					}}
				>
					<StopOctagon onClick={() => alert("STOP pressed")} />
				</div>
			</div>
				
        </div>
    );
}
