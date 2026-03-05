import React from "react";
import { Card } from "./Card";

export function BottomRow() {
	const bottomRowStyle = {
		display: "flex",
		gap: 18,
		marginTop: 5,
		marginBottom: 0,
	};

	return (
		<div style={bottomRowStyle}>
			{/* bottom-left empty spacer */}
			{/* <div style={{ marginTop: 100, height: 10,width: 200, flexShrink: 0 }}>
			</div> */}
			<Card
				title="Console"
				style={{ height: 337, width: 300, flexShrink: 0}}
			>
				<div style={{
						height: 270,
						background: "#2f2740",
						color: "#f0e6f2",
						borderRadius: 12,
						display: "flex",
						flexDirection: "column",
						justifyContent: "center",
						alignItems: "center",
						gap: 20,
						position: "relative"
					}}
				>
					<div
						style={{
							position: "absolute",
							top: 15,
							left: 15
						}}
					>
						&lt;&lt;
					</div>
					<div // the lines in the Console segment
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
					/>

				</div>
			</Card>
			<Card
				title="HV Battery"
				style={{
					height: 335,
					width: 720,
					flexShrink: 0,
					padding: 16,
				}}
			>
				<div>
					<div className="HVBatteryTable">
						<table>
							<tr style={{width: 60, fontSize: 12 }}>
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
										padding: "1px 11.5px",
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
					
				</div>
			</Card>
		</div>
	);
}
