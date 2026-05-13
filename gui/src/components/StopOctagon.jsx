import React from "react";

export function StopOctagon({ onClick, disabled = false }) {
	const [isHovered, setIsHovered] = React.useState(false);
	const fillColor = isHovered && !disabled ? "#a01a1a" : "#d62222";
	const cursor = disabled ? "not-allowed" : "pointer";

	React.useEffect(() => {
		const handleKeyPress = (e) => {
			if (e.code === "Space") {
				e.preventDefault();
				if (!disabled && typeof onClick === "function") onClick();
			}
		};

		window.addEventListener("keydown", handleKeyPress);
		return () => window.removeEventListener("keydown", handleKeyPress);
	}, [onClick, disabled]);

	return (
		<div style={{ cursor, opacity: disabled ? 0.4 : 1}} onClick={!disabled ? onClick : undefined}>
			<svg width="6vw" height="6vw" viewBox="0 0 100 100" style={{ pointerEvents: "none" }}>
				<polygon
					points="30,5 70,5 95,30 95,70 70,95 30,95 5,70 5,30"
					fill={fillColor}
					stroke="#fff"
					strokeWidth="2"
					style={{ cursor, pointerEvents: "auto" }}
					onMouseEnter={() => !disabled && setIsHovered(true)}
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