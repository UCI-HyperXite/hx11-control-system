import React from "react";
import { Card } from "./Card";

export function TopRow({ telemetry }) {
	const topRowStyle = {
		display: "flex",
		gap: 18,
		marginBottom: -300,
		flex: 1,
	};

	return (
		<div style={topRowStyle}>
			{/* left-top Pressure */}
			<Card title=""
				style={{height: 150, width: 180, flexShrink: 0}}
			>
				<div style={{ lineHeight: 1.5, display: "flex" }}>Time: {telemetry.time}</div>
				<div style={{ lineHeight: 1.5, display: "flex" }}>Distance: {telemetry.distance} cm</div>
				<div style={{ lineHeight: 1.5, display: "flex" }}>Position: {telemetry.position}</div>
				<div style={{ lineHeight: 1.5, display: "flex" }}>Speed: {telemetry.speed}</div>


			</Card>

			{/* top-center Rotation */}
			<Card title="Rotation" style={{ height: 150, width: 320, flexShrink: 0 }}>
				<div style={{ fontSize: 15, display: "flex", textAlign: "left" }}>
					Accel: <br />
					X: {telemetry.accelerationx} Y: {telemetry.accelerationy} Z: {telemetry.accelerationz} m/s²
				</div>
				<div style={{ fontSize: 15, display: "flex", textAlign: "left"}}>
					Gyro: <br />
					X: {telemetry.accelerationx} Y: {telemetry.accelerationy} Z: {telemetry.accelerationz} rad/s
				</div>
				
			</Card>

			{/* right-top Other Info */}
			<Card
				title="OTHER INFO"
				style={{ height: 440, width: 500, flexShrink: 0, display: "flex", flexDirection: "column", padding : 12 }}
			>
				<div
					style={{
						height: 380,
						background: "#2f2740",
						color: "#f0e6f2",
						borderRadius: 12,
						flexShrink: 0,
						paddingLeft: 15,
						paddingRight: 15
					}}
				>
					
					<div className="OtherInfoTable">
                        <table>
                            <tr>
                                <th style={{ width: 150 }}></th>
								<th style={{ width: 75 }}>Min</th>
                                <th style={{ width: 75 }}>Value</th>
								<th style={{ width: 75 }}>Max</th>
								<th style={{ width: 75 }}>Unit</th>

                            </tr>
                            <tr>
                                <td>LIM Voltage:</td>
								<td></td>
                                <td>{telemetry.limVoltage}</td>
								<td></td>
								<td>V</td>
                            </tr>
							<tr>
                                <td>LIM Current:</td>
								<td></td>
                                <td>{telemetry.limCurrent}</td>
								<td></td>
								<td>A</td>
                            </tr>
                            <tr>
                                <td>Batt Voltage:</td>
								<td></td>
                                <td>{telemetry.battVoltage}</td>
								<td></td>
								<td>V</td>
                            </tr>
							<tr>
                                <td>Batt Current:</td>
								<td></td>
                                <td>{telemetry.battCurrent}</td>
								<td></td>
								<td>A</td>
                            </tr>
							<tr>
                                <td>Batt SoC:</td>
								<td></td>
                                <td>{telemetry.battSoC}</td>
								<td></td>
								<td></td>
                            </tr>
							<tr>
                                <td>Batt Temp:</td>
								<td></td>
                                <td>{telemetry.battTemp}</td>
								<td></td>
								<td>°C</td>
                            </tr>
							<tr>
                                <td>IMD Status:</td>
								<td></td>
                                <td>{telemetry.imdStatus}</td>
								<td></td>
								<td></td>
                            </tr>

                        </table>
                    </div>
					{/*
					<div style={{ paddingLeft: 20, lineHeight: 3, display: "flex" }}>
						Batt Current: {telemetry.battCurrent}
					</div>
					<div style={{ paddingLeft: 20, lineHeight: 1, display: "flex" }}>
						Batt State of Charge: {telemetry.battSoC}
					</div>
					<div style={{ paddingLeft: 20, lineHeight: 3, display: "flex" }}>
						IMD Status: {telemetry.imdStatus}
					</div> */}
				</div>
			</Card>
		</div>
	);
}
