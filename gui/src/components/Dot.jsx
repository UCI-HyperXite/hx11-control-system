import React from "react";

export function Dot({ color = "#ff5f56", size = 30 }) {
	return (
		<div
			style={{
				width: size,
				height: size,
				borderRadius: "50%",
				background: color,
				boxShadow: "0 1px 2px rgba(0,0,0,0.2)",
			}}
		/>
	);
}
