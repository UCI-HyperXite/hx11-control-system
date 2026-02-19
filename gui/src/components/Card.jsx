import React from "react";

export function Card({ title, children, style = {} }) {
	return (
		<div
			style={{
				background: "#eadfd6",
				borderRadius: 9,
				padding: 18,
				boxSizing: "border-box",
				boxShadow: "inset 0 2px 0 rgba(0,0,0,0.03)",
				...style,
			}}
		>
			{title && (
				<div style={{ fontSize: 14, color: "#2b2b2b", marginBottom: 8, fontWeight: 700, textAlign: "left" }}>
					{title}
				</div>
			)}
			<div>{children}</div>
		</div>
	);
}
