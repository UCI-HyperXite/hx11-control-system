import React from "react";

export function Card({ title, children, style = {} }) {
	return (
		<div
			style={{
				background: "#eadfd6",
				borderRadius: "1vw",
				padding: "1vw",
				boxSizing: "border-box",
				boxShadow: "inset 0 0.167vw 0 rgba(0,0,0,0.03)",
				display: "flex",          // add
            flexDirection: "column",  // add
				...style,
			}}
		>
			{title && (
				<div style={{
					fontSize: "1.4vw",
					color: "#2b2b2b",
					marginBottom: "0.67vw",
					fontWeight: 650,
					textAlign: "left",
				}}>
					{title}
				</div>
			)}
			<div style={{ flex: 1 }}>{children}</div>
		</div>
	);
}
