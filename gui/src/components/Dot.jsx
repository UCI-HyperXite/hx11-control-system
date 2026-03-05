import React from "react";

export function Dot({ size = 30, style }) {
	return (
		<div
			style={{
				width: size,
				height: size,
				borderRadius: "50%",
				backgroundColor: "#ff5f56",
				border: "#F000FF",
				boxShadow: "0 1px 2px rgba(0,0,0,0.2)",
				...style,
			}}
		/>
	);
}
