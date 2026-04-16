import React from "react";
import { Card } from "./Card";

export function BottomRow({consoleLogs = [] }) {
	const bottomRowStyle = {
		display: "flex",
		gap: "1.5vw",
		marginTop: "0.2vw",
		width: "86.49vw"
	};

	return (
		<div style={bottomRowStyle}>
			<Card title="Console" style={{ height: "32.4vw", width: "24.99vw", flexShrink: 0}}>
				<div style={{
						height: "27.35vw",
						background: "#2f2740",
						color: "#f0e6f2",
						borderRadius: "0.993vw",
						overflowY: "auto",
						display: "flex",
						flexDirection: "column",
						position: "relative",
						overflow: "hidden",
						justifyContent: "center",
                        alignItems: "center",
						fontSize: "1.397vw",
						gap: "0.333vw"
					}}
				>
					<div
						style={{
							position: "absolute",
							top: "1.25vw",
							left: "1.25vw",
							fontSize: "1.868vw"
						}}
					>
						&lt;&lt;
					</div>

				<div
					style={{
					marginTop: "6vw",
					flex: 1,
					overflowY: "auto",
					padding: "0 0.993vw 0.667vw 0.993vw",
					display: "flex",
					flexDirection: "column",
					gap: 4,
           		 }}>
				{consoleLogs.length === 0 ? (

				<div style={{ color: "#a080b0", fontSize: "1.394vw", fontStyle: "italic" }}>
					Waiting for connection...
				</div>

            	) : (
              	consoleLogs.map((log, i) => (
                <div
                  key={i}
                  style={{
                    fontSize: "1.394vw",
                    fontFamily: "monospace",
                    color: log.includes("✓") ? "#7effa0"
                         : log.includes("failed") || log.includes("error") ? "#ff8080"
                         : "#f0e6f2",
                    borderBottom: "1px solid #3d3050",
                    paddingBottom: "0.25vw",
					wordBreak: "break-word",
                  }}
                >
                  {log}
                </div>
              ))
            )}
          </div>
		</div>

			</Card>

			<Card
				title="HV Battery"
				style={{
					height: "32.4vw",
					width:  "60vw",
					flexShrink: 0,
					padding: "1.332vw",
				}}
			>
				<div style={{ height: "100%", width: "100%", overflowX: "hidden" }}>
					
						<table style={{height: "100%", width: "100%", tableLayout: "fixed", fontSize: "1vw", lineHeight: "1.8vw", borderCollapse: "separate" }}>
							<tr >
								<th>Pack</th>
								<th>Cell 0</th>
								<th>Cell 1</th>
								<th>Cell 2</th>
								<th>Cell 3</th>
								<th>Cell 4</th>
								<th>Cell 5</th>
								<th>Cell 6</th>
								<th>Cell 7</th>
								<th>Cell 8</th>
								<th>Cell 9</th>
								<th>Cell 10</th>
								<th>Cell 11</th>
								<th>Cell 12</th>
								<th>Sum</th>
							</tr>
		
							{Array.from({ length: 9 }).map((_, rowIndex) => (
							<tr key={rowIndex}>
								{Array.from({ length: 15 }).map((_, colIndex) => {

								const isHighlighted = rowIndex === 2 && colIndex === 5;
								const isFirstColumn = colIndex === 0;
								const isFirstRow = rowIndex === 0 && colIndex !== 0;

								return (
									<td
									key={colIndex}
									style={{
										padding: "0.083vw 0.24vw",
										fontSize: "1.2vw",
										//backgroundColor: isHighlighted ? "red" : isFirstRow ? "green" : undefined,
										backgroundColor: isFirstColumn ? undefined: "white",
										fontWeight: isFirstColumn ? "bold" : "normal"
									}}
									>
									{isFirstColumn ? rowIndex + 1 : "0.0"}
									</td>
								);
								})}
							</tr>
							))}
							
						</table>
					
				</div>
			</Card>
		</div>
	);
}