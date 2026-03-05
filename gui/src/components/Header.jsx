import React from "react";
import { Dot } from "./Dot";
import HX_logo from "../assets/HX_logo.jpg";

export function Header({ podStates }) {
	const headerStyle = {
		display: "flex",
		alignItems: "center",
		justifyContent: "space-between",
		background: "#020203",
		padding: "18px 18px",
		marginBottom: 0,
	};

	const titleStyle = {
		display: "flex",
		alignItems: "center",
		gap: 12,
		color: "#fff",
	};

	return (
		<div className="header">
			<div style={headerStyle}>
			<div style={titleStyle}>
				<div
					style={{
						width: 44,
						height: 44,
						background: "#fff",
						borderRadius: 8,
						display: "flex",
						alignItems: "center",
						justifyContent: "center",
						color: "#5d3b73",
						fontWeight: 700,
					}}
				>
					<img src={HX_logo} alt="HX Logo" style = {{width: 50, height: 30, objectFit: "contain"}}/>
				</div>
				<div>
					<div style={{ fontSize: 28, fontWeight: 700 }}>HyperXite 11</div>
				</div>
			</div>

			<div style={{ display: "flex", alignItems: "center", gap: 10 }}>
				<div
					style={{
						width: 500,
						background: "#f7f7f7",
						padding: "6px 12px",
						borderRadius: 999,
						color: "#222",
						fontWeight: 700,
					}}
				>
					<div
						style={{
							display: "flex",
							alignItems: "center",
							gap: 10,
							fontSize: 20,
						}}
					>
						POD STATE
						<div
							style={{
								paddingLeft: "30px",
								display: "flex",
								gap: 20,
								alignItems: "center",
							}}
						>
							<Dot style={{ backgroundColor: "#FC95AD", border: "#F000FF" }} />
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
			</div>
			
		</div>
		
	);
}
