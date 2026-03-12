import React from "react";

export function Card({ title, children, style = {} }) {
	return (
		<div
			style={{
				background: "#eadfd6",
				borderRadius: "0.833vw",
				padding: "0.833vw",
				boxSizing: "border-box",
				boxShadow: "inset 0 0.139vw 0 rgba(0,0,0,0.03)",
				display: "flex",          // add
            flexDirection: "column",  // add
				...style,
			}}
		>
			{title && (
				<div style={{
					fontSize: "1.111vw",
					color: "#2b2b2b",
					marginBottom: "0.556vw",
					fontWeight: 700,
					textAlign: "left",
				}}>
					{title}
				</div>
			)}
			<div style={{ flex: 1 }}>{children}</div>
		</div>
	);
}
