import React from "react";

export function StopOctagon({ onClick }) {
	const [isHovered, setIsHovered] = React.useState(false);
	const fillColor = isHovered ? "#a01a1a" : "#d62222";

	React.useEffect(() => {
		const handleKeyPress = (e) => {
			if (e.code === "Space") {
				e.preventDefault();
				if (typeof onClick === "function") onClick();
			}
		};

		window.addEventListener("keydown", handleKeyPress);
		return () => window.removeEventListener("keydown", handleKeyPress);
	}, [onClick]);

	return (
		<div style={{ cursor: "pointer" }} onClick={onClick}>
			<svg width="110" height="110" viewBox="0 0 100 100" style={{ pointerEvents: "none" }}>
				<polygon
					points="30,5 70,5 95,30 95,70 70,95 30,95 5,70 5,30"
					fill={fillColor}
					stroke="#fff"
					strokeWidth="2"
					style={{
						cursor: "pointer",
						pointerEvents: "auto",
					}}
					onMouseEnter={() => setIsHovered(true)}
					onMouseLeave={() => setIsHovered(false)}
				/>
				<text
					x="50"
					y="58"
					textAnchor="middle"
					fontSize="26"
					fontWeight="700"
					fill="#fff"
					fontFamily="Arial, Helvetica, sans-serif"
				>
					STOP
                
				</text>
                <div style={{}}></div>
                <text
					x="50"
					y="75"
					textAnchor="middle"
					fontSize="10"
					fontWeight="700"
					fill="#000000"
					fontFamily="Arial, Helvetica, sans-serif"
				>
					(Spacebar)
                </text>
                
			</svg>
		</div>
	);
}
