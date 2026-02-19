import React from "react";
import { Dot } from "./Dot";

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
					HX
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
							<Dot color={podStates.INITSTATE} />
							<Dot color={podStates.LOADSTATE} />
							<Dot color={podStates.PRECHARGESTATE} />
							<Dot color={podStates.STARTSTATE} />
							<Dot color={podStates.STOPSTATE} />
							<Dot color={podStates.FAULTSTATE} />
							<Dot color={podStates.HALTSTATE} />
						</div>
					</div>
				</div>
			</div>
		</div>
	);
}
