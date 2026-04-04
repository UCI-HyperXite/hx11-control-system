import React from "react";
import { Card } from "./Card";

export function TopRow({ telemetry }) {
	return (
		<div style={{ 
			display: "flex", 
			gap: "1.25vw", 
			alignItems: "flex-start",
			width: "86.49vw"
			}}>

			{/* LEFT COLUMN */}
			<div style={{ 
				display: "flex", 
				flexDirection: "column", 
				gap: "1.5vw", 
				flexShrink: 0 }}>

				{/* Top two cards */}
				<div style={{ display: "flex", gap: "1.25vw" }}>
					<Card style={{ 
						height: "13.7vw", 
						width: "15vw", 
						flexShrink: 0, 
						fontWeight: "bold", 
						fontSize: "1.406vw", 
						lineHeight: 2, 
						paddingLeft: "1.2vw",
						paddingTop: "1.2vw" }}>
						<div>Time: {telemetry.time}</div>
						<div>Distance: {telemetry.distance} cm</div>
						<div>Position: {telemetry.position}</div>
						<div>Speed: {telemetry.speed}</div>
					</Card>

					<Card title="Rotation" 
					style={{ 
						width: "26.667vw", 
						flexShrink: 0,
						paddingBottom: "0.5vw"
					 }}>
					<div className="rotTable" style={{ marginTop: "-1vw" }}>
						<table style={{ fontSize: "1.4vw", fontWeight: "bold", width: "100%" }}>
							<thead>
								<tr>
									<th style={{ width: "55%" }}>Accel</th>
									<th style={{ width: "45%" }}>Gyro</th>
								</tr>
							</thead>
							<tbody>
								<tr>
									<td>
										<div>X: {telemetry.accelerationx} m/s²</div>
										<div>Y: {telemetry.accelerationy} m/s²</div>
										<div>Z: {telemetry.accelerationz} m/s²</div>
									</td>
									<td>
										<div>X: {telemetry.gyrox} rad/s</div>
										<div>Y: {telemetry.gyroy} rad/s</div>
										<div>Z: {telemetry.gyroz} rad/s</div>
									</td>
								</tr>
							</tbody>
						</table>
					</div>
					</Card>
				</div>

				{/* Middle cards below */}
				<div style={{ display: "flex", gap: "1.5vw" }}>
					<Card title="Pressure" style={{ width: "19.992vw", flexShrink: 0 }}>
						<div style={{ marginTop: "-0.5vw" }}>
							<table style={{ fontSize: "1.1256vw", fontWeight: "bold", width: "100%", }} className="INA219Table">
								<thead>
									<tr style={{fontSize: "1.3vw"}}>
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
										// ["vbus3:", `${telemetry.vbus3} mV`],
										// ["vShunt3:", `${telemetry.vShunt3} mV`],
										// ["current3:", `${telemetry.current3} mA`],
										// ["power3:", `${telemetry.power3} mW`],
									].map(([label, value]) => (
										<tr key={label}><td>{label}</td><td>{value}</td></tr>
									))}
								</tbody>
							</table>
						</div>
					</Card>

					<Card title="LIM Temperature" style={{ width: "21.667vw", flexShrink: 0}}>
						<table style={{ width: "100%" }} className="TempTable">
							<thead>
								<tr style = {{fontSize: "1.2vw"}}>
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
			<Card title="Other Info" style={{ width: "41.664vw", flexShrink: 0, alignSelf: "stretch" }}>
				<div style={{
					background: "#2f2740", 
					color: "#f0e6f2", 
					borderRadius: "0.833vw",
					padding: "0 1.042vw", 
					fontWeight: "bold", 
					fontSize: "1.0vw", 
					lineHeight: 3,
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
									
									["HV Batt Volt (avg):", telemetry.battVoltage, "V"],
									["HV Batt Current:", telemetry.battCurrent, "A"],
									["HV Batt SoC:", telemetry.battSoC, "%"],
									["LIM Voltage:", telemetry.limVoltage, "V"],
									["LIM Current:", telemetry.limCurrent, "A"],
									["LV Batt Voltage", telemetry.lvbattVoltage, "V"],
									
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