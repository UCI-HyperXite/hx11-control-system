import React from "react";
import { Card } from "./Card";

export function BottomRow({consoleLogs = [] }) {
	const bottomRowStyle = {
		display: "flex",
		gap: "1.25vw",
		width: "74.583vw",
		marginTop: "0.2vw",
	};

	return (
		<div style={bottomRowStyle}>
			<Card title="Console" style={{ height: "27vw", width: "20.833vw", flexShrink: 0}}>
				<div style={{
						height: "22.75vw",
						background: "#2f2740",
						color: "#f0e6f2",
						borderRadius: "0.833vw",
						overflowY: "auto",
						display: "flex",
						flexDirection: "column",
						position: "relative",
						overflow: "hidden",
						justifyContent: "center",
                        alignItems: "center",
						fontSize: "1.164vw",
						gap: "0.278vw"
					}}
				>
					<div
						style={{
							position: "absolute",
							top: "1.042vw",
							left: "1.042vw",
							fontSize: "1.564vw"
						}}
					>
						&lt;&lt;
					</div>

				<div
					style={{
					marginTop: "5vw",
					flex: 1,
					overflowY: "auto",
					padding: "0 0.833vw 0.556vw 0.833vw",
					display: "flex",
					flexDirection: "column",
					gap: 4,
           		 }}>
				{consoleLogs.length === 0 ? (

				<div style={{ color: "#a080b0", fontSize: "1.164vw", fontStyle: "italic" }}>
					Waiting for connection...
				</div>

            	) : (
              	consoleLogs.map((log, i) => (
                <div
                  key={i}
                  style={{
                    fontSize: "1.164vw",
                    fontFamily: "monospace",
                    color: log.includes("✓") ? "#7effa0"
                         : log.includes("failed") || log.includes("error") ? "#ff8080"
                         : "#f0e6f2",
                    borderBottom: "1px solid #3d3050",
                    paddingBottom: "0.208vw",
                  }}
                >
                  {log}
                </div>
              ))
            )}
          </div>
										
			{/* <div // the lines in the Console segment
				style={{
					width: "95%",
					height: 2,
					background: "#e1b9e9"
				}}
			/>
			<div
				style={{
					width: "95%",
					height: 2,
					background: "#e1b9e9"
				}}
			/>
			<div
				style={{
					width: "95%",
					height: 2,
					background: "#e1b9e9"
				}}
			/>
			<div
				style={{
					width: "95%",
					height: 2,
					background: "#e1b9e9"
				}}
			/> */}
		</div>

			</Card>

			<Card
				title="HV Battery"
				style={{
					height: "27vw",
					width:  "50vw",
					flexShrink: 0,
					padding: "1.111vw",
					fontSize:  "1.0vw"
				}}
			>
				<div style={{ height: "100%", width: "100%", overflowX: "hidden" }}>
					
						<table style={{height: "100%", width: "100%", tableLayout: "fixed", fontSize: "0.833vw", lineHeight: "1.5vw", borderCollapse: "separate" }}>
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
								<th>   Sum  </th>
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
										padding: "0.069vw 0.2vw",
										// backgroundColor: isHighlighted ? "red" : isFirstRow ? "green" : undefined,
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