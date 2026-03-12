import React from "react";
import { Dot } from "./Dot";
import HX_logo from "../assets/HX_logo.jpg";

export function Header({ podStates }) {
	const headerStyle = {
		display: "flex",
		justifyContent: "space-between",
		alignItems: "center",
		background: "#020203",
		padding: "1.25vw",
	};

	const titleStyle = {
		display: "flex",
		alignItems: "center",
		gap: "0.833vw",
		color: "#fff",
	};

	return (
		<div className="header">
			<div style={headerStyle}>
			<div style={titleStyle}>
				<div
					style={{
						width: "3.056vw",
						height: "3.056vw",
						background: "#fff",
						borderRadius: "0.556vw",
						display: "flex",
						alignItems: "center",
						justifyContent: "center",
						color: "#5d3b73",
						overflow: "hidden",
						fontWeight: 700,
					}}
				>
					<img src={HX_logo} alt="HX Logo" style = {{width: "3.472v", height: "2.083vw", objectFit: "contain"}}/>
				</div>
				<div>
					<div style={{ fontSize: "1.944vw", fontWeight: 700 }}>HyperXite 11</div>
				</div>
			</div>

			<div style={{
					background: "#f7f7f7",
					padding: "0.417vw 0.833vw",
					borderRadius: "69vw", 
					color: "#222", 
					fontWeight: 700,
					display: "flex", 
					alignItems: "center", 
					gap: "0.694vw",
					fontSize: "1.389vw", 
					minWidth: "34.722vw",
				}}>
						POD STATE
						<div
							style={{
								paddingLeft: "2.083vw",
								display: "flex",
								gap: "1.389vw",
								alignItems: "center",
							}}
						>
							<Dot style={{ backgroundColor: "#FC95AD", border: "#F000FF", }} />
							<Dot style={{ backgroundColor: "#3DADFF" }} />
							<Dot style={{ backgroundColor: "#FFCD29" }} />
							<Dot style={{ backgroundColor: "#359D43" }} />
							<Dot style={{ backgroundColor: "#F24822" }} />
							<Dot style={{ backgroundColor: "#1E1E1E" }} />
							<Dot style={{ backgroundColor: "#FFA629" }} />
						</div>
					</div>
				</div>
			</div>
			
			
	
		
	);
}
