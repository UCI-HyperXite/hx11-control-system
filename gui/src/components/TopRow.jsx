import React from "react";
import { Card } from "./Card";

export function TopRow({ telemetry }) {
	return (
		<div style={{ 
			display: "flex", 
			gap: "1.25vw", 
			width: "74.583vw", 
			alignItems: "flex-start" 
			}}>

			{/* LEFT COLUMN */}
			<div style={{ 
				display: "flex", 
				flexDirection: "column", 
				gap: "1.25vw", 
				flexShrink: 0 }}>

				{/* Top two cards */}
				<div style={{ display: "flex", gap: "1.25vw" }}>
					<Card style={{ 
						height: "11vw", 
						width: "12.5vw", 
						flexShrink: 0, 
						fontWeight: "bold", 
						fontSize: "1.172vw", 
						lineHeight: 2, 
						paddingLeft: "1vw" }}>
						<div>Time: {telemetry.time}</div>
						<div>Distance: {telemetry.distance} cm</div>
						<div>Position: {telemetry.position}</div>
						<div>Speed: {telemetry.speed}</div>
					</Card>

					<Card title="Rotation" style={{ 
						height: "11vw", 
						width: "22.222vw", 
						flexShrink: 0, 
						fontSize: "1.111vw", 
						fontWeight: "bold" }}>
						<div style={{ marginTop: "-0.5vw" }}>Accel:</div>
						<div style={{ marginLeft: "1.042vw", display: "flex", gap: "1.389vw" }}>
							<div>X: {telemetry.accelerationx}</div>
							<div>Y: {telemetry.accelerationy}</div>
							<div>Z: {telemetry.accelerationz} m/s²</div>
						</div>
						<div style={{ marginTop: "0.556vw" }}>Gyro:</div>
						<div style={{ marginLeft: "1.042vw", display: "flex", gap: "1.389vw" }}>
							<div>X: {telemetry.gyrox}</div>
							<div>Y: {telemetry.gyroy}</div>
							<div>Z: {telemetry.gyroz} rad/s</div>
						</div>
					</Card>
				</div>

				{/* Middle cards below */}
				<div style={{ display: "flex", gap: "1.25vw" }}>
					<Card title="Pressure" style={{ width: "16.667vw", flexShrink: 0 }}>
						<table style={{ fontSize: "0.938vw", fontWeight: "bold", width: "100%" }} className="INA219Table">
							<thead>
								<tr>
									<th style={{ width: "55%" }}>INA219</th>
									<th style={{ width: "45%" }}>Value</th>
								</tr>
							</thead>
							<tbody>
								{[
									["vbus1:", `${telemetry.vbus1} mV`],
									["vShunt1:", `${telemetry.vShunt1} mV`],
									["current1:", `${telemetry.current1} mA`],
									["power1:", `${telemetry.power1} mW`],
									["vbus2:", `${telemetry.vbus2} mV`],
									["vShunt2:", `${telemetry.vShunt2} mV`],
									["current2:", `${telemetry.current2} mA`],
									["power2:", `${telemetry.power2} mW`],
								].map(([label, value]) => (
									<tr key={label}><td>{label}</td><td>{value}</td></tr>
								))}
							</tbody>
						</table>
					</Card>

					<Card title="LIM Temperature" style={{ width: "18.056vw", flexShrink: 0}}>
						<table style={{ width: "100%" }} className="TempTable">
							<thead>
								<tr style = {{fontSize: "1vw"}}>
									<th style={{ width: "15%" }}>Th#</th>
									<th style={{ width: "35%" }}>Temp</th>
									<th style={{ width: "15%" }}>Th#</th>
									<th style={{ width: "35%" }}>Temp</th>
								</tr>
							</thead>
							<tbody>
								{[1, 2, 3, 4].map(i => (
									<tr key={i}>
										<td>{i}</td>
										<td>{telemetry[`therm${i}`]} °C</td>
										<td>{i + 4}</td>
										<td>{telemetry[`therm${i + 4}`]} °C</td>
									</tr>
								))}
							</tbody>
						</table>
					</Card>
				</div>

			</div>

			{/* RIGHT COLUMN — Other Info spans full height */}
			<Card title="Other Info" style={{ width: "34.722vw", flexShrink: 0, alignSelf: "stretch" }}>
				<div style={{
					background: "#2f2740", 
					color: "#f0e6f2", 
					borderRadius: "0.833vw",
					padding: "0 1.042vw", 
					fontWeight: "bold", 
					fontSize: "1.01vw", 
					lineHeight: 2.5,
				}}>
					<div className="OtherInfoTable">
						<table style={{ width: "100%" }}>
							<thead>
								<tr>
									<th style={{ width: "40%" }}></th>
									<th style={{ width: "15%" }}>Min</th>
									<th style={{ width: "15%" }}>Value</th>
									<th style={{ width: "15%" }}>Max</th>
									<th style={{ width: "15%" }}>Unit</th>
								</tr>
							</thead>
							<tbody>
								{[
									["LIM Voltage:", telemetry.limVoltage, "V"],
									["LIM Current:", telemetry.limCurrent, "A"],
									["HV Batt Volt (avg):", telemetry.battVoltage, "V"],
									["LV Batt Voltage", telemetry.lvbattVoltage, "V"],
									["HV Batt Current:", telemetry.battCurrent, "A"],
									["HV Batt SoC:", telemetry.battSoC, "%"],
									["Batt Temp:", telemetry.battTemp, "°C"],
									["IMD Status:", telemetry.imdStatus, ""],
								].map(([label, value, unit]) => (
									<tr key={label}>
										<td>{label}</td>
										<td></td>
										<td>{value}</td>
										<td></td>
										<td>{unit}</td>
									</tr>
								))}
							</tbody>
						</table>
					</div>
				</div>
			</Card>

		</div>
	);
}